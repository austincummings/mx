const PREC = {
  group: 21,
  member: 19,
  comptime_call: 18,
  call: 17,
  unary: 14,
  range: 13,
  multiplicative: 12, // *, /, %
  additive: 11,       // +, -
  shift: 10,          // <<, >>
  binary_and: 9,      // &
  binary_xor: 8,      // ^
  binary_or: 7,       // |
  comparison: 6,      // ==, !=, <, >, <=, >=
  logical_and: 4,     // and
  logical_or: 3,      // or
  struct: 2,
};

module.exports = grammar({
  name: "mx",

  word: $ => $.identifier,

  extras: $ => [
    /\s/,
    $.line_comment,
  ],

  conflicts: $ => [[$._expr, $.comptime_expr]],

  rules: {
    source_file: $ => repeat(choice($._decl)),

    // Declarations

    _decl: $ => seq(choice($.struct_decl, $.fn_decl, $.var_decl, $.const_decl)),

    fn_decl: $ => seq(
      "fn",
      field("name", $.identifier),
      optional(seq("[", field("comptime_params", optional($.param_list)), "]")),
      "(",
      field("params", optional($.param_list)),
      ")",
      ":",
      field("return_type", $.comptime_expr),
      field("body", $.block),
    ),

    var_decl: $ => seq(
      "var",
      field("name", $.identifier),
      optional(seq(":", field("type", $.comptime_expr))),
      optional(seq("=", field("value", $._expr))),
      ";",
    ),

    const_decl: $ => seq(
      "const",
      field("name", $.identifier),
      optional(seq(":", field("type", $.comptime_expr))),
      seq("=", field("value", $.comptime_expr)),
      ";",
    ),

    struct_decl: $ => seq(
      "struct",
      field("name", $.identifier),
      optional(seq("[", field("comptime_params", $.param_list), "]")),
      field("body", $.block)
    ),

    // Statements

    _stmt: $ => choice($.break_stmt, $.continue_stmt, $.return_stmt, $.if_stmt, $.loop_stmt, $.assign_stmt, $.expr_stmt),

    expr_stmt: $ => seq(field("expr", $._expr), ";"),

    break_stmt: $ => seq("break", ";"),

    continue_stmt: $ => seq("continue", ";"),

    return_stmt: $ => seq("return", field("expr", optional($._expr)), ";"),

    if_stmt: $ => seq(
      "if",
      field("condition", $._expr),
      field("then", $.block),
      field("else", optional(seq("else", choice($.block, $.if_stmt)))),
    ),

    loop_stmt: $ => seq(
      "loop",
      field("body", $.block),
    ),

    assign_stmt: $ => seq(
      field("lhs", $._expr),
      "=",
      field("rhs", $._expr),
      ";",
    ),

    // Expressions

    _expr: $ => choice(
      $.unary_expr,
      $.binary_expr,
      $.comptime_call_expr,
      $._primary_expr,
    ),

    comptime_expr: $ => field("expr", choice(
      $.unary_expr,
      $.binary_expr,
      $.comptime_call_expr,
      $._primary_expr,
    )),

    _primary_expr: $ => choice(
      $.int_literal,
      $.float_literal,
      $.string_literal,
      $.multiline_string_literal,
      $.bool_literal,
      $.call_expr,
      $.member_expr,
      $.variable_expr,
      $.group_expr,
      $.block,
    ),

    member_expr: $ => prec(PREC.member, seq(
      field("expr", $._expr),
      ".",
      field("member", $.identifier),
    )),

    variable_expr: $ => $.identifier,

    group_expr: $ => prec(PREC.group, seq("(", $._expr, ")")),

    unary_expr: $ => prec(PREC.unary, seq(
      field("operator", choice("-", "!")),
      field("expr", $._expr),
    )),

    binary_expr: $ => choice(
      prec.left(PREC.additive, seq(field("left", $._expr), field("operator", "+"), field("right", $._expr))),
      prec.left(PREC.additive, seq(field("left", $._expr), field("operator", "-"), field("right", $._expr))),
      prec.left(PREC.multiplicative, seq(field("left", $._expr), field("operator", "*"), field("right", $._expr))),
      prec.left(PREC.multiplicative, seq(field("left", $._expr), field("operator", "/"), field("right", $._expr))),
      prec.left(PREC.logical_and, seq(field("left", $._expr), field("operator", "and"), field("right", $._expr))),
      prec.left(PREC.logical_or, seq(field("left", $._expr), field("operator", "or"), field("right", $._expr))),
      prec.left(PREC.shift, seq(field("left", $._expr), field("operator", "<<"), field("right", $._expr))),
      prec.left(PREC.shift, seq(field("left", $._expr), field("operator", ">>"), field("right", $._expr))),
      prec.left(PREC.binary_and, seq(field("left", $._expr), field("operator", "&"), field("right", $._expr))),
      prec.left(PREC.binary_xor, seq(field("left", $._expr), field("operator", "^"), field("right", $._expr))),
      prec.left(PREC.binary_or, seq(field("left", $._expr), field("operator", "|"), field("right", $._expr))),
      prec.left(PREC.comparison, seq(field("left", $._expr), field("operator", "=="), field("right", $._expr))),
      prec.left(PREC.comparison, seq(field("left", $._expr), field("operator", "!="), field("right", $._expr))),
      prec.left(PREC.comparison, seq(field("left", $._expr), field("operator", ">"), field("right", $._expr))),
      prec.left(PREC.comparison, seq(field("left", $._expr), field("operator", ">="), field("right", $._expr))),
      prec.left(PREC.comparison, seq(field("left", $._expr), field("operator", "<"), field("right", $._expr))),
      prec.left(PREC.comparison, seq(field("left", $._expr), field("operator", "<="), field("right", $._expr))),
    ),

    comptime_call_expr: $ => prec.right(PREC.comptime_call, seq(
      field("callee", choice($._primary_expr)),
      seq("[", field("comptime_arguments", $.expr_list), "]"),
    )),

    call_expr: $ => prec(PREC.call, seq(
      field("callee", $.comptime_expr),
      "(",
      field("arguments", optional($.expr_list)),
      ")",
    )),

    block: $ => seq(
      "{",
      repeat(choice($._stmt, $._decl)),
      "}",
    ),

    int_literal: $ => /\d+/,

    float_literal: $ => /\d+\.\d+/,

    string_literal: $ => seq(
      "\"",
      repeat(choice(
        $.string_fragment,
        $.string_interpolation,
        $.escape_sequence,
      )),
      "\""
    ),

    multiline_string_literal: $ => seq(
      "\"\"\"",
      repeat(choice(
        $.line_terminator,
        $.escape_sequence,
        $.string_interpolation,
        $.multiline_string_fragment
      )),
      "\"\"\""
    ),

    line_terminator: _ => choice(seq(
      /\n/,
      /\r/,
      /\\u2028/,
      /\\u2029/
    )),

    string_fragment: _ => token.immediate(/[^"\\\r\n${}]+/),

    multiline_string_fragment: _ => token.immediate(/[^"\\${}]+/),

    escape_sequence: _ => token.immediate(seq(
      '\\',
      choice(
        /[^xu0-7]/,
        /[0-7]{1,3}/,
        /x[0-9a-fA-F]{2}/,
        /u[0-9a-fA-F]{4}/,
        /u\{[0-9a-fA-F]+\}/,
        /[\r?][\n\u2028\u2029]/,
      ),
    )),

    string_interpolation: $ => seq(
      "${",
      $._expr,
      "}",
    ),

    bool_literal: $ => choice("true", "false"),

    // Misc

    line_comment: $ => token(seq("//", /.*/)),

    param_list: $ => seq(
      $.param,
      repeat(seq(",", $.param)),
      optional(","),
    ),

    param: $ => seq(
      field("name", $.identifier),
      ":",
      field("type", $._expr),
    ),

    expr_list: $ => seq(
      $._expr,
      repeat(seq(",", $._expr)),
      optional(","),
    ),

    identifier: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,
  }
});
