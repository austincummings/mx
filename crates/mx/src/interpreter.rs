use std::collections::HashMap;

use crate::ast::AstNodeRef;
use crate::mxir::{MxirCallExpr, MxirNode, MxirNodeData, MxirNodeRef, MxirReturn};
use crate::source_file::AnalyzedSourceFile;

#[derive(Debug, Clone)]
pub enum InterpreterValue {
    Integer(i128),
    Float(f64),
    Boolean(bool),
    String(String),
}

#[derive(Debug, Clone)]
pub struct Frame {
    pub members: HashMap<String, InterpreterValue>,
}

impl Frame {
    pub fn new() -> Self {
        Self {
            members: HashMap::new(),
        }
    }
}

impl Default for Frame {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Debug, Clone)]
pub struct Interpreter<'a> {
    file: &'a AnalyzedSourceFile,
    symbols: HashMap<String, MxirNodeRef>,
    frames: Vec<Frame>,
}

impl<'a> Interpreter<'a> {
    pub fn new(file: &'a AnalyzedSourceFile) -> Self {
        Self {
            file,
            symbols: HashMap::new(),
            frames: vec![],
        }
    }

    pub fn execute(&mut self) {
        self.register_fn_decls();
        if let Some(main_node_ref) = self.symbols.get("main").cloned() {
            // Append a call expr to the mxir nodes
            let call_main = MxirNode {
                ast_node: AstNodeRef(0),
                self_ref: MxirNodeRef(0),
                data: MxirNodeData::CallExpr(MxirCallExpr {
                    fn_decl_ref: main_node_ref,
                    args: vec![],
                }),
            };
            let exit_value = self.eval_node(call_main);
            println!("Exited with: {:#?}", exit_value);
        }
    }

    fn register_fn_decls(&mut self) {
        for mxir_node_ref_index in 0..self.file.mxir().0.len() {
            let mxir_node_ref = MxirNodeRef(mxir_node_ref_index as u32);
            let mxir_node = self.node(mxir_node_ref);
            if let MxirNodeData::FnDecl(fn_decl) = mxir_node.data {
                self.symbols.insert(fn_decl.name.clone(), mxir_node_ref);
            }
        }
    }

    fn eval_node_ref(&mut self, node_ref: MxirNodeRef) -> Option<InterpreterValue> {
        let root_node = self.node(node_ref);
        self.eval_node(root_node)
    }

    fn eval_node(&mut self, node: MxirNode) -> Option<InterpreterValue> {
        match node.data {
            MxirNodeData::SourceFile(_) => self.eval_source_file(node.self_ref),
            MxirNodeData::CallExpr(call_expr) => self.eval_call_expr(call_expr),
            MxirNodeData::Return(ret) => self.eval_return(ret),
            MxirNodeData::ExprStmt(expr_stmt) => self.eval_node_ref(expr_stmt),
            MxirNodeData::IntLiteral(int_literal) => {
                Some(InterpreterValue::Integer(int_literal.value))
            }
            MxirNodeData::Nop(_) => None,
            _ => {
                todo!("Unhandled node in interpreter: {:#?}", node.data);
            }
        }
    }

    // I LOVE MY GIRLFRIEND (MADALYNN)!!
    // TODO: LOVE MY GIRLFRIEND (MADALYNN)
    fn eval_source_file(&mut self, node_ref: MxirNodeRef) -> Option<InterpreterValue> {
        self.eval_body(node_ref)
    }

    fn eval_call_expr(&mut self, call_expr: MxirCallExpr) -> Option<InterpreterValue> {
        self.push_frame();

        let fn_decl_node = self.node(call_expr.fn_decl_ref);
        let mut ret_val = None;
        if let MxirNodeData::FnDecl(fn_decl) = &fn_decl_node.data {
            let body_node = self.node(fn_decl.body);
            ret_val = self.eval_body(body_node.self_ref);
        } else if let MxirNodeData::BuiltinFnDecl(builtin_fn_decl) = &fn_decl_node.data {
            (builtin_fn_decl.fn_)();
            ret_val = None;
        }

        self.pop_frame();

        ret_val
    }

    fn eval_return(&mut self, node: MxirReturn) -> Option<InterpreterValue> {
        if let Some(expr_ref) = node.0 {
            self.eval_node_ref(expr_ref)
        } else {
            None
        }
    }

    fn eval_body(&mut self, node_ref: MxirNodeRef) -> Option<InterpreterValue> {
        let node = self.node(node_ref);

        let children = match node.data {
            MxirNodeData::SourceFile(source_file) => source_file.0,
            MxirNodeData::Block(block) => block.0,
            _ => {
                panic!("Node does not have a body");
            }
        };

        let mut result = None;
        for child_ref in children {
            result = self.eval_node_ref(child_ref);
        }

        result
    }

    fn node(&self, node_ref: MxirNodeRef) -> MxirNode {
        self.file
            .mxir()
            .0
            .get(node_ref.0 as usize)
            .cloned()
            .unwrap()
    }

    fn push_frame(&mut self) {
        self.frames.push(Frame::new());
    }

    fn pop_frame(&mut self) {
        self.frames.pop();
    }
}
