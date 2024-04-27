// Joseph Prichard
// 4/25/2024
// Xml parsing library for C++

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <optional>

bool DEBUG = false;

struct Text {
    std::string value;
};

// <?
struct OpenDeclTag {
    std::string name;
};

// ?>
struct CloseDeclTag {
};

// <Name
struct OpenBegTag {
    std::string name;
};

// />
struct CloseBegTag {
};

// </Name
struct OpenEndTag {
    std::string name;
};

// >
struct CloseTag {
};

struct AttrName {
    std::string name;
};

struct AttrValue {
    std::string value;
};

struct Assign {
};

using XmlToken = std::variant<Text, OpenDeclTag, CloseDeclTag, OpenBegTag,
    CloseBegTag, OpenEndTag, CloseTag, AttrName, Assign, AttrValue>;

struct XmlAttr {
    std::string name;
    std::string value;
};

using XmlText = std::string;

struct XmlDecl {
    std::string tag;
    std::vector<XmlAttr> attrs;
};

struct XmlElem {
    std::string tag;
    std::vector<XmlAttr> attrs;
    std::vector<std::variant<XmlElem, XmlText, XmlDecl>*> children;
};

using XmlNode = std::variant<XmlElem, XmlText, XmlDecl>;

struct Pos {
    int col;
    int row;
};

class InvalidToken : public std::exception {
private:
    std::string message;
public:
    explicit InvalidToken(const std::string& m) : message(m) {
        if (DEBUG) {
            printf("%s\n", m.c_str());
        }
    }

    const char* what() {
        return message.c_str();
    }
};

std::string serialize_token(XmlToken& token) {
    if (auto text = std::get_if<Text>(&token)) {
        return text->value;
    } else if (auto btag = std::get_if<OpenBegTag>(&token)) {
        return "<" + btag->name;
    } else if (std::get_if<OpenDeclTag>(&token)) {
        return "<?";
    } else if (std::get_if<CloseDeclTag>(&token)) {
        return "?>";
    } else if (std::get_if<CloseBegTag>(&token)) {
        return "/>";
    } else if (auto etag = std::get_if<OpenEndTag>(&token)) {
        return "</" + etag->name;
    } else if (std::get_if<CloseTag>(&token)) {
        return ">";
    } else if (auto attr_name = std::get_if<AttrName>(&token)) {
        return " " + attr_name->name;
    } else if (std::get_if<Assign>(&token)) {
        return "=";
    } else if (auto attr_value = std::get_if<AttrValue>(&token)) {
        return "\"" + attr_value->value + "\"";
    } else {
        throw InvalidToken("Invalid token encountered while serializing tokens");
    }
}

std::string serialize_tokens(std::vector<XmlToken>& tokens) {
    std::string str;
    for (auto& token : tokens) {
        str += serialize_token(token);
    }
    return str;
}

