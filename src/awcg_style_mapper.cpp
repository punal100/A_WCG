// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#include "awcg_style_mapper.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <map>
#include <cmath>

namespace awcg {

static const std::map<std::string, Color>& getNamedColors() {
    static std::map<std::string, Color> colors = {
        {"black", {0, 0, 0, 1}}, {"white", {1, 1, 1, 1}},
        {"red", {1, 0, 0, 1}}, {"green", {0, 0.5f, 0, 1}}, {"blue", {0, 0, 1, 1}},
        {"yellow", {1, 1, 0, 1}}, {"cyan", {0, 1, 1, 1}}, {"magenta", {1, 0, 1, 1}},
        {"gray", {0.5f, 0.5f, 0.5f, 1}}, {"grey", {0.5f, 0.5f, 0.5f, 1}},
        {"silver", {0.75f, 0.75f, 0.75f, 1}}, {"maroon", {0.5f, 0, 0, 1}},
        {"olive", {0.5f, 0.5f, 0, 1}}, {"lime", {0, 1, 0, 1}},
        {"aqua", {0, 1, 1, 1}}, {"teal", {0, 0.5f, 0.5f, 1}},
        {"navy", {0, 0, 0.5f, 1}}, {"fuchsia", {1, 0, 1, 1}},
        {"purple", {0.5f, 0, 0.5f, 1}}, {"orange", {1, 0.647f, 0, 1}},
        {"pink", {1, 0.753f, 0.796f, 1}}, {"brown", {0.647f, 0.165f, 0.165f, 1}},
        {"transparent", {0, 0, 0, 0}}
    };
    return colors;
}

static int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

bool StyleMapper::parseColor(const std::string& cssColor, Color& outColor) {
    std::string color = cssColor;
    size_t first = color.find_first_not_of(" \t");
    if (first == std::string::npos) return false;
    color.erase(0, first);
    
    size_t last = color.find_last_not_of(" \t");
    if (last != std::string::npos)
        color.erase(last + 1);
    std::transform(color.begin(), color.end(), color.begin(), ::tolower);

    if (color.empty()) return false;
    if (color[0] == '#') return parseHexColor(color, outColor);
    if (color.substr(0, 3) == "rgb") return parseRgbColor(color, outColor);
    return parseNamedColor(color, outColor);
}

bool StyleMapper::parseHexColor(const std::string& hexColor, Color& outColor) {
    std::string hex = hexColor.substr(1);
    int r = 0, g = 0, b = 0, a = 255;

    if (hex.length() == 3) {
        r = hexDigit(hex[0]) * 17; g = hexDigit(hex[1]) * 17; b = hexDigit(hex[2]) * 17;
    } else if (hex.length() == 4) {
        r = hexDigit(hex[0]) * 17; g = hexDigit(hex[1]) * 17; b = hexDigit(hex[2]) * 17; a = hexDigit(hex[3]) * 17;
    } else if (hex.length() == 6) {
        r = hexDigit(hex[0]) * 16 + hexDigit(hex[1]);
        g = hexDigit(hex[2]) * 16 + hexDigit(hex[3]);
        b = hexDigit(hex[4]) * 16 + hexDigit(hex[5]);
    } else if (hex.length() == 8) {
        r = hexDigit(hex[0]) * 16 + hexDigit(hex[1]);
        g = hexDigit(hex[2]) * 16 + hexDigit(hex[3]);
        b = hexDigit(hex[4]) * 16 + hexDigit(hex[5]);
        a = hexDigit(hex[6]) * 16 + hexDigit(hex[7]);
    } else {
        return false;
    }

    outColor = Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    return true;
}

bool StyleMapper::parseRgbColor(const std::string& rgbColor, Color& outColor) {
    size_t parenStart = rgbColor.find('(');
    size_t parenEnd = rgbColor.find(')');
    if (parenStart == std::string::npos || parenEnd == std::string::npos) return false;

    std::string values = rgbColor.substr(parenStart + 1, parenEnd - parenStart - 1);
    std::stringstream ss(values);
    std::string part;
    std::vector<float> components;

    while (std::getline(ss, part, ',')) {
        part.erase(0, part.find_first_not_of(" \t"));
        if (part.find_last_not_of(" \t") != std::string::npos)
            part.erase(part.find_last_not_of(" \t") + 1);
        components.push_back(std::stof(part));
    }

    if (components.size() < 3) return false;

    float r = components[0], g = components[1], b = components[2];
    float a = components.size() >= 4 ? components[3] : 1.0f;

    if (values.find('%') != std::string::npos) {
        r /= 100.0f; g /= 100.0f; b /= 100.0f;
    } else {
        r /= 255.0f; g /= 255.0f; b /= 255.0f;
    }

    outColor = Color(
        std::min(1.0f, std::max(0.0f, r)),
        std::min(1.0f, std::max(0.0f, g)),
        std::min(1.0f, std::max(0.0f, b)),
        std::min(1.0f, std::max(0.0f, a))
    );
    return true;
}

bool StyleMapper::parseNamedColor(const std::string& colorName, Color& outColor) {
    const auto& colors = getNamedColors();
    auto it = colors.find(colorName);
    if (it != colors.end()) {
        outColor = it->second;
        return true;
    }
    return false;
}

bool StyleMapper::parseSize(const std::string& cssSize, float& outValue, std::string& outUnit) {
    std::string size = cssSize;
    size_t first = size.find_first_not_of(" \t");
    if (first == std::string::npos) return false;
    size.erase(0, first);
    
    size_t last = size.find_last_not_of(" \t");
    if (last != std::string::npos)
        size.erase(last + 1);
    std::transform(size.begin(), size.end(), size.begin(), ::tolower);

    if (size.empty()) return false;

    std::string numericPart, unitPart;
    for (size_t i = 0; i < size.length(); i++) {
        char c = size[i];
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '+') {
            numericPart += c;
        } else {
            unitPart = size.substr(i);
            break;
        }
    }

