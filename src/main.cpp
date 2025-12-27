// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator
// Standalone CLI Tool

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <vector>

#include "awcg_types.h"
#include "awcg_html_parser.h"
#include "awcg_css_parser.h"
#include "awcg_element_mapper.h"
#include "awcg_style_mapper.h"
#include "awcg_json_generator.h"
#include "awcg_cpp_generator.h"
#include "awcg_html_preview.h"

namespace fs = std::filesystem;

void printUsage(const char* programName) {
    std::cout << "A_WCG - Atomic Web Component Generator v1.0.0\n";
    std::cout << "Converts HTML/CSS to P_MWCS-compatible UE widget specs\n\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --source, -s <file>     Source HTML file (required)\n";
    std::cout << "  --css <file>            CSS file (optional, auto-detected)\n";
    std::cout << "  --output, -o <dir>      Output directory (default: ./generated)\n";
    std::cout << "  --class, -c <name>      Widget class name (default: from HTML filename)\n";
    std::cout << "  --module, -m <name>     Module name for C++ API macro (default: MyProject)\n";
    std::cout << "  --no-header             Skip C++ header generation\n";
    std::cout << "  --no-json               Skip JSON spec generation\n";
    std::cout << "  --verbose, -v           Verbose output\n";
    std::cout << "  --help, -h              Show this help\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << programName << " -s index.html -o ./generated -c MainMenu\n";
}

std::string getFileNameWithoutExtension(const std::string& path) {
    fs::path p(path);
    return p.stem().string();
}

std::string toPascalCase(const std::string& input) {
    std::string result;
    bool capitalizeNext = true;
    
    for (char c : input) {
        if (c == '-' || c == '_' || c == ' ') {
            capitalizeNext = true;
        } else if (std::isalnum(static_cast<unsigned char>(c))) {
            if (capitalizeNext) {
                result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                capitalizeNext = false;
            } else {
                result += c;
            }
        }
    }
    
    return result;
}

