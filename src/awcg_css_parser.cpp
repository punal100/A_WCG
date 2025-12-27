// Copyright Punal Manalan. All Rights Reserved.
// A_WCG - Atomic Web Component Generator

#include "awcg_css_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace awcg {

CSSParser::CSSParser() = default;
CSSParser::~CSSParser() = default;

bool CSSParser::parse(const std::string& cssContent, StyleSheet& outStyleSheet,
                      std::vector<Diagnostic>& outDiagnostics) {
    outStyleSheet.rules.clear();

    std::vector<Token> tokens;
    if (!tokenize(cssContent, tokens, outDiagnostics)) {
        return false;
    }

    size_t index = 0;
    while (index < tokens.size() && tokens[index].type != TokenType::EndOfFile) {
        if (tokens[index].type == TokenType::Comment) {
            index++;
            continue;
        }

        if (tokens[index].type == TokenType::AtRule) {
            // Skip @rules for now
            int braceCount = 0;
            while (index < tokens.size()) {
                if (tokens[index].type == TokenType::BlockStart) braceCount++;
                if (tokens[index].type == TokenType::BlockEnd) {
                    braceCount--;
                    if (braceCount <= 0) { index++; break; }
                }
                index++;
            }
            continue;
        }

        CSSRule rule;
        if (parseRule(tokens, index, rule, outDiagnostics)) {
            outStyleSheet.rules.push_back(std::move(rule));
        } else {
            while (index < tokens.size() && 
                   tokens[index].type != TokenType::BlockEnd &&
                   tokens[index].type != TokenType::EndOfFile) {
                index++;
            }
            if (index < tokens.size() && tokens[index].type == TokenType::BlockEnd) index++;
        }
    }

    return std::none_of(outDiagnostics.begin(), outDiagnostics.end(),
        [](const Diagnostic& d) { return d.code == ErrorCode::E001_ParseError; });
}

bool CSSParser::parseFile(const std::string& filePath, StyleSheet& outStyleSheet,
                          std::vector<Diagnostic>& outDiagnostics) {
    currentFilePath_ = filePath;

    std::ifstream file(filePath);
    if (!file.is_open()) {
        addError(outDiagnostics, "Failed to open CSS file: " + filePath);
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str(), outStyleSheet, outDiagnostics);
}

void CSSParser::parseInlineStyle(const std::string& styleValue,
                                 std::vector<CSSProperty>& outProperties) {
    std::stringstream ss(styleValue);
    std::string decl;
    
    while (std::getline(ss, decl, ';')) {
        decl.erase(0, decl.find_first_not_of(" \t"));
        if (decl.find_last_not_of(" \t") != std::string::npos)
            decl.erase(decl.find_last_not_of(" \t") + 1);
        if (decl.empty()) continue;

        size_t colonPos = decl.find(':');
        if (colonPos != std::string::npos) {
            std::string name = decl.substr(0, colonPos);
            std::string value = decl.substr(colonPos + 1);

            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            bool important = false;
            if (value.length() > 10) {
                std::string suffix = value.substr(value.length() - 10);
                std::transform(suffix.begin(), suffix.end(), suffix.begin(), ::tolower);
                if (suffix == "!important") {
                    important = true;
                    value = value.substr(0, value.length() - 10);
                    value.erase(value.find_last_not_of(" \t") + 1);
                }
            }

            outProperties.emplace_back(name, value, important);
        }
    }
}

void CSSParser::applyStylesToNode(const StyleSheet& styleSheet, DOMNode& node) {
    node.computedStyles.clear();

    for (const auto& rule : styleSheet.rules) {
        std::stringstream ss(rule.selector);
        std::string selector;
        while (std::getline(ss, selector, ',')) {
            selector.erase(0, selector.find_first_not_of(" \t"));
            if (selector.find_last_not_of(" \t") != std::string::npos)
                selector.erase(selector.find_last_not_of(" \t") + 1);

            if (selectorMatchesNode(selector, node)) {
                for (const auto& prop : rule.properties) {
                    // Standard CSS Cascade: Last rule wins (unless !important logic is tracked)
                    // Since DOMNode doesn't track importance of existing values, we default to "Last Wins"
                    // which is better than the previous "First Wins" logic.
                    // TODO: Improve importance tracking if needed.
                    node.computedStyles[prop.name] = prop.value;
                }
            }
        }
    }

    for (const auto& prop : node.inlineStyles) {
        node.computedStyles[prop.name] = prop.value;
    }

    for (auto& child : node.children) {
        applyStylesToNode(styleSheet, child);
    }
}

bool CSSParser::selectorMatchesNode(const std::string& selector, const DOMNode& node) {
    std::vector<SelectorPart> parts;
    if (!parseSelectorParts(selector, parts)) return false;
    if (parts.empty()) return false;
    return partMatchesNode(parts.back(), node);
}

