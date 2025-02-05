use std::collections::{hash_map::Entry, HashMap};
use streaming_iterator::StreamingIterator;
use tree_sitter::{Language, Node, Query, QueryCursor, QueryMatch};

pub fn query<'tree>(
    node: Node<'tree>,
    src: &str,
    language: &Language,
    query_string: &str,
    direct_children_only: bool,
) -> HashMap<String, Vec<Node<'tree>>> {
    // Create the TSQuery from the query string
    let query = match Query::new(&language, query_string) {
        Ok(q) => q,
        Err(e) => {
            eprintln!("Failed to create TSQuery: {:?}", e);
            eprintln!("Query:\n{}", query_string);
            return HashMap::new();
        }
    };

    // Create a query cursor
    let mut cursor = QueryCursor::new();

    // Initialize the hash map
    let mut capture_map: HashMap<String, Vec<Node>> = HashMap::new();

    // Execute the query
    let mut matches = cursor.matches(&query, node, src.as_bytes());

    while let Some(m) = matches.next() {
        for c in m.captures {
            let captured_node = c.node;
            let capture_index = c.index;
            let capture_name = query.capture_names()[capture_index as usize];

            // Check if the capture index is already in the map
            match capture_map.entry(capture_name.to_string()) {
                Entry::Vacant(entry) => {
                    entry.insert(vec![captured_node]);
                }
                Entry::Occupied(mut entry) => {
                    if direct_children_only {
                        if let Some(parent) = captured_node.parent() {
                            if parent == node {
                                entry.get_mut().push(captured_node);
                            }
                        }
                    } else {
                        entry.get_mut().push(captured_node);
                    }
                }
            }
        }
    }

    capture_map
}
