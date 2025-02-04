use tree_sitter::Tree;

extern "C" {
    pub fn tree_sitter_mx() -> tree_sitter::Language;
}

pub struct Parser {}

impl Parser {
    pub fn new() -> Self {
        Self {}
    }

    pub fn parse(&self, input: &str) -> Tree {
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
            for i in (0..node.child_count()).rev() {
                stack.push((node.child(i).unwrap(), depth + 1));
            }
        }

        tree
    }
}
