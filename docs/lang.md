# MX Programming Language Reference

## Syntax Specification

MX implements a C-like syntax that minimizes the usage of sigils within the
language. The language implements a transcompilation pipeline targeting the
Rust programming language as its backend, providing seamless interoperability
with the Rust ecosystem and enabling direct access to the comprehensive Rust
standard library through native bindings.

### Reserved Keywords

- `comptime`: Designates expressions to be evaluated during compilation rather
  than at runtime
- `fn`: Declares a function definition
- `var`: Initializes a mutable variable instance
- `const`: Declares an immutable compile-time constant value
- `struct`: Defines a composite data structure type
- `break`: Terminates execution of the innermost enclosing loop
- `continue`: Transfers control to the loop continuation point
- `return`: Exits a function with an optional return value
- `if`: Implements conditional control flow
- `else`: Specifies the alternative execution path in conditional statements
- `loop`: Constructs an infinite iteration structure
- `not`: Performs logical negation on an expression
- `and`: Performs logical conjunction operation
- `or`: Performs logical disjunction operation
- `new`: Allocates and initializes a struct instance
- `true`: Represents the Boolean true literal
- `false`: Represents the Boolean false literal
- `map`: Instantiates a key-value association data structure

### Statement Syntax

MX supports the following statement constructs:

- **Expression Statements**: Expressions terminated by a semicolon

```mx
print("Hello world");
```

- **Variable Declaration**: Variable instantiation with optional type annotation
  and initializer

```mx
var x: int = 42;
var y = "hello";
```

- **Assignment**: Value binding operation

```mx
x = 100;
```

- **Return Statement**: Function termination with optional value

```mx
return 42;
```

- **If Statement**: Conditional execution path selection

```mx
if x > 0 {
    do_something();
} else {
    handle_zero();
}
```

- **Loop Statement**: Unbounded iteration construct

```mx
loop {
    if done {
        break;
    }
    do_work();
}
```

- **`break` Statement**: Loop termination construct
- **`continue` Statement**: Loop iteration advancement construct

## Expression Syntax

MX implements a comprehensive expression system:

### Literal Expression Syntax

- Integer literals: `42`, `100`, etc.
- Floating-point literals: `3.14`, `1.0`, etc.
- String literals: `"hello"`, `"""multiline text"""`
- Boolean literals: `true`, `false`
- List literals: `[1, 2, 3]`
- Map literals: `map{1: "one", 2: "two"}`

### Operator Syntax

- Arithmetic operators: `+`, `-`, `*`, `/`
- Relational operators: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical operators: `and`, `or`, `not`
- Bitwise operators: `&`, `|`, `^`, `<<`, `>>`

### Additional Expression Forms

- Function invocation: `add(1, 2)`
- Compile-time function invocation: `list[T][size]`
- Struct instantiation: `new Person { name: "Bob", age: 42 }`
- Member access operator: `person.name`
- Range specification: `1 to 10`
    - Open-ended range from start: `0 to _`
    - Open-ended range to end: `_ to 10`
- Parenthesized expressions: `(1 + 2) * 3`

String literals implement interpolation via the `${expr}` syntax:

```mx
name = "Bob";
"Hello ${name}!"; // Evaluates to "Hello Bob!"
```

Expressions with compile-time known inputs may be utilized in compile-time
evaluation contexts.

### Compile-Time Expression Evaluation

Compile-time expressions undergo evaluation during compilation and can be
utilized for constant initialization and static verification.

```mx
const PI = 3.14159;
const MAX = 100;

if MAX > 0 {
    println("MAX is positive");
} else {
    println("MAX is not positive");
}
```

#### Compile-Time Function Evaluation

Compile-time functions are evaluated during compilation and facilitate constant
initialization and static analysis. These functions represent specialized
overloads of standard functions but cannot accept runtime parameters.

```mx
comptime fn factorial[n: ComptimeInt](): ComptimeInt {
    if n == 0 {
        return 1;
    } else {
        return n * factorial[n - 1]();
    }
}

const FACTORIAL_5 = factorial[5]();
```

## Identifier Naming Conventions

Consistent identifier naming is essential for code readability and
maintainability. The following conventions are specified:

- Implement `snake_case` for variable and function identifiers
- Implement `PascalCase` for type identifiers
- Implement `SCREAMING_SNAKE_CASE` for constant identifiers

## User-Defined Types

### Structure Types

Structures represent composite data types that encapsulate multiple values of
potentially different types. They facilitate the organization of related data
elements.

```mx
struct Person {
    var name: String;
    var age: Int32;
}

const bob = new Person { name: "Bob", age: 42 };
```

#### Operator Overloading Functions

Operator overloading functions are methods associated with structures or
enumerations that define the semantics of operators when applied to instances
of these types.

- `+` -> `add`
- `-` -> `sub`
- `*` -> `mul`
- `/` -> `div`
- `==` -> `eq`
- `!=` -> `neq`
- `>` -> `gt`
- `<` -> `lt`
- `>=` -> `gte`
- `<=` -> `lte`
- `|` -> `bit_or`
- `&` -> `bit_and`
- `^` -> `bit_xor`
- `and` -> `logical_and`
- `or` -> `logical_or`
- `not` -> `logical_not`
