// Joseph Prichard
// 4/27/2024
// Implementation for the xml document parser

#include <iostream>
#include <stack>
#include <sstream>
#include "xtree.hpp"

using namespace xtree;

bool Document::RECURSIVE_PARSER = true;

struct XmlParser {
    const std::string& text;

    size_t index = 0;
    int row = 1;
    int col = 1;

    explicit XmlParser(const std::string& text) : text(text) {}

    char read_char() {
        return text[index++];
    }

    bool has_next() {
        return index < text.size();
    }

    bool has_ahead(size_t offset) {
        return index + offset < text.size();
    }

    int peek() {
        return (int) text[index];
    }

    int peek_ahead(size_t offset) {
        return (int) text[index + offset];
    }

    bool read_matching(const std::string& str) {
        auto match = peek_matching(str);
        if (match) {
            consume(str.size());
        }
        return match;
    }

    bool peek_matching(const std::string& str) {
        if (!has_ahead(str.size() - 1)) {
            return false;
        }
        for (size_t i = 0; i < str.size(); i++) {
            if (peek_ahead(i) != str[i]) {
                return false;
            }
        }
        return true;
    }

    int read() {
        if (index < text.size()) {
            char c = text[index++];
            if (c == '\n') {
                col = 1;
                row += 1;
            }
            else {
                col += 1;
            }
            return (int) c;
        }
        else {
            throw end_of_stream();
        }
    }

