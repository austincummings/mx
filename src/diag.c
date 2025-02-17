#include "diag.h"

const char *mx_diagnostic_kind_to_string(Arena *a, MXDiagnosticKind kind) {
    assert(a != NULL);

    switch (kind) {
    case MX_DIAG_UNHANDLED_BINDING:
        return "Unhandled binding";
    case MX_DIAG_SYNTAX_ERROR:
        return "Syntax error";
    case MX_DIAG_SYNTAX_ERROR_EXPECTED_END_BRACE:
        return "Syntax error: expected '}'";
    case MX_DIAG_SYNTAX_ERROR_EXPECTED_SEMICOLON:
        return "Syntax error: expected ';'";
    default:
        return "Unknown";
    }
}