    if (numericPart.empty()) return false;

    outValue = std::stof(numericPart);
    outUnit = unitPart.empty() ? "px" : unitPart;
    return true;
}

std::string StyleMapper::mapFontWeight(const std::string& fontWeight) {
    std::string weight = fontWeight;
    
    // Trim
    size_t first = weight.find_first_not_of(" \t");
    if (first == std::string::npos) return "Regular";
    weight.erase(0, first);
    
    size_t last = weight.find_last_not_of(" \t");
    if (last != std::string::npos)
        weight.erase(last + 1);
        
    std::transform(weight.begin(), weight.end(), weight.begin(), ::tolower);

    if (weight == "normal") return "Regular";
    if (weight == "bold") return "Bold";
    if (weight == "lighter") return "Light";
    if (weight == "bolder") return "Bold";

    // Validate if it's a number
    bool isNumber = !weight.empty() && std::all_of(weight.begin(), weight.end(), ::isdigit);
    if (!isNumber) return "Regular";

    try {
        int numWeight = std::stoi(weight);
        if (numWeight < 400) return "Light";
        if (numWeight < 600) return "Regular";
        if (numWeight < 800) return "Bold";
        return "Black";
    } catch (...) {
        return "Regular";
    }
}

std::string StyleMapper::mapTextAlign(const std::string& textAlign) {
    std::string align = textAlign;
    std::transform(align.begin(), align.end(), align.begin(), ::tolower);
    size_t first = align.find_first_not_of(" \t");
    if (first != std::string::npos) {
        align.erase(0, first);
        size_t last = align.find_last_not_of(" \t");
        if (last != std::string::npos)
            align.erase(last + 1);
    }

    if (align == "left" || align == "start") return "Left";
    if (align == "center") return "Center";
    if (align == "right" || align == "end") return "Right";
    return "Left";
}

std::string StyleMapper::mapVisibility(const std::string& display) {
    std::string disp = display;
    std::transform(disp.begin(), disp.end(), disp.begin(), ::tolower);

    if (disp == "none") return "Collapsed";
    if (disp == "hidden") return "Hidden";
    return "Visible";
}

