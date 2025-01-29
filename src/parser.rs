extern "C" {
    fn tree_sitter_mx() -> tree_sitter::Language;
}

pub struct Parser {}

impl Parser {
    pub fn parse(&self, input: &str) {
        let language = unsafe { tree_sitter_mx() };
        let mut ts_parser = tree_sitter::Parser::new();
        ts_parser.set_language(&language).unwrap();

        let tree = ts_parser.parse(input, None).unwrap();
        let root_node = tree.root_node();

        println!("{}", root_node.to_sexp());
    }
}
