use tree_sitter::{Query, QueryCursor, Tree};

use crate::mxir::*;

extern "C" {
    pub fn tree_sitter_mx() -> tree_sitter::Language;
}

pub struct Sema {
    tree: Tree,
    src: String,
}

impl Sema {
    pub fn new(tree: Tree, src: &str) -> Self {
        Self {
            tree,
            src: src.to_string(),
        }
    }

    pub fn analyze(&mut self) {
        let language = unsafe { tree_sitter_mx() };
        let root_node = self.tree.root_node();
        println!("{}", root_node.to_sexp());

        // Find all top level declarations
        let query_str = "(source_file (fn_decl))";
        let query = Query::new(&language, query_str).expect("Invalid query");

        // Use a QueryCursor to execute the query on the syntax tree
        let mut cursor = QueryCursor::new();
        let root_node = self.tree.root_node();

        // Execute the query and iterate over matches
        let matches = cursor.matches(&query, root_node, self.src.as_bytes());

        // for m in matches {
        //     for cap in m.captures {
        //         let node = cap.node;
        //         let var_name = node.utf8_text(self.src.as_bytes()).unwrap();
        //         println!("Found variable: {}", var_name);
        //     }
        // }
    }
}
