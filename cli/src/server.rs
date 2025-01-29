use mx::source_file::SourceFile;
use std::collections::HashMap;
use std::path::Path;
use std::sync::Arc;
use tokio::sync::Mutex;
use tower_lsp::jsonrpc::Error;
use tower_lsp::lsp_types::{
    DidChangeTextDocumentParams, DidOpenTextDocumentParams, DidSaveTextDocumentParams,
    GotoDefinitionParams, GotoDefinitionResponse, Hover, HoverContents, HoverParams,
    HoverProviderCapability, InitializeParams, InitializeResult, MarkedString, OneOf, SaveOptions,
    ServerCapabilities, ServerInfo, TextDocumentSyncCapability, TextDocumentSyncKind,
    TextDocumentSyncOptions, TextDocumentSyncSaveOptions,
};
use tower_lsp::{Client, LanguageServer};

#[derive(Debug)]
pub struct MXLanguageServer {
    pub client: Client,
    pub documents: Arc<Mutex<HashMap<String, SourceFile>>>,
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
        let src_file = SourceFile::new(uri.path().to_string(), text.as_str());

        self.documents
            .lock()
            .await
            .insert(uri.to_string(), src_file);

        // TODO: Implement diagnostics gathering and publishing
    }

    async fn did_change(&self, params: DidChangeTextDocumentParams) {
        let uri = params.text_document.uri;

        let src_file = self.documents.lock().await.get(&uri.to_string()).cloned();

        if let Some(mut src_file) = src_file {
            eprintln!("Changed {}", uri.path());

            let text = params
                .content_changes
                .get(0)
                .map(|change| change.text.clone())
                .unwrap_or_default();

            src_file.update_src(&text);

            // TODO: Implement diagnostics gathering and publishing
        }
    }

    async fn did_save(&self, params: DidSaveTextDocumentParams) {
        let uri = params.text_document.uri;

        let src_file = self.documents.lock().await.get(&uri.to_string()).cloned();

        if let Some(mut src_file) = src_file {
            eprintln!("Saved {}", uri.path());

            let text = params.text.unwrap_or_default();

            src_file.update_src(&text);

            eprintln!("AST: {:#?}", src_file.ast());

            // TODO: Implement diagnostics gathering and publishing
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
