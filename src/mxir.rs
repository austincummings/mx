/// MXIR is a tree-like intermediate representation used by the Sema stage of
/// the compiler.
///
/// Compile time expressions are resolved to MXIR nodes, e.g. the literal 1 is emitted as a
/// ComptimeInt node with the value 1.
pub struct MXIR(pub Vec<MXIRNode>);

pub struct MXIRNodeRef(u32);
pub struct TSTreeNodeRef(u32);

pub struct MXIRNode {
    ts_node: TSTreeNodeRef,
    inner: MXIRNodeInner,
}

pub enum MXIRNodeInner {
    Struct,
    Fn,
    Var,

    // Statements
    ExprStmt(MXIRNodeRef),
    Break,
    Continue,
    Return(MXIRNodeRef),
    If(MXIRNodeRef, MXIRNodeRef, Option<MXIRNodeRef>),
    Loop(MXIRNodeRef),
    Assign(MXIRNodeRef, MXIRNodeRef),

    // Expressions
    Block(Vec<MXIRNodeRef>),
    Group(MXIRNodeRef),
    Call(MXIRNodeRef, Vec<MXIRNodeRef>),
    Variable(String),
    MemberAccess(MXIRNodeRef, String),
    ComptimeInt(i64),
    ComptimeFloat(f64),
    ComptimeBool(bool),
    ComptimeString(String),
}
