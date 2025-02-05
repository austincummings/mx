use tree_sitter::{Language, Node, Tree};

use crate::diag::{MXDiagnostic, MXDiagnosticKind, MXDiagnosticSeverity, MXLocation};

pub struct MXParser {
    language: Language,
    diagnostics: Vec<MXDiagnostic>,
    tree: Option<Tree>,
}

impl MXParser {
    pub fn new(language: Language) -> Self {
        Self {
            language,
            diagnostics: vec![],
            tree: None,
        }
    }

    pub fn parse(&mut self, input: &str) {
        let mut ts_parser = tree_sitter::Parser::new();
        ts_parser.set_language(&self.language).unwrap();

        let tree = ts_parser.parse(input, None).unwrap();

        // Walk the tree sitter tree and collect all the named nodes into a Vec
        let mut nodes = vec![];
        let mut stack = vec![(tree.root_node(), 0)];
        while let Some((node, depth)) = stack.pop() {
            if node.is_named() {
                nodes.push((node, depth));
                println!("{:indent$}{}", "", node.kind(), indent = depth * 2);
            }

            if node.is_error() || node.is_missing() {
                let start_line = node.start_position().row;
                let start_col = node.start_position().column;
                let end_line = node.end_position().row;
                let end_col = node.end_position().column;
                let diag = MXDiagnostic {
                    location: MXLocation {
                        start: (start_line as usize, start_col as usize),
                        end: (end_line as usize, end_col as usize),
                    },
                    severity: MXDiagnosticSeverity::Error,
                    kind: MXDiagnosticKind::SyntaxError,
                };
                self.diagnostics.push(diag);
            }

            for i in (0..node.child_count()).rev() {
                stack.push((node.child(i).unwrap(), depth + 1));
            }
        }

        self.tree = Some(tree.clone());
    }

    pub fn get_node_at_position(&self, line: usize, col: usize) -> Option<Node> {
        let tree = self.tree.as_ref().unwrap();
        let mut stack = vec![(tree.root_node(), 0)];
        while let Some((node, _)) = stack.pop() {
            if node.start_position().row <= line
                && node.start_position().column <= col
                && node.end_position().row >= line
                && node.end_position().column >= col
            {
                return Some(node);
            }

            for i in (0..node.child_count()).rev() {
                stack.push((node.child(i).unwrap(), 0));
            }
        }

        None
    }

    pub fn tree(&self) -> Tree {
        self.tree.clone().unwrap()
    }

    pub fn diagnostics(&self) -> Vec<MXDiagnostic> {
        self.diagnostics.clone()
    }
}