int main(int argc, char* argv[]) {
    // Parse arguments
    std::string htmlPath;
    std::string cssPath;
    std::string outputDir = "./generated";
    std::string className;
    std::string moduleName = "MyProject";
    bool generateHeader = true;
    bool generateJson = true;
    bool verbose = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--source" || arg == "-s") {
            if (i + 1 < argc) htmlPath = argv[++i];
        } else if (arg == "--css") {
            if (i + 1 < argc) cssPath = argv[++i];
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) outputDir = argv[++i];
        } else if (arg == "--class" || arg == "-c") {
            if (i + 1 < argc) className = argv[++i];
        } else if (arg == "--module" || arg == "-m") {
            if (i + 1 < argc) moduleName = argv[++i];
        } else if (arg == "--no-header") {
            generateHeader = false;
        } else if (arg == "--no-json") {
            generateJson = false;
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        }
    }
    
    // Validate required arguments
    if (htmlPath.empty()) {
        std::cerr << "Error: --source is required\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // Derive class name from filename if not specified
    if (className.empty()) {
        className = toPascalCase(getFileNameWithoutExtension(htmlPath));
    }
    
    // Auto-detect CSS file
    if (cssPath.empty()) {
        fs::path htmlFile(htmlPath);
        fs::path cssFile = htmlFile.parent_path() / (htmlFile.stem().string() + ".css");
        if (fs::exists(cssFile)) {
            cssPath = cssFile.string();
            if (verbose) std::cout << "Auto-detected CSS: " << cssPath << "\n";
        }
    }
    
    if (verbose) {
        std::cout << "A_WCG - Atomic Web Component Generator\n";
        std::cout << "======================================\n";
        std::cout << "Source HTML: " << htmlPath << "\n";
        if (!cssPath.empty()) std::cout << "Source CSS: " << cssPath << "\n";
        std::cout << "Output Dir: " << outputDir << "\n";
        std::cout << "Class Name: " << className << "\n";
        std::cout << "Module: " << moduleName << "\n";
        std::cout << "\n";
    }
    
    // Create output directory
    fs::create_directories(outputDir);
    
    // Parse HTML
    awcg::ParsedWebSource source;
    source.htmlPath = htmlPath;
    source.cssPath = cssPath;
    std::vector<awcg::Diagnostic> diagnostics;
    
    awcg::HTMLParser htmlParser;
    if (!htmlParser.parseFile(htmlPath, source.rootNode, diagnostics)) {
        std::cerr << "Error: Failed to parse HTML file\n";
        for (const auto& diag : diagnostics) {
            std::cerr << diag.toString() << "\n";
        }
        return 1;
    }
    
    source.pageTitle = htmlParser.getPageTitle();
    
    if (verbose) {
        std::cout << "Parsed HTML successfully\n";
        std::cout << "  Page title: " << (source.pageTitle.empty() ? "(none)" : source.pageTitle) << "\n";
        std::cout << "  Style blocks: " << htmlParser.getStyleBlocks().size() << "\n";
        std::cout << "  Script blocks: " << htmlParser.getScriptBlocks().size() << "\n";
    }
    
    try {
        // Parse CSS (embedded + external)
        awcg::CSSParser cssParser;
        
        // Parse embedded styles
        for (const auto& styleBlock : htmlParser.getStyleBlocks()) {
            awcg::StyleSheet embedded;
            if (cssParser.parse(styleBlock, embedded, diagnostics)) {
                for (auto& rule : embedded.rules) {
                    source.styleSheet.rules.push_back(std::move(rule));
                }
            }
        }
        
        // Parse external CSS
        if (!cssPath.empty()) {
            awcg::StyleSheet external;
            if (cssParser.parseFile(cssPath, external, diagnostics)) {
                for (auto& rule : external.rules) {
                    source.styleSheet.rules.push_back(std::move(rule));
                }
            }
        }
        
        if (verbose) {
            std::cout << "Parsed CSS: " << source.styleSheet.rules.size() << " rules\n";
        }
        
        // Apply styles to DOM
        awcg::CSSParser::applyStylesToNode(source.styleSheet, source.rootNode);
        
        if (verbose) {
            std::cout << "Applied computed styles to DOM\n\n";
        }
        
        // Generate outputs
        int filesGenerated = 0;
        
        // Generate structured spec
        awcg::JSONGenerator::ChunkedSpec chunkedSpec = awcg::JSONGenerator::generateChunked(source, className, moduleName, diagnostics);
        
        if (generateJson) {
            std::string jsonPath = (fs::path(outputDir) / (className + ".json")).string();
            
            std::ofstream jsonFile(jsonPath);
            if (jsonFile.is_open()) {
                jsonFile << chunkedSpec.fullJson;
                jsonFile.close();
                filesGenerated++;
                if (verbose) std::cout << "Generated: " << jsonPath << "\n";
            } else {
                std::cerr << "Error: Could not write " << jsonPath << "\n";
            }
        }
        
        if (generateHeader) {
            std::string header = awcg::CppGenerator::generateHeader(source, className, moduleName, chunkedSpec, diagnostics);
            std::string headerPath = (fs::path(outputDir) / (className + ".h")).string();
            
            std::ofstream headerFile(headerPath);
            if (headerFile.is_open()) {
                headerFile << header;
                headerFile.close();
                filesGenerated++;
                if (verbose) std::cout << "Generated: " << headerPath << "\n";
            } else {
                std::cerr << "Error: Could not write " << headerPath << "\n";
            }
            
            std::string src = awcg::CppGenerator::generateSource(source, className, moduleName, chunkedSpec, diagnostics);
            std::string srcPath = (fs::path(outputDir) / (className + ".cpp")).string();
            
            std::ofstream srcFile(srcPath);
            if (srcFile.is_open()) {
                srcFile << src;
                srcFile.close();
                filesGenerated++;
                if (verbose) std::cout << "Generated: " << srcPath << "\n";
            } else {
                std::cerr << "Error: Could not write " << srcPath << "\n";
            }
        
        // Generate HTML Preview
        std::string previewHtml = awcg::HtmlPreviewGenerator::generate(source, className, diagnostics);
        std::string previewPath = (fs::path(outputDir) / (className + "_preview.html")).string();
        
        std::ofstream previewFile(previewPath);
        if (previewFile.is_open()) {
            previewFile << previewHtml;
            previewFile.close();
            filesGenerated++;
            if (verbose) std::cout << "Generated: " << previewPath << "\n";
        } else {
            std::cerr << "Error: Could not write " << previewPath << "\n";
        }
        }
        
        // Report diagnostics
        if (!diagnostics.empty()) {
            std::cout << "\nDiagnostics:\n";
            for (const auto& diag : diagnostics) {
                std::cout << "  " << diag.toString() << "\n";
            }
        }
        
        std::cout << "\nGenerated " << filesGenerated << " file(s) in " << outputDir << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: Uncaught exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "FATAL ERROR: Unknown exception occurred\n";
        return 1;
    }
    
    return diagnostics.empty() ? 0 : 1;
}
