#ifndef _MX_DIAG_H
#define _MX_DIAG_H

#include "array_list.h"
#include "loc.h"

typedef enum {
    MX_DIAG_UNHANDLED_BINDING,
    MX_DIAG_SYNTAX_ERROR,
    MX_DIAG_SYNTAX_ERROR_EXPECTED_END_BRACE,
    MX_DIAG_SYNTAX_ERROR_EXPECTED_SEMICOLON,
} MXDiagnosticKind;

typedef struct {
    MXRange range;
    MXDiagnosticKind kind;
} MXDiagnostic;

typedef ArrayList(MXDiagnostic) MXDiagnosticList;

const char *mx_diagnostic_kind_to_string(Arena *a, MXDiagnosticKind kind);

#endif // _MX_DIAG_H
