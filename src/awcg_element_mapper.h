// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#pragma once

#include "awcg_types.h"
#include <string>

namespace awcg {

/**
 * Element Mapper
 * 
 * Maps HTML elements to P_MWCS widget types.
 */
class ElementMapper {
public:
    /**
     * Map HTML element to widget type
     */
    static std::string mapElementToWidget(const DOMNode& node);

    /**
     * Determine container type from CSS display
     */
    static std::string determineContainerType(const DOMNode& node);

    /**
     * Check if element needs C++ binding
     */
    static bool shouldCreateBinding(const DOMNode& node);

    /**
     * Get UMG class name
     */
    static std::string getUMGClassName(const std::string& widgetType);

    /**
     * Convert to valid C++ identifier (PascalCase)
     */
    static std::string toValidIdentifier(const std::string& htmlName);

    /**
     * Generate widget name from node
     */
    static std::string generateWidgetName(const DOMNode& node, int index = 0);

    /**
     * Check if interactive element
     */
    static bool isInteractiveElement(const DOMNode& node);

    /**
     * Check if text element
     */
    static bool isTextElement(const DOMNode& node);

    /**
     * Check if input element
     */
    static bool isInputElement(const DOMNode& node);

    /**
     * Check if widget type can have children in UMG
     */
    static bool isContainerWidget(const std::string& widgetType);
};

} // namespace awcg
