// Joseph Prichard
// 4/27/2024
// Implementation for the xml document parser

#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"

#include <iostream>
#include <memory_resource>
#include <array>
#include "xml.hpp"

using namespace xtree;

bool Document::RECURSIVE_PARSER = true;

struct XmlParser {
    const std::string& text;
    int index = 0;
    int row = 0;
    int col = 0;

    explicit XmlParser(const std::string& text) : text(text) {}

    bool has_next() {
        return has_ahead(0);
    }

    bool has_ahead(unsigned long offset) {
        return index + offset < text.size();
    }

    char peek() {
        return peek_ahead(0);
    }

    char peek_ahead(unsigned long offset) {
        return text[index + offset];
    }

    bool matches(const std::string& str) {
        if (!has_ahead(str.size() - 1)) {
            return false;
        }
        for (int i = 0; i < str.size(); i++) {
            if (peek_ahead(i) != str[i]) {
                return false;
            }
        }
        consume(str.size());
        return true;
    }

    char read() {
        if (index < text.size()) {
            char c = text[index++];
            if (c == '\n') {
                col = 0;
                row += 1;
            }
            else {
                col += 1;
            }
            return c;
        }
        else {
            throw end_of_stream();
        }
    }

    void consume(unsigned long count) {
        for (int i = 0; i < count; i++) {
            read();
        }
    }

    void skip_spaces() {
        while (has_next()) {
            if (isspace(peek())) {
                read();
            }
            else {
                break;
            }
        }
    }

    TokenException invalid_symbol(char symbol, const std::string& custom_message) const {
        std::string message =
            "encountered invalid symbol in stream: '"
            + std::string(1, symbol) +
            "', "
            + custom_message +
            " at row "
            + std::to_string(row) +
            " at col "
            + std::to_string(col);
        return TokenException(std::move(message), row, col);
    }

    TokenException unexpected_token(const std::string& actual_tok, const std::string& expected_tok) const {
        std::string message =
            "encountered invalid token in stream: "
            + actual_tok +
            " but expected " +
            expected_tok +
            " at row " +
            std::to_string(row) +
            " at col " +
            std::to_string(col);
        return TokenException(std::move(message), row, col);
    }

    TokenException invalid_token(const std::string& m) const {
        std::string message =
            m +
            " at row " +
            std::to_string(row) +
            " at col " +
            std::to_string(col);
        return TokenException(std::move(message), row, col);
    }

    TokenException end_of_stream() const {
        return invalid_token("reached the end of the stream while parsing");
    }

    char read_escseq() {
        std::string str;
        while (has_next()) {
            char c = peek();
            str += c;
            read();
            if (c == ';') {
                break;
            }
        }
        if (str == "&quot;") {
            return '"';
        }
        else if (str == "&apos;") {
            return '\'';
        }
        else if (str == "&lt;") {
            return '<';
        }
        else if (str == "&gt;") {
            return '>';
        }
        if (str == "&amp;") {
            return '&';
        }
        else {
            throw invalid_token("encountered invalid esc sequence: " + str + " ");
        }
    }

    static void trim_spaces(std::string& str) {
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
    }

    void read_cdata(std::string& str) {
        while (has_next()) {
            char c = read();
            if (c == ']') {
                if (has_ahead(1)) {
                    char c1 = read();
                    char c2 = read();

                    if (c1 == ']' && c2 == '>') {
                        return;
                    }
                }
            }

            str += c;
        }
    }

    std::string read_rawtext() {
        skip_spaces();

        std::string str;
        while (has_next()) {
            char c = peek();

            if (c == '&') {
                auto esc_char = read_escseq();
                str += esc_char;
            }
            else if (c == '<') {
                if (matches("<![CDATA[")) {
                    read_cdata(str);
                }
                else {
                    break;
                }
            }
            else {
                str += c;
                read();
            }
        }

        trim_spaces(str);
        return str;
    }

    // non unicode compliant character detection
    bool static is_name_start_char(char c) {
        return isalpha(c) || c == ':' || c == '_';
    }

    bool static is_name_char(char c) {
        return is_name_start_char(c) || isdigit(c) || c == '-' || c == '.';
    }

    void read_tagname(std::string& str) {
        int i = 0;
        while (has_next()) {
            char c = peek();
            if (c == ' ' || c == '>' || c == '?' || c == '/') {
                break;
            }
            if (i == 0 && is_name_start_char(c) || i > 0 && is_name_char(c)) {
                str += c;
                read();
            }
            else {
                throw invalid_symbol(c, "begin tag must contain only alphanumerics");
            }
            i += 1;
        }
    }

