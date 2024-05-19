// Joseph Prichard
// 4/27/2024
// Implementation for parser

#include <iostream>
#include <memory_resource>
#include <array>
#include "xml.hpp"

enum Token {
    OpenComment, OpenDeclTag, CloseDeclTag, OpenBegTag, CloseBegTag, OpenEndTag, CloseTag, Assign, None
};

std::string token_strings[] = {"<!--", "<?", "?>", "<", "/>", "</", ">", "=", "none"};

struct XmlParser {
    const std::string& text;
    int index = 0;
    int row = 0;
    int col = 0;

    explicit XmlParser(const std::string& text) : text(text) {}

    bool has_next() {
        return index < text.size();
    }

    char peek_char() {
        return text[index];
    }

    char read_char() {
        if (index < text.size()) {
            char c = text[index++];
            if (c == '\n') {
                col = 0;
                row += 1;
            } else {
                col += 1;
            }
            return c;
        } else {
            throw invalid_token("Expected a character but reached end of stream");
        }
    }

    void skip_whitespace() {
        while (has_next()) {
            if (isspace(peek_char())) {
                read_char();
            } else {
                break;
            }
        }
    }

    [[nodiscard]] xml::TokenException invalid_symbol(char symbol, const std::string& custom_message) const {
        std::string message =
            "Encountered invalid symbol in stream: '" + std::string(1, symbol) +
            "', " + custom_message +
            " at row " + std::to_string(row) +
            " at col " + std::to_string(col);
        return xml::TokenException(message);
    }

    [[nodiscard]] xml::TokenException unexpected_token_str(const std::string& actual_tok, const std::string& expected_tok) const {
        std::string message =
            "Encountered invalid token in stream: '" + actual_tok +
            "' but expected " + expected_tok +
            " at row " + std::to_string(row) +
            " at col " + std::to_string(col);
        return xml::TokenException(message);
    }

    [[nodiscard]] xml::TokenException invalid_token(const std::string& m) const {
        std::string message = m +
            " at row " + std::to_string(row) +
            " at col " + std::to_string(col);
        return xml::TokenException(message);
    }

    [[nodiscard]] xml::TokenException unexpected_token(Token actual_tok, const std::string& expected_tok) const {
        return unexpected_token_str(token_strings[actual_tok], expected_tok);
    }

    char read_escseq() {
        std::string str;
        while (has_next()) {
            char c = peek_char();
            str += c;
            read_char();
            if (c == ';') {
                break;
            }
        }
        if (str == "&quot;") {
            return '"';
        } else if (str == "&apos;") {
            return '\'';
        } else if (str == "&lt;") {
            return '<';
        } else if (str == "&gt;") {
            return '>';
        } if (str == "&amp;") {
            return '&';
        } else {
            throw invalid_token("Encountered invalid esc sequence: " + str + " ");
        }
    }

    std::string read_rawtext() {
        skip_whitespace(); // skip all spaces before the start of a rawtext block

        std::string str;
        while (has_next()) {
            char c = peek_char();

            if (c == '&') {
                auto esc_char = read_escseq();
                str += esc_char;
            } else {
                // rawtext terminates on a new opening tag
                if (c == '<') {
                    break;
                }

                str += c;
                read_char();
            }
        }

        // trim all trailing spaces
        int i = (int) str.size() - 1;
        while (i >= 0) {
            if (!std::isspace(str[i])) {
                break;
            }
            i -= 1;
        }
        if (i < str.size()) {
            str.erase(i + 1);
        }
        return str;
    }

    std::string read_tagname() {
        std::string str;
        while (has_next()) {
            char c = peek_char();
            // open beg tag terminates on a closing symbols or a space
            if (c == ' ' || c == '>' || c == '?' || c == '/') {
                break;
            }
            if (!isalnum(c)) {
                throw invalid_symbol(c, "begin tag must contain only alphanumerics");
            }
            str += c;
            read_char();
        }
        return str;
    }

