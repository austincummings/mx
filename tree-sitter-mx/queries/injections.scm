(comptime_call_expr
  callee: ((identifier) @ident (#eq? @ident "c_type"))
  comptime_arguments: (expr_list (string_literal (string_fragment) @injection.content (#set! injection.language "c"))))

(comptime_call_expr
  callee: ((identifier) @ident (#eq? @ident "c_expr"))
  comptime_arguments: (expr_list (string_literal (string_fragment) @injection.content (#set! injection.language "c"))))

; (comptime_call_expr
;   callee: ((identifier) (#eq? "c"))
;   comptime_arguments: (expr_list (multiline_string_literal (multiline_string_fragment) @injection.content (#set! injection.language "c"))))
;
; (comptime_call_expr
;   callee: ((identifier) (#eq? "mx"))
;   comptime_arguments: (expr_list (string_literal (string_fragment) @injection.content (#set! injection.language "mx"))))
;
; (comptime_call_expr
;   callee: ((identifier) (#eq? "mx"))
;   comptime_arguments: (expr_list (multiline_string_literal (multiline_string_fragment) @injection.content (#set! injection.language "mx"))))
