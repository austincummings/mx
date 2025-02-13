#ifndef _MX_AST_H
#define _MX_AST_H

#include "array_list.h"
#include "diag.h"
#include "loc.h"
#include "map.h"
#include <stdint.h>

typedef uint32_t AstNodeRef;

typedef struct {
    AstNodeRef node;
    bool is_present;
} OptionalAstNodeRef;

typedef ArrayList(AstNodeRef) AstNodeRefList;

typedef struct {
    AstNodeRef self_ref;
    const char *type; // Owned by the permanent arena
    MXRange range;
    const char *text; // Owned by the permanent arena
    AstNodeRefList children;
} AstNode;

typedef ArrayList(AstNode) AstNodeList;

typedef struct {
    const char *src;   // Owned by the permanent arena
    AstNodeList nodes; // Owned by the permanent arena
    MXDiagnosticList diagnostics;
} Ast;

#endif // _MX_AST_H
