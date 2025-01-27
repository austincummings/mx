#!/usr/bin/env python

import os
import tempfile
import subprocess

def parse_test_corpus(file_path):
    """
    Parses a Tree-Sitter style test corpus file.
    Each test is structured as:
      - A description enclosed between lines of ==========
      - Input segment follows immediately after the description.
      - The input and expected output are separated by ---.
    """
    tests = []
    with open(file_path, 'r') as file:
        content = file.read()

    # Split the file into potential test cases using the description markers
    raw_tests = content.split("==========")
    for i in range(1, len(raw_tests), 2):  # Skip initial empty segment, process paired blocks
        description_block = raw_tests[i].strip()
        if i + 1 < len(raw_tests):
            test_block = raw_tests[i + 1].strip()

            # Parse description, input, and output
            description = description_block
            if "---" in test_block:
                input_segment, expected_output = test_block.split("---", 1)
                tests.append((description.strip(), input_segment.strip(), expected_output.strip()))
    return tests

def run_test(input_segment):
    """
    Run the ./mx executable with the input segment and capture its output.
    """
    with tempfile.NamedTemporaryFile(mode='w+', delete=False) as temp_file:
        temp_file.write(input_segment)
        temp_file_path = temp_file.name

    try:
        result = subprocess.run(
            ['./mx', temp_file_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        if result.returncode != 0:
            raise RuntimeError(f"./mx failed: {result.stderr}")
        return result.stdout.strip()
    finally:
        os.unlink(temp_file_path)

def show_diff(expected, actual):
    """
    Show a colored diff between the expected and actual output using the diff tool.
    """
    with tempfile.NamedTemporaryFile(mode='w+', delete=False) as expected_file, \
         tempfile.NamedTemporaryFile(mode='w+', delete=False) as actual_file:
        expected_file.write(expected)
        actual_file.write(actual)
        expected_path = expected_file.name
        actual_path = actual_file.name

    try:
        subprocess.run([
            'diff', '--color=always', '-u', expected_path, actual_path
        ])
    finally:
        os.unlink(expected_path)
        os.unlink(actual_path)

def main(test_corpus_path):
    """
    Main function to process the test corpus and run tests.
    """
    tests = parse_test_corpus(test_corpus_path)
    total_tests = len(tests)
    passed_tests = 0

    for index, (description, input_segment, expected_output) in enumerate(tests, start=1):
        print(f"Running test {index}/{total_tests}...")
        if description:
            print(f"Description: {description}")
        try:
            actual_output = run_test(input_segment)
            if actual_output == expected_output:
                print(f"Test {index} PASSED")
                passed_tests += 1
            else:
                print(f"Test {index} FAILED")
                print("Showing diff:")
                show_diff(expected_output, actual_output)
        except Exception as e:
            print(f"Test {index} ERROR: {e}")

    print(f"\n{passed_tests}/{total_tests} tests passed.")

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Process Tree-Sitter style test corpus files.")
    parser.add_argument("test_corpus", help="Path to the test corpus file")

    args = parser.parse_args()

    if not os.path.exists(args.test_corpus):
        print(f"Error: Test corpus file '{args.test_corpus}' does not exist.")
    else:
        main(args.test_corpus)
