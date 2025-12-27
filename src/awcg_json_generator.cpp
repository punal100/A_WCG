// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#include "awcg_json_generator.h"
#include "awcg_element_mapper.h"
#include "awcg_style_mapper.h"
#include <sstream>
#include <map>
#include <set>

namespace awcg {

struct GenerationContext {
    std::map<const DOMNode*, std::string> nodeNames;
    std::map<std::string, int> nameCounts;
    
    void assignUniqueName(const DOMNode* node) {
        std::string baseName = ElementMapper::generateWidgetName(*node);
        int count = nameCounts[baseName]++;
        
        std::string uniqueName = baseName;
        if (count > 0 || (nameCounts[baseName] > 1 && count == 0)) {
             // If we know there will be multiple, we might want _0, but simple collision:
             // First: Name
             // Second: Name_1
             if (count > 0) uniqueName += "_" + std::to_string(count);
        }
        // Force suffix if we want specific behavior, but standard _1, _2 is fine.
        // Wait, loop back: if I have 4 NavLinks, I get NavLink, NavLink_1, NavLink_2, NavLink_3.
        
        nodeNames[node] = uniqueName;
    }
    
    void precomputeNames(const DOMNode& node) {
        // Skip text nodes mostly, but mappable ones need names
        if (!node.isTextNode() || ElementMapper::mapElementToWidget(node) != "TextBlock") {
             assignUniqueName(&node);
        }
        
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
    
    // 2. Hierarchy
    std::ostringstream hier;
    hier << indent(1) << "\"Hierarchy\": {\n";
    hier << indent(2) << "\"Root\": {\n";
    hier << indent(3) << "\"Type\": \"CanvasPanel\",\n";
    hier << indent(3) << "\"Name\": \"RootCanvas\",\n";
    hier << indent(3) << "\"Children\": [\n";
    
    bool first = true;
    for (const auto& child : source.rootNode.children) {
        if (child.isTextNode()) continue;
        if (!first) hier << ",\n";
        first = false;
        hier << generateWidgetJson(child, 4); 
        // Note: generateWidgetJson needs to be updated to use context and SKIP Design/Bindings
        // But since generateWidgetJson doesn't take context, we need a helper.
    }
    
    hier << "\n" << indent(3) << "]\n";
    hier << indent(2) << "}\n";
    hier << indent(1) << "}";
    spec.hierarchy = hier.str();
    
    // Refactor: We need a recursive helper that knows about context and separates Design
    // Since we cannot change existing generateWidgetJson strictly (it's static string), 
    // we'll implement a static internal helper here or use lambda.
    
    // Retrying hierarchy generation with separation logic inside separate function
    struct HierarchyBuilder {
        GenerationContext& ctx;
        SlotConfig::SlotType parentType;
        
        std::string build(const DOMNode& node, int indentLevel, SlotConfig::SlotType parentSlotType) {
            std::ostringstream json;
            std::string type = ElementMapper::mapElementToWidget(node);
            std::string name = ctx.getName(&node);
            
            // Determine this node's container type for its children
            SlotConfig::SlotType myContainerType = StyleMapper::determineContainerType(node.computedStyles);
            
            json << indent(indentLevel) << "{\n";
            json << indent(indentLevel + 1) << "\"Type\": \"" << escapeJson(type) << "\",\n";
            json << indent(indentLevel + 1) << "\"Name\": \"" << escapeJson(name) << "\"";
            
            // Visibility
            if (node.computedStyles.count("display") || node.computedStyles.count("visibility")) {
                std::string display = "block";
                if (node.computedStyles.count("display")) display = node.computedStyles.at("display");
                if (node.computedStyles.count("visibility")) {
                    std::string vis = node.computedStyles.at("visibility");
                    if (vis == "hidden") display = "hidden"; // Treat visibility:hidden as Hidden
                }
                
                std::string visibility = StyleMapper::mapVisibility(display);
                if (visibility != "Visible") {
                    json << ",\n" << indent(indentLevel + 1) << "\"Visibility\": \"" << visibility << "\"";
                }
            }
            
            if (ElementMapper::shouldCreateBinding(node)) {
                 json << ",\n" << indent(indentLevel + 1) << "\"BindingType\": \"Required\"";
            }
            
            // Generate Slot config based on parent type
            if (parentSlotType != SlotConfig::SlotType::None) {
                SlotConfig slotCfg = StyleMapper::generateSlotConfig(node.computedStyles, parentSlotType);
                std::string slotJson = StyleMapper::slotToJson(slotCfg, 0);
                if (!slotJson.empty()) {
                    // slotToJson outputs "Slot": {...}, we need to re-indent it
                    json << ",\n";
                    // Output slot inline
                    if (slotCfg.type == SlotConfig::SlotType::Canvas) {
                        const auto& c = slotCfg.canvas;
                        json << indent(indentLevel + 1) << "\"Slot\": {\n";
                        json << indent(indentLevel + 2) << "\"Anchors\": {\"Min\": {\"X\": " << c.anchors.min.x << ", \"Y\": " << c.anchors.min.y << "}, \"Max\": {\"X\": " << c.anchors.max.x << ", \"Y\": " << c.anchors.max.y << "}},\n";
                        json << indent(indentLevel + 2) << "\"Offsets\": {\"Left\": " << c.offsetLeft << ", \"Top\": " << c.offsetTop << ", \"Right\": " << c.offsetRight << ", \"Bottom\": " << c.offsetBottom << "},\n";
                        json << indent(indentLevel + 2) << "\"Alignment\": {\"X\": " << c.alignment.x << ", \"Y\": " << c.alignment.y << "},\n";
                        json << indent(indentLevel + 2) << "\"AutoSize\": " << (c.autoSize ? "true" : "false") << "\n";
                        json << indent(indentLevel + 1) << "}";
                    } else if (slotCfg.type == SlotConfig::SlotType::Box) {
                        const auto& b = slotCfg.box;
                        json << indent(indentLevel + 1) << "\"Slot\": {\n";
                        json << indent(indentLevel + 2) << "\"HAlign\": \"" << b.hAlign << "\",\n";
                        json << indent(indentLevel + 2) << "\"VAlign\": \"" << b.vAlign << "\",\n";
                        json << indent(indentLevel + 2) << "\"Padding\": {\"Left\": " << b.paddingLeft << ", \"Top\": " << b.paddingTop << ", \"Right\": " << b.paddingRight << ", \"Bottom\": " << b.paddingBottom << "},\n";
                        json << indent(indentLevel + 2) << "\"Size\": {\"Rule\": \"" << b.sizeRule << "\"";
                        if (b.sizeRule == "Fill") json << ", \"Value\": " << b.fillWeight;
                        json << "}\n";
                        json << indent(indentLevel + 1) << "}";
                    }
                }
            }
            
            // Text Content
             if (type == "TextBlock") {
                std::string text;
                for (const auto& child : node.children) {
                    if (child.isTextNode()) text += child.textContent;
                }
                // Trim
                text.erase(0, text.find_first_not_of(" \t\n\r"));
                if (text.find_last_not_of(" \t\n\r") != std::string::npos)
                    text.erase(text.find_last_not_of(" \t\n\r") + 1);
                
                if (!text.empty()) {
                    json << ",\n" << indent(indentLevel + 1) << "\"Text\": \"" << escapeJson(text) << "\"";
                }
            }
            
            // Children
             std::vector<const DOMNode*> validChildren;
            for (const auto& child : node.children) {
                if (!child.isTextNode() || (type != "TextBlock" && !child.textContent.empty())) {
                    if (!child.isTextNode()) validChildren.push_back(&child);
                }
            }
            
            if (!validChildren.empty()) {
                json << ",\n" << indent(indentLevel + 1) << "\"Children\": [\n";
                bool first = true;
                for (const auto* child : validChildren) {
                    if (!first) json << ",\n";
                    first = false;
                    json << build(*child, indentLevel + 2, myContainerType);
                }
                json << "\n" << indent(indentLevel + 1) << "]";
            }
            
            json << "\n" << indent(indentLevel) << "}";
            return json.str();
        }
    } hierBuilder{ctx, SlotConfig::SlotType::Canvas};
    
    // Re-do hierarchy string using builder
    std::ostringstream hier2;
    hier2 << indent(1) << "\"Hierarchy\": {\n";
    hier2 << indent(2) << "\"Root\": {\n";
    hier2 << indent(3) << "\"Type\": \"CanvasPanel\",\n";
    hier2 << indent(3) << "\"Name\": \"RootCanvas\",\n";
    hier2 << indent(3) << "\"Children\": [\n";
    
    first = true;
    for (const auto& child : source.rootNode.children) {
        if (child.isTextNode()) continue;
        if (!first) hier2 << ",\n";
        first = false;
        hier2 << hierBuilder.build(child, 4, SlotConfig::SlotType::Canvas);
    }
    
    hier2 << "\n" << indent(3) << "]\n";
    hier2 << indent(2) << "}\n";
    hier2 << indent(1) << "}";
    spec.hierarchy = hier2.str();
    
    // 3. Design
    std::ostringstream design;
    design << indent(1) << "\"Design\": {\n";
    design << indent(2) << "\"RootCanvas\": { \"Note\": \"Main Container\" }";
    
    struct DesignBuilder {
        GenerationContext& ctx;
        std::ostringstream& out;
        
        void build(const DOMNode& node) {
            if (node.computedStyles.empty()) {
                 // Even if no styles, recurse
                 for (const auto& child : node.children) {
                    if (!child.isTextNode()) build(child);
                 }
                 return;
            }

            std::string name = ctx.getName(&node);
            std::ostringstream props;
            bool firstProp = true;

            // 1. ColorAndOpacity (Text Color context)
            if (node.computedStyles.count("color")) {
                Color color;
                if (StyleMapper::parseColor(node.computedStyles.at("color"), color)) {
                    if (!firstProp) props << ",\n";
                    props << indent(3) << "\"ColorAndOpacity\": " << colorToJson(color);
                    firstProp = false;
                }
            }

            // 2. Brush (Background Color)
            if (node.computedStyles.count("background-color")) {
                Color color;
                if (StyleMapper::parseColor(node.computedStyles.at("background-color"), color)) {
                    // Ignore transparent or fully alpha-0 backgrounds unless explicit
                    if (color.a > 0.0f) {
                        if (!firstProp) props << ",\n";
                        props << indent(3) << "\"Brush\": {\n";
                        props << indent(4) << "\"TintColor\": " << colorToJson(color) << ",\n";
                        props << indent(4) << "\"DrawAs\": \"Box\"\n"; // Default to Box for simple backgrounds
                        props << indent(3) << "}";
                        firstProp = false;
                    }
                }
            }

            // 3. Font Configuration
            if (node.computedStyles.count("font-size") || node.computedStyles.count("font-weight")) {
                float size = 24.0f; // Default
                std::string unit;
                std::string weight = "Regular";
                std::string family = "/Engine/EngineFonts/Roboto.Roboto";
                
                bool hasFont = false;
                
                if (node.computedStyles.count("font-size")) {
                    StyleMapper::parseSize(node.computedStyles.at("font-size"), size, unit);
                    hasFont = true;
                }
                if (node.computedStyles.count("font-weight")) {
                    weight = StyleMapper::mapFontWeight(node.computedStyles.at("font-weight"));
                    hasFont = true;
                }
                if (node.computedStyles.count("font-family")) {
                   family = StyleMapper::mapFontFamily(node.computedStyles.at("font-family"));
                   hasFont = true;
                }

                if (hasFont) {
                    if (!firstProp) props << ",\n";
                    props << indent(3) << "\"Font\": {\n";
                    props << indent(4) << "\"Size\": " << (int)size << ",\n";
                    props << indent(4) << "\"Typeface\": \"" << weight << "\",\n";
                    props << indent(4) << "\"FontObject\": \"" << escapeJson(family) << "\"\n";
                    props << indent(3) << "}";
                    firstProp = false;
                }
            }
            
            // 4. Render Opacity
            if (node.computedStyles.count("opacity")) {
                float opacity = 1.0f;
                std::string unit;
                if (StyleMapper::parseSize(node.computedStyles.at("opacity"), opacity, unit)) {
                    if (opacity < 1.0f) {
                        if (!firstProp) props << ",\n";
                        props << indent(3) << "\"RenderOpacity\": " << opacity;
                        firstProp = false;
                    }
                }
            }
            
            // 5. Justification (Text Align)
            if (node.computedStyles.count("text-align")) {
                 std::string align = StyleMapper::mapTextAlign(node.computedStyles.at("text-align"));
                 if (!firstProp) props << ",\n";
                 props << indent(3) << "\"Justification\": \"" << align << "\"";
                 firstProp = false;
            }

            std::string propsStr = props.str();
            if (!propsStr.empty()) {
                out << ",\n" << indent(2) << "\"" << escapeJson(name) << "\": {\n";
                out << propsStr << "\n";
                out << indent(2) << "}";
            }
            
            // Recurse
            for (const auto& child : node.children) {
                if (!child.isTextNode()) build(child);
            }
        }
    } designBuilder{ctx, design};
    
    // Visit all nodes to extract design
    for (const auto& child : source.rootNode.children) {
        if (!child.isTextNode()) designBuilder.build(child);
    }
    
    design << "\n" << indent(1) << "}";
    spec.design = design.str();
    
    // 4. Bindings
    std::ostringstream bind;
    bind << indent(1) << "\"Bindings\": {\n";
    bind << indent(2) << "\"Required\": [\n";
    
    std::vector<std::string> bindingsList;
    
    struct BindingsBuilder {
        GenerationContext& ctx;
        std::vector<std::string>& list;
        std::vector<std::pair<std::string, std::string>>& headerList;
        
        void build(const DOMNode& node) {
            if (ElementMapper::shouldCreateBinding(node)) {
                std::string name = ctx.getName(&node);
                std::string type = ElementMapper::getUMGClassName(ElementMapper::mapElementToWidget(node));
                
                std::ostringstream b;
                b << "{\"Name\": \"" << escapeJson(name) << "\", \"Type\": \"" << escapeJson(type) << "\"}";
                list.push_back(b.str());
                headerList.emplace_back(name, type);
            }
            for (const auto& child : node.children) {
                if (!child.isTextNode()) build(child);
            }
        }
    } bindingsBuilder{ctx, bindingsList, spec.bindingList};
    
    for (const auto& child : source.rootNode.children) {
        if (!child.isTextNode()) bindingsBuilder.build(child);
    }
    
    for (size_t i = 0; i < bindingsList.size(); i++) {
        if (i > 0) bind << ",\n";
        bind << indent(3) << bindingsList[i];
    }
    
    bind << "\n" << indent(2) << "],\n";
    bind << indent(2) << "\"Optional\": []\n";
    bind << indent(1) << "}";
    spec.bindings = bind.str();
    
    // Full JSON
    spec.fullJson = spec.metadata + ",\n\n" + spec.hierarchy + ",\n\n" + spec.design + ",\n\n" + spec.bindings + "\n}";
    
    return spec;
}

std::string JSONGenerator::generateWidgetJson(const DOMNode& node, int indentLevel) {
    // Legacy support/fallback (modified to use stateless local generation if needed, or minimal)
    // For now we just return empty or error? 
    // Actually main.cpp calls generate(), which calls generateChunked. 
    // generateWidgetJson is public though.
    // We'll leave a minimal implementation that includes Design inline just in case.
    std::ostringstream json;
    std::string widgetType = ElementMapper::mapElementToWidget(node);
    std::string widgetName = ElementMapper::generateWidgetName(node);
    
    json << indent(indentLevel) << "{\n";
    json << indent(indentLevel + 1) << "\"Type\": \"" << escapeJson(widgetType) << "\",\n";
    json << indent(indentLevel + 1) << "\"Name\": \"" << escapeJson(widgetName) << "\"\n";
    json << indent(indentLevel) << "}";
    return json.str();
}

std::string JSONGenerator::indent(int level) {
    return std::string(level * 2, ' ');
}

std::string JSONGenerator::escapeJson(const std::string& str) {
    std::ostringstream result;
    for (char c : str) {
        switch (c) {
            case '"': result << "\\\""; break;
            case '\\': result << "\\\\"; break;
            case '\n': result << "\\n"; break;
            case '\r': result << "\\r"; break;
            case '\t': result << "\\t"; break;
            default: result << c;
        }
    }
    return result.str();
}

std::string JSONGenerator::colorToJson(const Color& color) {
    std::ostringstream json;
    json << "{\"R\": " << color.r << ", \"G\": " << color.g 
         << ", \"B\": " << color.b << ", \"A\": " << color.a << "}";
    return json.str();
}

} // namespace awcg
