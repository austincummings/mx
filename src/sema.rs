use std::collections::HashMap;

use crate::{
    diag::{MXDiagnostic, MXDiagnosticKind, MXPosition, MXRange},
    mxir::{MXIRNode, MXIRNodeRef},
    parser::{AstNode, AstNodeRef},
};

#[derive(Debug, Copy, Clone)]
pub struct ComptimeEnvRef(pub u32);

#[derive(Debug, Clone)]
pub struct ComptimeEnv {
    pub self_ref: ComptimeEnvRef,
    pub parent: Option<ComptimeEnvRef>,
    pub bindings: HashMap<String, ComptimeBinding>,
}

impl ComptimeEnv {
    pub fn new(parent: Option<ComptimeEnvRef>) -> Self {
        Self {
            self_ref: ComptimeEnvRef(0),
            parent,
            bindings: HashMap::new(),
        }
    }

    pub fn bind(&mut self, name: String, value: Option<ComptimeValue>) -> Result<(), ()> {
        if self.bindings.contains_key(&name) {
            return Err(());
        }

        self.bindings.insert(
            name.clone(),
            ComptimeBinding {
                name,
                ty: None,
                value,
            },
        );

        Ok(())
    }

    pub fn lookup(&self, envs: &Vec<ComptimeEnv>, name: &str) -> Option<ComptimeEnvRef> {
        if let Some(value) = self.bindings.get(name) {
            return Some(self.self_ref);
        }

        if let Some(parent_ref) = self.parent {
            let parent = &envs[parent_ref.0 as usize];
            return parent.lookup(envs, name);
        }

        None
    }
}

#[derive(Debug, Clone)]
pub struct ComptimeBinding {
    pub name: String,
    pub ty: Option<ComptimeValue>,
    pub value: Option<ComptimeValue>,
}

#[derive(Debug)]
pub struct Sema {
    nodes: Vec<AstNode>,
    diagnostics: Vec<MXDiagnostic>,
    pub mxir: Vec<MXIRNode>,
    envs: Vec<ComptimeEnv>,
    current_env: ComptimeEnvRef,
}

impl Sema {
    pub fn new(nodes: Vec<AstNode>) -> Self {
        Self {
            nodes,
            diagnostics: vec![],
            mxir: vec![],
            envs: vec![ComptimeEnv::new(None)],
            current_env: ComptimeEnvRef(0),
        }
    }

    fn push_env(&mut self) -> ComptimeEnvRef {
        let parent = Some(self.current_env);
        let env = ComptimeEnv::new(parent);
        self.envs.push(env);
        self.current_env = ComptimeEnvRef(self.envs.len() as u32 - 1);
        self.current_env
    }

    fn pop_env(&mut self) -> ComptimeEnvRef {
        if let Some(parent) = self.envs[self.current_env.0 as usize].parent {
            self.current_env = parent;
            return self.current_env;
        } else {
            panic!("Cannot pop the root environment");
        }
    }

    fn declare(&mut self, name: String, value: Option<ComptimeValue>) {
        let current_env = &mut self.envs[self.current_env.0 as usize];
        current_env.bind(name, value).unwrap();
    }

    pub fn analyze(&mut self) {
        // Push a nop as the 0th node
        self.emit(MXIRNode::nop(MXIRNodeRef(0), AstNodeRef(0)));
        self.analyze_entrypoint();
    }

    fn analyze_entrypoint(&mut self) {
        if let Some(entrypoint_node_ref) = self.find_entrypoint_node() {
            self.analyze_fn(entrypoint_node_ref);
        } else {
            self.report_missing_main_function();
        }

        eprintln!("{:#?}", self.mxir);

        for env in &self.envs {
            eprintln!("{:#?}", env);
        }
    }

    fn analyze_fn(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();
        assert!(node.kind == "fn_decl");

        let name_node = self.node(*node.named_children.get("name").unwrap());
        let name = name_node.text.clone();

        let body_node = self.node(*node.named_children.get("body").unwrap());
        let block_mxir_ref = self.analyze_block(body_node.self_ref);

        self.declare(name.clone(), None);

        let self_ref = MXIRNodeRef(self.mxir.len() as u32);
        self.emit(MXIRNode::fn_(self_ref, node_ref, vec![], block_mxir_ref))
    }

    fn analyze_block(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();
        assert!(node.kind == "block");

        self.push_env();

        let mut children_refs = vec![];
        for child_ref in node.children {
            let child_node = self.node(child_ref).clone();

            if child_node.named {
                let child_mxir_ref = self.analyze_block_inner(child_node.self_ref);
                children_refs.push(child_mxir_ref);
            }
        }

        self.pop_env();

        let self_ref = MXIRNodeRef(self.mxir.len() as u32);
        self.emit(MXIRNode::block(self_ref, node_ref, children_refs))
    }

    fn analyze_block_inner(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();

        match node.kind.as_str() {
            "const_decl" => self.analyze_const_decl(node_ref),
            "return_stmt" => self.analyze_return_stmt(node_ref),
            "expr_stmt" => self.analyze_expr_stmt(node_ref),
            _ => {
                self.report(
                    node.range.clone(),
                    MXDiagnosticKind::Unimplemented(node.kind.clone()),
                );
                MXIRNodeRef(0)
            }
        }
    }

