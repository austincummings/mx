import XCTest
import SwiftTreeSitter
import TreeSitterMX

final class TreeSitterMXTests: XCTestCase {
    func testCanLoadGrammar() throws {
        let parser = Parser()
        let language = Language(language: tree_sitter_mx())
        XCTAssertNoThrow(try parser.setLanguage(language),
                         "Error loading MX grammar")
    }
}
