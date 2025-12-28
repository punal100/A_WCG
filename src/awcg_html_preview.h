// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#pragma once

#include "awcg_types.h"
#include <string>
#include <vector>

namespace awcg {

/**
 * HTML Preview Generator
 * 
 * Generates a purely client-side HTML/CSS verification file that
 * represents how A_WCG "sees" the widget structure.
 * 
 * This uses the EXACT SAME mapping logic as the widget generator
 * to ensure that what you see in the preview matches what you get in Unreal.
 */
class HtmlPreviewGenerator {
public:
    /**
     * Generate HTML preview file content
     */
    static std::string generate(const ParsedWebSource& source,
                               const std::string& className,
                               std::vector<Diagnostic>& outDiagnostics);

private:
    static std::string generateHead(const std::string& className);
    static std::string generateBody(const DOMNode& rootNode, bool isInsideLink, std::vector<Diagnostic>& outDiagnostics);
    static std::string generateScripts(const DOMNode& rootNode);
    static std::string generateNodeRecursive(const DOMNode& node, 
                                           int indentLevel, 
                                           SlotConfig::SlotType parentType,
                                           bool isInsideLink,
                                           std::vector<Diagnostic>& outDiagnostics);
    
    // CSS Helpers
    static std::string generateElementStyle(const DOMNode& node, SlotConfig::SlotType parentType, bool isInsideLink);
    static std::string colorToCss(const Color& color);
};

} // namespace awcg
