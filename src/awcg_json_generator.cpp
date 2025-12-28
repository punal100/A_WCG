// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#include "awcg_json_generator.h"
#include "awcg_element_mapper.h"
#include "awcg_style_mapper.h"
#include <sstream>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>

namespace awcg {



// Helper: Decode common HTML entities
static std::string decodeHtmlEntities(const std::string& input) {
    std::string result;
    result.reserve(input.length());
    
    size_t i = 0;
    while (i < input.length()) {
        if (input[i] == '&') {
            // Look for entity end
            size_t semicolon = input.find(';', i);
            if (semicolon != std::string::npos && semicolon - i <= 10) {
                std::string entity = input.substr(i, semicolon - i + 1);
                
                // Common HTML entities
                if (entity == "&nbsp;") { result += ' '; }
                else if (entity == "&amp;") { result += '&'; }
                else if (entity == "&lt;") { result += '<'; }
                else if (entity == "&gt;") { result += '>'; }
                else if (entity == "&quot;") { result += '"'; }
                else if (entity == "&#39;" || entity == "&apos;") { result += '\''; }
                else if (entity == "&ndash;") { result += '-'; }
                else if (entity == "&mdash;") { result += '-'; }
                else if (entity == "&copy;") { result += "(c)"; }
                else if (entity == "&reg;") { result += "(R)"; }
                else if (entity == "&trade;") { result += "(TM)"; }
                else if (entity == "&hellip;") { result += "..."; }
                else if (entity.length() > 3 && entity[1] == '#') {
                    // Numeric entity like &#160; or &#x20;
                    try {
                        int codepoint = 0;
                        if (entity[2] == 'x' || entity[2] == 'X') {
                            codepoint = std::stoi(entity.substr(3, entity.length() - 4), nullptr, 16);
                        } else {
                            codepoint = std::stoi(entity.substr(2, entity.length() - 3));
                        }
                        if (codepoint == 160) { result += ' '; } // Non-breaking space
                        else if (codepoint < 128) { result += (char)codepoint; }
                        else { result += entity; } // Keep as-is for complex Unicode
                    } catch (...) {
                        result += entity; // Keep original on parse error
                    }
                } else {
                    result += entity; // Unknown entity, keep as-is
                }
                
                i = semicolon + 1;
                continue;
            }
        }
        result += input[i];
        i++;
    }
    
    return result;
}

// Helper: Get default HTML font size for semantic elements
static int getDefaultFontSizeForTag(const std::string& tagName) {
    std::string tag = tagName;
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
    
    // Standard HTML default sizes (based on browser defaults, base 16px)
    if (tag == "h1") return 32;  // 2em
    if (tag == "h2") return 24;  // 1.5em
    if (tag == "h3") return 19;  // 1.17em
    if (tag == "h4") return 16;  // 1em
    if (tag == "h5") return 13;  // 0.83em
    if (tag == "h6") return 11;  // 0.67em
    if (tag == "p") return 16;
    if (tag == "span") return 16;
    if (tag == "label") return 16;
    if (tag == "a") return 16;
    if (tag == "li") return 16;
    // For raw text nodes, use 16 as base
    if (tag == "#text") return 16;
    
    return 0; // No default size
}

struct GenerationContext {
    std::map<const DOMNode*, std::string> nodeNames;
    std::map<std::string, int> nameCounts;
    
    void assignUniqueName(const DOMNode* node) {
        std::string baseName = ElementMapper::generateWidgetName(*node);
        // Special case for Text Nodes to avoid generic "Widget" spam
        if (node->isTextNode()) baseName = "Text";
        
        int count = nameCounts[baseName]++;
        
        std::string uniqueName = baseName;
        if (count > 0 || (nameCounts[baseName] > 1 && count == 0)) {
             uniqueName += "_" + std::to_string(count);
        }
        
        nodeNames[node] = uniqueName;
    }
    
    void precomputeNames(const DOMNode& node) {
        // ALWAYS assign names, even for text nodes now
        assignUniqueName(&node);
        
        for (const auto& child : node.children) {
            precomputeNames(child);
        }
    }
    
