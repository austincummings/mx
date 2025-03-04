mod server;

use mx::{sema::Sema, source_file::UnparsedSourceFile};
use server::MXLanguageServer;
use std::io::Read as _;
use std::io::*;
use tower_lsp::{LspService, Server};

#[tokio::main]
async fn main() {
    // Check first arg to get the command
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <command>", args[0]);
        std::process::exit(1);
    }

    match args[1].as_str() {
        "server" => {
            let stdin = tokio::io::stdin();
            let stdout = tokio::io::stdout();

            let (service, socket) = LspService::new(|client| MXLanguageServer::new(client));
            Server::new(stdin, stdout, socket).serve(service).await;
        }
        "compile" => {
            // Read all of stdin into a string
            let mut input = String::new();
            stdin()
                .read_to_string(&mut input)
                .expect("Failed to read from stdin");
            let src_file = UnparsedSourceFile::new("/dev/stdin".into(), input.as_str());
            let parsed_src_file = src_file.parse();

            let sema = Sema::new(&parsed_src_file);
            let _analyzed_file = sema.analyze();
        }
        "version" => {
            let version = env!("CARGO_PKG_VERSION");
            println!("{}", version);
        }
        _ => {
            eprintln!("Unknown command: {}", args[1]);
            std::process::exit(1);
        }
    }
}
