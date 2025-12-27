// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#pragma once

#include "awcg_types.h"
#include <string>
#include <vector>

namespace awcg {

/**
 * CSS Parser
 * 
 * Parses CSS content into structured style rules.
 */
class CSSParser {
public:
    CSSParser();
    ~CSSParser();

    /**
     * Parse CSS string
     */
    bool parse(const std::string& cssContent, StyleSheet& outStyleSheet,
               std::vector<Diagnostic>& outDiagnostics);

    /**
     * Parse CSS file
     */
    bool parseFile(const std::string& filePath, StyleSheet& outStyleSheet,
                   std::vector<Diagnostic>& outDiagnostics);

    /**
     * Parse inline style
     */
    static void parseInlineStyle(const std::string& styleValue, 
                                 std::vector<CSSProperty>& outProperties);

    /**
     * Apply stylesheet to DOM node
     */
    static void applyStylesToNode(const StyleSheet& styleSheet, DOMNode& node);

    /**
     * Check if selector matches node
     */
    static bool selectorMatchesNode(const std::string& selector, const DOMNode& node);

private:
    enum class TokenType {
        Selector, PropertyName, PropertyValue,
        BlockStart, BlockEnd, Semicolon, Colon, Comma,
        Comment, AtRule, EndOfFile
    };

    struct Token {
        TokenType type;
        std::string value;
        int lineNumber;
        int columnNumber;
    };

    bool tokenize(const std::string& content, std::vector<Token>& outTokens,
                  std::vector<Diagnostic>& outDiagnostics);
    bool parseRule(const std::vector<Token>& tokens, size_t& index,
                   CSSRule& outRule, std::vector<Diagnostic>& outDiagnostics);
    bool parseDeclarations(const std::vector<Token>& tokens, size_t& index,
                          std::vector<CSSProperty>& outProperties,
                          std::vector<Diagnostic>& outDiagnostics);

    void addError(std::vector<Diagnostic>& diags, const std::string& msg, int line = 0, int col = 0);

    struct SelectorPart {
        std::string tagName;
        std::string id;
        std::vector<std::string> classes;
        std::vector<std::pair<std::string, std::string>> attributes;
    };

    static bool parseSelectorParts(const std::string& selector, std::vector<SelectorPart>& outParts);
    static bool partMatchesNode(const SelectorPart& part, const DOMNode& node);

    std::string currentFilePath_;
};

} // namespace awcg
