// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#include "awcg_html_preview.h"
#include "awcg_element_mapper.h"
#include "awcg_style_mapper.h"
#include "awcg_json_generator.h" // For indent/escape helpers if needed, or we implement locally
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace awcg {

// Helper: Indent string
static std::string indent(int level) {
    return std::string(level * 2, ' ');
}

// Helper: Escape HTML content - preserve HTML entities
static std::string escapeHtml(const std::string& str) {
    std::ostringstream result;
    size_t i = 0;
    while (i < str.length()) {
        char c = str[i];
        // Check for HTML entity (starts with &, ends with ;)
        if (c == '&') {
            size_t semicolon = str.find(';', i);
            if (semicolon != std::string::npos && semicolon - i <= 10) {
                // Likely an HTML entity, preserve it
                result << str.substr(i, semicolon - i + 1);
                i = semicolon + 1;
                continue;
            }
            result << "&amp;";
        } else if (c == '<') {
            result << "&lt;";
        } else if (c == '>') {
            result << "&gt;";
        } else if (c == '"') {
            result << "&quot;";
        } else if (c == '\'') {
            result << "&#39;";
        } else {
            result << c;
        }
        i++;
    }
    return result.str();
}

// Helper: Recursively collect all text content from a node and its descendants
static void collectAllTextRecursive(const DOMNode& node, std::string& outText) {
    if (node.isTextNode()) {
        outText += node.textContent;
        return;
    }
    for (const auto& child : node.children) {
        collectAllTextRecursive(child, outText);
    }
}

static std::string collectAllText(const DOMNode& node) {
    std::string text;
    collectAllTextRecursive(node, text);
    // Trim
    text.erase(0, text.find_first_not_of(" \t\n\r"));
    if (text.find_last_not_of(" \t\n\r") != std::string::npos) {
        text.erase(text.find_last_not_of(" \t\n\r") + 1);
    }
    return text;
}

std::string HtmlPreviewGenerator::generate(const ParsedWebSource& source,
                                          const std::string& className,
                                          std::vector<Diagnostic>& outDiagnostics) {
    std::ostringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << generateHead(className);
    html << generateBody(source.rootNode, outDiagnostics);
    html << "</html>";
    
    return html.str();
}

std::string HtmlPreviewGenerator::generateHead(const std::string& className) {
    std::ostringstream head;
    head << "<head>\n";
    head << indent(1) << "<title>A_WCG Preview: " << escapeHtml(className) << "</title>\n";
    head << indent(1) << "<style>\n";
    head << indent(2) << "body { margin: 0; padding: 0; background-color: #1a1a1a; color: white; font-family: serif; font-size: 16px; }\n";
    head << indent(2) << "#preview-root { width: 100vw; min-height: 100vh; position: relative; overflow: auto; background-color: #1a1a1a; color: white; padding: 8px; box-sizing: border-box; }\n";
    head << indent(2) << ".debug-overlay { position: fixed; top: 10px; left: 10px; background: rgba(0,0,0,0.8); color: lime; padding: 10px; font-family: monospace; z-index: 9999; pointer-events: none; font-size: 12px; }\n";
    
    // Default widget mapping styles
    head << indent(2) << "/* Widget Mappings */\n";
    head << indent(2) << ".ue-widget { color: white; }\n";
    head << indent(2) << ".ue-canvas { position: absolute; top:0; left:0; right:0; bottom:0; overflow: hidden; }\n";
    head << indent(2) << ".ue-vbox { display: block; }\n";
    head << indent(2) << ".ue-hbox { display: flex; flex-direction: row; flex-wrap: wrap; }\n";
    head << indent(2) << ".ue-text { display: block; }\n";
    head << indent(2) << ".ue-image { display: inline-block; width: 100px; height: 100px; border: 1px dashed #555; background: #333; }\n";
    head << indent(2) << ".ue-button { display: inline; color: #88f; text-decoration: underline; cursor: pointer; }\n";
    head << indent(2) << ".ue-scrollbox { overflow: auto; }\n";
    
    // Browser-default styles for HTML elements
    head << indent(2) << "/* Browser Default Styles */\n";
    head << indent(2) << ".tag-h1 { font-size: 2em; font-weight: bold; margin: 0.67em 0; }\n";
    head << indent(2) << ".tag-h2 { font-size: 1.5em; font-weight: bold; margin: 0.83em 0; }\n";
    head << indent(2) << ".tag-h3 { font-size: 1.17em; font-weight: bold; margin: 1em 0; }\n";
    head << indent(2) << ".tag-h4 { font-size: 1em; font-weight: bold; margin: 1.33em 0; }\n";
    head << indent(2) << ".tag-h5 { font-size: 0.83em; font-weight: bold; margin: 1.67em 0; }\n";
    head << indent(2) << ".tag-h6 { font-size: 0.67em; font-weight: bold; margin: 2.33em 0; }\n";
    head << indent(2) << ".tag-p { margin: 1em 0; }\n";
    head << indent(2) << ".tag-a { color: #88f; text-decoration: underline; }\n";
    head << indent(2) << ".tag-span { display: inline; }\n";
    head << indent(2) << ".tag-div { display: block; }\n";
    head << indent(2) << ".tag-li { display: list-item; margin-left: 20px; }\n";
    head << indent(2) << ".tag-ol { margin: 1em 0; padding-left: 40px; list-style-type: decimal; }\n";
    head << indent(2) << ".tag-ul { margin: 1em 0; padding-left: 40px; list-style-type: disc; }\n";
    head << indent(2) << ".tag-nav { display: block; }\n";
    head << indent(2) << ".ue-button { display: inline; color: #3b82f6; text-decoration: underline; cursor: pointer; }\n";
    head << indent(2) << "/* Links preserve clickability */\n";
    head << indent(2) << "a.ue-text, a.ue-button { color: #3b82f6; text-decoration: underline; }\n";
    
    head << indent(1) << "</style>\n";
    head << "</head>\n";
    return head.str();
}

std::string HtmlPreviewGenerator::generateBody(const DOMNode& rootNode, std::vector<Diagnostic>& outDiagnostics) {
    std::ostringstream body;
    body << "<body>\n";
    body << indent(1) << "<div class=\"debug-overlay\">A_WCG PREVIEW MODE</div>\n";
    body << indent(1) << "<div id=\"preview-root\">\n";
    
    // We treat the root as a CanvasPanel by default in A_WCG
    // Inject generated scripts
    body << generateScripts(rootNode); 
    
    for (const auto& child : rootNode.children) {
        if (!child.isTextNode()) {
            body << generateNodeRecursive(child, 2, SlotConfig::SlotType::Canvas, outDiagnostics);
        }
    }
    
    body << indent(1) << "</div>\n";
    body << "</body>\n";
    return body.str();
}

std::string HtmlPreviewGenerator::generateScripts(const DOMNode& rootNode) {
    std::ostringstream script;
    script << indent(1) << "<script>\n";
    script << indent(2) << "/* Basic JS Support for A_WCG Preview */\n";
    script << indent(2) << "document.addEventListener('DOMContentLoaded', () => {\n";
    script << indent(3) << "console.log('A_WCG Preview Loaded');\n";
    script << indent(3) << "// Optional: Polyfill or smooth scroll logic if needed\n";
    script << indent(3) << "document.querySelectorAll('.ue-scrollbox').forEach(el => {\n";
    script << indent(4) << "if (getComputedStyle(el).scrollSnapType !== 'none') {\n";
    script << indent(5) << "console.log('Scroll snap detected on', el);\n";
    script << indent(4) << "}\n";
    script << indent(3) << "});\n";
    script << indent(2) << "});\n";
    script << indent(1) << "</script>\n";
    return script.str();
}

std::string HtmlPreviewGenerator::generateNodeRecursive(const DOMNode& node, 
                                                      int indentLevel, 
                                                      SlotConfig::SlotType parentType,
                                                      std::vector<Diagnostic>& outDiagnostics) {
    std::ostringstream html;
    
    // 1. Determine Type
    std::string widgetType = ElementMapper::mapElementToWidget(node);
    std::string widgetName = ElementMapper::generateWidgetName(node);
    SlotConfig::SlotType myContainerType = StyleMapper::determineContainerType(node.computedStyles);
    
    // 2. Generate CSS for this node based on UMG layout rules
    std::string css = generateElementStyle(node, parentType);
    
    // 3. Map to HTML tag and classes - use native HTML tags when possible for better structure
    std::string tag = "div";
    std::string classes = "ue-widget";
    
    // Use native HTML tags for better rendering
    std::string lowerTag;
    if (!node.isTextNode() && !node.tagName.empty() && node.tagName[0] != '#') {
        lowerTag = node.tagName;
        std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
    }
    
    // Map widget type to appropriate HTML tag
    if (widgetType == "CanvasPanel") { classes += " ue-canvas"; }
    else if (widgetType == "VerticalBox") { 
        // Use native list tags when appropriate
        if (lowerTag == "ol") {
            tag = "ol";
        } else if (lowerTag == "ul") {
            tag = "ul";
        } else if (lowerTag == "nav") {
            tag = "nav";
        }
        classes += " ue-vbox"; 
    }
    else if (widgetType == "HorizontalBox") { classes += " ue-hbox"; }
    else if (widgetType == "TextBlock") { 
        // Use native headings, paragraphs, spans for better visual match
        if (lowerTag == "h1" || lowerTag == "h2" || lowerTag == "h3" || 
            lowerTag == "h4" || lowerTag == "h5" || lowerTag == "h6") {
            tag = lowerTag;
        } else if (lowerTag == "p") {
            tag = "p";
        } else if (lowerTag == "span") {
            tag = "span";
        } else if (lowerTag == "li") {
            tag = "li";
        } else if (lowerTag == "a") {
            tag = "a";
        }
        classes += " ue-text"; 
    }
    else if (widgetType == "Image") { tag = "img"; classes += " ue-image"; }
    else if (widgetType == "Button") { 
        tag = "a"; 
        classes += " ue-button"; 
    }
    else if (widgetType == "ScrollBox") { classes += " ue-scrollbox"; }
    
    // Add original tag class for CSS
    if (!lowerTag.empty()) {
        classes += " tag-" + lowerTag;
    }
    
    // Open tag
    html << indent(indentLevel) << "<" << tag << " class=\"" << classes << "\" ";
    html << "style=\"" << css << "\" ";
    if (tag == "img") {
        html << "alt=\"" << widgetName << "\" ";
    }
    // Preserve href for links
    if (tag == "a" && node.attributes.count("href")) {
        html << "href=\"" << escapeHtml(node.attributes.at("href")) << "\" ";
    }
    html << "title=\"" << widgetName << " (" << widgetType << ")\">";
    // No newline after opening tag for inline elements
    if (tag != "span" && tag != "a" && tag != "li") html << "\n";
    
    // Self-closing tag for img
    if (tag == "img") {
        // No children, return now
        return html.str();
    }
    
    // 4. Content
    if (widgetType == "TextBlock" || widgetType == "Button") {
        // Use recursive text collection to handle nested spans, etc.
        std::string text = collectAllText(node);
        if (tag != "span" && tag != "a" && tag != "li") {
            html << indent(indentLevel + 1) << escapeHtml(text) << "\n";
        } else {
            html << escapeHtml(text);
        }
    }
    else {
        // Recurse
        for (const auto& child : node.children) {
            bool validChild = true;
            
            // Exclude script, style, and other non-visible tags from preview
            std::string childTag = child.tagName;
            std::transform(childTag.begin(), childTag.end(), childTag.begin(), ::tolower);
            if (childTag == "script" || childTag == "style" || childTag == "noscript" || childTag == "meta" || childTag == "link") {
                validChild = false;
            }
            
            if (validChild && child.isTextNode()) {
                // Only process text nodes if they have non-whitespace content
                std::string trimmed = child.textContent;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
                if (trimmed.empty()) {
                    validChild = false;
                }
            }
            
            if (validChild) {
                html << generateNodeRecursive(child, indentLevel + 1, myContainerType, outDiagnostics);
            }
        }
    }
    
    html << indent(indentLevel) << "</" << tag << ">\n";
    
    return html.str();
}

std::string HtmlPreviewGenerator::colorToCss(const Color& color) {
    std::ostringstream css;
    css << "rgba(" << (int)(color.r * 255) << ", " 
                   << (int)(color.g * 255) << ", " 
                   << (int)(color.b * 255) << ", " 
                   << color.a << ")";
    return css.str();
}

std::string HtmlPreviewGenerator::generateElementStyle(const DOMNode& node, SlotConfig::SlotType parentType) {
    std::ostringstream css;
    
    // --- Layout (Slot) Mappings ---
    
    if (parentType == SlotConfig::SlotType::Canvas) {
        css << "position: absolute; ";
        
        // Simulating Canvas Slots
        // In UE, anchors/offsets are complex to map exactly to CSS 'top/left/right/bottom' without JS,
        // but we can approximate "Fill" logic.
        
        CanvasSlotConfig slot = StyleMapper::generateCanvasSlot(node.computedStyles);
        
        // Simplified mapping for preview:
        // If Anchor Min/Max are equal, it's fixed size/pos.
        // If Min=0, Max=1, it's stretch.
        
        bool stretchX = (slot.anchors.min.x != slot.anchors.max.x);
        bool stretchY = (slot.anchors.min.y != slot.anchors.max.y);
        
        if (stretchX) {
            css << "left: " << slot.offsetLeft << "px; right: " << (slot.offsetRight * -1) << "px; "; // UE right offset is negative usually? No, UE offsets are additive.
            // Wait, standard CSS 'right' is distance from right edge. 
            // UE Offsets: Left, Top, Right, Bottom. 
            // If Anchors are split, Offsets are distances from anchor points.
            // So Anchor MinX=0 MaxX=1 => Left=0, Right=0 => CSS left:0, right:0.
            css << "left: " << slot.offsetLeft << "px; right: " << slot.offsetRight << "px; "; 
        } else {
            // Fixed X
            // Approximate with left: Percentage + Offset
            css << "left: calc(" << (slot.anchors.min.x * 100) << "% + " << slot.offsetLeft << "px); ";
             // If not auto-size, valid width?
             // Not easily known here without calculating size from CSS width/height.
        }
        
        if (stretchY) {
            css << "top: " << slot.offsetTop << "px; bottom: " << slot.offsetBottom << "px; ";
        } else {
            css << "top: calc(" << (slot.anchors.min.y * 100) << "% + " << slot.offsetTop << "px); ";
        }
        
        // Size mapping if provided in styles
        if (node.computedStyles.count("width")) css << "width: " << node.computedStyles.at("width") << "; ";
        if (node.computedStyles.count("height")) css << "height: " << node.computedStyles.at("height") << "; ";
        
    } else if (parentType == SlotConfig::SlotType::Box) {
        // Flex Item
         BoxSlotConfig slot = StyleMapper::generateBoxSlot(node.computedStyles);
         
         // Alignment
         std::string alignSelf;
         if (slot.hAlign == "Left") alignSelf = "flex-start";
         else if (slot.hAlign == "Right") alignSelf = "flex-end";
         else if (slot.hAlign == "Center") alignSelf = "center";
         else alignSelf = "stretch"; // Fill
         
         // Note: Cross axis depends on parent direction.
         // Since we don't know parent direction here easily without passing it...
         // Actually, VerticalBox children align horizontally, HorizontalBox children align vertically.
         
         // Simplification: Just use padding and size rules
         css << "margin: " << slot.paddingTop << "px " << slot.paddingRight << "px " << slot.paddingBottom << "px " << slot.paddingLeft << "px; ";
         
         if (slot.sizeRule == "Fill") {
             css << "flex-grow: " << slot.fillWeight << "; ";
         } else {
             css << "flex-grow: 0; ";
         }
         
         // Explicit sizes if present
         if (node.computedStyles.count("width")) css << "width: " << node.computedStyles.at("width") << "; ";
         if (node.computedStyles.count("height")) css << "height: " << node.computedStyles.at("height") << "; ";
    }
    
    // --- Appearance Mappings ---
    
    // Background Color
    if (node.computedStyles.count("background-color")) {
        Color c;
        if (StyleMapper::parseColor(node.computedStyles.at("background-color"), c)) {
            css << "background-color: " << colorToCss(c) << "; ";
        }
    }
    
    // Text Color
    if (node.computedStyles.count("color")) {
        Color c;
        if (StyleMapper::parseColor(node.computedStyles.at("color"), c)) {
            css << "color: " << colorToCss(c) << "; ";
        }
    }
    
    // Font
     if (node.computedStyles.count("font-size")) css << "font-size: " << node.computedStyles.at("font-size") << "; ";
     if (node.computedStyles.count("font-weight")) css << "font-weight: " << node.computedStyles.at("font-weight") << "; ";
     
     // Opacity
     if (node.computedStyles.count("opacity")) css << "opacity: " << node.computedStyles.at("opacity") << "; ";
     
     // Visibility
      if (node.computedStyles.count("display") && node.computedStyles.at("display") == "none") {
          css << "display: none; ";
      }

      // Scrolling & Snapping (Pass-through)
      if (node.computedStyles.count("overflow")) css << "overflow: " << node.computedStyles.at("overflow") << "; ";
      if (node.computedStyles.count("overflow-x")) css << "overflow-x: " << node.computedStyles.at("overflow-x") << "; ";
      if (node.computedStyles.count("overflow-y")) css << "overflow-y: " << node.computedStyles.at("overflow-y") << "; ";
      if (node.computedStyles.count("scroll-snap-type")) css << "scroll-snap-type: " << node.computedStyles.at("scroll-snap-type") << "; ";
      if (node.computedStyles.count("scroll-snap-align")) css << "scroll-snap-align: " << node.computedStyles.at("scroll-snap-align") << "; ";
      if (node.computedStyles.count("scroll-behavior")) css << "scroll-behavior: " << node.computedStyles.at("scroll-behavior") << "; ";

     
    return css.str();
}

} // namespace awcg
