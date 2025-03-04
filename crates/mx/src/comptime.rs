use crate::{ast::AstNodeRef, diag::Range, symbol_table::SymbolTableSet};

#[derive(Debug, Clone)]
pub enum ComptimeValue {
    Undefined,
    FnDecl(Box<FnDecl>),
    FnProto(Box<FnProto>),
    VarDecl(Box<VarDecl>),
    ComptimeInt(i128),
    ComptimeFloat(f64),
    ComptimeString(String),
    ComptimeBool(bool),
    Type,
}

#[derive(Debug, Clone)]
pub struct FnDecl {
    pub name: String,
    pub comptime_params: Vec<ParamDecl>,
    pub params: Vec<ParamDecl>,
    pub return_type: ComptimeValue,
    pub body_ref: AstNodeRef,
}

#[derive(Debug, Clone)]
pub struct FnProto {
    pub comptime_params: Vec<ParamDecl>,
    pub params: Vec<ParamDecl>,
    pub return_type: ComptimeValue,
}

#[derive(Debug, Clone)]
pub struct ParamDecl {
    pub name: String,
    pub ty: ComptimeValue,
}

#[derive(Debug, Clone)]
pub struct VarDecl {
    pub name: String,
    pub ty: Option<ComptimeValue>,
    pub value_ref: AstNodeRef,
}

#[derive(Debug, Clone)]
pub struct Expr(pub AstNodeRef);

#[derive(Debug, Clone)]
pub struct ComptimeBinding {
    pub node_ref: AstNodeRef,
    pub ty: Option<ComptimeValue>,
    pub value: ComptimeValue,
}

#[derive(Debug, Clone)]
pub struct ComptimeEnv(pub SymbolTableSet<ComptimeBinding, Range>);

impl ComptimeEnv {
    pub fn new() -> Self {
        ComptimeEnv(SymbolTableSet::new())
    }

    pub fn push_scope(&mut self, range: Range) {
        self.0.push_table(range);
    }

    pub fn pop_scope(&mut self) {
        self.0.pop_table();
    }

    pub fn get(&self, name: &str) -> Option<&ComptimeBinding> {
        self.0.get(name)
    }

    pub fn lookup(&self, name: &str) -> Option<&ComptimeBinding> {
        self.0.lookup(name)
    }

    pub fn declare_fn(
        &mut self,
        node_ref: AstNodeRef,
        name: &str,
        comptime_params: Vec<ParamDecl>,
        params: Vec<ParamDecl>,
        return_type: ComptimeValue,
        body_ref: AstNodeRef,
    ) -> Result<(), &'static str> {
        if self.0.get(name).is_some() {
            return Err("Duplicate declaration");
        }

        self.0.insert(
            name,
            ComptimeBinding {
                node_ref,
                ty: None,
                value: ComptimeValue::FnDecl(Box::new(FnDecl {
                    name: name.to_string(),
                    comptime_params,
                    params,
                    return_type,
                    body_ref,
                })),
            },
        );