    std::string read_attrvalue() {
        char open_char = read_char();
        if (open_char != '"' && open_char != '\'') {
            throw invalid_symbol(open_char, "attr value must begin with single or double quotes");
        }
        char close_char = open_char;

        std::string str;
        while (has_next()) {
            char c = peek_char();
            if (c == '&') {
                auto esc_char = read_escseq();
                str += esc_char;
            } else {
                read_char();
                if (c == close_char) {
                    break;
                }
                str += c;
            }
        }
        return str;
    }

    std::string read_attrname() {
        std::string str;
        while (has_next()) {
            char c = peek_char();
            if (!isalnum(c)) {
                break;
            }
            str += c;
            read_char();
        }
        return str;
    }

    // TODO: finish skip_comment implementation for rawtext nodes
    void skip_comment() {
        char prev = 0;
        while (has_next()) {
            char c = peek_char();
            if (c == '-' && prev == '-') {
                // comment ends on a double dash, and then a gt symbol
                char c1 = read_char();
                if (c1 != '>') {
                    throw invalid_token("Expected a comment to be closed with -->");
                }
            }
            prev = c;
            read_char();
        }
    }

    Token read_token() {
        skip_whitespace();
        if (!has_next()) {
            return None;
        }

        char c = peek_char();
        switch (c) {
        case '<':
            read_char();
            c = peek_char();
            switch (c) {
            case '/':
                read_char();
                return OpenEndTag;
            case '?':
                read_char();
                return OpenDeclTag;
            case '!': {
                read_char();
                char c1 = read_char();
                char c2 = read_char();
                if (c1 == '-' && c2 == '-') {
                    return OpenComment;
                } else {
                    throw invalid_token("invalid open comment tag, must be <!--");
                }
            }
            default:
                return OpenBegTag;
            }
        case '/': {
            read_char();
            c = peek_char();
            if (c == '>') {
                read_char();
                return CloseBegTag;
            } else {
                throw invalid_symbol(c, "first character after a / symbol must be >");
            }
        }
        case '=':
            read_char();
            return Assign;
        case '?':
            read_char();
            c = peek_char();
            if (c == '>') {
                read_char();
                return CloseDeclTag;
            } else {
                throw invalid_symbol(c, "first character after a ? symbol must be >");
            }
        case '>':
            read_char();
            return CloseTag;
        default:
            // no read_char(), because we didn't match a specific token
            return None;
        }
    }

    void parse_children(std::string& tag_name, std::vector<std::unique_ptr<xml::XmlNode>>& children) {
        while (has_next()) {
            auto tok = read_token();
            if (tok == OpenEndTag) {
                // stop searching for children on an open end tag - the tagname after the open end tag must match
                auto etag_name = read_tagname();
                if (etag_name != tag_name)
                    throw unexpected_token_str(std::string(etag_name), "closing tag '" + std::string(tag_name) + "'");
                // the open end tag should also be terminated by a close tag
                tok = read_token();
                if (tok != CloseTag) {
                    throw unexpected_token(tok, ">");
                }
                break;
            } else if (tok == None) {
                // no token that means we must attempt to parse a raw text node
                auto node = std::make_unique<xml::XmlNode>();
                node->data = read_rawtext();
                children.emplace_back(std::move(node));
            } else {
                // if the token is anything else just keep on parsing
                auto node = parse_elem();
                children.emplace_back(std::move(node));
            }
        }
    }

    std::unique_ptr<xml::XmlNode> parse_elem() {
        auto tag_name = read_tagname();

        std::vector<xml::XmlAttr> attrs;
        auto tok = parse_attrs(attrs);

        std::vector<std::unique_ptr<xml::XmlNode>> children;
        if (tok == CloseTag) {
            // close tag indicates we might have some children to parse
            parse_children(tag_name, children);
        } else if (tok != CloseBegTag) {
            throw invalid_token("Unclosed attributes list within tag - should end with a close tag");
        }

        auto xml_elem = std::make_unique<xml::XmlNode>(); // we must use placement new to make sure destructors on xml node are called
        xml_elem->data = xml::XmlElem(std::move(tag_name), std::move(attrs), std::move(children));
        return xml_elem;
    }

