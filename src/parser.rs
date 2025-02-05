use std::collections::HashMap;

use tree_sitter::{Language, Node, Tree};

use crate::diag::{MXDiagnostic, MXDiagnosticKind, MXPosition, MXRange};

#[derive(Debug, Copy, Clone)]
pub struct MXNodeRef(pub u32);

#[derive(Debug, Clone)]
pub struct MXNode {
    pub self_ref: MXNodeRef,
    pub kind: String,
    pub range: MXRange,
    pub text: String,
    pub named_children: HashMap<String, MXNodeRef>,
}

pub struct MXParser {
    language: Language,
    diagnostics: Vec<MXDiagnostic>,
    pub nodes: Vec<MXNode>,
}

impl MXParser {
    pub fn new(language: Language) -> Self {
        Self {
            language,
            diagnostics: vec![],
            nodes: vec![],
        }
    }

    fn walk_tree(&mut self, node: tree_sitter::Node, source: &str) -> MXNodeRef {
        let start = node.start_position();
        let end = node.end_position();

        let mx_range = MXRange {
            start: MXPosition {
                row: start.row,
                col: start.column,
            },
            end: MXPosition {
                row: end.row,
                col: end.column,
            },
        };

        let text = &source[node.start_byte()..node.end_byte()];

        // If it's an error, add a diagnostic
        if node.is_error() {
            self.diagnostics.push(MXDiagnostic {
                range: mx_range.clone(),
                kind: MXDiagnosticKind::SyntaxError,
            });
        }

        let node_ref = MXNodeRef(self.nodes.len() as u32);
        let mx_node = MXNode {
            self_ref: node_ref,
            kind: node.kind().to_string(),
            range: mx_range,
            text: text.to_string(),
            named_children: HashMap::new(),
        };
        self.nodes.push(mx_node);

        // Traverse children
        // for i in 0..node.child_count() {
        //     if let Some(child) = node.child(i) {
        //         let child_ref = self.walk_tree(child, source);
        //         self.nodes[node_ref.0 as usize].children.push(child_ref);
        //     }
        // }
        //
        // Traverse named children and map them
        for i in 0..node.named_child_count() {
            if let Some(child) = node.named_child(i) {
                let child_ref = self.walk_tree(child, source);
                self.nodes[node_ref.0 as usize]
                    .named_children
                    .insert(child.kind().to_string(), child_ref);
            }
        }

        node_ref
    }

    pub fn parse(&mut self, input: &str) {
        let mut ts_parser = tree_sitter::Parser::new();
        ts_parser.set_language(&self.language).unwrap();

        let tree = ts_parser.parse(input, None).unwrap();
        let root_node = tree.root_node();

        // Walk the tree and convert TreeSitter nodes to MXNodes
        self.nodes.clear();
        self.walk_tree(root_node, input);
    }

    pub fn diagnostics(&self) -> Vec<MXDiagnostic> {
        self.diagnostics.clone()
    }
}
