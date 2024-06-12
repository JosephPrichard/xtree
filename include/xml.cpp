// Joseph Prichard
// 4/27/2024
// Implementation for the xml document parser

#include <iostream>
#include <memory_resource>
#include <array>
#include "xml.hpp"

using namespace xtree;

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
        return look_ahead(0);
    }

    char look_ahead(unsigned long offset) {
        return text[index + offset];
    }

    bool matches(const std::string& str) {
        if (!has_ahead(str.size() - 1)) {
            return false;
        }
        for (int i = 0; i < str.size(); i++) {
            if (look_ahead(i) != str[i]) {
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

    [[nodiscard]] TokenException invalid_symbol(char symbol, const std::string& custom_message) const {
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

    [[nodiscard]] TokenException unexpected_token(const std::string& actual_tok, const std::string& expected_tok) const {
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

    [[nodiscard]] TokenException invalid_token(const std::string& m) const {
        std::string message =
            m +
            " at row " +
            std::to_string(row) +
            " at col " +
            std::to_string(col);
        return TokenException(std::move(message), row, col);
    }

    [[nodiscard]] TokenException end_of_stream() const {
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

    std::string read_tagname() {
        std::string str;

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
        return str;
    }

    std::string read_attrvalue() {
        char open_char = read();
        if (open_char != '"' && open_char != '\'') {
            throw invalid_symbol(open_char, "attr value must begin with single or double quotes");
        }
        char close_char = open_char;

        std::string str;
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
        return str;
    }

    std::string read_attrname() {
        std::string str;
        while (has_next()) {
            char c = peek();
            if (!is_name_char(c)) {
                break;
            }
            str += c;
            read();
        }
        return str;
    }

    static constexpr int OPEN_CMT_TOK = 0;
    static constexpr int CLOSE_CMT_TOK = 1;
    static constexpr int OPEN_DECL_TOK = 2;
    static constexpr int CLOSE_DECL_TOK = 3;
    static constexpr int OPEN_BEGIN_TOK = 4;
    static constexpr int CLOSE_BEGIN_TOK = 5;
    static constexpr int OPEN_END_TOK = 6;
    static constexpr int CLOSE_END_TOK = 7;
    static constexpr int TEXT_TOK = 8;
    static constexpr int NO_TOK = 9;

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
                read();
                char c2 = read();
                char c3 = read();
                if (c2 == '-' && c3 == '-') {
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

            if (look_ahead(1) == '>') {
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

            if (look_ahead(1) == '>') {
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

            char c1 = look_ahead(1);
            char c2 = look_ahead(2);

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

    void parse_children(const std::string& tag_name, std::vector<std::unique_ptr<Node>>& children) {
        while (has_next()) {
            auto tok = read_open_tok();

            if (tok == NO_TOK) {
                throw end_of_stream();
            }
            else if (tok == OPEN_END_TOK) {
                auto etag_name = read_tagname();
                if (etag_name != tag_name)
                    throw unexpected_token(etag_name, "closing tag '" + tag_name + "'");

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
                auto raw_text = read_rawtext();
                auto node = std::make_unique<Node>(std::move(raw_text));
                children.emplace_back(std::move(node));
            }
            else {
                throw unexpected_token(token_string(tok), "'</', '<!--', '<', <rawtext>");
            }
        }

        throw end_of_stream();
    }

    Comment parse_comment() {
        skip_spaces();

        std::string com_text;
        while (has_next()) {
            char c = peek();
            if (c == '-') {
                auto tok = read_close_tok();
                if (tok == NO_TOK) {
                    throw end_of_stream();
                }
                else if (tok == CLOSE_CMT_TOK) {
                    trim_spaces(com_text);
                    return Comment(std::move(com_text));
                }
            }
            com_text += read();
        }

        throw end_of_stream();
    }

    Elem parse_elem() {
        auto tag_name = read_tagname();

        std::vector<Attr> attrs;
        auto close_tok = parse_attrs(attrs);

        std::vector<std::unique_ptr<Node>> children;
        if (close_tok == CLOSE_END_TOK) {
            parse_children(tag_name, children);
        }
        else if (close_tok != CLOSE_BEGIN_TOK) {
            throw invalid_token("unclosed attributes list in tag");
        }

        return Elem(std::move(tag_name), std::move(attrs), std::move(children));
    }

    Decl parse_decl() {
        auto tag_name = read_tagname();

        std::vector<Attr> attrs;
        auto tok = parse_attrs(attrs);

        if (tok != CLOSE_DECL_TOK) {
            throw unexpected_token(token_string(tok), "?>");
        }

        return Decl(std::move(tag_name), std::move(attrs));
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
                attr.name = read_attrname();
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

            attr.value = read_attrvalue();
            attrs.emplace_back(std::move(attr));
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
                    document.set_root(parse_elem());
                    parsed_root = true;
                } else {
                    throw invalid_token("expected an xml document to only have a single root node");
                }
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

Elem* Elem::select_element(const std::string& ctag) {
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

std::ostream& xtree::operator<<(std::ostream& os, const Node& node) {
    if (auto elem = std::get_if<Elem>(&node.data)) {
        os << *elem;
    }
    else if (auto text = std::get_if<Text>(&node.data)) {
        os << *text;
    }
    else if (auto comment = std::get_if<Comment>(&node.data)) {
        os << *comment;
    }
    return os;
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
        os << child->serialize();
    }
    os << "</" << elem.tag << "> ";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Attr& attr) {
    os << attr.name << "=" << "\"" << attr.value << "\"";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Decl& decl) {
    os << "<?" << decl.tag;
    for (int i = 0; i < decl.attributes.size(); i++) {
        if (i == 0)
            os << " ";

        os << decl.attributes[i];

        if (i < decl.attributes.size() - 1)
            os << " ";
    }
    os << "?> ";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Comment& comment) {
    os << "<!-- " << comment.text << " -->";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const RootNode& node) {
    if (auto text = std::get_if<Decl>(&node.data)) {
        os << *text;
    }
    else if (auto decl = std::get_if<Comment>(&node.data)) {
        os << *decl;
    }
    return os;
}
