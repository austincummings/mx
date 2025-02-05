use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::Mutex;
use tower_lsp::lsp_types::*;
use tower_lsp::{Client, LanguageServer};

use crate::parser::MXParser;

#[derive(Debug)]
pub struct MXLanguageServer {
    pub client: Client,
    pub documents: Arc<Mutex<HashMap<String, String>>>,
}

impl MXLanguageServer {
    pub fn new(client: Client) -> Self {
        Self {
            client,
            documents: Arc::new(Mutex::new(HashMap::new())),
        }
    }

    fn gather_diagnostics(&self, text: &str) -> Vec<Diagnostic> {
        let mut parser = MXParser::new();
        let _tree = parser.parse(text);

        let mut diagnostics = vec![];
        for parse_error in parser.diagnostics() {
            let range = Range {
                start: Position {
                    line: parse_error.location().start.0 as u32,
                    character: parse_error.location().start.1 as u32,
                },
                end: Position {
                    line: parse_error.location().end.0 as u32,
                    character: parse_error.location().end.1 as u32,
                },
            };
            let diag = Diagnostic {
                range,
                severity: Some(DiagnosticSeverity::ERROR),
                message: parse_error.message(),
                ..Default::default()
            };

            diagnostics.push(diag);
        }

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
                        change: Some(TextDocumentSyncKind::INCREMENTAL),
                        save: Some(TextDocumentSyncSaveOptions::Supported(true)), // Enable save notifications
                        will_save: None,
                        will_save_wait_until: None,
                    },
                )),
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
        let uri = params.text_document.uri.to_string();
        let mut documents = self.documents.lock().await;

        if let Some(change) = params.content_changes.first() {
            documents.insert(uri, change.text.clone());
        }
    }

    async fn did_save(&self, params: DidSaveTextDocumentParams) {
        let uri = params.text_document.uri;
        let text = self
            .documents
            .lock()
            .await
            .get(&uri.to_string())
            .cloned()
            .unwrap_or_default();

        let diagnostics = self.gather_diagnostics(&text);

        self.client
            .publish_diagnostics(uri, diagnostics, None)
            .await;
    }
}