    void consume(unsigned long count) {
        for (size_t i = 0; i < count; i++) {
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

    ParseException invalid_symbol(char symbol, const std::string& custom_message) const {
        std::string message =
            "encountered invalid symbol in stream: '"
            + std::string(1, symbol) +
            "', "
            + custom_message +
            " at row "
            + std::to_string(row) +
            ", col "
            + std::to_string(col);
        return ParseException(message);
    }

    ParseException unexpected_token(const std::string& actual_tok, const std::string& expected_tok) const {
        std::string message =
            "encountered invalid token in stream: "
            + actual_tok +
            " but expected " +
            expected_tok +
            " at row " +
            std::to_string(row) +
            ", col " +
            std::to_string(col);
        return ParseException(message);
    }

    ParseException invalid_token(const std::string& m) const {
        std::string message =
            m +
            " at row " +
            std::to_string(row) +
            ", col " +
            std::to_string(col);
        return ParseException(message);
    }

    ParseException end_of_stream() const {
        return invalid_token("reached the end of the stream while parsing at row " + 
            std::to_string(row) + ", col" + std::to_string(col));
    }

    // reads into the buffer argument to avoid an additional allocation and erases after finished
    void read_escseq(std::string& str) {
        int i = 0;
        while (has_next()) {
            int c = read();
            str += c;
            i++;
            if (c == ';') {
                break;
            }
        }
        
        std::string_view view(str.data() + str.size() - i, i);

        char esc_ch;
        if (view == "&quot;") {
            esc_ch = '"';
        }
        else if (view == "&apos;") {
            esc_ch = '\'';
        }
        else if (view == "&lt;") {
            esc_ch = '<';
        }
        else if (view == "&gt;") {
            esc_ch = '>';
        }
        else if (view == "&amp;") {
            esc_ch = '&';
        }
        else {
            throw invalid_token("encountered invalid esc sequence: '" + str + "'");
        }

        str.erase(str.size() - i, str.size());
        str += esc_ch;
    }

    static void trim_spaces(std::string& str) {
        size_t i = str.size() - 1;
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
            int c = read();
            if (c == ']') {
                if (has_ahead(1)) {
                    int c1 = read();
                    int c2 = read();
                    if (c1 == ']' && c2 == '>') {
                        return;
                    }
                }
            }

            str += c;
        }
        str.shrink_to_fit();
    }

    Text read_rawtext() {
        skip_spaces();

        std::string str;
        while (has_next()) {
            int c = peek();

            if (c == '&') {
                read_escseq(str);
            }
            else if (c == '<') {
                if (read_matching("<![CDATA[")) {
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
        str.shrink_to_fit();
        return {str};
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
            int c = peek();
            if (c == ' ' || c == '>' || c == '?' || c == '/') {
                break;
            }
            if ((i == 0 && is_name_start_char(c)) || (i > 0 && is_name_char(c))) {
                str += c;
                read();
            }
            else {
                throw invalid_symbol(c, "begin tag must contain only alphanumerics");
            }
            i += 1;
        }
        str.shrink_to_fit();
    }

    void read_attrvalue(std::string& str) {
        char open_char = read();
        if (open_char != '"' && open_char != '\'') {
            throw invalid_symbol(open_char, "attr val must begin with single or double quotes");
        }
        char close_char = open_char;

        while (has_next()) {
            int c = peek();
            if (c == '&') {
                read_escseq(str);
            }
            else {
                read();
                if (c == close_char) {
                    break;
                }
                str += c;
            }
        }
        str.shrink_to_fit();
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
        str.shrink_to_fit();
    }

    enum Token {
        open_cmt = 0,
        close_cmt = 1,
        open_decl = 2,
        close_decl = 3,
        open_beg = 4,
        close_beg = 5,
        open_end = 6,
        close_end = 7,
        open_dtd = 8,
        raw_text = 9,
        end_tok = 10
    };

    static std::string token_string(Token tok) {
        switch (tok) {
        case open_cmt:
            return "'<!--'";
        case close_cmt:
            return "'-->'";
        case open_decl:
            return "'<?'";
        case close_decl:
            return "'?>'";
        case open_beg:
            return "'<'";
        case close_beg:
            return "'/>'";
        case open_end:
            return "'</'";
        case close_end:
            return "'>'";
        case open_dtd:
            return "<!DOCTYPE";
        case raw_text:
            return "<rawtext>";
        default:
            return "<unknown>";
        }
    }

    Token read_open_tok() {
        skip_spaces();

        if (!has_next()) {
            return end_tok;
        }

        int c = peek();
        if (c == '<') {
            if (!has_ahead(1)) {
                return raw_text;
            }
            c = peek_ahead(1);

            switch (c) {
            case '/':
                consume(2); // </
                return open_end;
            case '?':
                consume(2); // <?
                return open_decl;
            case '!': {
                if (read_matching("<!DOCTYPE")) {
                    return open_dtd; // <!DOCTYPE
                }
                if (peek_matching("<![CDATA[")) {
                    return raw_text; // <![CDATA[
                }
                if (!has_ahead(4)) {
                    // not enough characters to be a comment token, so it must be a text token...
                    return raw_text;
                }
                if (read_matching("<!--")) {
                    return open_cmt; // <!--
                }

                throw invalid_token("invalid open comment tag, must be '<!--'");
            }
            default:
                read(); // <
                return open_beg;
            }
        }
        else {
            return raw_text;
        }
    }

    Token read_close_tok() {
        skip_spaces();

        if (!has_next()) {
            return end_tok;
        }

        int c = peek();
        switch (c) {
        case '/': {
            if (!has_ahead(1)) {
                return raw_text;
            }

            if (peek_ahead(1) == '>') {
                consume(2); // />
                return close_beg;
            }
            else {
                return raw_text;
            }
        }
        case '?':
            if (!has_ahead(1)) {
                return raw_text;
            }

            if (peek_ahead(1) == '>') {
                consume(2); // ?>
                return close_decl;
            }
            else {
                return raw_text;
            }
        case '>':
            read(); // >
            return close_end;
        case '-': {
            if (!has_ahead(2)) {
                return raw_text;
            }

            char c1 = peek_ahead(1);
            char c2 = peek_ahead(2);

            if (c1 == '-' && c2 == '>') {
                consume(3); // -->
                return close_cmt;
            }
            if (c1 != '-') {
                return raw_text;
            }

            throw invalid_symbol(c, "unclosed '-->' tag in document");
        }
        case '<':
            throw unexpected_token(std::string(1, c), "close token: '-', '>', '?', or '/'");
        default:
            return raw_text;
        }
    }

    Token parse_attrs(std::vector<Attr>& attrs) {
        while (has_next()) {
            Attr attr;

            auto tok = read_close_tok();
            if (tok == end_tok) {
                throw end_of_stream();
            }
            else if (tok == close_end || tok == close_beg || tok == close_decl) {
                attrs.shrink_to_fit();
                return tok;
            }
            else if (tok == raw_text) {
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

            if (tok == end_tok) {
                throw end_of_stream();
            }
            else if (tok == open_end) {
                std::string etag_name;
                read_tagname(etag_name);

                if (etag_name != tag_name) {
                    throw unexpected_token("'" + etag_name + "'", "closing tag '" + tag_name + "'");
                }

                auto tok1 = read_close_tok();
                if (tok1 == end_tok) {
                    throw end_of_stream();
                }
                if (tok1 != close_end) {
                    throw unexpected_token(token_string(tok1), "'>'");
                }

                children.shrink_to_fit();
                return;
            }
            else if (tok == open_cmt) {
                auto comment = parse_comment();
                auto node = std::make_unique<Node>(comment);
                children.emplace_back(std::move(node));
            }
            else if (tok == open_beg) {
                auto elem = parse_elem();
                auto node = std::make_unique<Node>(elem);
                children.emplace_back(std::move(node));
            }
            else if (tok == raw_text) {
                auto raw_text = read_rawtext();
                auto node = std::make_unique<Node>(raw_text);
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

        if (close_tok == close_end) {
            parse_children(elem.tag, elem.children);
        }
        else if (close_tok != close_beg) {
            throw invalid_token("unclosed attrs list in tag");
        }

        return elem;
    }

    Elem parse_elem_tree() {
        std::stack<Elem*> stack;

        // start by parsing the root elem node
        Elem root;

        read_tagname(root.tag);
        auto close_tok = parse_attrs(root.attributes);

        if (close_tok == close_end) {
            stack.push(&root);
        }
        else if (close_tok == close_beg) {
            // the root has no child_nodes
            return root;
        } else {
            throw invalid_token("unclosed attrs list in tag");
        }

        //  parse until the stack is empty using tokens to decide when to push/pop
        while (!stack.empty()) {
            if (!has_next()) {
                throw end_of_stream();
            }
            Elem* top = stack.top();

            auto tok = read_open_tok();

            if (tok == end_tok) {
                throw end_of_stream();
            }
            else if (tok == open_end) {
                std::string etag_name;
                read_tagname(etag_name);

                if (etag_name != top->tag) {
                    throw unexpected_token("'" + etag_name + "'", "closing tag '" + top->tag + "'");
                }

                auto tok1 = read_close_tok();
                if (tok1 == end_tok) {
                    throw end_of_stream();
                }
                if (tok1 != close_end) {
                    throw unexpected_token(token_string(tok1), "'>'");
                }

                // reaching the end of this node means we back track
                top->children.shrink_to_fit();
                stack.pop();
            } else if (tok == open_cmt) {
                auto comment = parse_comment();
                auto node = std::make_unique<Node>(comment);
                top->children.emplace_back(std::move(node));
            }
            else if (tok == open_beg) {
                // read the next element to be processed by the parser
                auto node = std::make_unique<Node>(Elem());
                auto elem_ptr = &node.get()->as_elem();

                read_tagname(elem_ptr->tag);
                close_tok = parse_attrs(elem_ptr->attributes);

                if (close_tok == close_end) {
                    // this will be the next node we parse
                    stack.push(elem_ptr);
                }
                else if (close_tok != close_beg) { // Close_beg means the node has no children
                    throw invalid_token("unclosed attrs list in tag");
                }

                top->children.emplace_back(std::move(node));
            }
            else if (tok == raw_text) {
                auto raw_text = read_rawtext();
                auto node = std::make_unique<Node>(raw_text);
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
            int c = peek();
            if (c == '-') {
                auto tok = read_close_tok();
                if (tok == end_tok) {
                    throw end_of_stream();
                }
                else if (tok == close_cmt) {
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
        auto tok = parse_attrs(decl.attrs);

        if (tok != close_decl) {
            throw unexpected_token(token_string(tok), "?>");
        }

        return decl;
    }

    DocType parse_dtd() {
        DocType dtd;

        skip_spaces();

        while (has_next()) {
            int c = read();
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
            if (tok == end_tok) {
                return;
            }
            if (tok == open_beg) {
                if (!parsed_root) {
                    if (Document::RECURSIVE_PARSER) {
                        auto elem = parse_elem();
                        document.add_root(std::move(elem));
                    } else {
                        auto elem = parse_elem_tree();
                        document.add_root(std::move(elem));
                    }
                    parsed_root = true;
                } else {
                    throw invalid_token("expected an xml document to only have a single root node");
                }
            }
            else if (tok == open_dtd) {
                auto dtd = parse_dtd();
                auto node = std::make_unique<RootNode>(dtd);
                document.children.emplace_back(std::move(node));
            }
            else if (tok == open_decl) {
                auto decl = parse_decl();
                auto node = std::make_unique<RootNode>(decl);
                document.children.emplace_back(std::move(node));
            }
            else if (tok == open_cmt) {
                auto comment = parse_comment();
                auto node = std::make_unique<RootNode>(comment);
                document.children.emplace_back(std::move(node));
            }
            else {
                throw unexpected_token(token_string(tok), "'</', '<!--' or '<?'");
            }
        }
    }
};

Document::Document(const std::string& docstr) {
    XmlParser parser(docstr);
    parser.parse(*this);
}

std::string Document::serialize() const {
    std::ostringstream ss;
    ss << (*this);
    return ss.str();
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
        if (attr.name == attr_name)
            return &attr;
    return nullptr;
}

Elem& Elem::expect_elem(const std::string& ctag) {
    for (auto& child: children)
        if (auto elem = get_if<Elem>(&child->data))
            if (elem->tag == ctag)
                return *elem;
    throw NodeWalkException("Element does not contain child with tag name " + ctag);
}

Attr& Elem::select_attr_ex(const std::string& attr_name) {
    for (auto& attr: attributes)
        if (attr.name == attr_name)
            return attr;
    throw NodeWalkException("Element does not contain attribute with name " + attr_name);
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

Elem& Elem::operator=(const Elem& other) {
    if (this == &other) {
        return *this;
    }

    tag = other.tag;
    attributes = other.attributes;

    // we must store the copied nodes somewhere before we clear and append since other.child_nodes might be a child of child_nodes
    std::vector<std::unique_ptr<Node>> temp_nodes;
    temp_nodes.reserve(other.children.size());

    for (auto& node : other.children) {
        temp_nodes.emplace_back(std::make_unique<Node>(*node));
    }

    children.clear();
    for (auto& node : temp_nodes) {
        children.emplace_back(std::move(node));
    }

    return *this;
}

void DocumentWalker::walk_document(Document& document) {
    for (auto& child: document.children) {
        if (child->is_decl()) {
            on_decl(child->as_decl());
        } 
        else if (child->is_dtd()) {
            on_dtd(child->as_dtd());
        }
        else if (child->is_comment()) {
            on_comment(child->as_comment());
        }
    }

    on_elem(*document.root);

    std::stack<Elem*> stack;
    stack.push(document.root.get());

    while (!stack.empty()) {
        Elem* top = stack.top();
        stack.pop();

        for (auto& child: top->children) {
            if (child->is_text()) {
                on_text(child->as_text());
            }
            else if (child->is_elem()) {
                on_elem(child->as_elem());
                stack.push(&child->as_elem());
            }
            else if (child->is_comment()) {
                on_comment(child->as_comment());
            }
        }
    }
}

std::ostream& xtree::operator<<(std::ostream& os, const Attr& attr) {
    os << attr.name << "=" << "\"";
    for (auto c : attr.value) {
        switch (c) {
        case '"':
            os << "&quot;";
            break;
        case '\'':
            os << "&apos;";
            break;
        case '<':
            os << "&lt;";
            break;
        case '>':
            os << "&gt;";
            break;
        case '&':
            os << "&amp;";
            break;
        default:
            os << c;
        }
    }
    os << "\"";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Text& text) {
    for (auto c : text.data) {
        switch (c) {
        case '"':
            os << "&quot;";
            break;
        case '\'':
            os << "&apos;";
            break;
        case '<':
            os << "&lt;";
            break;
        case '>':
            os << "&gt;";
            break;
        case '&':
            os << "&amp;";
            break;
        default:
            os << c;
        }
    }
    os << ' ';
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Decl& decl) {
    os << "<?" << decl.tag;
    for (size_t i = 0; i < decl.attrs.size(); i++) {
        if (i == 0)
            os << " ";

        os << decl.attrs[i];

        if (i < decl.attrs.size() - 1)
            os << " ";
    }
    os << "?> ";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Comment& comment) {
    os << "<!-- " << comment.text << " -->";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const DocType& dtd) {
    os << "<!DOCTYPE " << dtd.text << ">";
    return os;
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

std::ostream& xtree::operator<<(std::ostream& os, const RootNode& node) {
    if (auto text = std::get_if<Decl>(&node.data)) {
        os << *text;
    }
    else if (auto decl = std::get_if<Comment>(&node.data)) {
        os << *decl;
    }
    else if (auto dtd = std::get_if<DocType>(&node.data)) {
        os << *dtd;
    }
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Elem& elem) {
    os << "<" << elem.tag;
    for (size_t i = 0; i < elem.attributes.size(); i++) {
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
