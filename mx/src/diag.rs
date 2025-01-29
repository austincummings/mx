use std::fmt::Display;

#[derive(Debug, Copy, Clone)]
pub struct Position {
    pub row: usize,
    pub col: usize,
}

#[derive(Debug, Copy, Clone)]
pub struct Range {
    pub start: Position,
    pub end: Position,
}

impl Display for Range {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{}:{}-{}:{}",
            self.start.row, self.start.col, self.end.row, self.end.col
        )
    }
}

#[derive(Debug, Clone)]
pub struct Diagnostic {
    pub path: String,
    pub range: Range,
    pub kind: DiagnosticKind,
}

#[derive(Clone, Debug)]
pub enum DiagnosticKind {
    SyntaxError,
    SyntaxErrorExpectedToken(String),
}

impl DiagnosticKind {
    pub fn message(&self) -> String {
        match self {
            DiagnosticKind::SyntaxError => "Syntax error".to_string(),
            DiagnosticKind::SyntaxErrorExpectedToken(token) => {
                format!("Syntax error, expected token: {}", token)
            }
        }
    }
}
