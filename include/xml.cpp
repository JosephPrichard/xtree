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
        return TokenException(message);
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
        return TokenException(message);
    }

    [[nodiscard]] TokenException invalid_token(const std::string& m) const {
        std::string message =
            m +
            " at row " +
            std::to_string(row) +
            " at col " +
            std::to_string(col);
        return TokenException(message);
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
                    char c2 = look_ahead(1);

                    if (c1 == ']' && c2 == '>') {
                        read();
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

    enum Token {
        OpenCmtTok,
        CloseCmtTok,
        OpenDeclTok,
        CloseDeclTok,
        OpenBeginTok,
        CloseBeginTok,
        OpenEndTok,
        CloseEndTok,
        AssignTok,
        TextTok,
    };

    static std::string token_string(Token token) {
        switch (token) {
        case OpenCmtTok:
            return "'<!--'";
        case CloseCmtTok:
            return "'-->'";
        case OpenDeclTok:
            return "'<?'";
        case CloseDeclTok:
            return "'?>'";
        case OpenBeginTok:
            return "'<'";
        case CloseBeginTok:
            return "'/>'";
        case OpenEndTok:
            return "'</'";
        case CloseEndTok:
            return "'>'";
        case AssignTok:
            return "'='";
        case TextTok:
            return "<rawtext>";
        }
    }

    std::optional<Token> read_token() {
        skip_spaces();

        if (!has_next()) {
            return std::nullopt;
        }

        char c = peek();
        switch (c) {
        case '<': {
            read();
            char c1 = peek();
            switch (c1) {
            case '/':
                read();
                return OpenEndTok;
            case '?':
                read();
                return OpenDeclTok;
            case '!': {
                read();
                char c2 = read();
                char c3 = read();
                if (c2 == '-' && c3 == '-') {
                    return OpenCmtTok;
                }
                else {
                    throw invalid_token("invalid open comment tag, must be '<!--'");
                }
            }
            default:
                return OpenBeginTok;
            }
        }
        case '/': {
            if (!has_ahead(1)) {
                return TextTok;
            }

            if (look_ahead(1) == '>') {
                consume(2);
                return CloseBeginTok;
            }
            else {
                return TextTok;
            }
        }
        case '=':
            read();
            return AssignTok;
        case '?':
            if (!has_ahead(1)) {
                return TextTok;
            }

            if (look_ahead(1) == '>') {
                consume(2);
                return CloseDeclTok;
            }
            else {
                return TextTok;
            }
        case '>':
            read();
            return CloseEndTok;
        case '-': {
            if (!has_ahead(2)) {
                return TextTok;
            }

            char c1 = look_ahead(1);
            char c2 = look_ahead(2);

            if (c1 == '-' && c2 == '>') {
                consume(3);
                return CloseCmtTok;
            }
            else if (c1 != '-') {
                return TextTok;
            }
            else {
                throw invalid_symbol(c, "unclosed '-->' tag in document");
            }
        }
        default:
            return TextTok;
        }
    }

    void parse_children(const std::string& tag_name, std::vector<std::unique_ptr<Node>>& children) {
        while (has_next()) {
            auto opt_tok = read_token();
            if (!opt_tok.has_value()) {
                throw end_of_stream();
            }

            auto tok = opt_tok.value();
            if (tok == OpenEndTok) {
                // stop searching for children on an open end tag - the tagname after the open end tag must match
                auto etag_name = read_tagname();
                if (etag_name != tag_name)
                    throw unexpected_token(std::string(etag_name), "closing tag '" + std::string(tag_name) + "'");

                // the open end tag should also be terminated by a close tag
                opt_tok = read_token();
                if (!opt_tok.has_value()) {
                    throw end_of_stream();
                }

                tok = opt_tok.value();
                if (tok != CloseEndTok) {
                    throw unexpected_token(token_string(tok), "'>'");
                }
                return;
            }
            else if (tok == OpenCmtTok) {
                auto node = parse_comment();
                children.emplace_back(std::move(node));
            }
            else if (tok == OpenBeginTok) {
                auto node = parse_elem();
                children.emplace_back(std::move(node));
            }
            else if (tok == TextTok) {
                auto raw_text = read_rawtext();
                auto node = std::make_unique<Node>(std::move(raw_text));
                children.emplace_back(std::move(node));
            }
            else {
                throw unexpected_token(token_string(tok), "'</', '<!--', '<', <rawtext>");
            }
        }
    }

    std::unique_ptr<Node> parse_comment() {
        skip_spaces();

        std::string com_text;
        while (has_next()) {
            char c = peek();
            if (c == '-') {
                auto opt_tok = read_token();
                if (!opt_tok.has_value()) {
                    throw end_of_stream();
                }

                auto tok = opt_tok.value();
                if (tok == CloseCmtTok) {
                    trim_spaces(com_text);
                    return std::make_unique<Node>(Comment(std::move(com_text)));
                }
            }
            com_text += read();
        }

        throw end_of_stream();
    }

    std::unique_ptr<Node> parse_elem() {
        auto tag_name = read_tagname();

        std::vector<Attr> attrs;
        auto close_tok = parse_attrs(attrs);

        std::vector<std::unique_ptr<Node>> children;
        if (close_tok == CloseEndTok) {
            // close tag indicates we might have some children to parse
            parse_children(tag_name, children);
        }
        else if (close_tok != CloseBeginTok) {
            throw invalid_token("unclosed attributes list within tag '-' should end with a close tag");
        }

        return std::make_unique<Node>(Elem(std::move(tag_name), std::move(attrs), std::move(children)));
    }

    std::unique_ptr<Node> parse_decl() {
        auto tag_name = read_tagname();

        std::vector<Attr> attrs;
        auto tok = parse_attrs(attrs);

        if (tok != CloseDeclTok) {
            throw unexpected_token(token_string(tok), "?>");
        }

        return std::make_unique<Node>(Decl(std::move(tag_name), std::move(attrs)));
    }

    Token parse_attrs(std::vector<Attr>& attrs) {
        while (has_next()) {
            Attr attr;

            auto opt_tok = read_token();
            if (!opt_tok.has_value()) {
                throw end_of_stream();
            }

            auto tok = opt_tok.value();
            if (tok == CloseEndTok || tok == CloseBeginTok || tok == CloseDeclTok) {
                return tok;
            }
            else if (tok == TextTok) {
                attr.name = read_attrname();
            }
            else {
                throw unexpected_token(token_string(tok), "attrname, '>', or '/>'");
            }

            auto opt_tok1 = read_token();
            if (!opt_tok1.has_value()) {
                throw end_of_stream();
            }

            auto tok1 = opt_tok1.value();
            if (tok1 != AssignTok) {
                throw unexpected_token(token_string(tok1), "'='");
            }

            attr.value = read_attrvalue();
            attrs.emplace_back(std::move(attr));
        }

        throw end_of_stream();
    }

    void parse(std::vector<std::unique_ptr<Node>>& root_children) {
        while (has_next()) {
            auto opt_tok = read_token();
            if (!opt_tok.has_value()) {
                return;
            }

            auto tok = opt_tok.value();
            if (tok == OpenBeginTok) {
                auto node = parse_elem();
                root_children.push_back(std::move(node));
            }
            else if (tok == OpenDeclTok) {
                auto node = parse_decl();
                root_children.push_back(std::move(node));
            }
            else {
                throw unexpected_token(token_string(tok), "'</' or '<?'");
            }
        }
    }
};

std::ostream& xtree::operator<<(std::ostream& os, const Node& node) {
    if (auto elem = std::get_if<Elem>(&node.data)) {
        os << "<" << elem->tag;
        for (int i = 0; i < elem->attributes.size(); i++) {
            if (i == 0)
                os << " ";

            auto& attr = elem->attributes[i];
            os << attr.name << "=" << "\"" << attr.value << "\"";

            if (i < elem->attributes.size() - 1)
                os << " ";
        }
        os << "> ";
        for (auto& child: elem->children) {
            os << child->serialize();
        }
        os << "</" << elem->tag << "> ";
    }
    else if (auto text = std::get_if<Text>(&node.data)) {
        os << *text;
    }
    else if (auto decl = std::get_if<Decl>(&node.data)) {
        os << "<?" << decl->tag;
        for (int i = 0; i < decl->attributes.size(); i++) {
            if (i == 0)
                os << " ";

            auto& attr = decl->attributes[i];
            os << attr.name << "=" << "\"" << attr.value << "\" ";

            if (i < decl->attributes.size() - 1)
                os << " ";
        }
        os << "?> ";
    }
    else if (auto comment = std::get_if<Comment>(&node.data)) {
        os << "<!-- " << comment->text << " -->";
    }
    return os;
}

Document::Document(const std::string& docstr) {
    XmlParser parser(docstr);
    parser.parse(children);
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
