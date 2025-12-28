// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#pragma once

#include "awcg_types.h"
#include <string>
#include <map>
#include <memory>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iomanip>

namespace awcg {

// Forward declaration
class JsonObject;

/**
 * Style Mapper
 * 
 * Maps CSS properties to P_MWCS Design fields.
 */
class StyleMapper {
public:
    static inline const std::map<std::string, Color>& getNamedColors() {
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

    static inline int hexDigit(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return 0;
    }

    static inline bool parseHexColor(const std::string& hexColor, Color& outColor) {
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

    static inline bool parseRgbColor(const std::string& rgbColor, Color& outColor) {
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

    static inline bool parseNamedColor(const std::string& colorName, Color& outColor) {
        const auto& colors = getNamedColors();
        auto it = colors.find(colorName);
        if (it != colors.end()) {
            outColor = it->second;
            return true;
        }
        return false;
    }

    static inline bool parseColor(const std::string& cssColor, Color& outColor) {
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

    static inline bool parseSize(const std::string& cssSize, float& outValue, std::string& outUnit) {
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

    static inline bool parseSpacing(const std::string& cssValue, 
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

    static inline std::string mapFontWeight(const std::string& fontWeight) {
        std::string weight = fontWeight;
        size_t first = weight.find_first_not_of(" \t");
        if (first == std::string::npos) return "Regular";
        weight.erase(0, first);
        size_t last = weight.find_last_not_of(" \t");
        if (last != std::string::npos) weight.erase(last + 1);
        std::transform(weight.begin(), weight.end(), weight.begin(), ::tolower);

        if (weight == "normal") return "Regular";
        if (weight == "bold") return "Bold";
        if (weight == "lighter") return "Light";
        if (weight == "bolder") return "Bold";

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

    static inline std::string mapTextAlign(const std::string& textAlign) {
        std::string align = textAlign;
        std::transform(align.begin(), align.end(), align.begin(), ::tolower);
        size_t first = align.find_first_not_of(" \t");
        if (first != std::string::npos) {
            align.erase(0, first);
            size_t last = align.find_last_not_of(" \t");
            if (last != std::string::npos) align.erase(last + 1);
        }
        if (align == "left" || align == "start") return "Left";
        if (align == "center") return "Center";
        if (align == "right" || align == "end") return "Right";
        return "Left";
    }

    static inline std::string mapVisibility(const std::string& display) {
        std::string disp = display;
        std::transform(disp.begin(), disp.end(), disp.begin(), ::tolower);
        if (disp == "none") return "Collapsed";
        if (disp == "hidden") return "Hidden";
        return "Visible";
    }

    static inline std::string mapPropertyName(const std::string& cssProperty) {
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

    static inline std::string mapFontFamily(const std::string& fontFamily) {
        return "/Engine/EngineFonts/Roboto.Roboto";
    }
    
    // Helper to get style value
    static inline std::string getStyle(const std::map<std::string, std::string>& styles, 
                                const std::string& prop, 
                                const std::string& defaultVal = "") {
        auto it = styles.find(prop);
        return it != styles.end() ? it->second : defaultVal;
    }

    // Helper to check percentage
    static inline bool isPercentageFill(const std::string& value) {
        return value == "100%" || value == "100vh" || value == "100vw";
    }

    static inline bool shouldFillParent(const std::map<std::string, std::string>& styles) {
        std::string width = getStyle(styles, "width");
        std::string height = getStyle(styles, "height");
        std::string position = getStyle(styles, "position");
        std::string display = getStyle(styles, "display");
        
        if (isPercentageFill(width) && isPercentageFill(height)) return true;
        if (isPercentageFill(width)) return true;
        
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
        
        if (display == "block" && width.empty()) return true;
        return false;
    }

    static inline CanvasSlotConfig generateCanvasSlot(const std::map<std::string, std::string>& styles) {
        CanvasSlotConfig slot;
        std::string position = getStyle(styles, "position", "static");
        std::string top = getStyle(styles, "top", "auto");
        std::string left = getStyle(styles, "left", "auto");
        std::string right = getStyle(styles, "right", "auto");
        std::string bottom = getStyle(styles, "bottom", "auto");
        std::string width = getStyle(styles, "width", "auto");
        std::string height = getStyle(styles, "height", "auto");
        std::string transform = getStyle(styles, "transform", "");
        
        if (shouldFillParent(styles)) {
            slot.anchors = Anchors::Fill();
            slot.autoSize = false;
            return slot;
        }
        
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
        
        float topVal = 0, leftVal = 0, rightVal = 0, bottomVal = 0;
        std::string unit;
        bool hasLeft = (left != "auto" && !left.empty());
        bool hasTop = (top != "auto" && !top.empty());
        bool hasRight = (right != "auto" && !right.empty());
        bool hasBottom = (bottom != "auto" && !bottom.empty());
        
        if (hasLeft) parseSize(left, leftVal, unit);
        if (hasTop) parseSize(top, topVal, unit);
        if (hasRight) parseSize(right, rightVal, unit);
        if (hasBottom) parseSize(bottom, bottomVal, unit);
        
        if (hasLeft && hasRight) {
            slot.anchors.min.x = 0; slot.anchors.max.x = 1;
            slot.offsetLeft = leftVal; slot.offsetRight = rightVal;
        } else if (hasRight) {
            slot.anchors.min.x = 1; slot.anchors.max.x = 1;
            slot.offsetLeft = -rightVal; slot.alignment.x = 1.0f;
        } else {
            slot.anchors.min.x = 0; slot.anchors.max.x = 0;
            slot.offsetLeft = leftVal;
        }
        
        if (hasTop && hasBottom) {
            slot.anchors.min.y = 0; slot.anchors.max.y = 1;
            slot.offsetTop = topVal; slot.offsetBottom = bottomVal;
        } else if (hasBottom) {
            slot.anchors.min.y = 1; slot.anchors.max.y = 1;
            slot.offsetTop = -bottomVal; slot.alignment.y = 1.0f;
        } else {
            slot.anchors.min.y = 0; slot.anchors.max.y = 0;
            slot.offsetTop = topVal;
        }
        
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

    static inline BoxSlotConfig generateBoxSlot(const std::map<std::string, std::string>& styles) {
        BoxSlotConfig slot;
        std::string alignSelf = getStyle(styles, "align-self", "auto");
        std::string justifySelf = getStyle(styles, "justify-self", "auto");
        std::string flex = getStyle(styles, "flex", "0 1 auto");
        std::string flexGrow = getStyle(styles, "flex-grow", "0");
        std::string width = getStyle(styles, "width", "auto");
        std::string height = getStyle(styles, "height", "auto");
        
        if (alignSelf == "flex-start" || alignSelf == "start") slot.vAlign = "Top";
        else if (alignSelf == "flex-end" || alignSelf == "end") slot.vAlign = "Bottom";
        else if (alignSelf == "center") slot.vAlign = "Center";
        else if (alignSelf == "stretch" || alignSelf == "auto") slot.vAlign = "Fill";
        
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
        
        std::string padding = getStyle(styles, "padding", "");
        if (!padding.empty()) parseSpacing(padding, slot.paddingTop, slot.paddingRight, slot.paddingBottom, slot.paddingLeft);
        
        std::string unit;
        if (styles.count("padding-top")) parseSize(styles.at("padding-top"), slot.paddingTop, unit);
        if (styles.count("padding-right")) parseSize(styles.at("padding-right"), slot.paddingRight, unit);
        if (styles.count("padding-bottom")) parseSize(styles.at("padding-bottom"), slot.paddingBottom, unit);
        if (styles.count("padding-left")) parseSize(styles.at("padding-left"), slot.paddingLeft, unit);
        
        std::string margin = getStyle(styles, "margin", "");
        if (!margin.empty()) {
            float mTop, mRight, mBottom, mLeft;
            if (parseSpacing(margin, mTop, mRight, mBottom, mLeft)) {
                slot.paddingTop += mTop; slot.paddingRight += mRight;
                slot.paddingBottom += mBottom; slot.paddingLeft += mLeft;
            }
        }
        
        // Handle individual margin properties (reuse 'unit' variable from above)
        if (styles.count("margin-top")) {
            float val; parseSize(styles.at("margin-top"), val, unit);
            slot.paddingTop += val;
        }
        if (styles.count("margin-right")) {
            float val; parseSize(styles.at("margin-right"), val, unit);
            slot.paddingRight += val;
        }
        if (styles.count("margin-bottom")) {
            float val; parseSize(styles.at("margin-bottom"), val, unit);
            slot.paddingBottom += val;
        }
        if (styles.count("margin-left")) {
            float val; parseSize(styles.at("margin-left"), val, unit);
            slot.paddingLeft += val;
        }
        return slot;
    }

    static inline SlotConfig generateSlotConfig(const std::map<std::string, std::string>& computedStyles,
                                         SlotConfig::SlotType parentType) {
        SlotConfig config;
        config.type = parentType;
        if (parentType == SlotConfig::SlotType::Canvas) config.canvas = generateCanvasSlot(computedStyles);
        else if (parentType == SlotConfig::SlotType::Box) config.box = generateBoxSlot(computedStyles);
        return config;
    }

    static inline SlotConfig::SlotType determineContainerType(const std::map<std::string, std::string>& styles) {
        std::string display = getStyle(styles, "display", "block");
        std::string position = getStyle(styles, "position", "static");
        if (display == "flex" || display == "inline-flex") return SlotConfig::SlotType::Box;
        if (position == "absolute" || position == "fixed") return SlotConfig::SlotType::Canvas;
        return SlotConfig::SlotType::Box;
    }

    static inline std::string slotToJson(const SlotConfig& slot, int indentLevel = 0) {
        std::ostringstream json;
        auto indent = [](int level) { return std::string(level * 2, ' '); };
        
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
};

} // namespace awcg