    void read_attrvalue(std::string& str) {
        char open_char = read();
        if (open_char != '"' && open_char != '\'') {
            throw invalid_symbol(open_char, "attr value must begin with single or double quotes");
        }
        char close_char = open_char;

        while (has_next()) {
            char c = peek();
            if (c == '&') {
                auto esc_char = read_escseq();
                str += esc_char;
            }
            else {
                read();
                if (c == close_char) {
                    break;
                }
                str += c;
            }
        }
    }

    void read_attrname(std::string& str) {
        while (has_next()) {
            char c = peek();
            if (!is_name_char(c)) {
                break;
            }
            str += c;
            read();
        }
    }

    static constexpr int OPEN_CMT_TOK = 0;
    static constexpr int CLOSE_CMT_TOK = 1;
    static constexpr int OPEN_DECL_TOK = 2;
    static constexpr int CLOSE_DECL_TOK = 3;
    static constexpr int OPEN_BEGIN_TOK = 4;
    static constexpr int CLOSE_BEGIN_TOK = 5;
    static constexpr int OPEN_END_TOK = 6;
    static constexpr int CLOSE_END_TOK = 7;
    static constexpr int OPEN_DTD_TOK = 8;
    static constexpr int TEXT_TOK = 9;
    static constexpr int NO_TOK = 10;

    static std::string token_string(int tok) {
        switch (tok) {
        case OPEN_CMT_TOK:
            return "'<!--'";
        case CLOSE_CMT_TOK:
            return "'-->'";
        case OPEN_DECL_TOK:
            return "'<?'";
        case CLOSE_DECL_TOK:
            return "'?>'";
        case OPEN_BEGIN_TOK:
            return "'<'";
        case CLOSE_BEGIN_TOK:
            return "'/>'";
        case OPEN_END_TOK:
            return "'</'";
        case CLOSE_END_TOK:
            return "'>'";
        case OPEN_DTD_TOK:
            return "<!DOCTYPE";
        case TEXT_TOK:
            return "<rawtext>";
        default:
            return "<unknown>";
        }
    }

    int read_open_tok() {
        skip_spaces();

        if (!has_next()) {
            return NO_TOK;
        }

        char c = peek();
        if (c == '<') {
            read();
            char c1 = peek();
            switch (c1) {
            case '/':
                read();
                return OPEN_END_TOK;
            case '?':
                read();
                return OPEN_DECL_TOK;
            case '!': {
                if (matches("!DOCTYPE")) {
                    return OPEN_DTD_TOK;
                }

                if (!has_ahead(2)) {
                    return TEXT_TOK;
                }

                char c2 = peek_ahead(1);
                char c3 = peek_ahead(2);

                if (c2 == '-' && c3 == '-') {
                    consume(3);
                    return OPEN_CMT_TOK;
                }
                else {
                    throw invalid_token("invalid open comment tag, must be '<!--'");
                }
            }
            default:
                return OPEN_BEGIN_TOK;
            }
        }
        else {
            return TEXT_TOK;
        }
    }

    int read_close_tok() {
        skip_spaces();

        if (!has_next()) {
            return NO_TOK;
        }

        char c = peek();
        switch (c) {
        case '/': {
            if (!has_ahead(1)) {
                return TEXT_TOK;
            }

            if (peek_ahead(1) == '>') {
                consume(2);
                return CLOSE_BEGIN_TOK;
            }
            else {
                return TEXT_TOK;
            }
        }
        case '?':
            if (!has_ahead(1)) {
                return TEXT_TOK;
            }

            if (peek_ahead(1) == '>') {
                consume(2);
                return CLOSE_DECL_TOK;
            }
            else {
                return TEXT_TOK;
            }
        case '>':
            read();
            return CLOSE_END_TOK;
        case '-': {
            if (!has_ahead(2)) {
                return TEXT_TOK;
            }

            char c1 = peek_ahead(1);
            char c2 = peek_ahead(2);

            if (c1 == '-' && c2 == '>') {
                consume(3);
                return CLOSE_CMT_TOK;
            }
            else if (c1 != '-') {
                return TEXT_TOK;
            }
            else {
                throw invalid_symbol(c, "unclosed '-->' tag in document");
            }
        }
        case '<':
            throw unexpected_token(std::string(1, c), "close token: '-', '>', '?', or '/'");
        default:
            return TEXT_TOK;
        }
    }

