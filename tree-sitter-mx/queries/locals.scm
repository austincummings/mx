(module) @local.scope
(block) @local.scope

(fn_decl name: (identifier) @local.definition)
(struct_decl name: (identifier) @local.definition)
(parameter name: (identifier) @local.definition)
(var_decl name: (identifier) @local.definition)
(static_decl name: (identifier) @local.definition)

(identifier) @local.reference
