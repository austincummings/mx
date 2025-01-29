use std::io::Read as _;

mod parser;

fn main() {
    // Read all of stdin into a string
    let mut input = String::new();
    std::io::stdin().read_to_string(&mut input).unwrap();

    // Instantiate the parser
    let parser = parser::Parser {};

    // Parse the input
    parser.parse(&input);

    // Return success
    std::process::exit(0);
}
