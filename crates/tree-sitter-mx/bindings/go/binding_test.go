package tree_sitter_mx_test

import (
	"testing"

	tree_sitter "github.com/tree-sitter/go-tree-sitter"
	tree_sitter_mx "github.com/tree-sitter/tree-sitter-mx/bindings/go"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_mx.Language())
	if language == nil {
		t.Errorf("Error loading MX grammar")
	}
}