    std::string getName(const DOMNode* node) {
        if (nodeNames.count(node)) return nodeNames[node];
        return "Unknown";
    }
};

std::string JSONGenerator::generate(const ParsedWebSource& source,
                                    const std::string& className,
                                    const std::string& moduleName,
                                    std::vector<Diagnostic>& outDiagnostics) {
    ChunkedSpec spec = generateChunked(source, className, moduleName, outDiagnostics);
    return spec.fullJson;
}

JSONGenerator::ChunkedSpec JSONGenerator::generateChunked(const ParsedWebSource& source,
                                     const std::string& className,
                                     const std::string& moduleName,
                                     std::vector<Diagnostic>& outDiagnostics) {
    ChunkedSpec spec;
    GenerationContext ctx;
    ctx.precomputeNames(source.rootNode);
    
    // 1. Metadata (P_MWCS compliant)
    std::ostringstream meta;
    meta << "{\n";
    meta << indent(1) << "\"ClassName\": \"" << escapeJson(className) << "\",\n";
    meta << indent(1) << "\"BlueprintName\": \"WBP_" << escapeJson(className) << "\",\n";
    meta << indent(1) << "\"ParentClass\": \"/Script/" << escapeJson(moduleName) << "." << escapeJson(className) << "\",\n";
    meta << indent(1) << "\"Version\": \"1.0.0\",\n";
    meta << indent(1) << "\"GeneratedBy\": \"A_WCG\",\n";
    meta << indent(1) << "\"Source\": \"" << escapeJson(source.htmlPath) << "\",\n";
    // DesignerPreview
    meta << indent(1) << "\"DesignerPreview\": {\n";
    meta << indent(2) << "\"SizeMode\": \"FillScreen\",\n";
    meta << indent(2) << "\"CustomSize\": {\"X\": 1920, \"Y\": 1080},\n";
    meta << indent(2) << "\"ZoomLevel\": 14,\n";
    meta << indent(2) << "\"ShowGrid\": false\n";
    meta << indent(1) << "}\n";
    spec.metadata = meta.str();
    
    // Helper to calculate font settings (shared between hierarchy and design)
    auto getFontSettings = [](const DOMNode& node, float& outSize, std::string& outTypeface, std::string& outFontObject) {
         int defaultSize = getDefaultFontSizeForTag(node.tagName);
         outSize = defaultSize > 0 ? (float)defaultSize : 16.0f;
         
         // If node is #text, look at parent for styles first (CSS inheritance lite)
         // NOTE: Computed styles should ideally be passed down during parsing/mapping, 
         // but here we only have the node's own styles. TextNodes generally don't have styles attached directly 
         // unless we improved the parser to copy parent styles. 
         // Assuming our CSS parser attaches inherited styles to children? 
         // If not, we might miss styles on raw text nodes. 
         // HOWEVER, for simple widget gen, we usually rely on the wrapping TextBlock.
         
         if (node.computedStyles.count("font-size")) {
             std::string unit;
             StyleMapper::parseSize(node.computedStyles.at("font-size"), outSize, unit);
         }
         
         outTypeface = "Regular";
         if (node.computedStyles.count("font-weight")) {
             outTypeface = StyleMapper::mapFontWeight(node.computedStyles.at("font-weight"));
         } else {
             std::string tag = node.tagName;
             std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
             if (tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6" || tag == "strong" || tag == "b") {
                 outTypeface = "Bold";
             }
         }
         
         outFontObject = "/Engine/EngineFonts/Roboto.Roboto";
         if (node.computedStyles.count("font-family")) {
             outFontObject = StyleMapper::mapFontFamily(node.computedStyles.at("font-family"));
         }
    };

    // 2. Hierarchy and Design Builder
    struct Builder {
        GenerationContext& ctx;
        std::ostringstream hier;
        std::ostringstream design;
        
        // State for List Counting
        std::vector<int> listCounters; // Stack of counters for nested OLs. -1 implies UL.

        void build(const DOMNode& node, int indentLevel, SlotConfig::SlotType parentSlotType) {
            std::string type = ElementMapper::mapElementToWidget(node);
            std::string name = ctx.getName(&node);
            
            // Handle Text Node -> TextBlock
            // Note: ElementMapper maps text nodes to "TextBlock" automatically
            
            std::string lowerTag = node.tagName;
            std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
            
            bool isListRoot = (lowerTag == "ol" || lowerTag == "ul");
            if (isListRoot) {
                if (lowerTag == "ol") listCounters.push_back(1);
                else listCounters.push_back(-1); // -1 for Unordered
            }

            // Determine container type for children
            SlotConfig::SlotType myContainerType = SlotConfig::SlotType::None;
            if (ElementMapper::isContainerWidget(type)) {
                // If it's a list item (HorizontalBox), use auto/fill logic for children
                if (lowerTag == "li") {
                    myContainerType = SlotConfig::SlotType::Box; 
                } else {
                    myContainerType = StyleMapper::determineContainerType(node.computedStyles);
                }
            }

            // HIERARCHY OUTPUT
            hier << indent(indentLevel) << "{\n";
            hier << indent(indentLevel + 1) << "\"Type\": \"" << escapeJson(type) << "\",\n";
            hier << indent(indentLevel + 1) << "\"Name\": \"" << escapeJson(name) << "\"";

            // Visibility
            if (node.computedStyles.count("display") || node.computedStyles.count("visibility")) {
                std::string display = "block";
                if (node.computedStyles.count("display")) display = node.computedStyles.at("display");
                if (node.computedStyles.count("visibility")) {
                    std::string vis = node.computedStyles.at("visibility");
                    if (vis == "hidden") display = "hidden";
                }
                std::string visibility = StyleMapper::mapVisibility(display);
                if (visibility != "Visible") {
                    hier << ",\n" << indent(indentLevel + 1) << "\"Visibility\": \"" << visibility << "\"";
                }
            }
            
            if (ElementMapper::shouldCreateBinding(node)) {
                 hier << ",\n" << indent(indentLevel + 1) << "\"BindingType\": \"Required\"";
            }
            
            // SLOT CONFIG
            if (parentSlotType != SlotConfig::SlotType::None) {
                std::map<std::string, std::string> effectiveStyles = node.computedStyles;
                
                // Inject default padding for lists
                if (lowerTag == "ul" || lowerTag == "ol") {
                    if (effectiveStyles.find("padding-left") == effectiveStyles.end()) {
                        effectiveStyles["padding-left"] = "40px";
                    }
                }

                SlotConfig slotCfg = StyleMapper::generateSlotConfig(effectiveStyles, parentSlotType);
                
                // If this is a Marker inside an LI, override to Auto
                // If this is the Content inside an LI, override to Fill? 
                // We'll handle LI children specifically below when iterating them.
                
                std::string slotJson = StyleMapper::slotToJson(slotCfg, 0);
                if (!slotJson.empty()) {
                    hier << ",\n";
                    if (slotCfg.type == SlotConfig::SlotType::Canvas) {
                        const auto& c = slotCfg.canvas;
                        hier << indent(indentLevel + 1) << "\"Slot\": {\n";
                        hier << indent(indentLevel + 2) << "\"Anchors\": {\"Min\": {\"X\": " << c.anchors.min.x << ", \"Y\": " << c.anchors.min.y << "}, \"Max\": {\"X\": " << c.anchors.max.x << ", \"Y\": " << c.anchors.max.y << "}},\n";
                        hier << indent(indentLevel + 2) << "\"Offsets\": {\"Left\": " << c.offsetLeft << ", \"Top\": " << c.offsetTop << ", \"Right\": " << c.offsetRight << ", \"Bottom\": " << c.offsetBottom << "},\n";
                        hier << indent(indentLevel + 2) << "\"Alignment\": {\"X\": " << c.alignment.x << ", \"Y\": " << c.alignment.y << "},\n";
                        hier << indent(indentLevel + 2) << "\"AutoSize\": " << (c.autoSize ? "true" : "false") << "\n";
                        hier << indent(indentLevel + 1) << "}";
                    } else if (slotCfg.type == SlotConfig::SlotType::Box) {
                        const auto& b = slotCfg.box;
                        hier << indent(indentLevel + 1) << "\"Slot\": {\n";
                        hier << indent(indentLevel + 2) << "\"HAlign\": \"" << b.hAlign << "\",\n";
                        hier << indent(indentLevel + 2) << "\"VAlign\": \"" << b.vAlign << "\",\n";
                        hier << indent(indentLevel + 2) << "\"Padding\": {\"Left\": " << b.paddingLeft << ", \"Top\": " << b.paddingTop << ", \"Right\": " << b.paddingRight << ", \"Bottom\": " << b.paddingBottom << "},\n";
                         // Size Rule
                        hier << indent(indentLevel + 2) << "\"Size\": {\"Rule\": \"" << b.sizeRule << "\"";
                        if (b.sizeRule == "Fill") hier << ", \"Value\": " << b.fillWeight;
                        hier << "}\n";
                        hier << indent(indentLevel + 1) << "}";
                    }
                }
            }

            // TEXT CONTENT (for TextBlock)
            if (type == "TextBlock") {
                // Determine text content
                std::string text = node.textContent;
                // If it's a TextBlock but has children (mapped from span/p), we might need to collect text or ignore children?
                // Mixed content implies we should process children.
                // But if it's strictly a TextBlock type, it can't have UWidget children in UE.
                // So if ElementMapper said it's TextBlock, it shouldn't have widget children.
                // Exception: "formatted text" with spans.
                // For now, assume pure TextBlock uses node.textContent or needs to collect all text if we treat it as leaf.
                
                // If node is not a text node but mapped to TextBlock (e.g. span), we should collect text.
                if (!node.isTextNode()) {
                    // Collect text (simple approach for now)
                    // TODO: A better approach is to use RichTextBlock for spans, but we stick to standard.
                    // If we want mixed colors, we'd need RichText.
                    // For now, let's just collect all text so we don't lose it.
                     // Helper: Recursively collect all text content
                     std::string collected;
                     auto collect = [&](auto&& self, const DOMNode& n) -> void {
                         if (n.isTextNode()) collected += n.textContent;
                         for(auto& c : n.children) self(self, c);
                     };
                     collect(collect, node);
                     text = collected;
                }
                
                // Trim
                text.erase(0, text.find_first_not_of(" \t\n\r"));
                if (text.find_last_not_of(" \t\n\r") != std::string::npos) {
                    text.erase(text.find_last_not_of(" \t\n\r") + 1);
                }
                text = decodeHtmlEntities(text);
                
                if (!text.empty()) {
                    hier << ",\n" << indent(indentLevel + 1) << "\"Text\": \"" << escapeJson(text) << "\"";
                    // Enable AutoWrap
                    hier << ",\n" << indent(indentLevel + 1) << "\"AutoWrapText\": true";
                }
                
                // Font settings inline for MWCS
                float fSize; std::string fType, fObj;
                // Helper lambda defined above is not accessible here easily unless we copy it or make method
                // duplicating logic slightly for simplicity
                int defaultSize = getDefaultFontSizeForTag(node.tagName); // Use original tag
                fSize = defaultSize > 0 ? (float)defaultSize : 16.0f;
                if (node.computedStyles.count("font-size")) {
                    std::string unit;
                    StyleMapper::parseSize(node.computedStyles.at("font-size"), fSize, unit);
                }
                
                hier << ",\n" << indent(indentLevel + 1) << "\"FontSize\": " << (int)fSize;
                
                if (node.computedStyles.count("text-align")) {
                    std::string justify = StyleMapper::mapTextAlign(node.computedStyles.at("text-align"));
                    hier << ",\n" << indent(indentLevel + 1) << "\"Justification\": \"" << justify << "\"";
                }
            }
            
            // LIST MARKER INJECTION
            // If this is an 'li', we inject a marker if we are inside a list
            if (lowerTag == "li" && !listCounters.empty()) {
                 int currentCounter = listCounters.back();
                 std::string markerText;
                 
                 if (currentCounter == -1) {
                     markerText = "\u2022"; // Bullet
                 } else {
                     markerText = std::to_string(currentCounter) + ".";
                     listCounters.back()++; // Increment for next item
                 }
                 
                 // We need to inject a child into this HorizontalBox BEFORE the actual content.
                 // But wait, we are about to iterate children.
                 // We can simply output the JSON for a child widget here before the loop
                 // BUT children array needs to be opened.
                 
                 // Wait, we need to check if we have kids. `li` usually has kids (text or blocks).
                 // We will open the "Children" array, emit Marker, then emit others.
            }
            
            // PROCESS CHILDREN
            // Only containers (or LI which is now a container)
            if (ElementMapper::isContainerWidget(type)) {
                
                // Need to filter children? No, we process all.
                // But check if we have any valid children to output.
                bool hasChildren = !node.children.empty();
                
                // Special case: Button needs child. If empty, maybe create label from text?
                // Logic handled by TextBlock check above? No.
                // If Button has text node child, `build` will create a TextBlock child. Correct.
                
                // LI Marker logic
                bool isListItem = (lowerTag == "li" && !listCounters.empty());
                

                if (hasChildren || isListItem) {
                    hier << ",\n" << indent(indentLevel + 1) << "\"Children\": [\n";
                    
                    // CHECK FOR BUTTON WRAPPING requirement
                    // buttons can only have 1 child. If we have multiple (or marker + child), we must wrap.
                    // We only wrap if we have > 1 item to output.
                    std::vector<const DOMNode*> validChildren;
                    for (const auto& child : node.children) {
                        if (child.isTextNode()) {
                             std::string t = child.textContent;
                             t.erase(0, t.find_first_not_of(" \t\n\r"));
                             if (t.empty()) continue;
                        }
                        validChildren.push_back(&child);
                    }
                    
                    bool needsWrapping = (type == "Button" || type == "TransparentButton") && (validChildren.size() + (isListItem ? 1 : 0) > 1);

                    int childIndent = indentLevel + 2;
                    
                    if (needsWrapping) {
                        // Open Wrapper
                        hier << indent(indentLevel + 2) << "{\n";
                        hier << indent(indentLevel + 3) << "\"Type\": \"HorizontalBox\",\n";
                        hier << indent(indentLevel + 3) << "\"Name\": \"" << escapeJson(name) << "_Wrapper\",\n";
                         // Wrapper slot inside button - usually fill/auto. Button content slot is specific.
                         // But we just omit slot here, it takes default button slot behavior (Align/Pad).
                        hier << indent(indentLevel + 3) << "\"Children\": [\n";
                        childIndent = indentLevel + 4;
                    }

                    bool firstChild = true;
                    
                    // 1. Inject List Marker if needed
                    if (isListItem) {
                        int currentCounter = listCounters.back();
                        std::string markerText = (currentCounter == -1) ? "\u2022" : (std::to_string(currentCounter) + ".");
                        if (currentCounter != -1) listCounters.back()++; // Increment
                        
                        hier << indent(childIndent) << "{\n";
                        hier << indent(childIndent + 1) << "\"Type\": \"TextBlock\",\n";
                        hier << indent(childIndent + 1) << "\"Name\": \"" << escapeJson(name) << "_Marker\",\n";
                        hier << indent(childIndent + 1) << "\"Slot\": {\"Size\": {\"Rule\": \"Auto\"}, \"Padding\": {\"Right\": 10}},\n";
                        hier << indent(childIndent + 1) << "\"Text\": \"" << escapeJson(markerText) << "\",\n";
                        hier << indent(childIndent + 1) << "\"FontSize\": 16,\n";
                        hier << indent(childIndent + 1) << "\"AutoWrapText\": false\n";
                        hier << indent(childIndent) << "}";
                        
                        firstChild = false;
                    }
                    
                    // 2. Iterate actual children
                    for (const auto* child : validChildren) {
                        if (!firstChild) hier << ",\n";
                        firstChild = false;
                        
                        // Pass wrapping container type if we wrapped
                        SlotConfig::SlotType effectiveContainer = needsWrapping ? SlotConfig::SlotType::Box : myContainerType;
                        build(*child, childIndent, effectiveContainer);
                    }
                    
                    if (needsWrapping) {
                        hier << "\n" << indent(indentLevel + 3) << "]\n"; // Wrapper Children
                        hier << indent(indentLevel + 2) << "}"; // Wrapper
                    }
                    
                    hier << "\n" << indent(indentLevel + 1) << "]";
                }
            }

            hier << "\n" << indent(indentLevel) << "}";

            // DESIGN OUTPUT
            // Generate design entry if needed. TextBlocks usually need color. 
            // Containers need background color.
            // Buttons need style.
            
            std::ostringstream props;
            bool firstProp = true;
            
            // Color / Opacity
            if (node.computedStyles.count("color")) {
                Color c;
                if (StyleMapper::parseColor(node.computedStyles.at("color"), c)) {
                    props << indent(3) << "\"ColorAndOpacity\": " << colorToJson(c);
                    firstProp = false;
                }
            } else if (type == "TextBlock") {
                // Default black/white? UE default is white. Let's explicit white.
                props << indent(3) << "\"ColorAndOpacity\": {\"R\": 1, \"G\": 1, \"B\": 1, \"A\": 1}";
                firstProp = false;
            }
            
            // Background / WidgetStyle
            if (type == "Button" || type == "TransparentButton") {
                if (!firstProp) props << ",\n";
                // Force transparent style for now as per requirement
                props << indent(3) << "\"WidgetStyle\": {\n";
                props << indent(4) << "\"Normal\": {\"DrawAs\": \"None\"},\n";
                props << indent(4) << "\"Hovered\": {\"DrawAs\": \"None\"},\n";
                props << indent(4) << "\"Pressed\": {\"DrawAs\": \"None\"}\n";
                props << indent(3) << "}";
                firstProp = false;
            } else if (node.computedStyles.count("background-color")) {
                Color c;
                if (StyleMapper::parseColor(node.computedStyles.at("background-color"), c)) {
                    if (c.a > 0.0f) {
                        if (!firstProp) props << ",\n";
                        props << indent(3) << "\"Brush\": {\n";
                        props << indent(4) << "\"TintColor\": " << colorToJson(c) << ",\n";
                        props << indent(4) << "\"DrawAs\": \"Box\"\n";
                        props << indent(3) << "}";
                        firstProp = false;
                    }
                }
            }
            
            // Fonts (only if not TextBlock, because TextBlock has it inline? 
            // MWCS hierarchy vs design separation: 
            // TextBlock requires FontSize/Justification in Hierarchy for some reason (constructor args?), 
            // but mapped properties in Design are also applied.
            // Let's put Font in Design too for completeness/updates.
            
            // Actually, if we put it in Design, it might double apply or override. 
            // Let's include it.
            float fSize; std::string fType, fObj;
            // logic to get font settings from node
             int defaultSize = getDefaultFontSizeForTag(node.tagName);
             fSize = defaultSize > 0 ? (float)defaultSize : 16.0f;
             if (node.computedStyles.count("font-size")) {
                 std::string unit;
                 StyleMapper::parseSize(node.computedStyles.at("font-size"), fSize, unit);
             }
             fType = "Regular";
             if (node.computedStyles.count("font-weight")) {
                 fType = StyleMapper::mapFontWeight(node.computedStyles.at("font-weight"));
             } else {
                 if (isListRoot) { } // nothing
                 // check tag for bold
                 std::string tag = lowerTag;
                 if (tag == "h1" || tag == "h2" || tag == "h3" || tag == "h4" || tag == "h5" || tag == "h6" || tag == "strong" || tag == "b") {
                     fType = "Bold";
                 }
             }
             fObj = "/Engine/EngineFonts/Roboto.Roboto";

            if (fSize != 16.0f || fType != "Regular") {
                 if (!firstProp) props << ",\n";
                 props << indent(3) << "\"Font\": {\n";
                 props << indent(4) << "\"Size\": " << (int)fSize << ",\n";
                 props << indent(4) << "\"Typeface\": \"" << fType << "\",\n";
                 props << indent(4) << "\"FontObject\": \"" << escapeJson(fObj) << "\"\n";
                 props << indent(3) << "}";
                 firstProp = false;
            }
            
            if (!props.str().empty()) {
                design << ",\n" << indent(2) << "\"" << escapeJson(name) << "\": {\n";
                design << props.str() << "\n";
                design << indent(2) << "}";
            }

            if (isListRoot) listCounters.pop_back();
        }

    } builder{ctx};

    // GENERATE HIERARCHY
    std::ostringstream hier;
    hier << indent(1) << "\"Hierarchy\": {\n";
    hier << indent(2) << "\"Root\": {\n";
    hier << indent(3) << "\"Type\": \"CanvasPanel\",\n";
    hier << indent(3) << "\"Name\": \"RootCanvas\",\n";
    hier << indent(3) << "\"Children\": [\n";

    // ScrollBox Wrapper
    // ScrollBox Wrapper
    SlotConfig scrollSlot;
    scrollSlot.type = SlotConfig::SlotType::Canvas;
    scrollSlot.canvas.anchors = Anchors::Fill();
    scrollSlot.canvas.autoSize = false;

    hier << indent(4) << "{\n";
    hier << indent(5) << "\"Type\": \"ScrollBox\",\n";
    hier << indent(5) << "\"Name\": \"ContentScroll\",\n";
    hier << StyleMapper::slotToJson(scrollSlot, 5) << ",\n";
    hier << indent(5) << "\"Children\": [\n";
    
    // Content Root (Vertical)
    hier << indent(6) << "{\n";
    hier << indent(7) << "\"Type\": \"VerticalBox\",\n";
    hier << indent(7) << "\"Name\": \"ContentRoot\",\n";
    hier << indent(7) << "\"Slot\": {\"HAlign\": \"Fill\", \"VAlign\": \"Fill\"},\n";
    hier << indent(7) << "\"Children\": [\n";

    // Build children
    bool first = true;
    for (const auto& child : source.rootNode.children) {
        // Skip empty text nodes
        if (child.isTextNode()) {
            std::string t = child.textContent;
            t.erase(0, t.find_first_not_of(" \t\n\r"));
            if (t.empty()) continue;
        }

        if (!first) builder.hier << ",\n";
        first = false;
        builder.build(child, 8, SlotConfig::SlotType::Box);
    }
    
    hier << builder.hier.str();

    hier << "\n" << indent(7) << "]\n"; // ContentRoot Children
    hier << indent(6) << "}\n"; // ContentRoot
    hier << indent(5) << "]\n"; // ScrollBox Children
    hier << indent(4) << "}\n"; // ScrollBox
    hier << indent(3) << "]\n"; // RootCanvas Children
    hier << indent(2) << "}\n"; // Root
    hier << indent(1) << "}";   // Hierarchy

    spec.hierarchy = hier.str();

    // GENERATE DESIGN
    std::ostringstream design;
    design << indent(1) << "\"Design\": {\n";
    design << indent(2) << "\"RootCanvas\": { \"Note\": \"Main Container\" }";
    design << builder.design.str();
    design << "\n" << indent(1) << "}";
    spec.design = design.str();

    // GENERATE BINDINGS
    std::ostringstream bind;
    bind << indent(1) << "\"Bindings\": {\n";
    bind << indent(2) << "\"Required\": [\n";
    
    // We need to collect bindings from the generated structure. 
    // Since we built it recursively, we can just walk the naming context or tree?
    // Doing a separate pass is cleaner to separate concerns, or we could have collected them in Builder.
    // Let's do a quick separate pass on the DOM to find bindings, matching what we did in build()
    // NOTE: This assumes 1:1 mapping between DOM nodes and Widgets that need bindings.
    // Since `assignUniqueName` was consistent, we can just check `shouldCreateBinding`.
    
    std::vector<std::string> bindingsList;
    
    // Helper to recurse
    auto collectBindings = [&](auto&& self, const DOMNode& node) -> void {
        if (ElementMapper::shouldCreateBinding(node)) {
            std::string name = ctx.getName(&node);
            std::string type = ElementMapper::mapElementToWidget(node);
            std::string umgType = ElementMapper::getUMGClassName(type);
            
            std::ostringstream b;
            b << "{\"Name\": \"" << escapeJson(name) << "\", \"Type\": \"" << escapeJson(umgType) << "\"}";
            bindingsList.push_back(b.str());
        }
        for (const auto& child : node.children) {
            self(self, child);
        }
    };
    
    collectBindings(collectBindings, source.rootNode);

    for (size_t i = 0; i < bindingsList.size(); i++) {
        if (i > 0) bind << ",\n";
        bind << indent(3) << bindingsList[i];
    }

    bind << "\n" << indent(2) << "],\n";
    bind << indent(2) << "\"Optional\": []\n";
    bind << indent(1) << "}";
    spec.bindings = bind.str();

    spec.fullJson = spec.metadata + ",\n\n" + spec.hierarchy + ",\n\n" + spec.design + ",\n\n" + spec.bindings + "\n}";
    return spec;
}

std::string JSONGenerator::generateWidgetJson(const DOMNode& node, int indentLevel) {
    // Deprecated / Placeholder
    return "{}";
}

std::string JSONGenerator::indent(int level) {
    if (level < 0) level = 0;
    return std::string(level * 2, ' ');
}

std::string JSONGenerator::escapeJson(const std::string& str) {
    std::string out;
    out.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

std::string JSONGenerator::colorToJson(const Color& color) {
    std::ostringstream ss;
    ss << "{\"R\": " << color.r << ", \"G\": " << color.g << ", \"B\": " << color.b << ", \"A\": " << color.a << "}";
    return ss.str();
}

} // namespace awcg