    fn analyze_const_decl(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();
        assert!(node.kind == "const_decl");

        let name_node = self.node(*node.named_children.get("name").unwrap());
        let name = name_node.text.clone();

        if let Some(ty_node_ref) = node.named_children.get("type") {
            let ty_node = self.node(*ty_node_ref);
            let ty_val = self.comptime_eval(ty_node.self_ref);
        }

        let value_node = self.node(*node.named_children.get("value").unwrap());
        let value_val = self.comptime_eval(value_node.self_ref);

        self.declare(name.clone(), Some(value_val));

        let self_ref = MXIRNodeRef(self.mxir.len() as u32);
        self.emit(MXIRNode::nop(self_ref, node_ref))
    }

    fn analyze_return_stmt(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();
        assert!(node.kind == "return_stmt");

        let expr_node = self.node(*node.named_children.get("expr").unwrap());
        let expr_val = self.analyze_expr(expr_node.self_ref);

        let self_ref = MXIRNodeRef(self.mxir.len() as u32);
        self.emit(MXIRNode::return_stmt(self_ref, node_ref, expr_val))
    }

    fn analyze_expr_stmt(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();
        assert!(node.kind == "expr_stmt");

        let expr_node = self.node(*node.named_children.get("expr").unwrap());
        let expr_val = self.analyze_expr(expr_node.self_ref);

        let self_ref = MXIRNodeRef(self.mxir.len() as u32);
        self.emit(MXIRNode::expr_stmt(self_ref, node_ref, expr_val))
    }

    fn analyze_expr(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();

        let mxir_node = match node.kind.as_str() {
            "int_literal" => self.analyze_comptime_expr(node_ref),
            "float_literal" => self.analyze_comptime_expr(node_ref),
            "bool_literal" => self.analyze_comptime_expr(node_ref),
            "string_literal" => self.analyze_comptime_expr(node_ref),
            "call_expr" => self.analyze_call_expr(node_ref),
            "variable_expr" => {
                self.report(
                    node.range.clone(),
                    MXDiagnosticKind::Unimplemented("variable_expr".to_string()),
                );
                return MXIRNodeRef(0);
            }
            "comptime_call_expr" => {
                self.report(
                    node.range.clone(),
                    MXDiagnosticKind::Unimplemented("need to impl comptime_call_expr".to_string()),
                );
                return MXIRNodeRef(0);
            }
            "comptime_expr" => {
                // Get the expr node ref
                let expr_node_ref = *node.children.get(0).unwrap();
                self.analyze_comptime_expr(expr_node_ref)
            }
            _ => {
                self.report(
                    node.range.clone(),
                    MXDiagnosticKind::Unimplemented(node.kind.clone()),
                );
                return MXIRNodeRef(0);
            }
        };

        mxir_node
    }

    fn analyze_call_expr(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();
        assert!(node.kind == "call_expr");

        let callee_node = self.node(*node.named_children.get("callee").unwrap());
        let callee_val = self.analyze_expr(callee_node.self_ref);

        let mut args = vec![];
        if let Some(args_node_ref) = node.named_children.get("arguments") {
            let args_node = self.node(*args_node_ref).clone();
            for arg_ref in &args_node.children {
                let arg_val = self.analyze_expr(*arg_ref);
                args.push(arg_val);
            }
        }

        let self_ref = MXIRNodeRef(self.mxir.len() as u32);
        self.emit(MXIRNode::call_expr(self_ref, node_ref, callee_val, args))
    }

    fn analyze_comptime_expr(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();

        match node.kind.as_str() {
            "int_literal" => {
                let value = self.comptime_eval(node_ref);
                let self_ref = MXIRNodeRef(self.mxir.len() as u32);
                self.emit(MXIRNode::comptime_int(self_ref, node_ref, value))
            }
            "float_literal" => {
                let value = self.comptime_eval(node_ref);
                let self_ref = MXIRNodeRef(self.mxir.len() as u32);
                self.emit(MXIRNode::comptime_float(self_ref, node_ref, value))
            }
            "bool_literal" => {
                let value = self.comptime_eval(node_ref);
                let self_ref = MXIRNodeRef(self.mxir.len() as u32);
                self.emit(MXIRNode::comptime_bool(self_ref, node_ref, value))
            }
            "string_literal" => {
                let value = self.comptime_eval(node_ref);
                let self_ref = MXIRNodeRef(self.mxir.len() as u32);
                self.emit(MXIRNode::comptime_string(self_ref, node_ref, value))
            }
            "variable_expr" => {
                self.report(
                    node.range.clone(),
                    MXDiagnosticKind::Unimplemented("variable_expr".to_string()),
                );
                return MXIRNodeRef(0);
            }
            "member_expr" => self.analyze_comptime_member_expr(node_ref),
            "comptime_expr" => {
                // Get the expr node ref
                let expr_node_ref = *node.children.get(0).unwrap();
                self.analyze_comptime_expr(expr_node_ref)
            }
            _ => {
                self.report(
                    node.range.clone(),
                    MXDiagnosticKind::Unimplemented(node.kind.clone()),
                );
                return MXIRNodeRef(0);
            }
        }
    }

