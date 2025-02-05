use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::Mutex;
use tower_lsp::jsonrpc::Error;
use tower_lsp::lsp_types::*;
use tower_lsp::{Client, LanguageServer};
use tree_sitter::Language;

use crate::parser::MXParser;
use crate::sema::Sema;

#[derive(Debug)]
pub struct MXLanguageServer {
    pub client: Client,
    pub documents: Arc<Mutex<HashMap<String, String>>>,

    language: Language,
}

impl MXLanguageServer {
    pub fn new(client: Client, language: Language) -> Self {
        Self {
            client,
            documents: Arc::new(Mutex::new(HashMap::new())),
            language,
        }
    }

    fn gather_diagnostics(&self, text: &str) -> Vec<Diagnostic> {
        let mut parser = MXParser::new(self.language.clone());
        parser.parse(text);

        let mut sema = Sema::new(self.language.clone(), parser.nodes.clone(), text);
        sema.analyze();

        let mut diagnostics = vec![];
        for parse_error in parser.diagnostics() {
            let range = Range {
                start: Position {
                    line: parse_error.range.start.row as u32,
                    character: parse_error.range.start.col as u32,
                },
                end: Position {
                    line: parse_error.range.end.row as u32,
                    character: parse_error.range.end.col as u32,
                },
            };
            let diag = Diagnostic {
                range,
                severity: Some(DiagnosticSeverity::ERROR),
                message: parse_error.kind.message(),
                ..Default::default()
            };

            diagnostics.push(diag);
        }

        // for sema_error in sema.diagnostics() {
        //     let range = Range {
        //         start: Position {
        //             line: sema_error.point.start.0 as u32,
        //             character: sema_error.point.start.1 as u32,
        //         },
        //         end: Position {
        //             line: sema_error.point.end.0 as u32,
        //             character: sema_error.point.end.1 as u32,
        //         },
        //     };
        //     let diag = Diagnostic {
        //         range,
        //         severity: Some(DiagnosticSeverity::ERROR),
        //         message: sema_error.kind.message(),
        //         ..Default::default()
        //     };
        //
        //     diagnostics.push(diag);
        // }

        diagnostics
    }
}

#[tower_lsp::async_trait]
impl LanguageServer for MXLanguageServer {
    async fn initialize(
        &self,
        _: tower_lsp::lsp_types::InitializeParams,
    ) -> tower_lsp::jsonrpc::Result<InitializeResult> {
        Ok(InitializeResult {
            capabilities: ServerCapabilities {
                text_document_sync: Some(TextDocumentSyncCapability::Options(
                    TextDocumentSyncOptions {
                        open_close: Some(true),
                        change: Some(TextDocumentSyncKind::FULL),
                        save: Some(TextDocumentSyncSaveOptions::SaveOptions(SaveOptions {
                            include_text: Some(true),
                        })),
                        will_save: None,
                        will_save_wait_until: None,
                    },
                )),
                hover_provider: Some(HoverProviderCapability::Simple(true)),
                ..ServerCapabilities::default()
            },
            server_info: Some(ServerInfo {
                name: "MX Language Server".into(),
                version: Some("0.1.0".into()),
            }),
        })
    }

    async fn shutdown(&self) -> tower_lsp::jsonrpc::Result<()> {
        Ok(())
    }

    async fn did_open(&self, params: DidOpenTextDocumentParams) {
        let uri = params.text_document.uri;
        let text = params.text_document.text;
        self.documents
            .lock()
            .await
            .insert(uri.to_string(), text.clone());

        let diagnostics = self.gather_diagnostics(&text);

        self.client
            .publish_diagnostics(uri, diagnostics, None)
            .await;
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        let uri = params.text_document.uri;

        let text = params
            .content_changes
            .get(0)
            .map(|change| change.text.clone())
            .unwrap_or_default();
        let diagnostics = self.gather_diagnostics(&text);

        self.client
            .publish_diagnostics(uri.clone(), diagnostics, None)
            .await;

        self.documents
            .lock()
            .await
            .insert(uri.to_string(), text.clone());
    }

    async fn did_save(&self, params: DidSaveTextDocumentParams) {
        let uri = params.text_document.uri;
        let text = params.text.unwrap_or_default();

        let diagnostics = self.gather_diagnostics(&text);

        self.client
            .publish_diagnostics(uri, diagnostics, None)
            .await;
    }

    async fn hover(&self, params: HoverParams) -> Result<Option<Hover>, Error> {
        let uri = params.text_document_position_params.text_document.uri;
        let line = params.text_document_position_params.position.line;
        let character = params.text_document_position_params.position.character + 1;

        let mut parser = MXParser::new(self.language.clone());
        let documents = self.documents.lock().await;
        let text = documents.get(&uri.to_string()).unwrap();
        parser.parse(text);

        // let node = parser.get_node_at_position(line as usize, character as usize);
        // if node.is_none() {
        //     return Ok(None);
        // }

        Ok(Some(Hover {
            contents: HoverContents::Scalar(MarkedString::String(format!(
                "## Location\n{} {}:{}\n\n## Syntax Type\n`{}`",
                uri.to_string(),
                line + 1,
                character + 1,
                "idk", //node.unwrap().kind()
            ))),
            range: None,
        }))
    }
}
