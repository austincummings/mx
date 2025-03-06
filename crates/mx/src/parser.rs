use std::collections::HashMap;

use tree_sitter::{Node, Tree};

use crate::{
    ast::{Ast, AstNode, AstNodeRef},
    diag::{Diagnostic, DiagnosticKind},
    position::{Point, Range},
    source_file::UnparsedSourceFile,
};

pub struct Parser<'a> {
    file: &'a UnparsedSourceFile,
    tree: Tree,
    ast: Ast,
    diagnostics: Vec<Diagnostic>,
}

impl<'a> Parser<'a> {
    pub fn new(file: &'a UnparsedSourceFile, tree: Tree) -> Self {
        Self {
            file,
            tree,
            ast: Ast::new(),
            diagnostics: Vec::new(),
        }
    }

    /// Parses the given input string and returns an AST. The parser can only be
    /// used once, and is destroyed after parsing.
    pub fn parse(mut self) -> (Ast, Vec<Diagnostic>) {
        // Walk the tree and convert TreeSitter nodes to MXNodes
        let tree = self.tree.clone();
        self.walk_tree(
            self.file.file.path.as_str(),
            tree.root_node().clone(),
            self.file.file.src.as_str(),
        );

        (self.ast, self.diagnostics)
    }

    fn walk_tree(&mut self, path: &str, node: Node, src: &str) -> AstNodeRef {
        let start = node.start_position();
        let end = node.end_position();

        let range = Range {
            start: Point {
                row: start.row,
                col: start.column,
            },
            end: Point {
                row: end.row,
                col: end.column,
            },
        };

        let text = &src[node.start_byte()..node.end_byte()];

        // Create the node first and get its reference
        let node_ref = AstNodeRef(self.ast.0.len() as u32);
        self.ast.0.push(AstNode {
            self_ref: node_ref,
            kind: node.kind().to_string(),
            range,
            text: text.to_string(),
            children: vec![],
            named_children: HashMap::new(),
        });

        // Report any syntax errors
        if node.is_error() {
            self.diagnostics.push(Diagnostic {
                kind: DiagnosticKind::SyntaxError,
                path: path.to_string(),
                range,
            });
        }

        self.check_extra_errors(node, src, range);

        // Traverse children once, adding them to both `children` and `named_children` if applicable
        for i in 0..node.named_child_count() {
            if let Some(child) = node.named_child(i) {
                let child_ref = self.walk_tree(path, child, src);
                self.ast.0[node_ref.0 as usize].children.push(child_ref);

                // Get the field name for the child if it has one
                if let Some(field_name) = node.field_name_for_named_child(i as u32) {
                    self.ast.0[node_ref.0 as usize]
                        .named_children
                        .insert(field_name.to_string(), child_ref);
                }
            }
        }

        node_ref
    }

    fn check_extra_errors(&mut self, node: Node, src: &str, range: Range) {
        if node.kind() == "block" {
            // Check that the block has a "end" field
            if let Some(end_node) = node.child_by_field_name("end") {
                let end_node_text = &src[end_node.start_byte()..end_node.end_byte()];
                if end_node_text != "}" {
                    self.diagnostics.push(Diagnostic {
                        kind: DiagnosticKind::SyntaxErrorExpectedToken("}".to_string()),
                        path: "".to_string(),
                        range,
                    });
                }
            } else {
                // If no "end" field, report a syntax error
                self.diagnostics.push(Diagnostic {
                    kind: DiagnosticKind::SyntaxErrorExpectedToken("}".to_string()),
                    path: "".to_string(),
                    range,
                });
            }
        }
    }
}
