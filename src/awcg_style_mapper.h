// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#pragma once

#include "awcg_types.h"
#include <string>
#include <map>
#include <memory>

namespace awcg {

// Forward declaration for JSON object (we'll use a simple implementation)
class JsonObject;

/**
 * Style Mapper
 * 
 * Maps CSS properties to P_MWCS Design fields.
 */
class StyleMapper {
public:
    /**
     * Parse CSS color to RGBA
     */
    static bool parseColor(const std::string& cssColor, Color& outColor);

    /**
     * Parse CSS size value
     */
    static bool parseSize(const std::string& cssSize, float& outValue, std::string& outUnit);

    /**
     * Map font-weight to typeface
     */
    static std::string mapFontWeight(const std::string& fontWeight);

    /**
     * Map text-align to justification
     */
    static std::string mapTextAlign(const std::string& textAlign);

    /**
     * Map display to visibility
     */
    static std::string mapVisibility(const std::string& display);

    /**
     * Map CSS property name to P_MWCS field
     */
    static std::string mapPropertyName(const std::string& cssProperty);

    /**
     * Parse margin/padding spacing
     */
    static bool parseSpacing(const std::string& cssValue, 
                            float& outTop, float& outRight, 
                            float& outBottom, float& outLeft);

    /**
     * Map font-family to UE font path
     */
    static std::string mapFontFamily(const std::string& fontFamily);

    /**
     * Generate slot configuration from computed CSS styles
     */
    static SlotConfig generateSlotConfig(const std::map<std::string, std::string>& computedStyles,
                                         SlotConfig::SlotType parentType);

    /**
     * Generate CanvasPanel slot from CSS positioning
     */
    static CanvasSlotConfig generateCanvasSlot(const std::map<std::string, std::string>& styles);

    /**
     * Generate Box slot from CSS flexbox properties
     */
    static BoxSlotConfig generateBoxSlot(const std::map<std::string, std::string>& styles);

    /**
     * Determine parent container type from CSS display
     */
    static SlotConfig::SlotType determineContainerType(const std::map<std::string, std::string>& styles);

    /**
     * Check if element should fill parent
     */
    static bool shouldFillParent(const std::map<std::string, std::string>& styles);

    /**
     * Convert slot config to JSON string
     */
    static std::string slotToJson(const SlotConfig& slot, int indentLevel = 0);

private:
    static bool parseHexColor(const std::string& hexColor, Color& outColor);
    static bool parseRgbColor(const std::string& rgbColor, Color& outColor);
    static bool parseNamedColor(const std::string& colorName, Color& outColor);
};

} // namespace awcg