    fn analyze_comptime_member_expr(&mut self, node_ref: AstNodeRef) -> MXIRNodeRef {
        let node = self.node(node_ref).clone();
        assert!(node.kind == "member_expr");

        let expr_node = self.node(*node.named_children.get("expr").unwrap());
        let expr_val = self.analyze_comptime_expr(expr_node.self_ref);

        let member = self
            .node(*node.named_children.get("member").unwrap())
            .text
            .clone();

        let self_ref = MXIRNodeRef(self.mxir.len() as u32);
        self.emit(MXIRNode::member_expr(self_ref, node_ref, expr_val, member))
    }

    fn find_entrypoint_node(&self) -> Option<AstNodeRef> {
        for i in 0..self.nodes.len() {
            let node = &self.nodes[i];
            if node.kind == "fn_decl" {
                if node.named_children.contains_key("name") {
                    let name_node_ref = node.named_children.get("name").unwrap();
                    let name_node = self.nodes[name_node_ref.0 as usize].clone();
                    if name_node.text == "main" {
                        return Some(node.self_ref);
                    }
                }
            }
        }

        None
    }

    fn comptime_instantiate_fn(
        &mut self,
        node_ref: AstNodeRef,
        comptime_args: HashMap<String, ComptimeValue>,
    ) {
        let node = self.nodes[node_ref.0 as usize].clone();
        // Get the comptime params definition

        let mut comptime_params = HashMap::new();
        if let Some(comptime_params_ref) = node.named_children.get("comptime_params") {
            let comptime_params_node = self.node(*comptime_params_ref);
            // Iterate over the children, each is a param
            for param_ref in &comptime_params_node.children.clone() {
                let param_node = self.node(*param_ref);
                let name = self.node(*param_node.named_children.get("name").unwrap());
                let ty = self.node(*param_node.named_children.get("type").unwrap());

                let name_str = name.text.clone();
                let ty_val = self.comptime_eval(ty.self_ref);

                let comptime_param = ComptimeParam {
                    name: name_str.clone(),
                    type_: ty_val,
                    value: None,
                };

                comptime_params.insert(name_str, comptime_param);
            }
        }

        // Bind the comptime args to the params values
        for (name, value) in comptime_args {
            if let Some(param) = comptime_params.get_mut(&name) {
                param.value = Some(value);
            }
        }

        // Ensure all comptime params have a value
        for (name, param) in &comptime_params {
            if param.value.is_none() {
                self.report(
                    node.range.clone(),
                    MXDiagnosticKind::MissingComptimeParam(name.clone()),
                );
            }
        }
    }

    fn comptime_eval(&mut self, node_ref: AstNodeRef) -> ComptimeValue {
        let node = self.node(node_ref).clone();
        match node.kind.as_str() {
            "int_literal" => ComptimeValue::ComptimeInt(node.text.parse().unwrap()),
            "float_literal" => ComptimeValue::ComptimeFloat(node.text.parse().unwrap()),
            "bool_literal" => ComptimeValue::ComptimeBool(node.text.parse().unwrap()),
            "string_literal" => ComptimeValue::ComptimeString(node.text.to_string()),
            "comptime_expr" => {
                let expr_node_ref = *node.children.get(0).unwrap();
                self.comptime_eval(expr_node_ref)
            }
            _ => {
                self.report(
                    node.range.clone(),
                    MXDiagnosticKind::Unimplemented(node.kind.clone()),
                );
                ComptimeValue::ComptimeInt(0)
            }
        }
    }

    fn emit(&mut self, node: MXIRNode) -> MXIRNodeRef {
        self.mxir.push(node);
        MXIRNodeRef(self.mxir.len() as u32 - 1)
    }

    pub fn diagnostics(&self) -> Vec<MXDiagnostic> {
        return self.diagnostics.clone();
    }

    fn report_missing_main_function(&mut self) {
        self.report(
            MXRange {
                start: MXPosition { row: 0, col: 0 },
                end: MXPosition { row: 0, col: 0 },
            },
            MXDiagnosticKind::MissingMainFunction,
        );
    }

    fn report(&mut self, range: MXRange, kind: MXDiagnosticKind) {
        self.diagnostics.push(MXDiagnostic { range, kind });
    }

    fn node(&self, node_ref: AstNodeRef) -> &AstNode {
        &self.nodes[node_ref.0 as usize]
    }
}

#[derive(Clone, Debug)]
struct ComptimeParam {
    name: String,
    type_: ComptimeValue,
    value: Option<ComptimeValue>,
}

#[derive(Clone, Debug)]
pub enum ComptimeValue {
    ComptimeInt(i64),
    ComptimeFloat(f64),
    ComptimeBool(bool),
    ComptimeString(String),
    Function,
}
