#[derive(Debug, Clone)]
pub struct MXPosition {
    pub row: usize,
    pub col: usize,
}

#[derive(Debug, Clone)]
pub struct MXRange {
    pub start: MXPosition,
    pub end: MXPosition,
}

#[derive(Debug, Clone)]
pub struct MXDiagnostic {
    pub range: MXRange,
    pub kind: MXDiagnosticKind,
}

#[derive(Clone, Debug)]
pub enum MXDiagnosticKind {
    SyntaxError,
    MissingMainFunction,
    MissingComptimeParam(String),
}

impl MXDiagnosticKind {
    pub fn message(&self) -> String {
        match self {
            MXDiagnosticKind::SyntaxError => "Syntax error".to_string(),
            MXDiagnosticKind::MissingMainFunction => "Missing main function".to_string(),
            MXDiagnosticKind::MissingComptimeParam(name) => {
                format!("Missing comptime param '{}'", name)
            }
        }
    }
}
