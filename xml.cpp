// Joseph Prichard
// 4/27/2024
// Tests and implementation for parser

#include <iostream>
#include <memory_resource>
#include "xml.hpp"

enum Token {
    OpenDeclTag, CloseDeclTag, OpenBegTag, CloseBegTag, OpenEndTag, CloseTag, Assign, None
};

std::string token_to_string(Token token) {
    switch (token) {
    case OpenDeclTag:
        return "<?";
    case CloseDeclTag:
        return "?>";
    case OpenBegTag:
        return "<";
    case CloseBegTag:
        return "/>";
    case OpenEndTag:
        return "</";
    case CloseTag:
        return ">";
    case Assign:
        return "=";
    default:
        return "none";
    }
}

xmlc::TokenException invalid_symbol(char symbol, const std::string& custom_message, int row, int col) {
    std::string message(
        "Encountered invalid symbol in stream: '" + std::string(1, symbol) +
        "', " + custom_message +
        " at row " + std::to_string(row) +
        " at col " + std::to_string(col));
    return xmlc::TokenException(message);
}

xmlc::TokenException unexpected_token_str(const std::string& actual_tok, const std::string& expected_tok, int row, int col) {
    std::string message(
        "Encountered invalid token in stream: '" + actual_tok +
        "' but expected " + expected_tok +
        " at row " + std::to_string(row) +
        " at col " + std::to_string(col));
    return xmlc::TokenException(message);
}

xmlc::TokenException unexpected_token(Token actual_tok, const std::string& expected_tok, int row, int col) {
    return unexpected_token_str(token_to_string(actual_tok), expected_tok, row, col);
}

struct XmlParser {
    const std::string& text;
    int index = 0;
    int row = 0;
    int col = 0;
    std::pmr::memory_resource& resource;
    std::pmr::polymorphic_allocator<xmlc::XmlNode> node_pa;

    explicit XmlParser(const std::string& text, std::pmr::memory_resource& resource)
        : text(text), node_pa(std::pmr::polymorphic_allocator<xmlc::XmlNode>(&resource)), resource(resource) {
    }

    bool has_next() {
        return index < text.size();
    }

    char peek_char() {
        return text[index];
    }