void serialize_node(XmlNode* node, std::string& str) {
    if (auto elem = std::get_if<XmlElem>(node)) {
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
        for (auto* child : elem->children) {
            serialize_node(child, str);
        }
        str += "</" + elem->tag + "> ";
    } else if (auto text = std::get_if<XmlText>(node)) {
        str += *text;
    } else if (auto decl = std::get_if<XmlDecl>(node)) {
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
}

std::string serialize_xml_tree(std::vector<XmlNode*>& tree) {
    std::string str;
    for (auto node : tree)
        serialize_node(node, str);
    return str;
}

InvalidToken invalid_symbol(char symbol, const std::string& custom_message, Pos pos) {
    std::string message;
    message += "Encountered invalid symbol in stream: '" + std::string(1, symbol)
               + "', " + custom_message + " at row " + std::to_string(pos.row) + " at col " + std::to_string(pos.col);
    return InvalidToken(message);
}

InvalidToken unexpected_token_str(const std::string& actual_tok, const std::string& expected_tok, Pos pos) {
    std::string message;
    message += "Encountered invalid token in stream: '" + actual_tok + "' but expected " + expected_tok + " at row "
               + std::to_string(pos.row) + " at col " + std::to_string(pos.col);
    return InvalidToken(message);
}

InvalidToken unexpected_token(XmlToken& actual_tok, const std::string& expected_tok, Pos pos) {
    return unexpected_token_str(serialize_token(actual_tok), expected_tok, pos);
}

struct Parser {
    const std::string& text;
    int index = 0;
    Pos pos = {.col = 0, .row = 0};
    std::pmr::polymorphic_allocator<XmlNode>& node_pa;

    explicit Parser(const std::string& text, std::pmr::polymorphic_allocator<XmlNode>& node_pa) : text(text), node_pa(node_pa) {}

    bool has_next() {
        return index < text.size();
    }

    char peek_char() {
        return text[index];
    }

    char read_char() {
        char c = text[index++];
        if (c == '\n') {
            pos.col = 0;
            pos.row += 1;
        } else {
            pos.col += 1;
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

    std::vector<XmlToken> read_tokens() {
        std::vector<XmlToken> tokens;
        while (true) {
            auto token = read_token();
            if (token.has_value()) {
                tokens.push_back(token.value());
            } else {
                break;
            }
        }
        return tokens;
    }

    std::string read_tagname() {
        std::string str;
        while (has_next()) {
            char c = peek_char();
            // open beg tag terminates on a closing angle bracket or a space
            if (c == ' ' || c  == '>') {
                break;
            }
            if (!isalpha(c)) {
                throw invalid_symbol(c, "begin tag must contain only alphanumerics", pos);
            }
            str += c;
            read_char();
        }
        return str;
    }

    XmlToken read_attrvalue() {
        std::string str;
        while (has_next()) {
            char c = read_char();
            if (c == '\"') {
                break;
            }
            str += c;
        }
        return AttrValue{.value = std::move(str)};
    }

    XmlToken read_attrname(char c) {
        std::string str;
        str += c;
        while (has_next()) {
            c = peek_char();
            if (!isalnum(c)) {
                break;
            }
            str += c;
            read_char();
        }
        return AttrName{.name = std::move(str)};
    }

    std::optional<XmlToken> read_token() {
        skip_whitespace();
        if (!has_next()) {
            return {};
        }

        char c = read_char();
        switch (c) {
        case '<':
            c = peek_char();
            switch (c) {
            case '/':
                read_char();
                return OpenEndTag{.name = read_tagname()};
            case '?':
                read_char();
                return OpenDeclTag{.name = read_tagname()};
            default:
                if (!isalpha(c)) {
                    throw invalid_symbol(c, "first character after a < symbol must be alphabetic", pos);
                }
                return OpenBegTag{.name = read_tagname()};
            }
        case '/': {
            c = peek_char();
            if (c == '>') {
                read_char();
                return CloseBegTag{};
            } else {
                throw invalid_symbol(c, "first character after a / symbol must be >", pos);
            }
        }
        case '=':
            return Assign{};
        case '\"':
            return read_attrvalue();
        case '?':
            c = peek_char();
            if (c == '>') {
                read_char();
                return CloseDeclTag{};
            } else {
                throw invalid_symbol(c, "first character after a ? symbol must be >", pos);
            }
        case '>':
            return CloseTag{};
        default:
            // we just assume a block of text is an attr name if there are no control characters to say otherwise...
            return read_attrname(c);
        }
    }

    XmlNode* parse_elem(std::string& tag_name) {
        auto xml_elem = new (node_pa.allocate(1)) XmlNode; // we must use placement new to make sure destructors on xml node are called
        auto attrs = parse_attrs();

        std::vector<XmlNode*> children;
        while (true) {
            auto tok = read_token();
            if (tok.has_value()) {
                // a children list should be terminated by an open end tag
                auto& tokval = tok.value();
                if (auto etag = get_if<OpenEndTag>(&tokval)) {
                    if (etag->name != tag_name)
                        throw unexpected_token(tokval, "<" + tag_name, pos);
                    break;
                } else {
                    auto node = parse_node(tokval);
                    children.emplace_back(node);
                }
            } else {
                break;
            }
        }

        auto tok2 = read_token();
        if (!tok2.has_value()) {
            throw unexpected_token_str("no token", ">", pos);
        }
        auto tokval2 = tok2.value();
        if (!get_if<CloseTag>(&tokval2)) {
            throw unexpected_token(tokval2, ">", pos);
        }

        *xml_elem = XmlElem{.tag = tag_name, .attrs = attrs, .children = children};
        return xml_elem;
    }

    XmlNode* parse_decl(std::string& tag_name) {

    }

    std::vector<XmlAttr> parse_attrs() {
        std::vector<XmlAttr> attrs;
        while (true) {
            XmlAttr attr;

            auto tok = read_token();
            if (!tok.has_value()) {
                throw unexpected_token_str("no token", "attr name, '>', or '/>'", pos);
            }
            // an attribute list should be terminated by a close tag or a closing begin tag
            auto& tokval = tok.value();
            if (get_if<CloseTag>(&tokval) || get_if<CloseBegTag>(&tokval)) {
                return attrs;
            } else if (auto* attr_name = get_if<AttrName>(&tokval)) {
                attr.name = attr_name->name;
            } else {
                throw unexpected_token(tokval, "attr name, '>', or '/>'", pos);
            }

            auto tok1 = read_token();
            if (!tok1.has_value()) {
                throw unexpected_token_str("no token", "=", pos);
            }
            auto& tokval1 = tok1.value();
            if (!get_if<Assign>(&tokval1)) {
                throw unexpected_token(tokval1, "=", pos);
            }

            auto tok2 = read_token();
            if (!tok2.has_value()) {
                throw unexpected_token_str("no token", "attr value", pos);
            }
            auto& tokval2 = tok2.value();
            if (auto attr_name = get_if<AttrValue>(&tokval2)) {
                attr.value = attr_name->value;
            } else {
                throw unexpected_token(tokval2, "attr value", pos);
            }

            attrs.emplace_back(attr);
        }
    }

    // parses a node given the first token used in the node
    XmlNode* parse_node(XmlToken& tokval) {
        if (auto btag = get_if<OpenBegTag>(&tokval)) {
            auto node = parse_elem(btag->name);
            return node;
        } else if (auto dtag = get_if<OpenDeclTag>(&tokval)) {
            auto node = parse_decl(dtag->name);
            return node;
        } else {
            throw unexpected_token(tokval, "'</' or '<?'", pos);
        }
    }

    std::vector<XmlNode*> parse() {
        std::vector<XmlNode*> nodes;
        while (true) {
            auto tok = read_token();
            if (!tok.has_value()) {
                return nodes;
            }
            auto node = parse_node(tok.value());
            if (node == nullptr) {
                return nodes;
            }
            nodes.push_back(node);
        }
    }
};

