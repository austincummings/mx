use std::collections::HashMap;

use crate::position::Range;

#[derive(Debug, Clone)]
pub struct Ast(pub Vec<AstNode>);

impl Default for Ast {
    fn default() -> Self {
        Self::new()
    }
}

impl Ast {
    pub fn new() -> Self {
        Self(Vec::new())
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct AstNodeRef(pub u32);

#[derive(Debug, Clone)]
pub struct AstNode {
    pub self_ref: AstNodeRef,
    pub kind: String,
    pub range: Range,
    pub text: String,
    pub named_children: HashMap<String, AstNodeRef>,
    pub children: Vec<AstNodeRef>,
}