    std::unique_ptr<xml::XmlNode> parse_decl() {
        auto tag_name = read_tagname();

        std::vector<xml::XmlAttr> attrs;
        auto tok = parse_attrs(attrs);

        if (tok != CloseDeclTag) {
            throw unexpected_token(tok, "?>");
        }

        auto xml_decl = std::make_unique<xml::XmlNode>(); // we must use placement new to make sure destructors on xml node are called
        xml_decl->data = xml::XmlDecl(std::move(tag_name), std::move(attrs));
        return xml_decl;
    }

    Token parse_attrs(std::vector<xml::XmlAttr>& attrs) {
        while (has_next()) {
            xml::XmlAttr attr;

            auto tok = read_token();
            if (tok == CloseTag || tok == CloseBegTag || tok == CloseDeclTag) {
                return tok;
            } else if (tok == None) {
                // no specific token match means we need to try to parse an attr name
                auto attr_name = read_attrname();
                attr.name = attr_name;
            } else {
                throw unexpected_token(tok, "attrname, '>', or '/>'");
            }

            tok = read_token();
            if (tok != Assign) {
                throw unexpected_token(tok, "=");
            }

            auto attr_value = read_attrvalue();
            attr.value = attr_value;

            attrs.emplace_back(attr);
        }

        throw invalid_token("Unclosed attributes list within tag - should end with a close tag");
    }

    void parse(std::vector<std::unique_ptr<xml::XmlNode>>& root_children) {
        while (has_next()) {
            auto token = read_token();
            if (token == OpenBegTag) {
                auto node = parse_elem();
                root_children.push_back(std::move(node));
            } else if (token == OpenDeclTag) {
                auto node = parse_decl();
                root_children.push_back(std::move(node));
            } else if (token == None) {
                return;
            } else {
                throw unexpected_token(token, "'</' or '<?'");
            }
        }
    }
};

std::string xml::XmlNode::serialize() {
    std::string str;
    if (auto elem = std::get_if<XmlElem>(&data)) {
        str += "<" + elem->tag;
        for (int i = 0; i < elem->attrs.size(); i++) {
            if (i == 0)
                str += " ";

            auto& attr = elem->attrs[i];
            str += attr.name + "=" + "\"" + attr.value + "\"";

            if (i < elem->attrs.size() - 1)
                str += " ";
        }
        str += "> ";
        for (auto& child: elem->children) {
            str += child->serialize();
        }
        str += "</" + elem->tag + "> ";
    } else if (auto text = std::get_if<XmlText>(&data)) {
        str += *text;
    } else if (auto decl = std::get_if<XmlDecl>(&data)) {
        str += "<?" + decl->tag;
        for (int i = 0; i < decl->attrs.size(); i++) {
            if (i == 0)
                str += " ";

            auto& attr = decl->attrs[i];
            str += attr.name + "=" + "\"" + attr.value + "\" ";

            if (i < decl->attrs.size() - 1)
                str += " ";
        }
        str += "?> ";
    }
    return str;
}

xml::XmlDocument::XmlDocument(const std::string& docstr) {
    XmlParser parser(docstr);
    parser.parse(children);
}

const xml::XmlElem* xml::XmlElem::find_child(const char* ctag) const {
    for (auto& child : children)
        if (auto elem = get_if<XmlElem>(&child->data))
            if (strcmp(elem->tag.c_str(), ctag) == 0)
                return elem;
    return nullptr;
}

const xml::XmlAttr* xml::XmlElem::find_attr(const char* attr_name) const {
    for (auto& attr : attrs)
        if (strcmp(attr.get_name(), attr_name) == 0)
            return &attr;
    return nullptr;
}