    int parse_attrs(std::vector<Attr>& attrs) {
        while (has_next()) {
            Attr attr;

            auto tok = read_close_tok();
            if (tok == NO_TOK) {
                throw end_of_stream();
            }
            else if (tok == CLOSE_END_TOK || tok == CLOSE_BEGIN_TOK || tok == CLOSE_DECL_TOK) {
                return tok;
            }
            else if (tok == TEXT_TOK) {
                read_attrname(attr.name);
            }
            else {
                throw unexpected_token(token_string(tok), "attrname, '>', or '/>'");
            }

            if (!has_next()) {
                throw end_of_stream();
            }
            char c = read();
            if (c != '=') {
                throw unexpected_token(std::string(1, c), "'='");
            }

            read_attrvalue(attr.value);
            attrs.emplace_back(std::move(attr));
        }

        throw end_of_stream();
    }

    void parse_children(const std::string& tag_name, std::vector<std::unique_ptr<Node>>& children) {
        while (has_next()) {
            auto tok = read_open_tok();

            if (tok == NO_TOK) {
                throw end_of_stream();
            }
            else if (tok == OPEN_END_TOK) {
                std::string etag_name;
                read_tagname(etag_name);

                if (etag_name != tag_name) {
                    throw unexpected_token(etag_name, "closing tag '" + tag_name + "'");
                }

                auto tok1 = read_close_tok();
                if (tok1 == NO_TOK) {
                    throw end_of_stream();
                }
                if (tok1 != CLOSE_END_TOK) {
                    throw unexpected_token(token_string(tok1), "'>'");
                }
                return;
            }
            else if (tok == OPEN_CMT_TOK) {
                auto node = std::make_unique<Node>(parse_comment());
                children.emplace_back(std::move(node));
            }
            else if (tok == OPEN_BEGIN_TOK) {
                auto node = std::make_unique<Node>(parse_elem());
                children.emplace_back(std::move(node));
            }
            else if (tok == TEXT_TOK) {
                auto node = std::make_unique<Node>(read_rawtext());
                children.emplace_back(std::move(node));
            }
            else {
                throw unexpected_token(token_string(tok), "'</', '<!--', '<', <rawtext>");
            }
        }

        throw end_of_stream();
    }

    Elem parse_elem() {
        Elem elem;

        read_tagname(elem.tag);
        auto close_tok = parse_attrs(elem.attributes);

        if (close_tok == CLOSE_END_TOK) {
            parse_children(elem.tag, elem.children);
        }
        else if (close_tok != CLOSE_BEGIN_TOK) {
            throw invalid_token("unclosed attributes list in tag");
        }

        return elem;
    }

    Elem parse_elem_tree() {
        std::vector<Elem*> stack;

        // start by parsing the root elem node
        Elem root;

        read_tagname(root.tag);
        auto close_tok = parse_attrs(root.attributes);

        if (close_tok == CLOSE_END_TOK) {
            stack.emplace_back(&root);
        }
        else if (close_tok == CLOSE_BEGIN_TOK) {
            // the root has no children
            return root;
        } else {
            throw invalid_token("unclosed attributes list in tag");
        }

        //  parse until the stack is empty using tokens to decide when to push/pop
        while (!stack.empty()) {
            if (!has_next()) {
                throw end_of_stream();
            }
            auto top = stack.back();

            auto tok = read_open_tok();

            if (tok == NO_TOK) {
                throw end_of_stream();
            }
            else if (tok == OPEN_END_TOK) {
                std::string etag_name;
                read_tagname(etag_name);

                if (etag_name != top->tag) {
                    throw unexpected_token(etag_name, "closing tag '" + top->tag + "'");
                }

                auto tok1 = read_close_tok();
                if (tok1 == NO_TOK) {
                    throw end_of_stream();
                }
                if (tok1 != CLOSE_END_TOK) {
                    throw unexpected_token(token_string(tok1), "'>'");
                }

                // reaching the end of this node means we back track
                stack.pop_back();
            } else if (tok == OPEN_CMT_TOK) {
                auto node = std::make_unique<Node>(parse_comment());
                top->children.emplace_back(std::move(node));
            }
            else if (tok == OPEN_BEGIN_TOK) {
                // read the next element to be processed by the parser
                auto node = std::make_unique<Node>(Elem());
                auto elem = &node.get()->as_elem();

                read_tagname(elem->tag);
                close_tok = parse_attrs(elem->attributes);

                if (close_tok == CLOSE_END_TOK) {
                    // this will be the next node we parse
                    stack.emplace_back(elem);
                }
                else if (close_tok != CLOSE_BEGIN_TOK) { // CL0SE_BEGIN_TOK means the node has no children
                    throw invalid_token("unclosed attributes list in tag");
                }

                top->children.emplace_back(std::move(node));
            }
            else if (tok == TEXT_TOK) {
                auto raw_text = read_rawtext();
                auto node = std::make_unique<Node>(std::move(raw_text));
                top->children.emplace_back(std::move(node));
            }
            else {
                throw unexpected_token(token_string(tok), "'</', '<!--', '<', <rawtext>");
            }
        }

        return root;
    }

