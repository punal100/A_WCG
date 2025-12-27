// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#include "awcg_html_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <set>

namespace awcg {

// Void elements (self-closing)
static const std::set<std::string> VOID_ELEMENTS = {
    "area", "base", "br", "col", "embed", "hr", "img", "input",
    "link", "meta", "param", "source", "track", "wbr"
};

// Raw text elements
static const std::set<std::string> RAW_TEXT_ELEMENTS = {"script", "style"};

HTMLParser::HTMLParser() = default;
HTMLParser::~HTMLParser() = default;

bool HTMLParser::parse(const std::string& htmlContent, DOMNode& outRootNode,
                       std::vector<Diagnostic>& outDiagnostics) {
    // Reset state
    currentTokenIndex_ = 0;
    tokens_.clear();
    styleBlocks_.clear();
    scriptBlocks_.clear();
    pageTitle_.clear();

    if (!tokenize(htmlContent, tokens_, outDiagnostics)) {
        return false;
    }

    outRootNode.tagName = "#document";

    while (!isAtEnd()) {
        const Token& token = currentToken();

        if (token.type == TokenType::Doctype || token.type == TokenType::Comment) {
            advance();
            continue;
        }

        if (token.type == TokenType::TagOpen) {
            DOMNode child;
            if (parseElement(child, outDiagnostics)) {
                outRootNode.children.push_back(std::move(child));
            }
        } else if (token.type == TokenType::Text) {
            std::string trimmed = token.value;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
            trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
            if (!trimmed.empty()) {
                DOMNode textNode;
                textNode.tagName = "#text";
                textNode.textContent = token.value;
                outRootNode.children.push_back(std::move(textNode));
            }
            advance();
        } else {
            advance();
        }
    }

    return std::none_of(outDiagnostics.begin(), outDiagnostics.end(),
        [](const Diagnostic& d) { return d.code == ErrorCode::E001_ParseError; });
}

bool HTMLParser::parseFile(const std::string& filePath, DOMNode& outRootNode,
                           std::vector<Diagnostic>& outDiagnostics) {
    currentFilePath_ = filePath;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        addError(outDiagnostics, "Failed to open file: " + filePath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str(), outRootNode, outDiagnostics);
}

bool HTMLParser::tokenize(const std::string& content, std::vector<Token>& outTokens,
                          std::vector<Diagnostic>& outDiagnostics) {
    size_t pos = 0;
    int line = 1, column = 1;
    const size_t len = content.length();

    auto advanceChar = [&]() -> char {
        if (pos >= len) return 0;
        char c = content[pos++];
        if (c == '\n') { line++; column = 1; }
        else { column++; }
        return c;
    };

    auto peekChar = [&](size_t offset = 0) -> char {
        size_t idx = pos + offset;
        return idx < len ? content[idx] : 0;
    };

    auto skipWhitespace = [&]() {
        while (pos < len && std::isspace(static_cast<unsigned char>(content[pos]))) {
            advanceChar();
        }
    };

    while (pos < len) {
        int tokenLine = line, tokenColumn = column;
        char c = peekChar();

        // Comment: <!-- ... -->
        if (c == '<' && peekChar(1) == '!' && peekChar(2) == '-' && peekChar(3) == '-') {
            advanceChar(); advanceChar(); advanceChar(); advanceChar();
            std::string comment;
            while (pos < len) {
                if (peekChar() == '-' && peekChar(1) == '-' && peekChar(2) == '>') {
                    advanceChar(); advanceChar(); advanceChar();
                    break;
                }
                comment += advanceChar();
            }
            outTokens.push_back({TokenType::Comment, comment, tokenLine, tokenColumn});
            continue;
        }

        // DOCTYPE
        if (c == '<' && peekChar(1) == '!') {
            advanceChar(); advanceChar();
            std::string doctype;
            while (pos < len && peekChar() != '>') {
                doctype += advanceChar();
            }
            if (peekChar() == '>') advanceChar();
            outTokens.push_back({TokenType::Doctype, doctype, tokenLine, tokenColumn});
            continue;
        }

        // Closing tag
        if (c == '<' && peekChar(1) == '/') {
            advanceChar(); advanceChar();
            std::string tagName;
            while (pos < len && std::isalnum(static_cast<unsigned char>(peekChar()))) {
                tagName += advanceChar();
            }
            skipWhitespace();
            if (peekChar() == '>') advanceChar();
            outTokens.push_back({TokenType::TagClose, normalizeTagName(tagName), tokenLine, tokenColumn});
            continue;
        }

        // Opening tag
        if (c == '<' && std::isalpha(static_cast<unsigned char>(peekChar(1)))) {
            advanceChar();
            std::string tagName;
            while (pos < len) {
                char ch = peekChar();
                if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_') {
                    tagName += advanceChar();
                } else break;
            }
            outTokens.push_back({TokenType::TagOpen, normalizeTagName(tagName), tokenLine, tokenColumn});

            // Parse attributes
            while (pos < len) {
                skipWhitespace();
                c = peekChar();

                if (c == '/' && peekChar(1) == '>') {
                    advanceChar(); advanceChar();
                    outTokens.push_back({TokenType::TagSelfClose, "", line, column});
                    break;
                }

                if (c == '>') {
                    advanceChar();
                    outTokens.push_back({TokenType::TagEnd, "", line, column});

                    std::string normTag = normalizeTagName(tagName);
                    if (isRawTextElement(normTag)) {
                        std::string rawContent;
                        std::string endTag = "</" + normTag + ">";
                        size_t endPos = content.find(endTag, pos);
                        if (endPos == std::string::npos) endPos = len;
                        
                        while (pos < endPos) rawContent += advanceChar();

                        if (normTag == "style") {
                            // Filter out ad-blocker style blocks by checking the class attribute
                            // Look back through tokens to find the class attribute of this style element
                            bool isAdblockStyle = false;
                            for (size_t i = outTokens.size(); i > 0 && outTokens[i-1].type != TokenType::TagOpen; i--) {
                                if (i >= 2 && 
                                    outTokens[i-2].type == TokenType::AttributeName &&
                                    outTokens[i-2].value == "class" &&
                                    outTokens[i-1].type == TokenType::AttributeValue) {
                                    std::string classValue = outTokens[i-1].value;
                                    // Check for known ad-blocker class names
                                    if (classValue.find("abn") != std::string::npos ||
                                        classValue.find("adblock") != std::string::npos ||
                                        classValue.find("Adblock") != std::string::npos) {
                                        isAdblockStyle = true;
                                        break;
                                    }
                                }
                            }
                            
                            if (!isAdblockStyle) {
                                styleBlocks_.push_back(rawContent);
                            }
                            // Skip ad-blocker style blocks silently
                        }
                        else if (normTag == "script") scriptBlocks_.push_back(rawContent);

                        outTokens.push_back({TokenType::Text, rawContent, line, column});
                    }
                    break;
                }

                if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == ':') {
                    std::string attrName;
                    while (pos < len) {
                        char ch = peekChar();
                        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_' || ch == ':') {
                            attrName += advanceChar();
                        } else break;
                    }
                    outTokens.push_back({TokenType::AttributeName, attrName, line, column});

                    skipWhitespace();
                    if (peekChar() == '=') {
                        advanceChar();
                        skipWhitespace();

                        std::string attrValue;
                        c = peekChar();
                        if (c == '"' || c == '\'') {
                            char quote = advanceChar();
                            while (pos < len && peekChar() != quote) {
                                attrValue += advanceChar();
                            }
                            if (peekChar() == quote) advanceChar();
                        } else {
                            while (pos < len && !std::isspace(static_cast<unsigned char>(peekChar()))
                                   && peekChar() != '>' && peekChar() != '/') {
                                attrValue += advanceChar();
                            }
                        }
                        outTokens.push_back({TokenType::AttributeValue, attrValue, line, column});
                    }
                } else {
                    advanceChar();
                }
            }
            continue;
        }

        // Text content
        if (c != '<') {
            std::string text;
            while (pos < len && peekChar() != '<') {
                text += advanceChar();
            }
            outTokens.push_back({TokenType::Text, text, tokenLine, tokenColumn});
            continue;
        }

        advanceChar();
    }

    outTokens.push_back({TokenType::EndOfFile, "", line, column});
    return true;
}

