use crate::{parser::AstNodeRef, sema::ComptimeValue};

/// MXIR is a tree-like intermediate representation used by the Sema stage of
/// the compiler.
///
/// Compile time expressions are resolved to MXIR nodes, e.g. the literal 1 is emitted as a
/// ComptimeInt node with the value 1.
pub struct MXIR(pub Vec<MXIRNode>);

#[derive(Debug, Copy, Clone)]
pub struct MXIRNodeRef(pub u32);

#[derive(Debug, Clone)]
pub struct MXIRNode {
    pub self_ref: MXIRNodeRef,
    pub ast_ref: AstNodeRef,
    pub inner: MXIRNodeInner,
}

impl MXIRNode {
    pub fn nop(self_ref: MXIRNodeRef, ast_ref: AstNodeRef) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::Nop,
        }
    }

    pub fn fn_(
        self_ref: MXIRNodeRef,
        ast_ref: AstNodeRef,
        params: Vec<MXIRNodeRef>,
        body: MXIRNodeRef,
    ) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::Fn { params, body },
        }
    }

    pub fn block(self_ref: MXIRNodeRef, ast_ref: AstNodeRef, nodes: Vec<MXIRNodeRef>) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::Block(nodes),
        }
    }

    pub fn expr_stmt(self_ref: MXIRNodeRef, ast_ref: AstNodeRef, expr: MXIRNodeRef) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::ExprStmt(expr),
        }
    }

    pub fn return_stmt(self_ref: MXIRNodeRef, ast_ref: AstNodeRef, value: MXIRNodeRef) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::Return(value),
        }
    }

    pub fn call_expr(
        self_ref: MXIRNodeRef,
        ast_ref: AstNodeRef,
        callee: MXIRNodeRef,
        args: Vec<MXIRNodeRef>,
    ) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::Call { callee, args },
        }
    }

    pub fn member_expr(
        self_ref: MXIRNodeRef,
        ast_ref: AstNodeRef,
        object: MXIRNodeRef,
        member: String,
    ) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::MemberAccess(object, member),
        }
    }

    pub fn comptime_int(self_ref: MXIRNodeRef, ast_ref: AstNodeRef, value: ComptimeValue) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::ComptimeInt(value),
        }
    }

    pub fn comptime_float(
        self_ref: MXIRNodeRef,
        ast_ref: AstNodeRef,
        value: ComptimeValue,
    ) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::ComptimeFloat(value),
        }
    }

    pub fn comptime_bool(self_ref: MXIRNodeRef, ast_ref: AstNodeRef, value: ComptimeValue) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::ComptimeBool(value),
        }
    }

    pub fn comptime_string(
        self_ref: MXIRNodeRef,
        ast_ref: AstNodeRef,
        value: ComptimeValue,
    ) -> Self {
        Self {
            self_ref,
            ast_ref,
            inner: MXIRNodeInner::ComptimeString(value),
        }
    }
}

#[derive(Debug, Clone)]
pub enum MXIRNodeInner {
    Nop,
    Struct,
    Fn {
        params: Vec<MXIRNodeRef>,
        body: MXIRNodeRef,
    },
    Var,

    // Statements
    ExprStmt(MXIRNodeRef),
    Break,
    Continue,
    Return(MXIRNodeRef),
    If {
        cond: MXIRNodeRef,
        then: MXIRNodeRef,
        else_: Option<MXIRNodeRef>,
    },
    Loop(MXIRNodeRef),
    Assign {
        lhs: MXIRNodeRef,
        rhs: MXIRNodeRef,
    },

    // Expressions
    Block(Vec<MXIRNodeRef>),
    Group(MXIRNodeRef),
    Call {
        callee: MXIRNodeRef,
        args: Vec<MXIRNodeRef>,
    },
    VariableAccess(String),
    MemberAccess(MXIRNodeRef, String),
    ComptimeInt(ComptimeValue),
    ComptimeFloat(ComptimeValue),
    ComptimeBool(ComptimeValue),
    ComptimeString(ComptimeValue),
}
