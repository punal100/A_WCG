// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#pragma once

#include "awcg_types.h"
#include <string>
#include <vector>

namespace awcg {

/**
 * JSON Generator
 * 
 * Generates P_MWCS-compatible JSON widget specifications.
 */
class JSONGenerator {
public:
    /**
     * Chunked widget specification parts
     */
    struct ChunkedSpec {
        std::string metadata;
        std::string hierarchy;
        std::string design;
        std::string bindings;
        std::string fullJson; // For JSON file output
        
        // List of bindings for C++ header generation {Name, Type}
        std::vector<std::pair<std::string, std::string>> bindingList;
    };

    /**
     * Generate JSON spec from parsed web source
     */
    static std::string generate(const ParsedWebSource& source,
                               const std::string& className,
                               const std::string& moduleName,
                               std::vector<Diagnostic>& outDiagnostics);

    /**
     * Generate chunked JSON spec components
     */
    static ChunkedSpec generateChunked(const ParsedWebSource& source,
                                     const std::string& className,
                                     const std::string& moduleName,
                                     std::vector<Diagnostic>& outDiagnostics);

    /**
     * Generate JSON for a single widget from DOM node
     */
    static std::string generateWidgetJson(const DOMNode& node, int indentLevel = 0);

private:
    static std::string indent(int level);
    static std::string escapeJson(const std::string& str);
    static std::string colorToJson(const Color& color);
};

} // namespace awcg