std::string StyleMapper::mapPropertyName(const std::string& cssProperty) {
    static std::map<std::string, std::string> propertyMap = {
        {"color", "ColorAndOpacity"}, {"font-size", "Font.Size"},
        {"font-family", "Font.FontObject"}, {"font-weight", "Font.Typeface"},
        {"text-align", "Justification"}, {"background-color", "Brush.TintColor"},
        {"background", "Brush.TintColor"}, {"width", "Size.X"}, {"height", "Size.Y"},
        {"min-width", "MinDesiredSize.X"}, {"min-height", "MinDesiredSize.Y"},
        {"max-width", "MaxDesiredSize.X"}, {"max-height", "MaxDesiredSize.Y"},
        {"margin", "Slot.Padding"}, {"padding", "Padding"}, {"opacity", "RenderOpacity"},
        {"align-items", "Slot.VAlign"}, {"justify-content", "Slot.HAlign"}
    };

    std::string prop = cssProperty;
    std::transform(prop.begin(), prop.end(), prop.begin(), ::tolower);
    auto it = propertyMap.find(prop);
    return it != propertyMap.end() ? it->second : "";
}

bool StyleMapper::parseSpacing(const std::string& cssValue,
                               float& outTop, float& outRight,
                               float& outBottom, float& outLeft) {
    std::stringstream ss(cssValue);
    std::string part;
    std::vector<float> values;
    std::string unit;

    while (ss >> part) {
        float val;
        if (parseSize(part, val, unit)) {
            values.push_back(val);
        }
    }

    if (values.size() == 1) {
        outTop = outRight = outBottom = outLeft = values[0];
        return true;
    } else if (values.size() == 2) {
        outTop = outBottom = values[0];
        outRight = outLeft = values[1];
        return true;
    } else if (values.size() == 3) {
        outTop = values[0];
        outRight = outLeft = values[1];
        outBottom = values[2];
        return true;
    } else if (values.size() >= 4) {
        outTop = values[0]; outRight = values[1];
        outBottom = values[2]; outLeft = values[3];
        return true;
    }

    return false;
}

std::string StyleMapper::mapFontFamily(const std::string& fontFamily) {
    // Default to Roboto
    return "/Engine/EngineFonts/Roboto.Roboto";
}

// Helper to get style value with default
static std::string getStyle(const std::map<std::string, std::string>& styles, 
                            const std::string& prop, 
                            const std::string& defaultVal = "") {
    auto it = styles.find(prop);
    return it != styles.end() ? it->second : defaultVal;
}

// Helper to check if value indicates percentage/fill
static bool isPercentageFill(const std::string& value) {
    return value == "100%" || value == "100vh" || value == "100vw";
}

SlotConfig::SlotType StyleMapper::determineContainerType(const std::map<std::string, std::string>& styles) {
    std::string display = getStyle(styles, "display", "block");
    std::string position = getStyle(styles, "position", "static");
    
    if (display == "flex" || display == "inline-flex") {
        return SlotConfig::SlotType::Box;
    }
    if (position == "absolute" || position == "fixed") {
        return SlotConfig::SlotType::Canvas;
    }
    // Default to Box for flow layouts
    return SlotConfig::SlotType::Box;
}

bool StyleMapper::shouldFillParent(const std::map<std::string, std::string>& styles) {
    std::string width = getStyle(styles, "width");
    std::string height = getStyle(styles, "height");
    std::string position = getStyle(styles, "position");
    std::string display = getStyle(styles, "display");
    
    // Check for 100% or viewport units (both dimensions)
    if (isPercentageFill(width) && isPercentageFill(height)) {
        return true;
    }
    
    // Check for 100% width alone (common for block elements)
    if (isPercentageFill(width)) {
        return true;
    }
    
    // Check for position:absolute/fixed with all edges set to 0
    if (position == "absolute" || position == "fixed") {
        std::string top = getStyle(styles, "top", "auto");
        std::string left = getStyle(styles, "left", "auto");
        std::string right = getStyle(styles, "right", "auto");
        std::string bottom = getStyle(styles, "bottom", "auto");
        
        if ((top == "0" || top == "0px") && (left == "0" || left == "0px") && 
            (right == "0" || right == "0px") && (bottom == "0" || bottom == "0px")) {
            return true;
        }
    }
    
    // Block-level elements with no explicit width default to 100% width
    if (display == "block" && width.empty()) {
        return true;  // Block elements fill by default
    }
    
    return false;
}

