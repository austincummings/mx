#ifndef _MX_AST_H
#define _MX_AST_H

typedef enum {
    AST_NODE_SOURCE_FILE,
    AST_NODE_FN_DECL,
    AST_NODE_STRUCT_DECL,

} AstNodeKind;

typedef struct {
    AstNodeKind kind;
    union {
        struct {
            const char *name;
        } source_file;
        struct {
            const char *name;
        } fn_decl;
    };
} AstNode;

typedef struct {
    const char *src; // Owned by the permanent arena
    AstNode nodes[];
} Ast;

#endif // _MX_AST_H