bool HTMLParser::parseElement(DOMNode& outNode, std::vector<Diagnostic>& outDiagnostics) {
    if (currentToken().type != TokenType::TagOpen) return false;

    outNode.tagName = currentToken().value;
    advance();

    // Parse attributes
    while (!isAtEnd()) {
        const Token& token = currentToken();

        if (token.type == TokenType::AttributeName) {
            std::string attrName = token.value;
            advance();

            std::string attrValue;
            if (currentToken().type == TokenType::AttributeValue) {
                attrValue = currentToken().value;
                advance();
            }

            outNode.attributes[attrName] = attrValue;

            // Special attributes
            std::string lowerName = attrName;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            
            if (lowerName == "id") outNode.id = attrValue;
            else if (lowerName == "class") outNode.classes = parseClassAttribute(attrValue);
            else if (lowerName == "style") parseInlineStyle(attrValue, outNode.inlineStyles);
        }
        else if (token.type == TokenType::TagSelfClose) {
            outNode.selfClosing = true;
            advance();
            return true;
        }
        else if (token.type == TokenType::TagEnd) {
            advance();
            break;
        }
        else break;
    }

    if (isVoidElement(outNode.tagName)) {
        outNode.selfClosing = true;
        return true;
    }

    // Parse children
    while (!isAtEnd()) {
        const Token& token = currentToken();

        if (token.type == TokenType::TagClose) {
            std::string lowerTag = token.value;
            std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
            std::string lowerNodeTag = outNode.tagName;
            std::transform(lowerNodeTag.begin(), lowerNodeTag.end(), lowerNodeTag.begin(), ::tolower);
            
            if (lowerTag == lowerNodeTag) {
                // Extract title
                if (outNode.tagName == "title" && !outNode.children.empty()) {
                    for (const auto& child : outNode.children) {
                        if (child.isTextNode()) pageTitle_ += child.textContent;
                    }
                    // Trim
                    pageTitle_.erase(0, pageTitle_.find_first_not_of(" \t\n\r"));
                    pageTitle_.erase(pageTitle_.find_last_not_of(" \t\n\r") + 1);
                }
                advance();
                return true;
            }
            return true;  // Implicit close
        }
        else if (token.type == TokenType::TagOpen) {
            DOMNode child;
            if (parseElement(child, outDiagnostics)) {
                outNode.children.push_back(std::move(child));
            }
        }
        else if (token.type == TokenType::Text) {
            std::string trimmed = token.value;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
            if (trimmed.find_last_not_of(" \t\n\r") != std::string::npos) {
                trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);
            }
            if (!trimmed.empty() || !outNode.children.empty()) {
                DOMNode textNode;
                textNode.tagName = "#text";
                textNode.textContent = token.value;
                outNode.children.push_back(std::move(textNode));
            }
            advance();
        }
        else if (token.type == TokenType::Comment) {
            advance();
        }
        else if (token.type == TokenType::EndOfFile) {
            break;
        }
        else {
            advance();
        }
    }

    return true;
}

