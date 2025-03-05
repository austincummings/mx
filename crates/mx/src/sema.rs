use std::collections::HashSet;

use crate::{
    ast::{AstNode, AstNodeRef},
    comptime::{ComptimeBinding, ComptimeEnv, ComptimeValue, FnProto, ParamDecl},
    diag::{Diagnostic, DiagnosticKind, Range},
    mxir::{
        Mxir, MxirBlock, MxirFnDecl, MxirIntLiteral, MxirNode, MxirNodeData, MxirNodeRef,
        MxirVarDecl,
    },
    source_file::ParsedSourceFile,
};

#[derive(Debug, Clone)]
pub struct Sema<'a> {
    file: &'a ParsedSourceFile,

    env: ComptimeEnv,
    mxir: Mxir,
    diagnostics: Vec<Diagnostic>,
}

impl<'a> Sema<'a> {
    pub fn new(file: &'a ParsedSourceFile) -> Self {
        Sema {
            file,
            env: ComptimeEnv::new(),
            mxir: Mxir(vec![]),
            diagnostics: vec![],
        }
    }

    pub fn analyze(mut self) -> (Mxir, Vec<Diagnostic>) {
        self.analyze_source_file();

        (self.mxir, self.diagnostics)
    }

    fn analyze_source_file(&mut self) {
        let source_file_node_ref = AstNodeRef(0);
        let source_file_node = self.file.node(source_file_node_ref);

        if let Some(source_file_node) = source_file_node {
            let mxir_source_file_node_ref = self.emit(
                source_file_node_ref,
                MxirNodeData::SourceFile(MxirBlock(vec![])),
            );

            self.env.push_scope(source_file_node.range);

            let mut stmts = self.analyze_body(source_file_node_ref);

            // Lookup the entry point and mimic a function call
            if let Some(main_fn_decl) = self.env.get("main").cloned() {
                stmts.push(self.analyze_resolved_fn_call(main_fn_decl, vec![], vec![]));
            } else {
                self.report(
                    source_file_node_ref,
                    DiagnosticKind::MissingEntrypointFunction,
                );
            }

            // Update the source file node with the analyzed statements
            let mxir_source_file_node = self
                .mxir
                .0
                .get_mut(mxir_source_file_node_ref.0 as usize)
                .unwrap();
            if let MxirNodeData::SourceFile(mxir_source_file_node) = &mut mxir_source_file_node.data
            {
                mxir_source_file_node.0.extend(stmts);
            }

            self.env.pop_scope();
        }
    }

    fn analyze_node(&mut self, node_ref: AstNodeRef) -> MxirNodeRef {
        let node = self.node(node_ref);
        match node.kind.as_str() {
            "fn_decl" => self.analyze_fn_decl(node_ref),
            "const_decl" => self.analyze_const_decl(node_ref),
            "var_decl" => self.analyze_var_decl(node_ref),
            "block" => self.analyze_block(node_ref),
            _ => {
                // eprintln!("Unsupported node type: {}", node.kind);
                self.emit_nop(node_ref, node.kind.as_str())
            }
        }
    }

    fn analyze_fn_decl(&mut self, node_ref: AstNodeRef) -> MxirNodeRef {
        let node = self.node(node_ref);

        let proto_ref = node
            .named_children
            .get("proto")
            .copied()
            .expect("Function prototype not found");
        let proto_node = self.node(proto_ref);

        // Check if name is provided in the function declaration
        let name = if let Some(name_ref) = proto_node.named_children.get("name").copied() {
            let name_node = self.file.node(name_ref).expect("Name node not found");
            name_node.text.clone()
        } else {
            // Report error for missing function name
            self.report(node_ref, DiagnosticKind::MissingFunctionName);
            // Use a placeholder name to continue analysis
            "<anonymous>".to_string()
        };

        // Push a comptime scope for the function parameters
        self.env.push_scope(proto_node.range);

        // Extract and bind comptime parameters
        let comptime_params = self.bind_comptime_params(proto_ref, "comptime_params");

        // Extract parameters using the shared function
        let params = self.extract_params(proto_ref, "params");

        // Now analyze the return type with comptime params in scope
        let return_type_ref = proto_node
            .named_children
            .get("return_type")
            .copied()
            .expect("Return type not found");
        let return_type = self.comptime_eval_comptime_expr(return_type_ref);

        let body_ref = node
            .named_children
            .get("body")
            .copied()
            .expect("Function body not found");

        // Pop the comptime scope
        self.env.pop_scope();

        // Only register the function in the environment if it has a valid name
        if name != "<anonymous>" {
            if let Err(_) = self.env.declare_fn(
                node_ref,
                &name,
                comptime_params,
                params,
                return_type,
                body_ref,
            ) {
                // Report duplicate definition
                self.report(node_ref, DiagnosticKind::DuplicateDefinition);
            }
        }

        self.emit_nop(node_ref, "fn_decl")
    }

