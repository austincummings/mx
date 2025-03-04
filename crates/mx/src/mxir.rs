use crate::ast::AstNodeRef;

#[derive(Debug, Clone)]
pub struct Mxir(pub Vec<MxirNode>);

#[derive(Debug, Clone)]
pub struct MxirNodeRef(pub u32);

#[derive(Debug, Clone)]
pub struct MxirNode {
    self_ref: MxirNodeRef,
    ast_node: AstNodeRef,
    data: MxirNodeData,
}

#[derive(Debug, Clone)]
pub enum MxirNodeData {
    Nop,

    // Declarations
    Fn(MxirFn),
    Var(MxirVar),

    // Statements
    ExprStmt(MxirNodeRef),
    ReturnStmt(MxirNodeRef),
    // Expressions
}

#[derive(Debug, Clone)]
pub struct MxirFn {
    name: String,
}

#[derive(Debug, Clone)]
pub struct MxirVar {
    name: String,
}
