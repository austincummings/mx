use std::collections::HashSet;

use crate::{
    ast::{AstNode, AstNodeRef},
    comptime::{ComptimeBinding, ComptimeEnv, ComptimeValue, FnProto, ParamDecl},
    diag::{Diagnostic, DiagnosticKind, Range},
    mxir::Mxir,
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
            self.env.push_scope(source_file_node.range);
            for child_ref in source_file_node.children {
                self.analyze_node(child_ref);
            }

            // Lookup the entry point
            if let Some(main_fn_decl) = self.env.get("main").cloned() {
                self.emit_fn(main_fn_decl, vec![]);
            } else {
                self.report(
                    source_file_node_ref,
                    DiagnosticKind::MissingEntrypointFunction,
                );
            }

            self.env.pop_scope();
            eprintln!("Scopes: {:#?}", self.env);
        }
    }

    fn analyze_node(&mut self, node_ref: AstNodeRef) {
        let node = self.node(node_ref);
        match node.kind.as_str() {
            "fn_decl" => self.analyze_fn_decl(node_ref),
            "const_decl" => self.analyze_const_decl(node_ref),
            "var_decl" => self.analyze_var_decl(node_ref),
            "block" => self.analyze_block(node_ref),
            _ => {
                eprintln!("Unsupported node type: {}", node.kind);
            }
        }
    }

    fn analyze_fn_decl(&mut self, node_ref: AstNodeRef) {
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
        let return_type = self.analyze_comptime_expr(return_type_ref);

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
                params,
                comptime_params,
                return_type,
                body_ref,
            ) {
                // Report duplicate definition
                self.report(node_ref, DiagnosticKind::DuplicateDefinition);
            }
        }
    }

    fn analyze_const_decl(&mut self, node_ref: AstNodeRef) {
        let node = self.node(node_ref);

        let name_ref = node
            .named_children
            .get("name")
            .copied()
            .expect("Constant name not found");
        let name_node = self.file.node(name_ref).expect("Name node not found");
        let name = name_node.text.as_str();

        let ty = if let Some(ty_ref) = node.named_children.get("type").copied() {
            Some(self.analyze_comptime_expr(ty_ref))
        } else {
            None
        };

        let value_ref = node
            .named_children
            .get("value")
            .copied()
            .expect("Constant value not found");
        let value = self.analyze_comptime_expr(value_ref);

        if let Err(_) = self.env.declare_const(node_ref, name, ty, value) {
            // Report duplicate definition
            self.report(node_ref, DiagnosticKind::DuplicateDefinition);
        }
    }

    fn analyze_var_decl(&mut self, node_ref: AstNodeRef) {
        let node = self.node(node_ref);

        let name_ref = node
            .named_children
            .get("name")
            .copied()
            .expect("Variable name not found");
        let name_node = self.file.node(name_ref).expect("Name node not found");
        let name = name_node.text.as_str();

        let ty = if let Some(ty_ref) = node.named_children.get("type").copied() {
            Some(self.analyze_comptime_expr(ty_ref))
        } else {
            None
        };

        let value_ref = node
            .named_children
            .get("value")
            .copied()
            .expect("Variable value not found");

        if let Err(_) = self.env.declare_var(node_ref, name, ty, value_ref) {
            // Report duplicate definition
            self.report(node_ref, DiagnosticKind::DuplicateDefinition);
        }
    }

    fn analyze_block(&mut self, node_ref: AstNodeRef) {
        let node = self.node(node_ref);

        self.env.push_scope(node.range);

        for child_ref in node.children {
            self.analyze_node(child_ref);
        }

        self.env.pop_scope();
    }

    fn analyze_comptime_expr(&mut self, node_ref: AstNodeRef) -> ComptimeValue {
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
            "fn_proto" => self.analyze_fn_proto(expr_node_ref),
            _ => panic!("Unsupported comptime expression: {}", expr_node.kind),
        }
    }

    fn analyze_fn_proto(&mut self, expr_node_ref: AstNodeRef) -> ComptimeValue {
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
        let return_type = self.analyze_comptime_expr(return_type_ref);

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
                let param_ty = self.analyze_comptime_expr(param_type_ref);

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
                let param_ty = self.analyze_comptime_expr(param_type_ref);

                let param_decl = ParamDecl {
                    name: param_name,
                    ty: param_ty,
                };
                params.push(param_decl);
            }
        }

        params
    }

    fn emit_fn(&mut self, binding: ComptimeBinding, comptime_args: Vec<(String, ComptimeValue)>) {
        // Check that it is indeed a function
        if let ComptimeValue::FnDecl(fn_decl) = &binding.value {
            // Create a new environment with the root scope as the parent
            let mut fn_env = ComptimeEnv::new();

            // Get the function node
            let fn_node_range = self.node_range(binding.node_ref);

            // Push a scope for the function
            fn_env.push_scope(fn_node_range);

            // Push a scope for the function arguments
            fn_env.push_scope(fn_node_range);

            // Assign the comptime args to the function environment
            for (name, value) in comptime_args {
                // Initialize the comptime parameter in the environment
                let _ = fn_env.declare_const(binding.node_ref, &name, None, value.clone());
            }

            fn_env.pop_scope(); // Pop the scope for the function arguments
            fn_env.pop_scope(); // Pop the scope for the function
        } else {
            panic!("Not a function");
        }
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