    Comment parse_comment() {
        skip_spaces();

        Comment comment;

        while (has_next()) {
            char c = peek();
            if (c == '-') {
                auto tok = read_close_tok();
                if (tok == NO_TOK) {
                    throw end_of_stream();
                }
                else if (tok == CLOSE_CMT_TOK) {
                    trim_spaces(comment.text);
                    return comment;
                }
            }
            comment.text += read();
        }

        throw end_of_stream();
    }

    Decl parse_decl() {
        Decl decl;

        read_tagname(decl.tag);
        auto tok = parse_attrs(decl.attributes);

        if (tok != CLOSE_DECL_TOK) {
            throw unexpected_token(token_string(tok), "?>");
        }

        return decl;
    }

    DocType parse_dtd() {
        DocType dtd;

        skip_spaces();

        while (has_next()) {
            auto c = read();
            if (c == '>') {
                trim_spaces(dtd.text);
                return dtd;
            }
            dtd.text += c;
        }

        throw end_of_stream();
    }

    void parse(Document& document) {
        bool parsed_root = false;

        while (has_next()) {
            auto tok = read_open_tok();
            if (tok == NO_TOK) {
                return;
            }
            if (tok == OPEN_BEGIN_TOK) {
                if (!parsed_root) {
                    if (Document::RECURSIVE_PARSER) {
                        document.set_root(parse_elem());
                    } else {
                        document.set_root(parse_elem_tree());
                    }
                    parsed_root = true;
                } else {
                    throw invalid_token("expected an xml document to only have a single root node");
                }
            }
            else if (tok == OPEN_DTD_TOK) {
                auto node = std::make_unique<RootNode>(parse_dtd());
                document.children.emplace_back(std::move(node));
            }
            else if (tok == OPEN_DECL_TOK) {
                auto node = std::make_unique<RootNode>(parse_decl());
                document.children.emplace_back(std::move(node));
            }
            else if (tok == OPEN_CMT_TOK) {
                auto node = std::make_unique<RootNode>(parse_comment());
                document.children.emplace_back(std::move(node));
            }
            else {
                throw unexpected_token(token_string(tok), "'</' or '<?'");
            }
        }
    }
};

Document::Document(const std::string& docstr) {
    XmlParser parser(docstr);
    parser.parse(*this);
}

Elem* Elem::select_elem(const std::string& ctag) {
    for (auto& child: children)
        if (auto elem = get_if<Elem>(&child->data))
            if (elem->tag == ctag)
                return elem;
    return nullptr;
}

Attr* Elem::select_attr(const std::string& attr_name) {
    for (auto& attr: attributes)
        if (attr.get_name() == attr_name)
            return &attr;
    return nullptr;
}

void Elem::remove_node(const std::string& rtag) {
    auto it = children.begin();
    while (it != children.end()) {
        if ((*it)->is_elem() && (*it)->as_elem().tag == rtag) {
            it = children.erase(it);
        } else {
            it++;
        }
    }
}

std::ostream& xtree::operator<<(std::ostream& os, const Elem& elem) {
    os << "<" << elem.tag;
    for (int i = 0; i < elem.attributes.size(); i++) {
        if (i == 0)
            os << " ";

        os << elem.attributes[i];

        if (i < elem.attributes.size() - 1)
            os << " ";
    }
    os << "> ";
    for (auto& child: elem.children) {
        os << *child;
    }
    os << "</" << elem.tag << "> ";
    return os;
}

#pragma clang diagnostic pop