bool CSSParser::tokenize(const std::string& content, std::vector<Token>& outTokens,
                         std::vector<Diagnostic>& /*outDiagnostics*/) {
    size_t pos = 0;
    int line = 1, column = 1;
    const size_t len = content.length();

    auto advanceChar = [&]() -> char {
        if (pos >= len) return 0;
        char c = content[pos++];
        if (c == '\n') { line++; column = 1; } else { column++; }
        return c;
    };

    auto peekChar = [&](size_t offset = 0) -> char {
        size_t idx = pos + offset;
        return idx < len ? content[idx] : 0;
    };

    auto skipWhitespace = [&]() {
        while (pos < len && std::isspace(static_cast<unsigned char>(content[pos])))
            advanceChar();
    };

    while (pos < len) {
        skipWhitespace();
        if (pos >= len) break;

        int tokenLine = line, tokenColumn = column;
        char c = peekChar();

        if (c == '/' && peekChar(1) == '*') {
            advanceChar(); advanceChar();
            std::string comment;
            while (pos < len) {
                if (peekChar() == '*' && peekChar(1) == '/') {
                    advanceChar(); advanceChar(); break;
                }
                comment += advanceChar();
            }
            outTokens.push_back({TokenType::Comment, comment, tokenLine, tokenColumn});
            continue;
        }

        if (c == '@') {
            std::string atRule;
            atRule += advanceChar();
            while (pos < len && (std::isalnum(static_cast<unsigned char>(peekChar())) || peekChar() == '-'))
                atRule += advanceChar();
            outTokens.push_back({TokenType::AtRule, atRule, tokenLine, tokenColumn});
            continue;
        }

        if (c == '{') { advanceChar(); outTokens.push_back({TokenType::BlockStart, "{", tokenLine, tokenColumn}); continue; }
        if (c == '}') { advanceChar(); outTokens.push_back({TokenType::BlockEnd, "}", tokenLine, tokenColumn}); continue; }
        if (c == ';') { advanceChar(); outTokens.push_back({TokenType::Semicolon, ";", tokenLine, tokenColumn}); continue; }
        if (c == ':') { advanceChar(); outTokens.push_back({TokenType::Colon, ":", tokenLine, tokenColumn}); continue; }
        if (c == ',') { advanceChar(); outTokens.push_back({TokenType::Comma, ",", tokenLine, tokenColumn}); continue; }

        std::string text;
        while (pos < len) {
            c = peekChar();
            if (c == '{' || c == '}' || c == ';' || c == ':' || c == ',' || (c == '/' && peekChar(1) == '*'))
                break;
            text += advanceChar();
        }

        text.erase(0, text.find_first_not_of(" \t\n\r"));
        if (text.find_last_not_of(" \t\n\r") != std::string::npos)
            text.erase(text.find_last_not_of(" \t\n\r") + 1);

        if (!text.empty()) {
            outTokens.push_back({TokenType::Selector, text, tokenLine, tokenColumn});
        }
    }

    outTokens.push_back({TokenType::EndOfFile, "", line, column});
    return true;
}

bool CSSParser::parseRule(const std::vector<Token>& tokens, size_t& index,
                          CSSRule& outRule, std::vector<Diagnostic>& outDiagnostics) {
    if (index >= tokens.size() || tokens[index].type == TokenType::EndOfFile) return false;

    std::string fullSelector;
    while (index < tokens.size()) {
        const Token& token = tokens[index];
        if (token.type == TokenType::BlockStart) break;
        if (token.type == TokenType::EndOfFile || token.type == TokenType::BlockEnd) return false;
        if (token.type == TokenType::Selector) {
            if (!fullSelector.empty()) fullSelector += " ";
            fullSelector += token.value;
        } else if (token.type == TokenType::Comma) {
            fullSelector += ",";
        }
        index++;
    }

    if (fullSelector.empty()) return false;
    outRule.selector = fullSelector;

    if (index >= tokens.size() || tokens[index].type != TokenType::BlockStart) {
        addError(outDiagnostics, "Expected '{' after selector",
                 index < tokens.size() ? tokens[index].lineNumber : 0,
                 index < tokens.size() ? tokens[index].columnNumber : 0);
        return false;
    }
    index++;

    if (!parseDeclarations(tokens, index, outRule.properties, outDiagnostics)) return false;

    if (index >= tokens.size() || tokens[index].type != TokenType::BlockEnd) {
        addError(outDiagnostics, "Expected '}' after declarations",
                 index < tokens.size() ? tokens[index].lineNumber : 0,
                 index < tokens.size() ? tokens[index].columnNumber : 0);
        return false;
    }
    index++;

    return true;
}

