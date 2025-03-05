use std::fmt::Write;

use crate::{
    diag::Diagnostic,
    mxir::{MxirBlock, MxirFnDecl, MxirNode, MxirNodeData, MxirNodeRef, MxirVarDecl},
    source_file::AnalyzedSourceFile,
};

#[derive(Debug, Clone)]
pub struct CEmitter<'a> {
    file: &'a AnalyzedSourceFile,

    c: String,
    diagnostics: Vec<Diagnostic>,
}

impl<'a> CEmitter<'a> {
    pub fn new(file: &'a AnalyzedSourceFile) -> Self {
        Self {
            file,
            c: String::new(),
            diagnostics: Vec::new(),
        }
    }

    pub fn emit(mut self) -> (String, Vec<Diagnostic>) {
        eprintln!("MXIR: {:#?}", self.file.mxir());

        self.emit_preamble();

        let source_file_mxir_node_ref = MxirNodeRef(0);
        self.emit_node(source_file_mxir_node_ref);

        // Emit C code here
        (self.c, self.diagnostics)
    }

    fn emit_preamble(&mut self) {
        write!(self.c, "#include <stdio.h>\n\n").unwrap();
    }

    fn emit_node(&mut self, node_ref: MxirNodeRef) {
        let node = self.node(node_ref).clone();
        match &node.data {
            MxirNodeData::Nop(str) => {
                write!(self.c, "/* Nop(MxirNodeRef {}, {}) */", node_ref.0, str).unwrap();
            }
            MxirNodeData::SourceFile(ref src_file) => {
                self.emit_source_file(src_file);
            }
            MxirNodeData::VarDecl(ref var_decl) => {
                self.emit_var_decl(var_decl);
            }
            MxirNodeData::FnDecl(ref fn_decl) => {
                self.emit_fn_decl(fn_decl);
            }
            MxirNodeData::Block(ref block) => {
                self.emit_block(block);
            }
            MxirNodeData::IntLiteral(ref int_literal) => {
                write!(self.c, "{}", int_literal.value).unwrap();
            }
            _ => {
                write!(self.c, "/* Unhandled node: {} */\n", node_ref.0).unwrap();
            }
        }
    }

    fn emit_source_file(&mut self, block: &MxirBlock) {
        self.emit_body(&block.0);
    }

    fn emit_var_decl(&mut self, var_decl: &MxirVarDecl) {
        write!(self.c, "int {}", var_decl.name).unwrap();
        if let Some(value_ref) = var_decl.value {
            write!(self.c, " = ").unwrap();
            self.emit_node(value_ref);
        }
        write!(self.c, ";\n").unwrap();
    }

    fn emit_fn_decl(&mut self, fn_decl: &MxirFnDecl) {
        write!(self.c, "void {}(", fn_decl.name).unwrap();
        // for (i, param) in fn_decl.params.iter().enumerate() {
        //     if i > 0 {
        //         write!(self.c, ", ").unwrap();
        //     }
        //     write!(self.c, "{}", param.name).unwrap();
        // }
        write!(self.c, ")").unwrap();
        self.emit_node(fn_decl.body);
    }

    fn emit_block(&mut self, block: &MxirBlock) {
        write!(self.c, "{{\n").unwrap();
        self.emit_body(&block.0);
        write!(self.c, "}}\n").unwrap();
    }

    fn emit_body(&mut self, node_refs: &Vec<MxirNodeRef>) {
        for node_ref in node_refs {
            self.emit_node(*node_ref);
            write!(self.c, "\n").unwrap();
        }
    }

    fn node(&self, node_ref: MxirNodeRef) -> &MxirNode {
        &self.file.mxir().0[node_ref.0 as usize]
    }
}
