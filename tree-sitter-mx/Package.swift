// swift-tools-version:5.3
import PackageDescription

let package = Package(
    name: "TreeSitterMX",
    products: [
        .library(name: "TreeSitterMX", targets: ["TreeSitterMX"]),
    ],
    dependencies: [
        .package(url: "https://github.com/ChimeHQ/SwiftTreeSitter", from: "0.8.0"),
    ],
    targets: [
        .target(
            name: "TreeSitterMX",
            dependencies: [],
            path: ".",
            sources: [
                "src/parser.c",
                // NOTE: if your language has an external scanner, add it here.
            ],
            resources: [
                .copy("queries")
            ],
            publicHeadersPath: "bindings/swift",
            cSettings: [.headerSearchPath("src")]
        ),
        .testTarget(
            name: "TreeSitterMXTests",
            dependencies: [
                "SwiftTreeSitter",
                "TreeSitterMX",
            ],
            path: "bindings/swift/TreeSitterMXTests"
        )
    ],
    cLanguageStandard: .c11
)