    fn analyze_const_decl(&mut self, node_ref: AstNodeRef) -> MxirNodeRef {
        let node = self.node(node_ref);

        let name_ref = node
            .named_children
            .get("name")
            .copied()
            .expect("Constant name not found");
        let name_node = self.file.node(name_ref).expect("Name node not found");
        let name = name_node.text.as_str();

        let ty = if let Some(ty_ref) = node.named_children.get("type").copied() {
            Some(self.comptime_eval_comptime_expr(ty_ref))
        } else {
            None
        };

        let value_ref = node
            .named_children
            .get("value")
            .copied()
            .expect("Constant value not found");
        let value = self.comptime_eval_comptime_expr(value_ref);

        if let Err(_) = self.env.declare_const(node_ref, name, ty, value) {
            // Report duplicate definition
            self.report(node_ref, DiagnosticKind::DuplicateDefinition);
        }

        self.emit_nop(node_ref, node.kind.as_str())
    }

    fn analyze_var_decl(&mut self, node_ref: AstNodeRef) -> MxirNodeRef {
        let node = self.node(node_ref);

        let name_ref = node
            .named_children
            .get("name")
            .copied()
            .expect("Variable name not found");
        let name_node = self.file.node(name_ref).expect("Name node not found");
        let name = name_node.text.as_str();

        let ty = if let Some(ty_ref) = node.named_children.get("type").copied() {
            Some(self.comptime_eval_comptime_expr(ty_ref))
        } else {
            None
        };

        let value_ref = node.named_children.get("value").copied();

        let mxir_value_ref = if let Some(value_ref) = value_ref {
            Some(self.analyze_expr(value_ref))
        } else {
            None
        };

        if let Err(_) = self.env.declare_var(node_ref, name, ty.clone()) {
            // Report duplicate definition
            self.report(node_ref, DiagnosticKind::DuplicateDefinition);
            return self.emit_nop(node_ref, "duplicate definition");
        }

        // Emit the variable declaration
        self.emit(
            node_ref,
            MxirNodeData::VarDecl(MxirVarDecl {
                name: name.to_string(),
                ty,
                value: mxir_value_ref,
            }),
        )
    }

    fn analyze_block(&mut self, node_ref: AstNodeRef) -> MxirNodeRef {
        let node = self.node(node_ref);

        self.env.push_scope(node.range);

        let stmts = self.analyze_body(node_ref);

        self.env.pop_scope();

        self.emit(node_ref, MxirNodeData::Block(MxirBlock(stmts)))
    }

    fn analyze_body(&mut self, node_ref: AstNodeRef) -> Vec<MxirNodeRef> {
        let node = self.node(node_ref);

        let mut stmts = Vec::new();
        for child_ref in node.children {
            stmts.push(self.analyze_node(child_ref));
        }

        stmts
    }

    fn analyze_expr(&mut self, node_ref: AstNodeRef) -> MxirNodeRef {
        let node = self.node(node_ref);
        match node.kind.as_str() {
            _ => self.emit_nop(node_ref, node.kind.as_str()),
        }
    }

    fn comptime_eval_comptime_expr(&mut self, node_ref: AstNodeRef) -> ComptimeValue {
        let node = self.node(node_ref);
        assert!(node.kind == "comptime_expr");

        let expr_node_ref = node
            .named_children
            .get("expr")
            .copied()
            .expect("Expression node not found");

        let expr_node = self.node(expr_node_ref);

        match expr_node.kind.as_str() {
            "int_literal" => {
                let value = node.text.parse().expect("Invalid integer literal");
                ComptimeValue::ComptimeInt(value)
            }
            "variable_expr" => {
                let name = expr_node.text.as_str();
                if let Some(binding) = self.env.lookup(name) {
                    binding.value.clone()
                } else {
                    self.report(
                        expr_node_ref,
                        DiagnosticKind::SymbolNotFound(name.to_string()),
                    );
                    ComptimeValue::Undefined
                }
            }
            "fn_proto" => self.comptime_eval_fn_proto(expr_node_ref),
            _ => panic!("Unsupported comptime expression: {}", expr_node.kind),
        }
    }

    fn comptime_eval_fn_proto(&mut self, expr_node_ref: AstNodeRef) -> ComptimeValue {
        let expr_node = self.node(expr_node_ref);

        // Push a comptime scope for analyzing the function prototype
        self.env.push_scope(expr_node.range);

        // Extract and bind comptime parameters
        let comptime_params = self.bind_comptime_params(expr_node_ref, "comptime_params");

        // Extract parameters using the shared function
        let params = self.extract_params(expr_node_ref, "params");

        // Get the return type by evaluating the return type node with comptime params in scope
        let return_type_ref = expr_node
            .named_children
            .get("return_type")
            .copied()
            .expect("Return type not found");
        let return_type = self.comptime_eval_comptime_expr(return_type_ref);

        // Pop the comptime scope
        self.env.pop_scope();

        let fn_proto = Box::new(FnProto {
            comptime_params,
            params,
            return_type,
        });
        ComptimeValue::FnProto(fn_proto)
    }