CanvasSlotConfig StyleMapper::generateCanvasSlot(const std::map<std::string, std::string>& styles) {
    CanvasSlotConfig slot;
    
    std::string position = getStyle(styles, "position", "static");
    std::string top = getStyle(styles, "top", "auto");
    std::string left = getStyle(styles, "left", "auto");
    std::string right = getStyle(styles, "right", "auto");
    std::string bottom = getStyle(styles, "bottom", "auto");
    std::string width = getStyle(styles, "width", "auto");
    std::string height = getStyle(styles, "height", "auto");
    std::string transform = getStyle(styles, "transform", "");
    
    // Check for fill pattern (all edges 0)
    if (shouldFillParent(styles)) {
        slot.anchors = Anchors::Fill();
        slot.offsetLeft = 0;
        slot.offsetTop = 0;
        slot.offsetRight = 0;
        slot.offsetBottom = 0;
        slot.autoSize = false;
        return slot;
    }
    
    // Check for centered pattern (50% + translate)
    bool centeredX = (left == "50%" || left.find("50%") != std::string::npos);
    bool centeredY = (top == "50%" || top.find("50%") != std::string::npos);
    bool hasTranslateCenter = transform.find("translate(-50%") != std::string::npos || 
                              transform.find("translateX(-50%") != std::string::npos;
    
    if (centeredX && centeredY && hasTranslateCenter) {
        slot.anchors = Anchors::Center();
        slot.alignment = {0.5f, 0.5f};
        slot.autoSize = true;
        return slot;
    }
    
    // Parse individual position values
    float topVal = 0, leftVal = 0, rightVal = 0, bottomVal = 0;
    std::string unit;
    
    // Determine anchor based on which edges are set
    bool hasLeft = (left != "auto" && !left.empty());
    bool hasTop = (top != "auto" && !top.empty());
    bool hasRight = (right != "auto" && !right.empty());
    bool hasBottom = (bottom != "auto" && !bottom.empty());
    
    // Parse values
    if (hasLeft) parseSize(left, leftVal, unit);
    if (hasTop) parseSize(top, topVal, unit);
    if (hasRight) parseSize(right, rightVal, unit);
    if (hasBottom) parseSize(bottom, bottomVal, unit);
    
    // Determine anchors based on which edges are specified
    if (hasLeft && hasRight) {
        // Horizontal stretch
        slot.anchors.min.x = 0;
        slot.anchors.max.x = 1;
        slot.offsetLeft = leftVal;
        slot.offsetRight = rightVal;
    } else if (hasRight) {
        // Anchored to right
        slot.anchors.min.x = 1;
        slot.anchors.max.x = 1;
        slot.offsetLeft = -rightVal;
        slot.alignment.x = 1.0f;
    } else {
        // Anchored to left (default)
        slot.anchors.min.x = 0;
        slot.anchors.max.x = 0;
        slot.offsetLeft = leftVal;
    }
    
    if (hasTop && hasBottom) {
        // Vertical stretch
        slot.anchors.min.y = 0;
        slot.anchors.max.y = 1;
        slot.offsetTop = topVal;
        slot.offsetBottom = bottomVal;
    } else if (hasBottom) {
        // Anchored to bottom
        slot.anchors.min.y = 1;
        slot.anchors.max.y = 1;
        slot.offsetTop = -bottomVal;
        slot.alignment.y = 1.0f;
    } else {
        // Anchored to top (default)
        slot.anchors.min.y = 0;
        slot.anchors.max.y = 0;
        slot.offsetTop = topVal;
    }
    
    // If width/height are specified and not stretching, set autoSize false
    if (width != "auto" && !isPercentageFill(width)) {
        float widthVal = 0;
        if (parseSize(width, widthVal, unit)) {
            slot.autoSize = false;
            if (!hasRight) slot.offsetRight = widthVal;
        }
    }
    if (height != "auto" && !isPercentageFill(height)) {
        float heightVal = 0;
        if (parseSize(height, heightVal, unit)) {
            slot.autoSize = false;
            if (!hasBottom) slot.offsetBottom = heightVal;
        }
    }
    
    return slot;
}