        Ok(())
    }

    pub fn declare_const(
        &mut self,
        node_ref: AstNodeRef,
        name: &str,
        ty: Option<ComptimeValue>,
        value: ComptimeValue,
    ) -> Result<(), &'static str> {
        if self.0.get(name).is_some() {
            return Err("Duplicate declaration");
        }

        self.0.insert(
            name,
            ComptimeBinding {
                node_ref,
                ty,
                value,
            },
        );

        Ok(())
    }

    pub fn declare_var(
        &mut self,
        node_ref: AstNodeRef,
        name: &str,
        ty: Option<ComptimeValue>,
        value_ref: AstNodeRef,
    ) -> Result<(), &'static str> {
        if self.0.get(name).is_some() {
            return Err("Duplicate declaration");
        }

        self.0.insert(
            name,
            ComptimeBinding {
                node_ref,
                ty: ty.clone(),
                value: ComptimeValue::VarDecl(Box::new(VarDecl {
                    name: name.to_string(),
                    ty,
                    value_ref,
                })),
            },
        );

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ast::AstNodeRef;
    use crate::diag::{Point, Range};

    fn make_range(start: usize, end: usize) -> Range {
        Range {
            start: Point { row: start, col: 0 },
            end: Point { row: end, col: 0 },
        }
    }

    #[test]
    fn test_push_pop_scope() {
        let mut env = ComptimeEnv::new();

        // We'll test scope management by declaring and looking up values
        env.push_scope(make_range(0, 10));
        env.declare_const(AstNodeRef(1), "test1", None, ComptimeValue::ComptimeInt(1));

        env.push_scope(make_range(2, 8));
        env.declare_const(AstNodeRef(2), "test2", None, ComptimeValue::ComptimeInt(2));

        // Both values should be accessible
        assert!(env.lookup("test1").is_some());
        assert!(env.lookup("test2").is_some());

        env.pop_scope();

        // After popping, only test1 should be accessible
        assert!(env.lookup("test1").is_some());
        assert!(env.lookup("test2").is_none());

        env.pop_scope();

        // After popping again, neither should be accessible
        assert!(env.lookup("test1").is_none());
        assert!(env.lookup("test2").is_none());
    }

    #[test]
    fn test_declare_fn() {
        let mut env = ComptimeEnv::new();
        env.push_scope(make_range(0, 100));

        let fn_node_ref = AstNodeRef(1);
        let body_ref = AstNodeRef(2);
        let return_type = ComptimeValue::Type;

        env.declare_fn(
            fn_node_ref,
            "test_fn",
            vec![],
            vec![],
            return_type,
            body_ref,
        );

        let binding = env.get("test_fn").unwrap();
        if let ComptimeValue::FnDecl(fn_decl) = &binding.value {
            assert_eq!(fn_decl.name, "test_fn");
            // Check that the body_ref is the same we passed
            assert!(matches!(fn_decl.body_ref, AstNodeRef(2)));
            // Check the return type is Type
            assert!(matches!(fn_decl.return_type, ComptimeValue::Type));
        } else {
            panic!("Expected FnDecl");
        }
    }

    #[test]
    fn test_declare_const() {
        let mut env = ComptimeEnv::new();
        env.push_scope(make_range(0, 100));

        let const_node_ref = AstNodeRef(3);
        let const_type = Some(ComptimeValue::Type);
        let const_value = ComptimeValue::ComptimeInt(42);

        env.declare_const(const_node_ref, "MY_CONST", const_type, const_value);

        let binding = env.get("MY_CONST").unwrap();
        assert!(matches!(binding.node_ref, AstNodeRef(3)));

        // Check the type is Type
        if let Some(ty) = &binding.ty {
            assert!(matches!(ty, ComptimeValue::Type));
        } else {
            panic!("Expected Some type");
        }

        // Check the value is 42
        if let ComptimeValue::ComptimeInt(val) = &binding.value {
            assert_eq!(*val, 42);
        } else {
            panic!("Expected ComptimeInt");
        }
    }

    #[test]
    fn test_declare_var() {
        let mut env = ComptimeEnv::new();
        env.push_scope(make_range(0, 100));

        let var_node_ref = AstNodeRef(4);
        let var_value_ref = AstNodeRef(5);
        let var_type = Some(ComptimeValue::Type);

        env.declare_var(var_node_ref, "my_var", var_type, var_value_ref);

        let binding = env.get("my_var").unwrap();
        assert!(matches!(binding.node_ref, AstNodeRef(4)));

        // Check the type
        if let Some(ty) = &binding.ty {
            assert!(matches!(ty, ComptimeValue::Type));
        } else {
            panic!("Expected Some type");
        }

        // Check the value is a VarDecl with the expected properties
        if let ComptimeValue::VarDecl(var_decl) = &binding.value {
            assert_eq!(var_decl.name, "my_var");
            assert!(matches!(var_decl.value_ref, AstNodeRef(5)));

            if let Some(ty) = &var_decl.ty {
                assert!(matches!(ty, ComptimeValue::Type));
            } else {
                panic!("Expected Some type in VarDecl");
            }
        } else {
            panic!("Expected VarDecl");
        }
    }

    #[test]
    fn test_lookup_across_scopes() {
        let mut env = ComptimeEnv::new();

        // Outer scope
        env.push_scope(make_range(0, 100));
        env.declare_const(AstNodeRef(1), "outer", None, ComptimeValue::ComptimeInt(1));

        // Inner scope
        env.push_scope(make_range(10, 90));
        env.declare_const(AstNodeRef(2), "inner", None, ComptimeValue::ComptimeInt(2));

        // Test lookup from inner scope
        let outer_binding = env.lookup("outer").unwrap();
        if let ComptimeValue::ComptimeInt(val) = &outer_binding.value {
            assert_eq!(*val, 1);
        } else {
            panic!("Expected ComptimeInt");
        }

        let inner_binding = env.lookup("inner").unwrap();
        if let ComptimeValue::ComptimeInt(val) = &inner_binding.value {
            assert_eq!(*val, 2);
        } else {
            panic!("Expected ComptimeInt");
        }

        // Pop inner scope
        env.pop_scope();

        // Outer variable should still be accessible
        let outer_binding = env.lookup("outer").unwrap();
        if let ComptimeValue::ComptimeInt(val) = &outer_binding.value {
            assert_eq!(*val, 1);
        } else {
            panic!("Expected ComptimeInt");
        }

        // Inner variable should not be accessible
        assert!(env.lookup("inner").is_none());
    }

    #[test]
    fn test_shadowing() {
        let mut env = ComptimeEnv::new();

        // Outer scope
        env.push_scope(make_range(0, 100));
        env.declare_const(AstNodeRef(1), "x", None, ComptimeValue::ComptimeInt(1));

        // Inner scope
        env.push_scope(make_range(10, 90));
        env.declare_const(AstNodeRef(2), "x", None, ComptimeValue::ComptimeInt(2));

        // Should get inner x
        let x_binding = env.lookup("x").unwrap();
        if let ComptimeValue::ComptimeInt(val) = &x_binding.value {
            assert_eq!(*val, 2);
        } else {
            panic!("Expected ComptimeInt");
        }

        // Pop inner scope
        env.pop_scope();

        // Should get outer x
        let x_binding = env.lookup("x").unwrap();
        if let ComptimeValue::ComptimeInt(val) = &x_binding.value {
            assert_eq!(*val, 1);
        } else {
            panic!("Expected ComptimeInt");
        }
    }

    #[test]
    fn test_comptime_value_variants() {
        // Test FnDecl
        let fn_decl = ComptimeValue::FnDecl(Box::new(FnDecl {
            name: "test_fn".to_string(),
            comptime_params: vec![],
            params: vec![],
            return_type: ComptimeValue::Type,
            body_ref: AstNodeRef(1),
        }));

        // Test VarDecl
        let var_decl = ComptimeValue::VarDecl(Box::new(VarDecl {
            name: "test_var".to_string(),
            ty: Some(ComptimeValue::Type),
            value_ref: AstNodeRef(2),
        }));

        // Test primitive values
        let int_val = ComptimeValue::ComptimeInt(42);
        let float_val = ComptimeValue::ComptimeFloat(3.14);
        let string_val = ComptimeValue::ComptimeString("hello".to_string());
        let bool_val = ComptimeValue::ComptimeBool(true);
        let type_val = ComptimeValue::Type;

        // Use pattern matching to check variant types
        assert!(matches!(fn_decl, ComptimeValue::FnDecl(_)));
        assert!(matches!(var_decl, ComptimeValue::VarDecl(_)));
        assert!(matches!(int_val, ComptimeValue::ComptimeInt(42)));
        assert!(matches!(float_val, ComptimeValue::ComptimeFloat(_)));
        assert!(matches!(string_val, ComptimeValue::ComptimeString(_)));
        assert!(matches!(bool_val, ComptimeValue::ComptimeBool(true)));
        assert!(matches!(type_val, ComptimeValue::Type));

        // For float, need more specific check
        if let ComptimeValue::ComptimeFloat(val) = float_val {
            assert!((val - 3.14).abs() < f64::EPSILON);
        }

        // For string, check the content
        if let ComptimeValue::ComptimeString(val) = string_val {
            assert_eq!(val, "hello");
        }
    }

    #[test]
    fn test_no_cyclic_scopes() {
        let mut env = ComptimeEnv::new();

        // Push a few scopes
        env.push_scope(make_range(0, 100));
        env.push_scope(make_range(10, 90));
        env.push_scope(make_range(20, 80));

        // Add some bindings to each scope
        env.declare_const(AstNodeRef(1), "a", None, ComptimeValue::ComptimeInt(1));
        env.pop_scope();

        env.declare_const(AstNodeRef(2), "b", None, ComptimeValue::ComptimeInt(2));
        env.pop_scope();

        env.declare_const(AstNodeRef(3), "c", None, ComptimeValue::ComptimeInt(3));

        // Push a new scope
        env.push_scope(make_range(30, 70));

        // In a cyclic implementation, we might incorrectly find bindings from a popped scope
        assert!(
            env.lookup("a").is_none(),
            "Should not find binding from a popped scope"
        );
        assert!(
            env.lookup("b").is_none(),
            "Should not find binding from a popped scope"
        );

        // We should still find bindings from the current active scope chain
        assert!(
            env.lookup("c").is_some(),
            "Should find binding from parent scope"
        );

        // Add a binding with the same name to verify we're getting the most local one
        env.declare_const(AstNodeRef(4), "c", None, ComptimeValue::ComptimeInt(4));

        let c_binding = env.lookup("c").unwrap();
        if let ComptimeValue::ComptimeInt(val) = &c_binding.value {
            assert_eq!(
                *val, 4,
                "Should get the most recent binding, not from parent"
            );
        } else {
            panic!("Expected ComptimeInt");
        }

        // Final verification that scope chain is working correctly
        env.pop_scope();
        let c_binding = env.lookup("c").unwrap();
        if let ComptimeValue::ComptimeInt(val) = &c_binding.value {
            assert_eq!(*val, 3, "Should get the parent scope binding after popping");
        } else {
            panic!("Expected ComptimeInt");
        }
    }
}