    char read_char() {
        char c = text[index++];
        if (c == '\n') {
            col = 0;
            row += 1;
        } else {
            col += 1;
        }
        return c;
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


    std::pmr::string read_rawtext() {
        skip_whitespace(); // skip all spaces before the start of a rawtext block

        std::pmr::string str(&resource);
        while (has_next()) {
            char c = peek_char();
            // rawtext terminates on a new opening tag
            if (c == '<') {
                break;
            }
            str += c;
            read_char();
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

    std::pmr::string read_tagname() {
        std::pmr::string str(&resource);
        while (has_next()) {
            char c = peek_char();
            // open beg tag terminates on a closing symbols or a space
            if (c == ' ' || c == '>' || c == '?' || c == '/') {
                break;
            }
            if (!isalnum(c)) {
                throw invalid_symbol(c, "begin tag must contain only alphanumerics", row, col);
            }
            str += c;
            read_char();
        }
        return str;
    }

    std::pmr::string read_attrvalue() {
        char c = read_char();
        if (c != '\"') {
            throw invalid_symbol(c, "attr value must begin with a '\"' symbol", row, col);
        }
        std::pmr::string str(&resource);
        while (has_next()) {
            c = read_char();
            if (c == '\"') {
                break;
            }
            str += c;
        }
        return str;
    }

    std::pmr::string read_attrname() {
        std::pmr::string str(&resource);
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
                throw invalid_symbol(c, "first character after a / symbol must be >", row, col);
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
                throw invalid_symbol(c, "first character after a ? symbol must be >", row, col);
            }
        case '>':
            read_char();
            return CloseTag;
        default:
            // no read_char(), because we didn't match a specific token
            return None;
        }
    }

    void parse_children(std::pmr::string& tag_name, std::pmr::vector<xmlc::XmlNode*>& children) {
        while (has_next()) {
            auto tok = read_token();
            if (tok == OpenEndTag) {
                // stop searching for children on an open end tag - the tagname after the open end tag must match
                auto etag_name = read_tagname();
                if (etag_name != tag_name)
                    throw unexpected_token_str(std::string(etag_name), "closing tag '" + std::string(tag_name) + "'", row, col);
                // the open end tag should also be termianted by a close tag
                tok = read_token();
                if (tok != CloseTag) {
                    throw unexpected_token(tok, ">", row, col);
                }
                break;
            } else if (tok == None) {
                // no token that means we must attempt to parse a raw text node
                auto node = new(node_pa.allocate(1)) xmlc::XmlNode;
                node->data = read_rawtext();
                children.emplace_back(node);
            } else {
                // if the token is anything else just keep on parsing
                auto node = parse_elem();
                children.emplace_back(node);
            }
        }
    }

    xmlc::XmlNode* parse_elem() {
        auto xml_elem = new(node_pa.allocate(1)) xmlc::XmlNode; // we must use placement new to make sure destructors on xmlc node are called

        auto tag_name = read_tagname();

        std::pmr::vector<xmlc::XmlAttr> attrs(&resource);
        auto tok = parse_attrs(attrs);

        std::pmr::vector<xmlc::XmlNode*> children(&resource);
        if (tok == CloseTag) {
            // close tag indicates we might have some children to parse
            parse_children(tag_name, children);
        } else if (tok != CloseBegTag) {
            throw xmlc::TokenException("Unclosed attributes list within tag - should end with a close tag");
        }

        xml_elem->data = xmlc::XmlElem{
            .tag = tag_name,
            .attrs =  std::move(attrs),
            .children = std::move(children)
        };
        return xml_elem;
    }

    xmlc::XmlNode* parse_decl() {
        auto xml_decl = new(node_pa.allocate(1)) xmlc::XmlNode; // we must use placement new to make sure destructors on xmlc node are called

        auto tag_name = read_tagname();

        std::pmr::vector<xmlc::XmlAttr> attrs(&resource);
        auto tok = parse_attrs(attrs);

        if (tok != CloseDeclTag) {
            throw unexpected_token(tok, "?>", row, col);
        }

        xml_decl->data = xmlc::XmlDecl{
            .tag = tag_name,
            .attrs =  std::move(attrs)
        };
        return xml_decl;
    }

    Token parse_attrs(std::pmr::vector<xmlc::XmlAttr>& attrs) {
        while (has_next()) {
            xmlc::XmlAttr attr;

            auto tok = read_token();
            if (tok == CloseTag || tok == CloseBegTag || tok == CloseDeclTag) {
                return tok;
            } else if (tok == None) {
                // no specific token match means we need to try to parse an attr name
                auto attr_name = read_attrname();
                attr.name = attr_name;
            } else {
                throw unexpected_token(tok, "attrname, '>', or '/>'", row, col);
            }

            tok = read_token();
            if (tok != Assign) {
                throw unexpected_token(tok, "=", row, col);
            }

            auto attr_value = read_attrvalue();
            attr.value = attr_value;

            attrs.emplace_back(attr);
        }

        throw xmlc::TokenException("Unclosed attributes list within tag - should end with a close tag");
    }

    std::pmr::vector<xmlc::XmlNode*> parse() {
        std::pmr::vector<xmlc::XmlNode*> nodes(&resource);
        while (has_next()) {
            auto token = read_token();
            if (token == OpenBegTag) {
                auto node = parse_elem();
                nodes.push_back(node);
            } else if (token == OpenDeclTag) {
                auto node = parse_decl();
                nodes.push_back(node);
            } else if (token == None) {
                return nodes;
            } else {
                throw unexpected_token(token, "'</' or '<?'", row, col);
            }
        }
        return nodes;
    }
};

const xmlc::XmlElem* xmlc::XmlElem::find_child(const char* ctag) const {
    for (auto child : children)
        if (auto elem = get_if<XmlElem>(&child->data))
            if (strcmp(elem->tag.c_str(), ctag) == 0)
                return elem;
    return nullptr;
}

const xmlc::XmlAttr* xmlc::XmlElem::find_attr(const char* attr_name) const {
    for (auto& attr : attrs)
        if (strcmp(attr.get_name(), attr_name) == 0)
            return &attr;
    return nullptr;
}

std::string xmlc::XmlNode::serialize() {
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
        for (auto* child: elem->children) {
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

xmlc::XmlDocument xmlc::parse_document(const std::string& docstr, std::pmr::memory_resource& resource) {
    XmlParser parser(docstr, resource);
    XmlDocument document{.children = parser.parse()};
    return document;
}