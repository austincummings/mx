#ifndef _MX_DIAG_H
#define _MX_DIAG_H

#include "array_list.h"
#include "loc.h"

typedef enum {
    MX_DIAG_SYNTAX_ERROR,
} MXDiagnosticKind;

typedef struct {
    MXRange range;
    MXDiagnosticKind kind;
} MXDiagnostic;

typedef ArrayList(MXDiagnostic) MXDiagnosticList;

#endif // _MX_DIAG_H
