// Joseph Prichard
// 4/27/2024
// Tests and implementation for parser

#include <iostream>
#include <memory_resource>
#include "xml.hpp"

[[nodiscard]] const char* xmlc::XmlAttr::get_name() const {
    return name.c_str();
}

[[nodiscard]] const char* xmlc::XmlAttr::get_value() const {
    return value.c_str();
}

const xmlc::XmlElem* xmlc::XmlElem::first_child(const char* ctag) const {
    for (auto child : children)
        if (auto elem = get_if<XmlElem>(&child->data))
            if (strcmp(elem->tag.c_str(), ctag) == 0)
                return elem;
    return nullptr;
}

const xmlc::XmlAttr* xmlc::XmlElem::first_attr(const char* attr_name) const {
    for (auto& attr : attrs)
        if (strcmp(attr.get_name(), attr_name) == 0)
            return &attr;
    return nullptr;
}

[[nodiscard]] bool xmlc::XmlNode::is_text() const {
    return holds_alternative<XmlText>(data);
}

[[nodiscard]] bool xmlc::XmlNode::is_elem() const {
    return holds_alternative<XmlElem>(data);
}

[[nodiscard]] const char* xmlc::XmlNode::as_text() const {
    if (auto text = get_if<XmlText>(&data))
        return text->c_str();
    throw TypeException("XML Node is not a text type node");
}

[[nodiscard]] const xmlc::XmlElem* xmlc::XmlNode::as_elem() const {
    if (auto elem = get_if<XmlElem>(&data))
        return elem;
    throw TypeException("XML Node is not an elem type node");
}

[[nodiscard]] const xmlc::XmlElem* xmlc::XmlNode::first_child(const char* tag) const {
    if (auto elem = get_if<XmlElem>(&data))
        return elem->first_child(tag);
    return nullptr;
}

