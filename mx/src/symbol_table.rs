use std::collections::HashMap;

#[derive(Debug, Clone, Copy)]
pub struct SymbolTableRef(pub u32);

#[derive(Debug, Clone)]
pub struct SymbolTableSet<TValue, TTableData> {
    pub tables: Vec<SymbolTable<TValue, TTableData>>,
    pub stack: Vec<SymbolTableRef>,
}

impl<TValue, TTableData> SymbolTableSet<TValue, TTableData> {
    pub fn new() -> Self {
        SymbolTableSet {
            tables: vec![],
            stack: vec![],
        }
    }

    pub fn current_table_ref(&self) -> Option<SymbolTableRef> {
        self.stack.last().copied()
    }

    pub fn current_table(&self) -> Option<&SymbolTable<TValue, TTableData>> {
        self.stack
            .last()
            .and_then(|&r| self.tables.get(r.0 as usize))
    }

    pub fn current_table_mut(&mut self) -> Option<&mut SymbolTable<TValue, TTableData>> {
        self.stack
            .last()
            .and_then(|&r| self.tables.get_mut(r.0 as usize))
    }

    pub fn push_table(&mut self, table_data: TTableData) -> SymbolTableRef {
        // Add a new table to the set, then push it onto the stack
        let parent_ref = self.current_table_ref();
        let table_ref = SymbolTableRef(self.tables.len() as u32);
        self.tables.push(SymbolTable::new(parent_ref, table_data));
        self.stack.push(table_ref);
        table_ref
    }

    pub fn pop_table(&mut self) -> Option<SymbolTableRef> {
        self.stack.pop()
    }

    pub fn insert(&mut self, name: &str, val: TValue) {
        if let Some(table) = self.current_table_mut() {
            table.insert(name, val);
        }
    }

    pub fn get(&self, name: &str) -> Option<&TValue> {
        if let Some(table) = self.current_table() {
            table.get(name)
        } else {
            None
        }
    }

    pub fn lookup(&self, name: &str) -> Option<&TValue> {
        // Recursively search the current table and then the parent tables until a match is found
        let mut table_ref = self.current_table_ref()?;

        loop {
            let current_table = self
                .tables
                .get(table_ref.0 as usize)
                .expect("Table not found");

            if let Some(val) = current_table.get(name) {
                return Some(val);
            }

            if let Some(parent_ref) = current_table.parent {
                table_ref = parent_ref;
            } else {
                break;
            }
        }

        None
    }
}

#[derive(Debug, Clone)]
pub struct SymbolTable<TValue, TTableData> {
    data: TTableData,
    parent: Option<SymbolTableRef>,
    members: HashMap<String, TValue>,
}

impl<TValue, TTableData> SymbolTable<TValue, TTableData> {
    pub fn new(parent: Option<SymbolTableRef>, data: TTableData) -> Self {
        SymbolTable {
            data,
            parent,
            members: HashMap::new(),
        }
    }

    pub fn insert(&mut self, name: &str, val: TValue) {
        self.members.insert(name.to_string(), val);
    }

    pub fn get(&self, name: &str) -> Option<&TValue> {
        self.members.get(name)
    }
}
