use std::collections::HashMap;

use tree_sitter::{Language, Node, Tree};

use crate::{
    diag::{MXDiagnostic, MXDiagnosticKind, MXPosition, MXRange},
    parser::{MXNode, MXNodeRef},
    query::query,
};

#[derive(Debug)]
pub struct Sema {
    src: String,
    language: Language,
    nodes: Vec<MXNode>,
    diagnostics: Vec<MXDiagnostic>,
}

impl Sema {
    pub fn new(language: Language, nodes: Vec<MXNode>, src: &str) -> Self {
        Self {
            language,
            nodes,
            src: src.to_string(),
            diagnostics: vec![],
        }
    }

    pub fn analyze(&mut self) {
        // Find main function
        let entrypoint_node = self.find_entrypoint_node();
        eprintln!("{:?}", entrypoint_node);
        if let Some(node) = entrypoint_node.clone() {
            // Check if the main function has comptime params
            // let comptime_args = HashMap::new();
            // self.comptime_instantiate_fn(node, comptime_args);
        } else {
            self.report_missing_main_function();
        }
    }

    fn find_entrypoint_node(&self) -> Option<MXNodeRef> {
        for i in 0..self.nodes.len() {
            let node = &self.nodes[i];
            if node.kind == "fn_decl" {
                eprintln!("{:?}", node);
                if node.named_children.contains_key("name") {
                    let name_node_ref = node.named_children.get("name").unwrap();
                    let name_node = self.nodes[name_node_ref.0 as usize].clone();
                    if name_node.text == "main" {
                        return Some(node.self_ref);
                    }
                }
            }
        }

        None
    }

    fn comptime_instantiate_fn(
        &mut self,
        node: Node,
        comptime_args: HashMap<String, ComptimeValue>,
    ) {
        todo!()
        // // Query the comptime_params
        // let params_query = r#"
        //     (fn_decl
        //         comptime_params: (param_list)? @comptime_params
        //     )
        // "#;
        //
        // let mut comptime_params = HashMap::new();
        //
        // let params_nodes = query(node, &self.src, &self.language, params_query, true);
        // if params_nodes.contains_key("comptime_params") {
        //     let comptime_params_node = params_nodes.get("comptime_params").unwrap()[0];
        //     // Iterate over children of the param_list
        //     for i in 0..comptime_params_node.child_count() {
        //         let param_node = comptime_params_node.child(i).unwrap();
        //         if param_node.kind() == "param" {
        //             let param_query = r#"
        //                 (param
        //                     name: (identifier) @name
        //                     type: (_) @type
        //                 )
        //             "#;
        //             let param_nodes =
        //                 query(param_node, &self.src, &self.language, param_query, true);
        //
        //             let name = param_nodes.get("name").unwrap()[0];
        //             let type_node = param_nodes.get("type").unwrap()[0];
        //
        //             let name = name.utf8_text(self.src.as_bytes()).unwrap().to_string();
        //
        //             let comptime_param = ComptimeParam {
        //                 name: name.clone(),
        //                 type_: self.comptime_eval(type_node),
        //                 value: None,
        //             };
        //
        //             comptime_params.insert(name, comptime_param.clone());
        //         }
        //     }
        // }
        //
        // // Check if all comptime params are provided
        // for (name, param) in comptime_params.iter() {
        //     if !comptime_args.contains_key(name) {
        //         self.report(
        //             MXPoint {
        //                 start: (0, 0),
        //                 end: (0, 0),
        //             },
        //             MXDiagnosticKind::MissingComptimeParam(name.clone()),
        //         );
        //     }
        // }
    }

    fn comptime_eval(&self, node: Node) -> ComptimeValue {
        match node.kind() {
            "int_literal" => {
                let text = node.utf8_text(self.src.as_bytes()).unwrap();
                ComptimeValue::ComptimeInt(text.parse().unwrap())
            }
            "float_literal" => {
                let text = node.utf8_text(self.src.as_bytes()).unwrap();
                ComptimeValue::ComptimeFloat(text.parse().unwrap())
            }
            "bool_literal" => {
                let text = node.utf8_text(self.src.as_bytes()).unwrap();
                ComptimeValue::ComptimeBool(text.parse().unwrap())
            }
            "string_literal" => {
                let text = node.utf8_text(self.src.as_bytes()).unwrap();
                ComptimeValue::ComptimeString(text.to_string())
            }
            _ => unimplemented!(),
        }
    }

    pub fn diagnostics(&self) -> Vec<MXDiagnostic> {
        return self.diagnostics.clone();
    }

    fn report_missing_main_function(&mut self) {
        self.report(
            MXRange {
                start: MXPosition { row: 0, col: 0 },
                end: MXPosition { row: 0, col: 0 },
            },
            MXDiagnosticKind::MissingMainFunction,
        );
    }

    fn report(&mut self, range: MXRange, kind: MXDiagnosticKind) {
        self.diagnostics.push(MXDiagnostic { range, kind });
    }
}

#[derive(Clone, Debug)]
struct ComptimeParam {
    name: String,
    type_: ComptimeValue,
    value: Option<ComptimeValue>,
}

#[derive(Clone, Debug)]
pub enum ComptimeValue {
    ComptimeInt(i64),
    ComptimeFloat(f64),
    ComptimeBool(bool),
    ComptimeString(String),
}