std::string xmlc::XmlNode::serialize() {
    std::string str;
    if (auto elem = std::get_if<xmlc::XmlElem>(&data)) {
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

std::string xmlc::XmlDocument::serialize() {
    std::string str;
    for (auto node : children)
        str += node->serialize();
    return str;
}

struct Text {
    std::pmr::string value;
};

struct OpenDeclTag {
    std::pmr::string name;
};

struct CloseDeclTag {
};

struct OpenBegTag {
    std::pmr::string name;
};

struct CloseBegTag {
};

struct OpenEndTag {
    std::pmr::string name;
};

struct CloseTag {
};

struct AttrName {
    std::pmr::string name;
};

struct AttrValue {
    std::pmr::string value;
};

struct Assign {
};

using XmlToken = std::variant<Text, OpenDeclTag, CloseDeclTag, OpenBegTag,
    CloseBegTag, OpenEndTag, CloseTag, AttrName, Assign, AttrValue>;

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

std::string serialize_token(XmlToken& token) {
    if (auto text = std::get_if<Text>(&token)) {
        return std::string(text->value);
    } else if (auto btag = std::get_if<OpenBegTag>(&token)) {
        return "<" + std::string(btag->name);
    } else if (std::get_if<OpenDeclTag>(&token)) {
        return "<?";
    } else if (std::get_if<CloseDeclTag>(&token)) {
        return "?>";
    } else if (std::get_if<CloseBegTag>(&token)) {
        return "/>";
    } else if (auto etag = std::get_if<OpenEndTag>(&token)) {
        return "</" + std::string(etag->name);
    } else if (std::get_if<CloseTag>(&token)) {
        return ">";
    } else if (auto attr_name = std::get_if<AttrName>(&token)) {
        return " " + std::string(attr_name->name);
    } else if (std::get_if<Assign>(&token)) {
        return "=";
    } else if (auto attr_value = std::get_if<AttrValue>(&token)) {
        return "\"" + std::string(attr_value->value) + "\"";
    } else {
        throw xmlc::TokenException("Invalid token encountered while serializing tokens");
    }
}

std::string serialize_tokens(std::vector<XmlToken>& tokens) {
    std::string str;
    for (auto& token: tokens) {
        str += serialize_token(token);
    }
    return str;
}

xmlc::TokenException unexpected_token(XmlToken& actual_tok, const std::string& expected_tok, int row, int col) {
    return unexpected_token_str(serialize_token(actual_tok), expected_tok, row, col);
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

    std::pmr::string read_tagname() {
        std::pmr::string str(&resource);
        while (has_next()) {
            char c = peek_char();
            // open beg tag terminates on a closing angle bracket or a space
            if (c == ' ' || c == '>') {
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

    XmlToken read_attrvalue() {
        std::pmr::string str(&resource);
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
        std::pmr::string str(&resource);
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
                return OpenEndTag{.name = std::move(read_tagname())};
            case '?':
                read_char();
                return OpenDeclTag{.name = std::move(read_tagname())};
            default:
                if (!isalpha(c)) {
                    throw invalid_symbol(c, "first character after a < symbol must be alphabetic", row, col);
                }
                return OpenBegTag{.name = std::move(read_tagname())};
            }
        case '/': {
            c = peek_char();
            if (c == '>') {
                read_char();
                return CloseBegTag{};
            } else {
                throw invalid_symbol(c, "first character after a / symbol must be >", row, col);
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
                throw invalid_symbol(c, "first character after a ? symbol must be >", row, col);
            }
        case '>':
            return CloseTag{};
        default:
            // we just assume a block of text is an first_attr name if there are no control characters to say otherwise...
            return read_attrname(c);
        }
    }

    xmlc::XmlNode* parse_elem(std::pmr::string&& tag_name) {
        auto xml_elem = new(node_pa.allocate(1)) xmlc::XmlNode; // we must use placement new to make sure destructors on xml node are called
        auto attrs = parse_attrs();

        std::pmr::vector<xmlc::XmlNode*> children(&resource);
        while (true) {
            auto tok = read_token();
            if (tok.has_value()) {
                // a children list should be terminated by an open end tag
                auto& tokval = tok.value();
                if (auto etag = get_if<OpenEndTag>(&tokval)) {
                    if (etag->name != tag_name)
                        throw unexpected_token(tokval, "<" + std::string(tag_name), row, col);
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
            throw unexpected_token_str("no token", ">", row, col);
        }
        auto& tokval2 = tok2.value();
        if (!get_if<CloseTag>(&tokval2)) {
            throw unexpected_token(tokval2, ">", row, col);
        }

        xml_elem->data = xmlc::XmlElem{
            .tag = tag_name,
            .attrs =  std::move(attrs),
            .children = std::move(children)
        };
        return xml_elem;
    }

    xmlc::XmlNode* parse_decl(std::pmr::string&& tag_name) {
        auto xml_decl = new(node_pa.allocate(1)) xmlc::XmlNode; // we must use placement new to make sure destructors on xml node are called
        auto attrs = parse_attrs();

        auto tok = read_token();
        if (!tok.has_value()) {
            throw unexpected_token_str("no token", "?>", row, col);
        }
        auto& tokval = tok.value();
        if (!get_if<CloseDeclTag>(&tokval)) {
            throw unexpected_token(tokval, "?>", row, col);
        }

        xml_decl->data = xmlc::XmlDecl{
            .tag = tag_name,
            .attrs =  std::move(attrs)
        };
        return xml_decl;
    }

    std::pmr::vector<xmlc::XmlAttr> parse_attrs() {
        std::pmr::vector<xmlc::XmlAttr> attrs(&resource);
        while (true) {
            xmlc::XmlAttr attr;

            auto tok = read_token();
            if (!tok.has_value()) {
                throw unexpected_token_str("no token", "first_attr name, '>', or '/>'", row, col);
            }
            // an attribute list should be terminated by a close tag or a closing begin tag
            auto& tokval = tok.value();
            if (get_if<CloseTag>(&tokval) || get_if<CloseBegTag>(&tokval)) {
                return attrs;
            } else if (auto* attr_name = get_if<AttrName>(&tokval)) {
                attr.name = attr_name->name;
            } else {
                throw unexpected_token(tokval, "first_attr name, '>', or '/>'", row, col);
            }

            auto tok1 = read_token();
            if (!tok1.has_value()) {
                throw unexpected_token_str("no token", "=", row, col);
            }
            auto& tokval1 = tok1.value();
            if (!get_if<Assign>(&tokval1)) {
                throw unexpected_token(tokval1, "=", row, col);
            }

            auto tok2 = read_token();
            if (!tok2.has_value()) {
                throw unexpected_token_str("no token", "first_attr value", row, col);
            }
            auto& tokval2 = tok2.value();
            if (auto attr_name = get_if<AttrValue>(&tokval2)) {
                attr.value = attr_name->value;
            } else {
                throw unexpected_token(tokval2, "first_attr value", row, col);
            }

            attrs.emplace_back(attr);
        }
    }

    // parses a node given the first token used in the node
    xmlc::XmlNode* parse_node(XmlToken& tokval) {
        if (auto btag = get_if<OpenBegTag>(&tokval)) {
            auto node = parse_elem(std::move(btag->name));
            return node;
        } else if (auto dtag = get_if<OpenDeclTag>(&tokval)) {
            auto node = parse_decl(std::move(dtag->name));
            return node;
        } else {
            throw unexpected_token(tokval, "'</' or '<?'", row, col);
        }
    }

    std::pmr::vector<xmlc::XmlNode*> parse() {
        std::pmr::vector<xmlc::XmlNode*> nodes(&resource);
        while (true) {
            auto tok = read_token();
            if (!tok.has_value()) {
                return nodes;
            }
            auto node = parse_node(tok.value());
            nodes.push_back(node);
        }
    }
};

xmlc::XmlDocument xmlc::parse_document(const std::string& docstr, std::pmr::memory_resource& resource) {
    return XmlDocument{
        .children = XmlParser(docstr, resource).parse()
    };
}

void assert_eq(std::string& expected, std::string& actual) {
    auto cond = "Pass";
    auto stream = stdout;
    if (expected != actual) {
        cond = "Fail";
        stream = stderr;
    }
    fprintf(stream, "%s: \nexpected\n %s\ngot \n %s\n\n", cond, expected.c_str(), actual.c_str());
}

void test_read_tokens() {
    std::string str(R"(<Test TestId="0001" TestType="CMD"> <Name></Name> </Test>)");

    std::pmr::unsynchronized_pool_resource resource;

    XmlParser parser(str, resource);
    auto actual_tokens = parser.read_tokens();

    std::vector<XmlToken> expected_tokens;
    expected_tokens.emplace_back(OpenBegTag{.name = "Test"});
    expected_tokens.emplace_back(AttrName{.name = "TestId"});
    expected_tokens.emplace_back(Assign{});
    expected_tokens.emplace_back(AttrValue{.value = "0001"});
    expected_tokens.emplace_back(AttrName{.name = "TestType"});
    expected_tokens.emplace_back(Assign{});
    expected_tokens.emplace_back(AttrValue{.value = "CMD"});
    expected_tokens.emplace_back(CloseTag{});
    expected_tokens.emplace_back(OpenBegTag{.name = "Name"});
    expected_tokens.emplace_back(CloseTag{});
    expected_tokens.emplace_back(OpenEndTag{.name = "Name"});
    expected_tokens.emplace_back(CloseTag{});
    expected_tokens.emplace_back(OpenEndTag{.name = "Test"});
    expected_tokens.emplace_back(CloseTag{});

    auto actual_serialized = serialize_tokens(actual_tokens);
    auto expected_serialized = serialize_tokens(expected_tokens);

    assert_eq(expected_serialized, actual_serialized);
}

void test_parse_only_elems() {
    std::string str(R"(<Test TestId="0001" TestType="CMD"> <Name> </Name> </Test> )");

    std::pmr::monotonic_buffer_resource resource;
    auto document = xmlc::parse_document(str, resource);

    auto actual_serialized = document.serialize();
    assert_eq(str, actual_serialized);
}

int main() {
    xmlc::DEBUG = true;
    test_read_tokens();
    test_parse_only_elems();
}