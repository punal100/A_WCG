// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator
// Standalone CLI Tool - No Unreal Engine Dependencies

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace awcg {

/**
 * ============================================================================
 * A_WCG Type Definitions
 * 
 * Core data structures for web source parsing and conversion.
 * These types represent parsed HTML/CSS/JS content before conversion
 * to P_MWCS-compatible widget specifications.
 * ============================================================================
 */

/**
 * CSS property-value pair
 */
struct CSSProperty {
    std::string name;
    std::string value;
    bool important = false;

    CSSProperty() = default;
    CSSProperty(const std::string& n, const std::string& v, bool imp = false)
        : name(n), value(v), important(imp) {}
};

/**
 * CSS rule (selector + declarations)
 */
struct CSSRule {
    std::string selector;
    std::vector<CSSProperty> properties;
};

/**
 * Parsed stylesheet
 */
struct StyleSheet {
    std::vector<CSSRule> rules;
};

/**
 * JavaScript variable declaration
 */
struct JSVariable {
    std::string name;
    std::string declarationType;  // "const", "let", "var"
    std::string inferredCppType;  // "int32", "std::string", "bool", etc.
    std::string defaultValue;
};

/**
 * JavaScript function declaration
 */
struct JSFunction {
    std::string name;
    std::vector<std::string> parameters;
    bool isAsync = false;  // Not supported, flagged for error
};

/**
 * JavaScript event handler binding
 */
struct JSEventHandler {
    std::string elementSelector;
    std::string eventType;      // "click", "mouseenter", "change"
    std::string functionName;
    std::string umgDelegateType; // "OnClicked", "OnHovered"
};

/**
 * Parsed JavaScript information
 */
struct ScriptInfo {
    std::vector<JSVariable> variables;
    std::vector<JSFunction> functions;
    std::vector<JSEventHandler> eventHandlers;
};

/**
 * DOM Node representation
 */
struct DOMNode {
    std::string tagName;       // "div", "button", or "#text"
    std::string id;
    std::vector<std::string> classes;
    std::map<std::string, std::string> attributes;
    std::string textContent;
    std::vector<CSSProperty> inlineStyles;
    std::map<std::string, std::string> computedStyles;
    std::vector<DOMNode> children;
    bool selfClosing = false;

    bool isTextNode() const { return tagName == "#text"; }
    bool hasClass(const std::string& className) const {
        return std::find(classes.begin(), classes.end(), className) != classes.end();
    }
    std::string getAttribute(const std::string& name) const {
        auto it = attributes.find(name);
        return it != attributes.end() ? it->second : "";
    }
};

/**
 * Complete parsed web source
 */
struct ParsedWebSource {
    // Source paths
    std::string htmlPath;
    std::string cssPath;
    std::string jsPath;

    // Parsed content
    DOMNode rootNode;
    StyleSheet styleSheet;
    ScriptInfo scriptInfo;

    // Dependencies
    std::vector<std::string> dependencies;

    // Metadata
    std::string pageTitle;
    bool parseSuccessful = false;
    std::vector<std::string> parseErrors;
};

/**
 * Error codes (aligned with PLAN.md §2.8)
 */
enum class ErrorCode {
    None = 0,
    E001_ParseError = 1,      // HTML/CSS/JS invalid syntax
    E002_MappingError = 2,    // Unsupported element or property
    E003_SchemaError = 3,     // Generated JSON rejected by P_MWCS
    E004_ParityError = 4      // Round-trip mismatch
};

/**
 * Diagnostic message
 */
struct Diagnostic {
    ErrorCode code = ErrorCode::None;
    std::string message;
    std::string filePath;
    int lineNumber = 0;
    int columnNumber = 0;

    std::string getErrorCodeString() const {
        switch (code) {
            case ErrorCode::E001_ParseError: return "E001";
            case ErrorCode::E002_MappingError: return "E002";
            case ErrorCode::E003_SchemaError: return "E003";
            case ErrorCode::E004_ParityError: return "E004";
            default: return "";
        }
    }

    std::string toString() const {
        if (lineNumber > 0) {
            return "[" + getErrorCodeString() + "] " + filePath + ":" + 
                   std::to_string(lineNumber) + ":" + std::to_string(columnNumber) + 
                   ": " + message;
        }
        return "[" + getErrorCodeString() + "] " + message;
    }
};

/**
 * Conversion result
 */
struct ConversionResult {
    bool success = false;
    std::string jsonSpec;
    std::string cppHeader;
    std::string cppSource;
    std::vector<Diagnostic> diagnostics;

    bool hasErrors() const {
        for (const auto& diag : diagnostics) {
            if (diag.code != ErrorCode::None) return true;
        }
        return false;
    }
};

/**
 * RGBA Color (0-1 range)
 */
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    Color(float r_, float g_, float b_, float a_ = 1.0f)
        : r(r_), g(g_), b(b_), a(a_) {}
};

/**
 * 2D Vector for positions/sizes
 */
struct Vector2D {
    float x = 0.0f;
    float y = 0.0f;
    
    Vector2D() = default;
    Vector2D(float x_, float y_) : x(x_), y(y_) {}
};

/**
 * Anchor configuration (Min/Max points)
 */
struct Anchors {
    Vector2D min = {0.0f, 0.0f};
    Vector2D max = {0.0f, 0.0f};
    
    // Common presets
    static Anchors TopLeft() { return {{0,0}, {0,0}}; }
    static Anchors TopCenter() { return {{0.5f,0}, {0.5f,0}}; }
    static Anchors TopRight() { return {{1,0}, {1,0}}; }
    static Anchors CenterLeft() { return {{0,0.5f}, {0,0.5f}}; }
    static Anchors Center() { return {{0.5f,0.5f}, {0.5f,0.5f}}; }
    static Anchors CenterRight() { return {{1,0.5f}, {1,0.5f}}; }
    static Anchors BottomLeft() { return {{0,1}, {0,1}}; }
    static Anchors BottomCenter() { return {{0.5f,1}, {0.5f,1}}; }
    static Anchors BottomRight() { return {{1,1}, {1,1}}; }
    static Anchors FillHorizontal() { return {{0,0}, {1,0}}; }
    static Anchors FillVertical() { return {{0,0}, {0,1}}; }
    static Anchors Fill() { return {{0,0}, {1,1}}; }
};

/**
 * Slot configuration for CanvasPanel children
 */
struct CanvasSlotConfig {
    Anchors anchors;
    float offsetLeft = 0.0f;
    float offsetTop = 0.0f;
    float offsetRight = 0.0f;
    float offsetBottom = 0.0f;
    Vector2D alignment = {0.0f, 0.0f};
    bool autoSize = true;
    int zOrder = 0;
};

/**
 * Slot configuration for Box (Horizontal/Vertical) children
 */
struct BoxSlotConfig {
    std::string hAlign = "Fill";   // Left, Center, Right, Fill
    std::string vAlign = "Fill";   // Top, Center, Bottom, Fill
    float paddingLeft = 0.0f;
    float paddingTop = 0.0f;
    float paddingRight = 0.0f;
    float paddingBottom = 0.0f;
    std::string sizeRule = "Auto"; // Auto, Fill
    float fillWeight = 1.0f;
};

/**
 * Unified slot configuration
 */
struct SlotConfig {
    enum class SlotType { None, Canvas, Box, Overlay };
    SlotType type = SlotType::None;
    CanvasSlotConfig canvas;
    BoxSlotConfig box;
};

} // namespace awcg
