use mx::diag::Diagnostic;
use mx::source_file::{AnalyzedSourceFile, UnparsedSourceFile};
use std::collections::HashMap;
use std::path::Path;
use std::sync::Arc;
use tokio::sync::Mutex;
use tower_lsp::jsonrpc::Error;
use tower_lsp::lsp_types::{
    Diagnostic as LspDiagnostic, DidChangeTextDocumentParams, DidOpenTextDocumentParams,
    DidSaveTextDocumentParams, GotoDefinitionParams, GotoDefinitionResponse, Hover, HoverContents,
    HoverParams, HoverProviderCapability, InitializeParams, InitializeResult, MarkedString, OneOf,
    SaveOptions, ServerCapabilities, ServerInfo, TextDocumentSyncCapability, TextDocumentSyncKind,
    TextDocumentSyncOptions, TextDocumentSyncSaveOptions,
};
use tower_lsp::{Client, LanguageServer};

#[derive(Debug)]
pub struct MXLanguageServer {
    pub client: Client,
    pub files: Arc<Mutex<HashMap<String, AnalyzedSourceFile>>>,
}

impl MXLanguageServer {
    pub fn new(client: Client) -> Self {
        Self {
            client,
            files: Arc::new(Mutex::new(HashMap::new())),
        }
    }
}
impl MXLanguageServer {
    fn convert_diagnostics(&self, diagnostics: Vec<Diagnostic>) -> Vec<LspDiagnostic> {
        diagnostics
            .into_iter()
            .map(|diagnostic| LspDiagnostic {
                range: tower_lsp::lsp_types::Range {
                    start: tower_lsp::lsp_types::Position {
                        line: diagnostic.range.start.row as u32,
                        character: diagnostic.range.start.col as u32,
                    },
                    end: tower_lsp::lsp_types::Position {
                        line: diagnostic.range.end.row as u32,
                        character: diagnostic.range.end.col as u32,
                    },
                },
                severity: Some(tower_lsp::lsp_types::DiagnosticSeverity::ERROR),
                code: None,
                code_description: None,
                source: Some("mx".to_string()),
                message: diagnostic.kind.message(),
                related_information: None,
                tags: None,
                data: None,
            })
            .collect()
    }

    async fn send_diagnostics(&self, uri: tower_lsp::lsp_types::Url, diagnostics: Vec<Diagnostic>) {
        let lsp_diagnostics = self.convert_diagnostics(diagnostics);
        self.client
            .publish_diagnostics(uri, lsp_diagnostics, None)
            .await;
    }
}

#[tower_lsp::async_trait]
impl LanguageServer for MXLanguageServer {
    async fn initialize(
        &self,
        _: InitializeParams,
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
                definition_provider: Some(OneOf::Left(true)),
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
        let ext = Path::new(uri.path()).extension();

        if let Some(ext) = ext {
            if ext.to_str() != Some("mx") {
                return;
            }
        }

        eprintln!("Opened {}", uri.path());

        let text = params.text_document.text;
        let src_file = UnparsedSourceFile::new(uri.path(), text.as_str());
        let parsed_file = src_file.parse();
        let analyzed_file = parsed_file.analyze();

        self.files
            .lock()
            .await
            .insert(uri.to_string(), analyzed_file.clone());

        let diagnostics = analyzed_file.file().diagnostics.clone();
        self.send_diagnostics(uri, diagnostics).await;
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        let uri = params.text_document.uri;

        let text = params
            .content_changes.first()
            .map(|change| change.text.clone())
            .unwrap_or_default();

        let mut files = self.files.lock().await;

        if let Some(src_file) = files.get_mut(&uri.to_string()) {
            eprintln!("Changed {}", uri.path());

            src_file.update(&text);

            let diagnostics = src_file.file().diagnostics.clone();
            drop(files); // Release lock before async operation
            self.send_diagnostics(uri, diagnostics).await;
        }
    }

    async fn did_save(&self, params: DidSaveTextDocumentParams) {
        let uri = params.text_document.uri;
        let text = params.text.unwrap_or_default();

        let mut files = self.files.lock().await;

        if let Some(src_file) = files.get_mut(&uri.to_string()) {
            eprintln!("Saved {}", uri.path());

            src_file.update(&text);

            let diagnostics = src_file.file().diagnostics.clone();
            drop(files); // Release lock before async operation
            self.send_diagnostics(uri, diagnostics).await;
        }
    }

    async fn hover(&self, params: HoverParams) -> Result<Option<Hover>, Error> {
        let uri = params.text_document_position_params.text_document.uri;
        let path = Path::new(uri.path());

        if let Some(ext) = path.extension() {
            if ext.to_str() != Some("mx") {
                return Ok(None);
            }
        }
        let line = params.text_document_position_params.position.line;
        let character = params.text_document_position_params.position.character + 1;

        Ok(Some(Hover {
            contents: HoverContents::Scalar(MarkedString::String(format!(
                "URI: {}\n\nLine: {}\n\nCharacter: {}",
                uri, line, character
            ))),
            range: None,
        }))
    }

    async fn goto_definition(
        &self,
        _params: GotoDefinitionParams,
    ) -> Result<Option<GotoDefinitionResponse>, Error> {
        return Ok(None);
    }
}
