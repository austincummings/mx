---@diagnostic disable: undefined-global
---
if vim.b.mx_loaded then
	return
end
vim.b.mx_loaded = true

local parser_config = require("nvim-treesitter.parsers").get_parser_configs()
parser_config.mx = {
	install_info = {
		url = "/home/austin/src/tree-sitter-mx/",
		files = { "src/parser.c" },
		branch = "main",
		generate_requires_npm = false,
		requires_generate_from_grammar = false,
	},
	filetype = "mx",
}

vim.treesitter.language.register("mx", "mx")
