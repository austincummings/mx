#ifndef _MX_AST_H
#define _MX_AST_H

#include "array_list.h"
#include "diag.h"
#include "loc.h"
#include "map.h"
#include "str.h"
#include <stdint.h>

typedef uint32_t AstNodeRef;

typedef struct {
    AstNodeRef node;
    bool is_present;
} OptionalAstNodeRef;

typedef ArrayList(AstNodeRef) AstNodeRefList;

typedef struct {
    String *type;
    String *text;

    AstNodeRef self_ref;
    MXRange range;

    AstNodeRefList *children;
    HashMap *named_children;
} AstNode;

typedef ArrayList(AstNode) AstNodeList;

typedef struct {
    const char *src;   // Owned by the permanent arena
    AstNodeList nodes; // Owned by the permanent arena
    MXDiagnosticList diagnostics;
} Ast;

#endif // _MX_AST_H
