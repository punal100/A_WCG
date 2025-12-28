// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#include "awcg_element_mapper.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>

namespace awcg {

static const std::map<std::string, std::string>& getElementMapping() {
    static std::map<std::string, std::string> mapping = {
        {"div", "Container"}, {"section", "VerticalBox"}, {"article", "VerticalBox"},
        {"main", "CanvasPanel"}, {"header", "VerticalBox"}, {"footer", "VerticalBox"},
        {"nav", "HorizontalBox"}, {"aside", "VerticalBox"}, {"form", "VerticalBox"},
        {"span", "TextBlock"}, {"p", "TextBlock"}, {"h1", "TextBlock"}, {"h2", "TextBlock"},
        {"h3", "TextBlock"}, {"h4", "TextBlock"}, {"h5", "TextBlock"}, {"h6", "TextBlock"},
        {"label", "TextBlock"}, {"button", "Button"}, {"a", "Button"}, {"img", "Image"},
        {"input", "Input"}, {"textarea", "MultiLineEditableTextBox"}, {"select", "ComboBoxString"},
        {"ul", "VerticalBox"}, {"ol", "VerticalBox"}, {"li", "HorizontalBox"},
        {"table", "GridPanel"}, {"tr", "HorizontalBox"}, {"td", "Border"}, {"th", "Border"}
    };
    return mapping;
}

static const std::map<std::string, std::string>& getInputTypeMapping() {
    static std::map<std::string, std::string> mapping = {
        {"text", "EditableTextBox"}, {"password", "EditableTextBox"}, {"email", "EditableTextBox"},
        {"number", "EditableTextBox"}, {"search", "EditableTextBox"}, {"tel", "EditableTextBox"},
        {"url", "EditableTextBox"}, {"checkbox", "CheckBox"}, {"radio", "CheckBox"},
        {"button", "Button"}, {"submit", "Button"}, {"reset", "Button"},
        {"range", "Slider"}, {"file", "Button"}
    };
    return mapping;
}

std::string ElementMapper::mapElementToWidget(const DOMNode& node) {
    if (node.isTextNode()) return "TextBlock";

    std::string tagName = node.tagName;
    std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);

    if (tagName == "input") {
        std::string inputType = node.getAttribute("type");
        std::transform(inputType.begin(), inputType.end(), inputType.begin(), ::tolower);
        if (inputType.empty()) inputType = "text";

        const auto& inputMapping = getInputTypeMapping();
        auto it = inputMapping.find(inputType);
        if (it != inputMapping.end()) return it->second;
        return "EditableTextBox";
    }

    if (tagName == "div") {
        // Check for background image
        if (node.computedStyles.count("background-image")) {
             std::string bg = node.computedStyles.at("background-image");
             // Simple check for url() and not none
             if (bg.find("url(") != std::string::npos && bg.find("none") == std::string::npos) {
                 return "Image";
             }
        }
        
        // NEW: Check if this div contains ONLY text content (no element children)
        // If so, treat it as a TextBlock to preserve text
        bool hasElementChildren = false;
        bool hasTextContent = false;
        for (const auto& child : node.children) {
            if (child.isTextNode()) {
                std::string trimmed = child.textContent;
                trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
                if (trimmed.find_last_not_of(" \t\n\r") != std::string::npos) {
                    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
                }
                if (!trimmed.empty()) {
                    hasTextContent = true;
                }
            } else {
                hasElementChildren = true;
            }
        }
        
        // If div has text but no element children, treat as TextBlock
        if (hasTextContent && !hasElementChildren) {
            return "TextBlock";
        }
        
        return determineContainerType(node);
    }

    const auto& mapping = getElementMapping();
    auto it = mapping.find(tagName);
    if (it != mapping.end()) {
        if (it->second == "Container") return determineContainerType(node);
        return it->second;
    }

    return "VerticalBox";
}