bool CSSParser::parseDeclarations(const std::vector<Token>& tokens, size_t& index,
                                  std::vector<CSSProperty>& outProperties,
                                  std::vector<Diagnostic>& /*outDiagnostics*/) {
    while (index < tokens.size()) {
        const Token& token = tokens[index];
        if (token.type == TokenType::BlockEnd || token.type == TokenType::EndOfFile) break;
        if (token.type == TokenType::Comment || token.type == TokenType::Semicolon) { index++; continue; }

        if (token.type == TokenType::Selector) {
            std::string propName = token.value;
            std::transform(propName.begin(), propName.end(), propName.begin(), ::tolower);
            index++;

            if (index >= tokens.size() || tokens[index].type != TokenType::Colon) continue;
            index++;

            std::string propValue;
            while (index < tokens.size()) {
                const Token& valToken = tokens[index];
                if (valToken.type == TokenType::Semicolon || 
                    valToken.type == TokenType::BlockEnd || 
                    valToken.type == TokenType::EndOfFile) break;
                if (valToken.type == TokenType::Selector) {
                    if (!propValue.empty()) propValue += " ";
                    propValue += valToken.value;
                }
                index++;
            }

            propValue.erase(0, propValue.find_first_not_of(" \t"));
            if (propValue.find_last_not_of(" \t") != std::string::npos)
                propValue.erase(propValue.find_last_not_of(" \t") + 1);

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

            if (!propName.empty() && !propValue.empty()) {
                outProperties.emplace_back(propName, propValue, important);
            }
        } else {
            index++;
        }
    }

    return true;
}

bool CSSParser::parseSelectorParts(const std::string& selector, std::vector<SelectorPart>& outParts) {
    std::stringstream ss(selector);
    std::string segment;
    
    while (ss >> segment) {
        if (segment == ">" || segment == "+" || segment == "~") continue;

        SelectorPart part;
        size_t pos = 0;
        const size_t len = segment.length();

        while (pos < len) {
            char c = segment[pos];
            if (c == '#' || c == '.' || c == '[' || c == ':') break;
            part.tagName += c;
            pos++;
        }

        while (pos < len) {
            char c = segment[pos];
            if (c == '#') {
                pos++;
                std::string id;
                while (pos < len && segment[pos] != '.' && segment[pos] != '#' && segment[pos] != '[' && segment[pos] != ':')
                    id += segment[pos++];
                part.id = id;
            } else if (c == '.') {
                pos++;
                std::string className;
                while (pos < len && segment[pos] != '.' && segment[pos] != '#' && segment[pos] != '[' && segment[pos] != ':')
                    className += segment[pos++];
                part.classes.push_back(className);
            } else if (c == '[') {
                pos++;
                std::string attrName, attrValue;
                while (pos < len && segment[pos] != '=' && segment[pos] != ']')
                    attrName += segment[pos++];
                if (pos < len && segment[pos] == '=') {
                    pos++;
                    if (pos < len && (segment[pos] == '"' || segment[pos] == '\'')) {
                        char quote = segment[pos++];
                        while (pos < len && segment[pos] != quote)
                            attrValue += segment[pos++];
                        if (pos < len) pos++;
                    } else {
                        while (pos < len && segment[pos] != ']')
                            attrValue += segment[pos++];
                    }
                }
                while (pos < len && segment[pos] != ']') pos++;
                if (pos < len) pos++;
                part.attributes.emplace_back(attrName, attrValue);
            } else if (c == ':') {
                while (pos < len && segment[pos] != '.' && segment[pos] != '#' && segment[pos] != '[')
                    pos++;
            } else {
                pos++;
            }
        }

        outParts.push_back(std::move(part));
    }

    return !outParts.empty();
}

bool CSSParser::partMatchesNode(const SelectorPart& part, const DOMNode& node) {
    if (!part.tagName.empty() && part.tagName != "*") {
        std::string lower = part.tagName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string nodeTag = node.tagName;
        std::transform(nodeTag.begin(), nodeTag.end(), nodeTag.begin(), ::tolower);
        if (lower != nodeTag) return false;
    }

    if (!part.id.empty() && part.id != node.id) return false;

    for (const auto& cls : part.classes) {
        if (!node.hasClass(cls)) return false;
    }

    for (const auto& attr : part.attributes) {
        auto it = node.attributes.find(attr.first);
        if (it == node.attributes.end()) return false;
        if (!attr.second.empty() && it->second != attr.second) return false;
    }

    return true;
}

void CSSParser::addError(std::vector<Diagnostic>& diags, const std::string& msg, int line, int col) {
    Diagnostic diag;
    diag.code = ErrorCode::E001_ParseError;
    diag.message = msg;
    diag.filePath = currentFilePath_;
    diag.lineNumber = line;
    diag.columnNumber = col;
    diags.push_back(diag);
}

} // namespace awcg
