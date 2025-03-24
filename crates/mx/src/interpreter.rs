use std::collections::HashMap;
use std::env::Vars;

use crate::ast::AstNodeRef;
use crate::mxir::{
    MxirCallExpr, MxirIf, MxirLoop, MxirNode, MxirNodeData, MxirNodeRef, MxirReturn, MxirVarDecl,
    MxirVarExpr,
};
use crate::source_file::AnalyzedSourceFile;

#[derive(Debug, Clone, PartialEq)]
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

#[derive(Debug, Clone, PartialEq)]
pub enum ControlFlow {
    Continue,                         // Normal execution
    Break,                            // Break out of a loop
    Return(Option<InterpreterValue>), // Return from a function with optional value
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
            if let MxirNodeData::FnDecl(fn_decl) = &mxir_node.data {
                self.symbols.insert(fn_decl.name.clone(), mxir_node_ref);
            }
        }
    }

    fn eval_node_ref(&mut self, node_ref: MxirNodeRef) -> Option<InterpreterValue> {
        let root_node = self.node(node_ref);
        match self.eval_node_with_control_flow(root_node) {
            (Some(value), _) => Some(value),
            (None, _) => None,
        }
    }

    fn eval_node(&mut self, node: MxirNode) -> Option<InterpreterValue> {
        let (value, _) = self.eval_node_with_control_flow(node);
        value
    }

    fn eval_node_with_control_flow(
        &mut self,
        node: MxirNode,
    ) -> (Option<InterpreterValue>, ControlFlow) {
        match node.data {
            MxirNodeData::SourceFile(_) => self.eval_source_file(node.self_ref),
            MxirNodeData::CallExpr(call_expr) => self.eval_call_expr(call_expr),
            MxirNodeData::Return(ret) => self.eval_return(ret),
            MxirNodeData::ExprStmt(expr_stmt) => self.eval_expr_stmt(expr_stmt),
            MxirNodeData::IntLiteral(int_literal) => (
                Some(InterpreterValue::Integer(int_literal.value)),
                ControlFlow::Continue,
            ),
            MxirNodeData::Loop(loop_stmt) => self.eval_loop(loop_stmt),
            MxirNodeData::Block(_) => self.eval_block(node.self_ref),
            MxirNodeData::If(if_stmt) => self.eval_if(if_stmt),
            MxirNodeData::Break => (None, ControlFlow::Break),
            MxirNodeData::Continue => (None, ControlFlow::Continue),
            MxirNodeData::Nop(_) => (None, ControlFlow::Continue),
            MxirNodeData::BoolLiteral(bool_literal) => (
                Some(InterpreterValue::Boolean(bool_literal.value)),
                ControlFlow::Continue,
            ),
            MxirNodeData::VarDecl(var_decl) => self.eval_var_decl(var_decl),
            MxirNodeData::VarExpr(var_expr) => self.eval_var_expr(var_expr),
            MxirNodeData::Assign(lhs, rhs) => self.eval_assign(node.self_ref),
            _ => {
                todo!("Unhandled node in interpreter: {:#?}", node.data);
            }
        }
    }

    // I LOVE MY GIRLFRIEND (MADALYNN)!!
    // TODO: LOVE MY GIRLFRIEND (MADALYNN)
    fn eval_source_file(
        &mut self,
        node_ref: MxirNodeRef,
    ) -> (Option<InterpreterValue>, ControlFlow) {
        self.eval_body(node_ref)
    }

    fn eval_call_expr(
        &mut self,
        call_expr: MxirCallExpr,
    ) -> (Option<InterpreterValue>, ControlFlow) {
        self.push_frame();

        let fn_decl_node = self.node(call_expr.fn_decl_ref);
        let mut ret_val = None;
        let mut control_flow = ControlFlow::Continue;

        if let MxirNodeData::FnDecl(fn_decl) = &fn_decl_node.data {
            let body_node = self.node(fn_decl.body);
            let (result, flow) = self.eval_body(body_node.self_ref);
            ret_val = result;
            // For function calls, we want to propagate Return control flow out of the function,
            // but leave it as Continue so the caller continues normally
            if let ControlFlow::Return(return_value) = flow {
                ret_val = return_value;
                control_flow = ControlFlow::Continue;
            } else {
                control_flow = flow;
            }
        } else if let MxirNodeData::BuiltinFnDecl(builtin_fn_decl) = &fn_decl_node.data {
            (builtin_fn_decl.fn_)();
            ret_val = None;
            control_flow = ControlFlow::Continue;
        }

        self.pop_frame();

        (ret_val, control_flow)
    }

    fn eval_return(&mut self, node: MxirReturn) -> (Option<InterpreterValue>, ControlFlow) {
        let value = if let Some(expr_ref) = node.0 {
            self.eval_node_ref(expr_ref)
        } else {
            None
        };

        (None, ControlFlow::Return(value))
    }

    fn eval_expr_stmt(
        &mut self,
        expr_stmt: MxirNodeRef,
    ) -> (Option<InterpreterValue>, ControlFlow) {
        let (_, control_flow) = self.eval_node_with_control_flow(self.node(expr_stmt));
        (None, control_flow)
    }

    fn eval_if(&mut self, if_stmt: MxirIf) -> (Option<InterpreterValue>, ControlFlow) {
        // Evaluate the condition
        let (condition_value, condition_flow) =
            self.eval_node_with_control_flow(self.node(if_stmt.condition));

        // If condition evaluation produced a control flow change, propagate it
        if condition_flow != ControlFlow::Continue {
            return (None, condition_flow);
        }

        // Check the condition value to determine which branch to execute
        let condition_bool = match condition_value {
            Some(InterpreterValue::Boolean(value)) => value,
            Some(InterpreterValue::Integer(value)) => value != 0,
            Some(_) => false, // Treat other types as falsy
            None => false,    // No value is treated as falsy
        };

        // Execute the appropriate branch
        if condition_bool {
            self.eval_node_with_control_flow(self.node(if_stmt.then_branch))
        } else if let Some(else_branch) = if_stmt.else_branch {
            self.eval_node_with_control_flow(self.node(else_branch))
        } else {
            // If no else branch and condition is false, continue with no value
            (None, ControlFlow::Continue)
        }
    }

    fn eval_var_decl(&mut self, var_decl: MxirVarDecl) -> (Option<InterpreterValue>, ControlFlow) {
        let var_name = var_decl.name;
        let var_value = if let Some(value_ref) = var_decl.value {
            self.eval_node_with_control_flow(self.node(value_ref)).0
        } else {
            None
        };

        if let Some(frame) = self.frames.last_mut() {
            if let Some(value) = var_value {
                eprintln!("Variable '{}' initialized with value {:?}", var_name, value);
                frame.members.insert(var_name, value);
            }
        }

        (None, ControlFlow::Continue)
    }

    fn eval_var_expr(&mut self, var_expr: MxirVarExpr) -> (Option<InterpreterValue>, ControlFlow) {
        if let Some(frame) = self.frames.last() {
            if let Some(value) = frame.members.get(&var_expr.name) {
                (Some(value.clone()), ControlFlow::Continue)
            } else {
                (None, ControlFlow::Continue)
            }
        } else {
            (None, ControlFlow::Continue)
        }
    }

    fn eval_assign(&mut self, node_ref: MxirNodeRef) -> (Option<InterpreterValue>, ControlFlow) {
        let node = self.node(node_ref);
        let MxirNodeData::Assign(lhs, rhs) = node.data else {
            panic!("Expected Assign node");
        };

        let lhs_node = self.node(lhs);
        let rhs_node = self.node(rhs);

        // First evaluate the right-hand side
        let (rhs_value, control_flow) = self.eval_node_with_control_flow(rhs_node);

        // If control flow is not Continue, return early
        if control_flow != ControlFlow::Continue {
            return (None, control_flow);
        }

        // Now handle the assignment with the evaluated value
        if let Some(frame) = self.frames.last_mut() {
            // Extract variable name from the VarExpr
            if let MxirNodeData::VarExpr(var_expr) = &lhs_node.data {
                if let Some(value) = frame.members.get_mut(&var_expr.name) {
                    if let Some(rhs_val) = rhs_value {
                        *value = rhs_val;
                        return (Some(value.clone()), ControlFlow::Continue);
                    }
                }
            }
        }

        (None, ControlFlow::Continue)
    }

    fn eval_loop(&mut self, loop_stmt: MxirLoop) -> (Option<InterpreterValue>, ControlFlow) {
        if let Some(body_ref) = loop_stmt.0 {
            loop {
                let (_, control_flow) = self.eval_node_with_control_flow(self.node(body_ref));
                match control_flow {
                    ControlFlow::Break => break,
                    ControlFlow::Return(value) => return (None, ControlFlow::Return(value)),
                    ControlFlow::Continue => {} // Continue looping
                }
            }
            (None, ControlFlow::Continue)
        } else {
            // Empty loop body
            (None, ControlFlow::Continue)
        }
    }

    fn eval_block(&mut self, node_ref: MxirNodeRef) -> (Option<InterpreterValue>, ControlFlow) {
        self.eval_body(node_ref)
    }

    fn eval_body(&mut self, node_ref: MxirNodeRef) -> (Option<InterpreterValue>, ControlFlow) {
        let node = self.node(node_ref);

        let children = match &node.data {
            MxirNodeData::SourceFile(source_file) => &source_file.0,
            MxirNodeData::Block(block) => &block.0,
            _ => {
                panic!("Node does not have a body");
            }
        };

        let mut result = None;
        for child_ref in children {
            let (value, control_flow) = self.eval_node_with_control_flow(self.node(*child_ref));
            result = value;

            // Check if we need to exit early due to control flow
            match control_flow {
                ControlFlow::Continue => continue,
                ControlFlow::Break | ControlFlow::Return(_) => return (result, control_flow),
            }
        }

        (result, ControlFlow::Continue)
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