std::string ElementMapper::determineContainerType(const DOMNode& node) {
    auto displayIt = node.computedStyles.find("display");
    auto flexDirIt = node.computedStyles.find("flex-direction");
    auto positionIt = node.computedStyles.find("position");

    if (positionIt != node.computedStyles.end()) {
        std::string pos = positionIt->second;
        std::transform(pos.begin(), pos.end(), pos.begin(), ::tolower);
        if (pos == "absolute" || pos == "fixed") return "CanvasPanel";
    }

    if (displayIt != node.computedStyles.end()) {
        std::string display = displayIt->second;
        std::transform(display.begin(), display.end(), display.begin(), ::tolower);

        if (display == "none") return "VerticalBox";

        if (display == "flex" || display == "inline-flex") {
            if (flexDirIt != node.computedStyles.end()) {
                std::string dir = flexDirIt->second;
                std::transform(dir.begin(), dir.end(), dir.begin(), ::tolower);
                if (dir == "column" || dir == "column-reverse") return "VerticalBox";
            }
            return "HorizontalBox";
        }

        if (display == "grid" || display == "inline-grid") return "CanvasPanel";
        if (display == "inline" || display == "inline-block") return "HorizontalBox";
    }

    return "VerticalBox";
}

bool ElementMapper::shouldCreateBinding(const DOMNode& node) {
    if (!node.id.empty()) return true;
    if (isInteractiveElement(node)) return true;
    if (isInputElement(node)) return true;
    return false;
}

std::string ElementMapper::getUMGClassName(const std::string& widgetType) {
    static std::map<std::string, std::string> classNames = {
        {"CanvasPanel", "UCanvasPanel"}, {"VerticalBox", "UVerticalBox"},
        {"HorizontalBox", "UHorizontalBox"}, {"Button", "UButton"},
        {"TextBlock", "UTextBlock"}, {"Image", "UImage"},
        {"EditableTextBox", "UEditableTextBox"}, {"MultiLineEditableTextBox", "UMultiLineEditableTextBox"},
        {"CheckBox", "UCheckBox"}, {"ComboBoxString", "UComboBoxString"},
        {"Slider", "USlider"}, {"ProgressBar", "UProgressBar"},
        {"ScrollBox", "UScrollBox"}, {"Border", "UBorder"},
        {"Overlay", "UOverlay"}, {"GridPanel", "UGridPanel"}, {"Spacer", "USpacer"}
    };

    auto it = classNames.find(widgetType);
    if (it != classNames.end()) return it->second;
    return "U" + widgetType;
}

std::string ElementMapper::toValidIdentifier(const std::string& htmlName) {
    if (htmlName.empty()) return "Widget";

    std::string result;
    bool capitalizeNext = true;

    for (char c : htmlName) {
        if (c == '-' || c == '_' || c == ' ') {
            capitalizeNext = true;
            continue;
        }

        if (std::isalnum(static_cast<unsigned char>(c))) {
            if (capitalizeNext) {
                result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                capitalizeNext = false;
            } else {
                result += c;
            }
        }
    }

    if (!result.empty() && std::isdigit(static_cast<unsigned char>(result[0]))) {
        result = "W" + result;
    }

    return result.empty() ? "Widget" : result;
}

std::string ElementMapper::generateWidgetName(const DOMNode& node, int index) {
    if (!node.id.empty()) return toValidIdentifier(node.id);
    if (!node.classes.empty()) return toValidIdentifier(node.classes[0]);

    std::string tagPascal = toValidIdentifier(node.tagName);
    if (index > 0) return tagPascal + std::to_string(index);
    return tagPascal;
}

bool ElementMapper::isInteractiveElement(const DOMNode& node) {
    static std::set<std::string> interactive = {"button", "a", "input", "select", "textarea", "label"};
    std::string tag = node.tagName;
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
    return interactive.count(tag) > 0;
}

bool ElementMapper::isTextElement(const DOMNode& node) {
    static std::set<std::string> text = {"span", "p", "h1", "h2", "h3", "h4", "h5", "h6", "label", "strong", "em", "b", "i", "small", "mark"};
    std::string tag = node.tagName;
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
    return text.count(tag) > 0 || node.isTextNode();
}

bool ElementMapper::isInputElement(const DOMNode& node) {
    static std::set<std::string> inputs = {"input", "textarea", "select"};
    std::string tag = node.tagName;
    std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
    return inputs.count(tag) > 0;
}

bool ElementMapper::isContainerWidget(const std::string& widgetType) {
    static std::set<std::string> containers = {
        "CanvasPanel", "VerticalBox", "HorizontalBox", "Button", 
        "ScrollBox", "Border", "Overlay", "GridPanel", "Container"
    };
    return containers.count(widgetType) > 0;
}

} // namespace awcg
