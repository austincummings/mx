(static_call_expr
  callee: ((identifier) @ident (#eq? @ident "c_type"))
  static_arguments: (expr_list (string_literal (string_fragment) @injection.content (#set! injection.language "c"))))

(static_call_expr
  callee: ((identifier) @ident (#eq? @ident "c_expr"))
  static_arguments: (expr_list (string_literal (string_fragment) @injection.content (#set! injection.language "c"))))

; (static_call_expr
;   callee: ((identifier) (#eq? "c"))
;   static_arguments: (expr_list (multiline_string_literal (multiline_string_fragment) @injection.content (#set! injection.language "c"))))
;
; (static_call_expr
;   callee: ((identifier) (#eq? "mx"))
;   static_arguments: (expr_list (string_literal (string_fragment) @injection.content (#set! injection.language "mx"))))
;
; (static_call_expr
;   callee: ((identifier) (#eq? "mx"))
;   static_arguments: (expr_list (multiline_string_literal (multiline_string_fragment) @injection.content (#set! injection.language "mx"))))
