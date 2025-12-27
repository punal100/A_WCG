// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#pragma once

#include "awcg_types.h"
#include "awcg_json_generator.h"
#include <string>
#include <vector>

namespace awcg {

/**
 * C++ Generator
 * 
 * Generates C++ header file for P_MWCS widget bindings.
 */
class CppGenerator {
public:
    /**
     * Generate C++ header from parsed web source
     */
    static std::string generateHeader(const ParsedWebSource& source,
                                      const std::string& className,
                                      const std::string& moduleName,
                                      const JSONGenerator::ChunkedSpec& chunkedSpec,
                                      std::vector<Diagnostic>& outDiagnostics);

    /**
     * Generate C++ source (implementation file) with embedded JSON spec
     */
    static std::string generateSource(const ParsedWebSource& source,
                                       const std::string& className,
                                       const std::string& moduleName,
                                       const JSONGenerator::ChunkedSpec& chunkedSpec,
                                       std::vector<Diagnostic>& outDiagnostics);

private:
    static void collectBindings(const DOMNode& node,
                                std::vector<std::pair<std::string, std::string>>& outBindings);
};

} // namespace awcg
