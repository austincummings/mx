use std::collections::HashMap;

use tree_sitter::{Language, Node, Tree};

use crate::diag::{MXDiagnostic, MXDiagnosticKind, MXPosition, MXRange};

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct AstNodeRef(pub u32);

#[derive(Debug, Clone)]
pub struct AstNode {
    pub self_ref: AstNodeRef,
    pub kind: String,
    pub named: bool,
    pub range: MXRange,
    pub text: String,
    pub named_children: HashMap<String, AstNodeRef>,
    pub children: Vec<AstNodeRef>,
}

pub struct MXParser {
    language: Language,
    diagnostics: Vec<MXDiagnostic>,
    pub nodes: Vec<AstNode>,
}

impl MXParser {
    pub fn new(language: Language) -> Self {
        Self {
            language,
            diagnostics: vec![],
            nodes: vec![],
        }
    }

    fn walk_tree(&mut self, node: tree_sitter::Node, source: &str) -> AstNodeRef {
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

        // Create the node first and get its reference
        let node_ref = AstNodeRef(self.nodes.len() as u32);
        self.nodes.push(AstNode {
            self_ref: node_ref,
            kind: node.kind().to_string(),
            named: node.is_named(),
            range: mx_range,
            text: text.to_string(),
            children: vec![],
            named_children: HashMap::new(),
        });

        // Report any syntax errors
        if node.is_error() {
            self.diagnostics.push(MXDiagnostic {
                kind: MXDiagnosticKind::SyntaxError,
                range: mx_range,
            });
        }

        // Traverse children once, adding them to both `children` and `named_children` if applicable
        for i in 0..node.child_count() {
            if let Some(child) = node.child(i) {
                let child_ref = self.walk_tree(child, source);
                self.nodes[node_ref.0 as usize].children.push(child_ref);

                // Get the field name for the child if it has one
                if let Some(field_name) = node.field_name_for_child(i as u32) {
                    self.nodes[node_ref.0 as usize]
                        .named_children
                        .insert(field_name.to_string(), child_ref);
                }
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

    pub fn find_node_at_position(&self, pos: MXPosition) -> Option<AstNodeRef> {
        let root_node_ref = AstNodeRef(0); // Assuming root node is always at index 0
        self.find_deepest_matching_node(root_node_ref, pos)
    }

    fn find_deepest_matching_node(
        &self,
        node_ref: AstNodeRef,
        pos: MXPosition,
    ) -> Option<AstNodeRef> {
        let node = self.nodes.get(node_ref.0 as usize)?;

        // If the node does not contain the position, return None
        if !self.position_in_range(pos, &node.range) {
            return None;
        }

        // Search children for a more specific match
        for &child_ref in &node.children {
            if let Some(found_ref) = self.find_deepest_matching_node(child_ref, pos) {
                return Some(found_ref);
            }
        }

        // No deeper match, return current node
        Some(node_ref)
    }

    fn position_in_range(&self, pos: MXPosition, range: &MXRange) -> bool {
        (range.start.row < pos.row || (range.start.row == pos.row && range.start.col <= pos.col))
            && (range.end.row > pos.row || (range.end.row == pos.row && range.end.col >= pos.col))
    }
}
