#!/usr/bin/env -S uv run
# /// script
# requires-python = ">=3.13"
# dependencies = [
#     "tree-sitter",
#     "tree-sitter-corpus",
# ]
#
# [tool.uv.sources]
# tree-sitter-corpus = { git = "https://github.com/datwaft/tree-sitter-corpus" }
# ///

import os

def main():
    # Read all txt files in the test/corpus directory
    for file in os.listdir("test/corpus"):
        if file.endswith(".txt"):
            with open(f"test/corpus/{file}", "r") as f:
                text = f.read()
                print(text)

if __name__ == "__main__":
    main()
