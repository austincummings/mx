use crate::{ast::AstNodeRef, comptime::ComptimeValue};

#[derive(Debug, Clone)]
pub struct Mxir(pub Vec<MxirNode>);

#[derive(Debug, Clone, Copy)]
pub struct MxirNodeRef(pub u32);

#[derive(Debug, Clone)]
pub struct MxirNode {
    pub self_ref: MxirNodeRef,
    pub ast_node: AstNodeRef,
    pub data: MxirNodeData,
}

#[derive(Debug, Clone)]
pub enum MxirNodeData {
    SourceFile(MxirBlock),
    Nop(String),

    // Declarations
    BuiltinFnDecl(MxirBuiltinFnDecl),
    FnDecl(MxirFnDecl),
    VarDecl(MxirVarDecl),

    // Statements
    ExprStmt(MxirNodeRef),
    Return(MxirReturn),
    Loop(MxirLoop),
    If(MxirIf),
    Break,
    Continue,
    Assign(MxirNodeRef, MxirNodeRef),

    // Expressions
    Block(MxirBlock),
    IntLiteral(MxirIntLiteral),
    FloatLiteral(MxirFloatLiteral),
    StringLiteral(MxirStringLiteral),
    BoolLiteral(MxirBoolLiteral),
    VarExpr(MxirVarExpr),
    CallExpr(MxirCallExpr),
}

#[derive(Debug, Clone)]
pub struct MxirReturn(pub Option<MxirNodeRef>);

#[derive(Debug, Clone)]
pub struct MxirLoop(pub Option<MxirNodeRef>);

#[derive(Debug, Clone)]
pub struct MxirBuiltinFnDecl {
    pub name: String,
    pub fn_: fn() -> (),
}

#[derive(Debug, Clone)]
pub struct MxirFnDecl {
    pub name: String,
    pub body: MxirNodeRef,
}

#[derive(Debug, Clone)]
pub struct MxirVarDecl {
    pub name: String,
    pub ty: Option<ComptimeValue>,
    pub value: Option<MxirNodeRef>,
}

#[derive(Debug, Clone)]
pub struct MxirBlock(pub Vec<MxirNodeRef>);

#[derive(Debug, Clone)]
pub struct MxirIntLiteral {
    pub value: i128,
}

#[derive(Debug, Clone)]
pub struct MxirFloatLiteral {
    pub value: f64,
}

#[derive(Debug, Clone)]
pub struct MxirStringLiteral {
    pub value: String,
}

#[derive(Debug, Clone)]
pub struct MxirBoolLiteral {
    pub value: bool,
}

#[derive(Debug, Clone)]
pub struct MxirVarExpr {
    pub name: String,
}

#[derive(Debug, Clone)]
pub struct MxirCallExpr {
    pub fn_decl_ref: MxirNodeRef,
    pub args: Vec<MxirNodeRef>,
}

#[derive(Debug, Clone)]
pub struct MxirIf {
    pub condition: MxirNodeRef,
    pub then_branch: MxirNodeRef,
    pub else_branch: Option<MxirNodeRef>,
}
