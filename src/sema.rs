use tree_sitter::{Language, Node, Tree};

use crate::{
    diag::{MXDiagnostic, MXDiagnosticKind, MXDiagnosticSeverity, MXLocation},
    query::query,
};

#[derive(Debug)]
pub struct Sema {
    tree: Tree,
    src: String,
    language: Language,
    diagnostics: Vec<MXDiagnostic>,
}

impl Sema {
    pub fn new(language: Language, tree: Tree, src: &str) -> Self {
        Self {
            language,
            tree,
            src: src.to_string(),
            diagnostics: vec![],
        }
    }

    pub fn analyze(&mut self) {
        // Find main function
        let entrypoint_node = self.find_entrypoint_node();
        if entrypoint_node.is_none() {
            self.diagnostics.push(MXDiagnostic {
                location: MXLocation {
                    start: (0, 0),
                    end: (0, 0),
                },
                severity: MXDiagnosticSeverity::Error,
                kind: MXDiagnosticKind::MissingMainFunction,
            });
        }
    }

    fn find_entrypoint_node(&mut self) -> Option<Node> {
        let root_node = self.tree.root_node();

        // Find main function
        let query_str = r#"
            (source_file
                (fn_decl name: ((identifier) @name)
                                (#eq? @name "main")) @main
            )
        "#;
        let nodes = query(root_node, &self.src, &self.language, query_str, true);
        if nodes.contains_key("main") {
            return Some(nodes.get("main").unwrap()[0]);
        }

        None
    }

    pub fn diagnostics(&self) -> Vec<MXDiagnostic> {
        return self.diagnostics.clone();
    }
}
