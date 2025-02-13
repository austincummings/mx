#include "diag.h"

const char *mx_diagnostic_kind_to_string(Arena *a, MXDiagnosticKind kind) {
    assert(a != NULL);

    switch (kind) {
    case MX_DIAG_SYNTAX_ERROR:
        return "Syntax error";
    default:
        return "Unknown";
    }
}
