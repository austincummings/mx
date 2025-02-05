use tree_sitter::{Node, Tree};

extern "C" {
    pub fn tree_sitter_mx() -> tree_sitter::Language;
}

#[derive(Debug, Clone)]
pub struct Location {
    pub start: (usize, usize),
    pub end: (usize, usize),
}

#[derive(Debug, Clone)]
pub enum ParseError {
    SyntaxError { location: Location },
}

impl ParseError {
    pub fn message(&self) -> String {
        match self {
            ParseError::SyntaxError { location: _ } => "Syntax error".to_string(),
        }
    }

    pub fn location(&self) -> Location {
        match self {
            ParseError::SyntaxError { location } => location.clone(),
        }
    }
}

pub struct MXParser {
    diagnostics: Vec<ParseError>,
}

impl MXParser {
    pub fn new() -> Self {
        Self {
            diagnostics: vec![],
        }
    }

    pub fn parse(&mut self, input: &str) -> Tree {
        let language = unsafe { tree_sitter_mx() };
        let mut ts_parser = tree_sitter::Parser::new();
        ts_parser.set_language(&language).unwrap();

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
                let diag = ParseError::SyntaxError {
                    location: Location {
                        start: (start_line, start_col),
                        end: (end_line, end_col),
                    },
                };
                self.diagnostics.push(diag);
            }

            for i in (0..node.child_count()).rev() {
                stack.push((node.child(i).unwrap(), depth + 1));
            }
        }

        tree
    }

    pub fn diagnostics(&self) -> Vec<ParseError> {
        self.diagnostics.clone()
    }
}
