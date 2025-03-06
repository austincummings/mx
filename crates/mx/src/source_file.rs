use tree_sitter::Tree;

use crate::{
    ast::{Ast, AstNode, AstNodeRef},
    c_emitter::CEmitter,
    diag::Diagnostic,
    mxir::Mxir,
    parser::Parser,
    sema::Sema,
};

extern "C" {
    pub fn tree_sitter_mx() -> tree_sitter::Language;
}

#[derive(Debug, Clone)]
pub struct SourceFile {
    pub path: String,
    pub src: String,
    pub diagnostics: Vec<Diagnostic>,
}

#[derive(Debug, Clone)]
pub struct UnparsedSourceFile {
    pub file: SourceFile,
}

impl UnparsedSourceFile {
    pub fn new(path: &str, src: &str) -> Self {
        UnparsedSourceFile {
            file: SourceFile {
                path: path.to_string(),
                src: src.to_string(),
                diagnostics: Vec::new(),
            },
        }
    }

    pub fn parse(mut self) -> ParsedSourceFile {
        let language = unsafe { tree_sitter_mx() };

        let mut ts_parser = tree_sitter::Parser::new();
        ts_parser
            .set_language(&language)
            .expect("Error setting language");

        let tree = ts_parser.parse(self.file.src.as_str(), None).unwrap();

        let parser = Parser::new(&self, tree.clone());
        let (ast, diagnostics) = parser.parse();

        self.file.diagnostics.extend(diagnostics);

        ParsedSourceFile {
            data: self.file,
            tree,
            ast,
        }
    }
}

#[derive(Debug, Clone)]
pub struct ParsedSourceFile {
    data: SourceFile,

    tree: Tree,
    ast: Ast,
}

impl ParsedSourceFile {
    pub fn update(&mut self, src: &str) {
        let src_file = UnparsedSourceFile::new(&self.data.path, src);
        let parsed_file = src_file.parse();
        self.data = parsed_file.data;
        self.tree = parsed_file.tree;
        self.ast = parsed_file.ast;
    }

    pub fn analyze(mut self) -> AnalyzedSourceFile {
        let sema = Sema::new(&self);
        let (mxir, diagnostics) = sema.analyze();

        self.data.diagnostics.extend(diagnostics);

        AnalyzedSourceFile {
            data: self.data,
            tree: self.tree,
            ast: self.ast,
            mxir,
        }
    }

    pub fn path(&self) -> &str {
        &self.data.path
    }

    pub fn ast(&self) -> &Ast {
        &self.ast
    }

    pub fn node(&self, node_ref: AstNodeRef) -> Option<AstNode> {
        self.ast.0.get(node_ref.0 as usize).cloned()
    }
}

#[derive(Debug, Clone)]
pub struct AnalyzedSourceFile {
    data: SourceFile,

    tree: Tree,
    ast: Ast,
    mxir: Mxir,
}

impl AnalyzedSourceFile {
    pub fn update(&mut self, src: &str) {
        let src_file = UnparsedSourceFile::new(&self.data.path, src);
        let parsed_file = src_file.parse();
        let analyzed_file = parsed_file.analyze();
        self.data = analyzed_file.data;
        self.tree = analyzed_file.tree;
        self.ast = analyzed_file.ast;
        self.mxir = analyzed_file.mxir;
    }

    pub fn file(&self) -> &SourceFile {
        &self.data
    }

    pub fn ast(&self) -> &Ast {
        &self.ast
    }

    pub fn mxir(&self) -> &Mxir {
        &self.mxir
    }

    pub fn emit_c(mut self) -> CSourceFile {
        let c_emitter = CEmitter::new(&self);
        let (c, diagnostics) = c_emitter.emit();

        self.data.diagnostics.extend(diagnostics);

        CSourceFile {
            file: self.data,
            tree: self.tree,
            ast: self.ast,
            mxir: self.mxir,
            c,
        }
    }
}

#[derive(Debug, Clone)]
pub struct CSourceFile {
    file: SourceFile,

    tree: Tree,
    ast: Ast,
    mxir: Mxir,
    c: String,
}

impl CSourceFile {
    pub fn update(&mut self, src: &str) {
        let src_file = UnparsedSourceFile::new(&self.file.path, src);
        let parsed_file = src_file.parse();
        let analyzed_file = parsed_file.analyze();
        self.file = analyzed_file.data;
        self.tree = analyzed_file.tree;
        self.ast = analyzed_file.ast;
        self.mxir = analyzed_file.mxir;
    }

    pub fn file(&self) -> &SourceFile {
        &self.file
    }

    pub fn c(&self) -> &str {
        &self.c
    }
}

#[cfg(test)]
mod tests {}