void HTMLParser::parseInlineStyle(const std::string& styleValue, std::vector<CSSProperty>& outProperties) {
    std::stringstream ss(styleValue);
    std::string declaration;
    
    while (std::getline(ss, declaration, ';')) {
        // Trim
        declaration.erase(0, declaration.find_first_not_of(" \t"));
        declaration.erase(declaration.find_last_not_of(" \t") + 1);
        if (declaration.empty()) continue;

        size_t colonPos = declaration.find(':');
        if (colonPos != std::string::npos) {
            std::string propName = declaration.substr(0, colonPos);
            std::string propValue = declaration.substr(colonPos + 1);

            // Trim
            propName.erase(0, propName.find_first_not_of(" \t"));
            propName.erase(propName.find_last_not_of(" \t") + 1);
            propValue.erase(0, propValue.find_first_not_of(" \t"));
            propValue.erase(propValue.find_last_not_of(" \t") + 1);

            // Convert name to lowercase
            std::transform(propName.begin(), propName.end(), propName.begin(), ::tolower);

            bool important = false;
            if (propValue.length() > 10) {
                std::string suffix = propValue.substr(propValue.length() - 10);
                std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
                if (suffix == "!important") {
                    important = true;
                    propValue = propValue.substr(0, propValue.length() - 10);
                    propValue.erase(propValue.find_last_not_of(" \t") + 1);
                }
            }

            outProperties.emplace_back(propName, propValue, important);
        }
    }
}

std::vector<std::string> HTMLParser::parseClassAttribute(const std::string& classValue) {
    std::vector<std::string> classes;
    std::stringstream ss(classValue);
    std::string className;
    while (ss >> className) {
        classes.push_back(className);
    }
    return classes;
}

std::string HTMLParser::normalizeTagName(const std::string& tagName) {
    std::string result = tagName;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool HTMLParser::isVoidElement(const std::string& tagName) {
    std::string lower = tagName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return VOID_ELEMENTS.count(lower) > 0;
}

bool HTMLParser::isRawTextElement(const std::string& tagName) {
    std::string lower = tagName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return RAW_TEXT_ELEMENTS.count(lower) > 0;
}

const HTMLParser::Token& HTMLParser::currentToken() const {
    static Token invalid = {TokenType::EndOfFile, "", 0, 0};
    return currentTokenIndex_ < tokens_.size() ? tokens_[currentTokenIndex_] : invalid;
}

void HTMLParser::advance() {
    if (currentTokenIndex_ < tokens_.size()) currentTokenIndex_++;
}

bool HTMLParser::isAtEnd() const {
    return currentTokenIndex_ >= tokens_.size() || currentToken().type == TokenType::EndOfFile;
}

void HTMLParser::addError(std::vector<Diagnostic>& diags, const std::string& msg, int line, int col) {
    Diagnostic diag;
    diag.code = ErrorCode::E001_ParseError;
    diag.message = msg;
    diag.filePath = currentFilePath_;
    diag.lineNumber = line;
    diag.columnNumber = col;
    diags.push_back(diag);
}

} // namespace awcg
