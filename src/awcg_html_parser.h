// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#pragma once

#include "awcg_types.h"
#include <string>
#include <vector>

namespace awcg {

/**
 * HTML Parser
 * 
 * Parses HTML source into a DOM tree structure.
 * Extracts <style> and <script> blocks for downstream processing.
 */
class HTMLParser {
public:
    HTMLParser();
    ~HTMLParser();

    /**
     * Parse HTML string into DOM tree
     */
    bool parse(const std::string& htmlContent, DOMNode& outRootNode, 
               std::vector<Diagnostic>& outDiagnostics);

    /**
     * Parse HTML file
     */
    bool parseFile(const std::string& filePath, DOMNode& outRootNode,
                   std::vector<Diagnostic>& outDiagnostics);

    /**
     * Get extracted <style> blocks
     */
    const std::vector<std::string>& getStyleBlocks() const { return styleBlocks_; }

    /**
     * Get extracted <script> blocks
     */
    const std::vector<std::string>& getScriptBlocks() const { return scriptBlocks_; }

    /**
     * Get page title
     */
    const std::string& getPageTitle() const { return pageTitle_; }

private:
    enum class TokenType {
        Text, TagOpen, TagClose, TagSelfClose, TagEnd,
        AttributeName, AttributeValue, Comment, Doctype, EndOfFile
    };

    struct Token {
        TokenType type;
        std::string value;
        int lineNumber;
        int columnNumber;
    };

    bool tokenize(const std::string& content, std::vector<Token>& outTokens,
                  std::vector<Diagnostic>& outDiagnostics);
    bool parseElement(DOMNode& outNode, std::vector<Diagnostic>& outDiagnostics);
    void parseInlineStyle(const std::string& styleValue, std::vector<CSSProperty>& outProperties);
    
    static std::vector<std::string> parseClassAttribute(const std::string& classValue);
    static std::string normalizeTagName(const std::string& tagName);
    static bool isVoidElement(const std::string& tagName);
    static bool isRawTextElement(const std::string& tagName);

    const Token& currentToken() const;
    void advance();
    bool isAtEnd() const;
    void addError(std::vector<Diagnostic>& diags, const std::string& msg, int line = 0, int col = 0);

    std::vector<Token> tokens_;
    size_t currentTokenIndex_ = 0;
    std::string currentFilePath_;
    std::vector<std::string> styleBlocks_;
    std::vector<std::string> scriptBlocks_;
    std::string pageTitle_;
};

} // namespace awcg
