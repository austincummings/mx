#[derive(Debug, Clone)]
pub struct MXLocation {
    pub start: (usize, usize),
    pub end: (usize, usize),
}

#[derive(Debug, Clone)]
pub struct MXDiagnostic {
    pub location: MXLocation,
    pub severity: MXDiagnosticSeverity,
    pub kind: MXDiagnosticKind,
}

#[derive(Clone, Debug)]
pub enum MXDiagnosticSeverity {
    Error,
    Warning,
    Info,
}

#[derive(Clone, Debug)]
pub enum MXDiagnosticKind {
    SyntaxError,
    MissingMainFunction,
}

impl MXDiagnosticKind {
    pub fn message(&self) -> String {
        match self {
            MXDiagnosticKind::SyntaxError => "Syntax error".to_string(),
            MXDiagnosticKind::MissingMainFunction => "Missing main function".to_string(),
        }
    }
}
