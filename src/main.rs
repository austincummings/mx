mod diag;
mod mxir;
mod parser;
mod sema;
mod server;

use parser::Parser;
use sema::Sema;
use server::MXLanguageServer;
use std::io::*;
use std::sync::{Arc, Mutex};
use std::{collections::HashMap, io::Read as _};
use tower_lsp::{LspService, Server};

// fn main() {
//     // Read all of stdin into a string
//     let mut input = String::new();
//     stdin().read_to_string(&mut input).unwrap();
//
//     // Instantiate the parser
//     let parser = Parser::new();
//
//     // Parse the input
//     let tree = parser.parse(&input);
//
//     let mut sema = Sema::new(tree, &input);
//     sema.analyze();
//
//     // Return success
//     std::process::exit(0);
// }

#[tokio::main]
async fn main() {
    // Check first arg to get the command
    // Can be lsp, or compile-c
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <command>", args[0]);
        std::process::exit(1);
    }

    match args[1].as_str() {
        "lsp" => {
            let stdin = tokio::io::stdin();
            let stdout = tokio::io::stdout();

            let (service, socket) = LspService::new(|client| MXLanguageServer::new(client));
            Server::new(stdin, stdout, socket).serve(service).await;
        }
        "compile-c" => {
            // Read all of stdin into a string
            let mut input = String::new();
            stdin().read_to_string(&mut input).unwrap();

            // Instantiate the parser
            let parser = Parser::new();

            // Parse the input
            let tree = parser.parse(&input);

            let mut sema = Sema::new(tree, &input);
            sema.analyze();
        }
        _ => {
            eprintln!("Unknown command: {}", args[1]);
            std::process::exit(1);
        }
    }
}
