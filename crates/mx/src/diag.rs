use crate::position::Range;

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
    InvalidFunctionCall,
    IncorrectArgumentCount,
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
            DiagnosticKind::InvalidFunctionCall => "Invalid function call".to_string(),
            DiagnosticKind::IncorrectArgumentCount => "Incorrect argument count".to_string(),
            DiagnosticKind::SymbolNotFound(symbol) => format!("Symbol not found: {}", symbol),
            DiagnosticKind::SyntaxError => "Syntax error".to_string(),
            DiagnosticKind::SyntaxErrorExpectedToken(token) => {
                format!("Syntax error, expected token: {}", token)
            }
        }
    }
}