BoxSlotConfig StyleMapper::generateBoxSlot(const std::map<std::string, std::string>& styles) {
    BoxSlotConfig slot;
    
    std::string alignSelf = getStyle(styles, "align-self", "auto");
    std::string justifySelf = getStyle(styles, "justify-self", "auto");
    std::string flex = getStyle(styles, "flex", "0 1 auto");
    std::string flexGrow = getStyle(styles, "flex-grow", "0");
    std::string width = getStyle(styles, "width", "auto");
    std::string height = getStyle(styles, "height", "auto");
    
    // Map align-self to VAlign
    if (alignSelf == "flex-start" || alignSelf == "start") {
        slot.vAlign = "Top";
    } else if (alignSelf == "flex-end" || alignSelf == "end") {
        slot.vAlign = "Bottom";
    } else if (alignSelf == "center") {
        slot.vAlign = "Center";
    } else if (alignSelf == "stretch" || alignSelf == "auto") {
        slot.vAlign = "Fill";
    }
    
    // Determine size rule from flex properties
    if (flexGrow != "0" || flex.find("1") == 0) {
        slot.sizeRule = "Fill";
        std::string unit;
        parseSize(flexGrow, slot.fillWeight, unit);
        if (slot.fillWeight == 0) slot.fillWeight = 1.0f;
    } else if (isPercentageFill(width) || isPercentageFill(height)) {
        slot.sizeRule = "Fill";
    } else {
        slot.sizeRule = "Auto";
    }
    
    // Parse padding/margin for slot padding
    std::string margin = getStyle(styles, "margin", "0");
    parseSpacing(margin, slot.paddingTop, slot.paddingRight, slot.paddingBottom, slot.paddingLeft);
    
    return slot;
}

SlotConfig StyleMapper::generateSlotConfig(const std::map<std::string, std::string>& computedStyles,
                                           SlotConfig::SlotType parentType) {
    SlotConfig config;
    config.type = parentType;
    
    if (parentType == SlotConfig::SlotType::Canvas) {
        config.canvas = generateCanvasSlot(computedStyles);
    } else if (parentType == SlotConfig::SlotType::Box) {
        config.box = generateBoxSlot(computedStyles);
    }
    
    return config;
}

static std::string indent(int level) {
    return std::string(level * 2, ' ');
}

std::string StyleMapper::slotToJson(const SlotConfig& slot, int indentLevel) {
    std::ostringstream json;
    
    if (slot.type == SlotConfig::SlotType::Canvas) {
        const auto& c = slot.canvas;
        json << indent(indentLevel) << "\"Slot\": {\n";
        json << indent(indentLevel + 1) << "\"Anchors\": {\n";
        json << indent(indentLevel + 2) << "\"Min\": {\"X\": " << c.anchors.min.x << ", \"Y\": " << c.anchors.min.y << "},\n";
        json << indent(indentLevel + 2) << "\"Max\": {\"X\": " << c.anchors.max.x << ", \"Y\": " << c.anchors.max.y << "}\n";
        json << indent(indentLevel + 1) << "},\n";
        json << indent(indentLevel + 1) << "\"Offsets\": {\"Left\": " << c.offsetLeft << ", \"Top\": " << c.offsetTop 
             << ", \"Right\": " << c.offsetRight << ", \"Bottom\": " << c.offsetBottom << "},\n";
        json << indent(indentLevel + 1) << "\"Alignment\": {\"X\": " << c.alignment.x << ", \"Y\": " << c.alignment.y << "},\n";
        json << indent(indentLevel + 1) << "\"AutoSize\": " << (c.autoSize ? "true" : "false") << "\n";
        json << indent(indentLevel) << "}";
    } else if (slot.type == SlotConfig::SlotType::Box) {
        const auto& b = slot.box;
        json << indent(indentLevel) << "\"Slot\": {\n";
        json << indent(indentLevel + 1) << "\"HAlign\": \"" << b.hAlign << "\",\n";
        json << indent(indentLevel + 1) << "\"VAlign\": \"" << b.vAlign << "\",\n";
        json << indent(indentLevel + 1) << "\"Padding\": {\"Left\": " << b.paddingLeft << ", \"Top\": " << b.paddingTop
             << ", \"Right\": " << b.paddingRight << ", \"Bottom\": " << b.paddingBottom << "},\n";
        json << indent(indentLevel + 1) << "\"Size\": {\"Rule\": \"" << b.sizeRule << "\"";
        if (b.sizeRule == "Fill") {
            json << ", \"Value\": " << b.fillWeight;
        }
        json << "}\n";
        json << indent(indentLevel) << "}";
    }
    
    return json.str();
}

} // namespace awcg
