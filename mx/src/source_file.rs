use tree_sitter::Tree;

use crate::{ast::Ast, diag::Diagnostic, parser::Parser};

extern "C" {
    pub fn tree_sitter_mx() -> tree_sitter::Language;
}

#[derive(Debug, Clone)]
pub struct SourceFile {
    path: String,
    src: String,
    ast: Ast,

    diagnostics: Vec<Diagnostic>,

    tree: tree_sitter::Tree,
}

impl SourceFile {
    pub fn new(path: String, src: &str) -> Self {
        let (ast, diagnostics, tree) = Self::parse(src);

        SourceFile {
            path,
            src: src.to_string(),
            ast,
            diagnostics,
            tree,
        }
    }

    pub fn update_src(&mut self, src: &str) {
        let (ast, diagnostics, tree) = Self::parse(src);

        self.src = src.to_string();
        self.ast = ast;
        self.diagnostics = diagnostics;
        self.tree = tree;
    }

    pub fn path(&self) -> &str {
        &self.path
    }

    pub fn ast(&self) -> &Ast {
        &self.ast
    }

    fn parse(src: &str) -> (Ast, Vec<Diagnostic>, Tree) {
        let language = unsafe { tree_sitter_mx() };

        let mut ts_parser = tree_sitter::Parser::new();
        ts_parser
            .set_language(&language)
            .expect("Error setting language");

        let tree = ts_parser.parse(src, None).unwrap();

        let parser = Parser::new();
        let (ast, diagnostics) = parser.parse(&tree, src);

        (ast, diagnostics, tree)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
}
