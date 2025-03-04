use std::fmt::Display;

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct Point {
    pub row: usize,
    pub col: usize,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct Range {
    pub start: Point,
    pub end: Point,
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

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Diagnostic {
    pub path: String,
    pub range: Range,
    pub kind: DiagnosticKind,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum DiagnosticKind {
    MissingEntrypointFunction,
    MissingFunctionName,
    DuplicateDefinition,
    DuplicateParamName,
    SymbolNotFound(String),
    SyntaxError,
    SyntaxErrorExpectedToken(String),
}

impl DiagnosticKind {
    pub fn message(&self) -> String {
        match self {
            DiagnosticKind::MissingEntrypointFunction => "Missing entrypoint function".to_string(),
            DiagnosticKind::MissingFunctionName => "Missing function name".to_string(),
            DiagnosticKind::DuplicateDefinition => "Duplicate definition".to_string(),
            DiagnosticKind::DuplicateParamName => "Duplicate parameter name".to_string(),
            DiagnosticKind::SymbolNotFound(symbol) => format!("Symbol not found: {}", symbol),
            DiagnosticKind::SyntaxError => "Syntax error".to_string(),
            DiagnosticKind::SyntaxErrorExpectedToken(token) => {
                format!("Syntax error, expected token: {}", token)
            }
        }
    }
}