    // Helper function to bind comptime parameters to the current scope
    fn bind_comptime_params(
        &mut self,
        proto_ref: AstNodeRef,
        param_list_field: &str,
    ) -> Vec<ParamDecl> {
        let proto_node = self.node(proto_ref);
        let mut comptime_params = vec![];
        let mut param_names = HashSet::new();

        if let Some(param_list_node_ref) = proto_node.named_children.get(param_list_field).copied()
        {
            let param_list_node = self.node(param_list_node_ref);
            for param_node_ref in param_list_node.children {
                let param_node = self.node(param_node_ref);

                let param_name_ref = param_node
                    .named_children
                    .get("name")
                    .copied()
                    .expect("Param name not found");
                let param_name_node = self.node(param_name_ref);
                let param_name = param_name_node.text.clone();

                // Check for duplicate parameter names
                if param_names.contains(&param_name) {
                    self.report(param_name_ref, DiagnosticKind::DuplicateParamName);
                } else {
                    param_names.insert(param_name.clone());
                }

                let param_type_ref = param_node
                    .named_children
                    .get("type")
                    .copied()
                    .expect("Param type not found");
                let param_ty = self.comptime_eval_comptime_expr(param_type_ref);

                // Create a placeholder value for the parameter
                // In a real implementation, this would be filled in when the function is called
                let param_value = param_ty.clone();

                // Declare the comptime parameter in the current scope
                let _ = self.env.declare_const(
                    param_node_ref,
                    &param_name,
                    Some(param_ty.clone()),
                    param_value,
                );

                let param_decl = ParamDecl {
                    name: param_name,
                    ty: param_ty,
                };
                comptime_params.push(param_decl);
            }
        }

        comptime_params
    }

    fn extract_params(&mut self, proto_ref: AstNodeRef, param_list_field: &str) -> Vec<ParamDecl> {
        let proto_node = self.node(proto_ref);
        let mut params = vec![];
        let mut param_names = HashSet::new();

        if let Some(param_list_node_ref) = proto_node.named_children.get(param_list_field).copied()
        {
            let param_list_node = self.node(param_list_node_ref);
            for param_node_ref in param_list_node.children {
                let param_node = self.node(param_node_ref);

                let param_name_ref = param_node
                    .named_children
                    .get("name")
                    .copied()
                    .expect("Param name not found");
                let param_name_node = self.node(param_name_ref);
                let param_name = param_name_node.text.clone();

                if param_names.contains(&param_name) {
                    self.report(param_name_ref, DiagnosticKind::DuplicateParamName);
                } else {
                    param_names.insert(param_name.clone());
                }

                let param_type_ref = param_node
                    .named_children
                    .get("type")
                    .copied()
                    .expect("Param type not found");
                let param_ty = self.comptime_eval_comptime_expr(param_type_ref);

                let param_decl = ParamDecl {
                    name: param_name,
                    ty: param_ty,
                };
                params.push(param_decl);
            }
        }

        params
    }

    fn analyze_resolved_fn_call(
        &mut self,
        binding: ComptimeBinding,
        comptime_args: Vec<ComptimeValue>,
        args: Vec<ComptimeValue>,
    ) -> MxirNodeRef {
        self.env.push_scope(self.node_range(binding.node_ref));

        // Check that the binding comptime value is a function
        if let ComptimeValue::FnDecl(fn_decl) = binding.value {
            // Check that the function has the correct number of comptime parameters/args
            if fn_decl.comptime_params.len() != comptime_args.len() {
                self.report(binding.node_ref, DiagnosticKind::InvalidFunctionCall);
                return self.emit_nop(binding.node_ref, "invalid function call");
            }
        } else {
            self.report(binding.node_ref, DiagnosticKind::InvalidFunctionCall);
            return self.emit_nop(binding.node_ref, "invalid function call");
        }

        let node = self.node(binding.node_ref);

        let proto_node_ref = node
            .named_children
            .get("proto")
            .copied()
            .expect("Function proto node not found");
        let proto_node = self.node(proto_node_ref);

        let name_node_ref = proto_node
            .named_children
            .get("name")
            .copied()
            .expect("Function name node not found");
        let name = self.node(name_node_ref).text;

        let block_ref = node.named_children.get("body").unwrap().clone();

        let mxir_body_ref = self.analyze_node(block_ref);

        let fn_decl_ref = self.emit(
            AstNodeRef(0),
            MxirNodeData::FnDecl(MxirFnDecl {
                name,
                body: mxir_body_ref,
            }),
        );

        self.env.pop_scope();

        fn_decl_ref
    }

