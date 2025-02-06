(line_comment) @comment

(string_literal "\"") @string
(multiline_string_literal "\"\"\"") @string
(string_fragment) @string
(multiline_string_fragment) @string
(escape_sequence) @string.escape
(int_literal) @number
(float_literal) @number

; Match identifiers to highlight them as variables
(identifier) @variable

; Call expressions
(call_expr) @function.call

; Match fn_decl name identifier to a function
(fn_decl name: (identifier) @function)

(binary_expr operator: "or" @keyword)
(binary_expr operator: "and" @keyword)

((identifier) @type
 (#match? @type "^[A-Z].*")) @type

; Match capital snake case as a constant
((identifier) @constant
  (#match? @constant "^[A-Z][A-Z_]+$"))

; Built-in types
((identifier) @variable.builtin
  (#eq? @variable.builtin "self"))

((identifier) @variable.builtin
  (#eq? @variable.builtin "comptime"))

((identifier) @variable.builtin
  (#eq? @variable.builtin "cast"))

((identifier) @variable.builtin
  (#eq? @variable.builtin "import"))

[
 "fn"
 "if"
 "else"
 "return"
 "const"
 "var"
 "struct"
 "loop"
 "break"
 "continue"
 "new"
 ; "comptime"
] @keyword

[
 "+"
 "-"
 "*"
 "/"
] @operator

[
 "("
 ")"
 "{"
 "}"
 "["
 "]"
 ","
 ";"
 ":"
] @punctuation

(string_interpolation
  "${" @string.escape
  "}" @string.escape) @embedded

[
 "true"
 "false"
] @boolean

(fn_decl name: (identifier) @function) @function

(ERROR) @error
