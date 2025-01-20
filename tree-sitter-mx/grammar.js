const PREC = {
  group: 21,
  member: 19,
  static_call: 18,
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

  conflicts: $ => [],

  rules: {
    module: $ => repeat(choice($._decl, $._stmt)),

    // Declarations

    _decl: $ => seq(choice($.fn_decl, $.var_decl, $.static_decl, $.struct_decl)),

    fn_decl: $ => seq(
      "fn",
      field("name", $.identifier),
      optional(seq("[", field("static_parameters", optional($.parameter_list)), "]")),
      "(",
      field("parameters", optional($.parameter_list)),
      ")",
      ":",
      field("return_type", $.static_expr),
      field("body", $.block),
    ),

    var_decl: $ => seq(
      "var",
      field("name", $.identifier),
      optional(seq(":", field("type", $.static_expr))),
      optional(seq("=", field("value", $._expr))),
      ";",
    ),

    static_decl: $ => seq(
      "static",
      field("name", $.identifier),
      optional(seq(":", field("type", $.static_expr))),
      seq("=", field("value", $.static_expr)),
      ";",
    ),

    struct_decl: $ => seq(
      "struct",
      field("name", $.identifier),
      optional(seq("[", field("static_parameters", $.parameter_list), "]")),
      field("body", $.block)
    ),

    // Statements

    _stmt: $ => choice($.break_stmt, $.continue_stmt, $.return_stmt, $.if_stmt, $.loop_stmt, $.for_stmt, $.assign_stmt, $.expr_stmt),

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

    for_stmt: $ => seq(
      "for",
      field("ident", $.identifier),
      "in",
      field("expr", $._expr),
      field("body", $.block)
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
      $.static_call_expr,
      $.range_expr,
      $._primary_expr,
    ),

    static_expr: $ => field("expr", choice(
      $.unary_expr,
      $.binary_expr,
      $.static_call_expr,
      $.range_expr,
      $._primary_expr,
    )),

    _primary_expr: $ => choice(
      $.int_literal,
      $.float_literal,
      $.string_literal,
      $.multiline_string_literal,
      $.bool_literal,
      $.list_literal,
      $.map_literal,
      $.identifier,
      $.ellipsis_expr,
      $.call_expr,
      $.member_expr,
      $.struct_expr,
      $.group_expr,
      $.block,
    ),

    member_expr: $ => prec(PREC.member, seq(
      field("expr", $._expr),
      ".",
      field("member", $.identifier),
    )),

    struct_expr: $ => prec(PREC.struct, seq(
      "new",
      field("name", optional($.identifier)),
      "{",
      field("fields", optional($.struct_field_expr_list)),
      "}",
    )),

    struct_field_expr_list: $ => seq(
      $.struct_field_expr,
      repeat(seq(",", $.struct_field_expr)),
      optional(","),
    ),

    struct_field_expr: $ => seq(
      field("name", $.identifier),
      optional(seq(":", field("value", $._expr)))
    ),

    group_expr: $ => prec(PREC.group, seq("(", $._expr, ")")),

    range_expr: $ => prec(PREC.range, seq(field("from", $._primary_expr), "to", field("to", $._primary_expr))),

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

    static_call_expr: $ => prec.right(PREC.static_call, seq(
      field("callee", choice($._primary_expr)),
      seq("[", field("static_arguments", $.expr_list), "]"),
    )),

    call_expr: $ => prec(PREC.call, seq(
      field("callee", choice($.static_call_expr, $._expr)),
      "(",
      field("arguments", optional($.expr_list)),
      ")",
    )),

    ellipsis_expr: _ => "...",

    block: $ => seq(
      "{",
      field("stmts", repeat(choice($._stmt, $._decl))),
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

    list_literal: $ => seq("[", field("exprs", optional(seq(
      $._expr,
      repeat(seq(",", $._expr)))
    )), "]"),

    map_literal: $ => seq(
      "map",
      "[",
      field("pairs", optional(seq($.kv_pair, repeat(seq(",", $.kv_pair))))),
      "]"
    ),

    kv_pair: $ => seq(field("key", $._expr), ":", field("value", $._expr)),

    // Misc

    line_comment: $ => token(seq("//", /.*/)),

    parameter_list: $ => seq(
      $.parameter,
      repeat(seq(",", $.parameter)),
      optional(","),
    ),

    parameter: $ => seq(
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