    fn emit(&mut self, ast_node: AstNodeRef, data: MxirNodeData) -> MxirNodeRef {
        let self_ref = MxirNodeRef(self.mxir.0.len() as u32);
        let mxir_node = MxirNode {
            self_ref,
            ast_node,
            data,
        };
        self.mxir.0.push(mxir_node);
        self_ref
    }

    fn emit_nop(&mut self, ast_node: AstNodeRef, msg: &str) -> MxirNodeRef {
        let self_ref = MxirNodeRef(self.mxir.0.len() as u32);
        let mxir_node = MxirNode {
            self_ref,
            ast_node,
            data: MxirNodeData::Nop(msg.to_string()),
        };
        self.mxir.0.push(mxir_node);
        self_ref
    }

    fn report(&mut self, node_ref: AstNodeRef, diag_kind: DiagnosticKind) {
        let node = self.node(node_ref);
        let diag = Diagnostic {
            path: self.file.path().to_string(),
            range: node.range,
            kind: diag_kind,
        };
        self.diagnostics.push(diag);
    }

    fn node(&self, node_ref: AstNodeRef) -> AstNode {
        self.file.node(node_ref).expect("Node not found")
    }

    fn node_range(&self, node_ref: AstNodeRef) -> Range {
        self.node(node_ref).range
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::source_file::UnparsedSourceFile;

    fn analyze_source(source: &str) -> (Mxir, Vec<Diagnostic>) {
        // Parse the source into a parsed source file
        let unparsed = UnparsedSourceFile::new("test.mx", source);
        let parsed = unparsed.parse();

        // Perform semantic analysis
        let sema = Sema::new(&parsed);
        sema.analyze()
    }

    #[test]
    fn test_simple_function_declaration() {
        let source = "fn main(): 0 { }";
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics - main function exists
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_missing_main_function() {
        let source = "fn other(): 0 { }";
        let (_, diagnostics) = analyze_source(source);

        // Should report a missing entrypoint function
        assert_eq!(diagnostics.len(), 1);
        assert_eq!(
            diagnostics[0].kind,
            DiagnosticKind::MissingEntrypointFunction
        );
    }

    #[test]
    fn test_function_with_return_type() {
        let source = "fn test(): 42 { }";
        let (_, diagnostics) = analyze_source(source);

        // The return type is a comptime int, which should parse successfully
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_const_declaration() {
        let source = r#"
            const MY_CONST: 42 = 42;
            fn main(): 0 { }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_var_declaration() {
        let source = r#"
            fn main(): 0 {
                var x: 42 = 10;
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_nested_blocks() {
        let source = r#"
            fn main(): 0 {
                {
                    var x: 42 = 10;
                    {
                        var y: 42 = 20;
                    }
                }
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_multiple_function_declarations() {
        let source = r#"
            fn helper(): 0 {
                // Helper function
            }

            fn main(): 0 {
                // Main function
            }

            fn another(): 0 {
                // Another function
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics - main function exists
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_combined_declarations() {
        let source = r#"
            const PI: 42 = 3;

            fn calculate(): 0 {
                var radius: 42 = 5;
                var area: 42 = 0;
            }

            fn main(): 0 {
                var value: 42 = 10;
                {
                    var temp: 42 = 20;
                }
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics - main function exists
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_function_in_function() {
        let source = r#"
            fn main(): 0 {
                fn inner(): 0 {
                    // This is not allowed in MX - functions should be at top level
                }
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // The current implementation doesn't check for nested functions,
        // but this test is added for when that validation is implemented
        // For now, this will pass without diagnostics
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_complex_return_type() {
        let source = r#"
            fn complex(): 42 {
                // Function with complex return type
            }

            fn main(): 0 {
                // Main function
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_comptime_expressions() {
        let source = r#"
            const TYPE: 42 = 42;

            fn main(): TYPE {
                // Main function with comptime return type
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // The current implementation doesn't fully resolve comptime expressions,
        // so this should pass without diagnostics
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_environment_scoping() {
        let source = r#"
            const GLOBAL: 42 = 100;

            fn main(): 0 {
                const LOCAL: 42 = 200;
                {
                    const NESTED: 42 = 300;
                }
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }

    #[test]
    fn test_function_with_body() {
        let source = r#"
            fn main(): 0 {
                var x: 42 = 10;
                var y: 42 = 20;
                var z: 42 = 30;

                // Add some nesting
                {
                    var inner: 42 = 40;
                    {
                        var deeplyNested: 42 = 50;
                    }
                }
            }
        "#;
        let (_, diagnostics) = analyze_source(source);

        // Should not report any diagnostics
        assert!(
            diagnostics.is_empty(),
            "Unexpected diagnostics: {:?}",
            diagnostics
        );
    }
}
