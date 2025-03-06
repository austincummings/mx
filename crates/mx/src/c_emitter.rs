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
    level: u16,
}

impl<'a> CEmitter<'a> {
    pub fn new(file: &'a AnalyzedSourceFile) -> Self {
        Self {
            file,
            c: String::new(),
            diagnostics: Vec::new(),
            level: 0,
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
        self.emit_line(format!("#include <stdio.h>\n\n"));
    }

    fn emit_node(&mut self, node_ref: MxirNodeRef) {
        let node = self.node(node_ref).clone();
        match &node.data {
            MxirNodeData::Nop(str) => {
                self.emit_line(format!("/* Nop(MxirNodeRef {}, {}) */", node_ref.0, str));
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
            MxirNodeData::ExprStmt(ref expr) => {
                self.emit_node(*expr);
                self.emit_inline(format!(";"));
            }
            MxirNodeData::Block(ref block) => {
                self.emit_block(block);
            }
            MxirNodeData::IntLiteral(ref int_literal) => {
                self.emit_inline(format!("{}", int_literal.value));
            }
            MxirNodeData::StringLiteral(ref string_literal) => {
                self.emit_inline(format!("{}", string_literal.value));
            }
            MxirNodeData::VarExpr(ref var_expr) => {
                self.emit_inline(format!("{}", var_expr.name));
            }
            MxirNodeData::Return(ref ret) => {
                self.emit_line(format!("return "));
                if let Some(ret_ref) = ret.0 {
                    self.emit_node(ret_ref);
                }
                self.emit_inline(format!(";"));
            }
            _ => {
                self.emit_inline(format!("/* Unhandled node: {} */", node_ref.0));
            }
        }
    }

    fn emit_source_file(&mut self, block: &MxirBlock) {
        self.emit_body(&block.0);
    }

    fn emit_var_decl(&mut self, var_decl: &MxirVarDecl) {
        self.emit_line(format!("int {}", var_decl.name));
        if let Some(value_ref) = var_decl.value {
            self.emit_inline(format!(" = "));
            self.emit_node(value_ref);
        }
        self.emit_inline(format!(";"));
    }

    fn emit_fn_decl(&mut self, fn_decl: &MxirFnDecl) {
        self.emit_line(format!("void {}(", fn_decl.name));
        // for (i, param) in fn_decl.params.iter().enumerate() {
        //     if i > 0 {
        //         self.emit_code(format!(", "));
        //     }
        //     self.emit_code(format!("{}", param.name));
        // }
        self.emit_inline(format!(") "));
        self.emit_node(fn_decl.body);
    }

    fn emit_block(&mut self, block: &MxirBlock) {
        self.emit_line(format!("{{\n"));
        self.indent();
        self.emit_body(&block.0);
        self.dedent();
        self.emit_line(format!("}}\n"));
    }

    fn emit_body(&mut self, node_refs: &Vec<MxirNodeRef>) {
        for node_ref in node_refs {
            self.emit_node(*node_ref);
            self.emit_inline(format!("\n"));
        }
    }

    fn node(&self, node_ref: MxirNodeRef) -> &MxirNode {
        &self.file.mxir().0[node_ref.0 as usize]
    }

    fn emit_line(&mut self, s: String) {
        // Calculate the indentation
        let indent = self.level * 4;
        write!(self.c, "{}{}", " ".repeat(indent as usize), s).unwrap();
    }

    fn emit_inline(&mut self, s: String) {
        write!(self.c, "{}", s).unwrap();
    }

    fn indent(&mut self) {
        self.level += 1;
    }

    fn dedent(&mut self) {
        self.level -= 1;
    }
}
