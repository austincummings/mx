use std::collections::HashMap;
use std::sync::Arc;
use tokio::sync::Mutex;
use tower_lsp::lsp_types::*;
use tower_lsp::{Client, LanguageServer};

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
                name: "Basic MX LSP".into(),
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

        let diagnostics = text
            .lines()
            .enumerate()
            .filter_map(|(line_num, line)| {
                if line.contains("TODO") {
                    Some(Diagnostic {
                        range: Range {
                            start: Position {
                                line: line_num as u32,
                                character: line.find("TODO").unwrap() as u32,
                            },
                            end: Position {
                                line: line_num as u32,
                                character: (line.find("TODO").unwrap() as u32) + 4,
                            },
                        },
                        severity: Some(DiagnosticSeverity::WARNING),
                        message: "Test".to_string(),
                        ..Diagnostic::default()
                    })
                } else {
                    None
                }
            })
            .collect::<Vec<_>>();

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

        let diagnostics = text
            .lines()
            .enumerate()
            .filter_map(|(line_num, line)| {
                if line.contains("TODO") {
                    Some(Diagnostic {
                        range: Range {
                            start: Position {
                                line: line_num as u32,
                                character: line.find("TODO").unwrap() as u32,
                            },
                            end: Position {
                                line: line_num as u32,
                                character: (line.find("TODO").unwrap() as u32) + 4,
                            },
                        },
                        severity: Some(DiagnosticSeverity::WARNING),
                        message: "Hetest".to_string(),
                        ..Diagnostic::default()
                    })
                } else {
                    None
                }
            })
            .collect::<Vec<_>>();

        self.client
            .publish_diagnostics(uri, diagnostics, None)
            .await;
    }
}
