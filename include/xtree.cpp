// Joseph Prichard
// 4/27/2024
// Implementation for the xml document parser

#include <iostream>
#include <stack>
#include <sstream>
#include <fstream>
#include <cassert>
#include <optional>
#include "xtree.hpp"

#define DEBUG_XTREE_IMPL false

using namespace xtree;

struct Reader {
    virtual int pop_char() {};

    virtual int peek_char() {};

    virtual int peek_ahead(int index) {};

    virtual bool peek_match(int skip, const std::string& str) {};
};

struct StringReader : Reader {
    const std::string& data;
    int position = 0;

    explicit StringReader(const std::string& data) : data(data) {}

    int pop_char() override {
        if (position >= data.size()) {
            return EOF;
        }
        return data[position++];
    }

    int peek_char() override {
        if (position >= data.size()) {
            return EOF;
        }
        return data[position];
    }

    int peek_ahead(int index) override {
        if (position + index >= data.size()) {
            return EOF;
        }
        return data[position + index];
    }

    bool peek_match(int skip, const std::string& str) override {
        for (int i = 0; i < str.size(); i++) {
            auto index = position + i + skip;
            if (index >= data.size()) {
                return false;
            }
            char c = data[index];
            if (c != str[i]) {
                return false;
            }
        }
        return true;
    }
};

struct StreamReader : Reader {
    static constexpr int LB_SIZ = 12; // maximum number of characters we can look ahead

    std::istream& file;
    int lbuf[LB_SIZ] = {0}; // lookahead ring buffer
    int head = 0;
    int tail = 0;
    int size = 0;

    explicit StreamReader(std::istream& file) : file(file) {}

    void push_lbuf(int c) {
        assert(size != LB_SIZ);

        lbuf[tail] = c;
        tail = (tail + 1) % LB_SIZ;
        size++;
    }

    int scan_lbuf(int index) {
        return lbuf[(head + index) % LB_SIZ];
    }

    int pop_lbuf() {
        int c = lbuf[head];
        head = (head + 1) % LB_SIZ;
        size--;
        return c;
    }

    int size_lbuf() const {
        return size;
    }

    int pop_char() override {
        int c;
        if (size_lbuf() == 0)
            c = file.get();
        else
            c = pop_lbuf();
        return c;
    }

    int peek_char() override {
        return peek_ahead(0);
    }

    int peek_ahead(int index) override {
        int buf_size = size_lbuf();

        for (int i = 0; i < index - buf_size + 1; i++) {
            int c = file.get();
            if (c == EOF) {
                return EOF;
            }
            push_lbuf(c);
        }

        char c = scan_lbuf(index);
        return c;
    }

    bool peek_match(int skip, const std::string& str) override {
        int str_size = str.size();
        int buf_size = size_lbuf();

        // we will read maximally skip + str_size ahead in the scan loop, so ensure the lbuf is pre-filled
        for (int i = 0; i < skip + str_size - buf_size; i++) {
            int c = file.get();
            if (c == EOF) {
                return false;
            }
            push_lbuf(c);
        }

        for (int i = 0; i < str_size; i++) {
            char c = scan_lbuf(i + skip);
            if (c == EOF) {
                return false;
            }
            if (c != str[i]) {
                return false;
            }
        }

        return true;
    }
};

struct Parser {
    Reader& reader;
    int row = 1;
    int col = 1;

#if DEBUG_XTREE_IMPL
    std::string readlog;
#endif

    explicit Parser(Reader& reader) : reader(reader) {}

    int peek_char() {
        return reader.peek_char();
    }

    int peek_ahead(int index) {
        return reader.peek_ahead(index);
    }

    int read_char() {
        int c = reader.pop_char();
        if (c == EOF)
            return EOF;

#if DEBUG_XTREE_IMPL
        if (!isspace(c))
            printf("Get: %c\n", c);
#endif

        if (c == '\n') {
            col = 1;
            row += 1;
        }
        else {
            col += 1;
        }

#if DEBUG_XTREE_IMPL
        readlog += c;
#endif
        return c;
    }

    bool read_match(int skip, const std::string& str) {
        bool match = reader.peek_match(skip, str);
        if (match)
            for (int i = 0; i < skip + str.size(); i++)
                read_char();
        return match;
    }

    void consume(int len) {
        for (int i = 0; i < len; i++)
            read_char();
    }

    void skip_spaces() {
        while (true) {
            int c = peek_char();
            if (c == EOF) {
                return;
            }
            if (isspace(c)) {
                read_char();
            }
            else {
                break;
            }
        }
    }

    ParseException parse_error(const std::string& message, ParseError code) const {
        std::string error_message = message + " at row: " + std::to_string(col) + ", col: " + std::to_string(row);
        return ParseException(error_message, code);
    }

    static std::string char_string(int c) {
        return {static_cast<char>(c), 1};
    }

    // reads into the buffer argument to avoid an additional allocation and erases after finished
    void read_escseq(std::string& str) {
        int i = 0;
        while (true) {
            int c = read_char();
            if (c == EOF) {
                throw parse_error("reached the end of the stream while parsing escseq", ParseError::EndOfStream);
            }
            str += c;
            i++;
            if (c == ';') {
                break;
            }
        }
        std::string_view view(str.data() + str.size() - i, i);

        char escch;
        if (view == "&quot;")
            escch = '"';
        else if (view == "&apos;")
            escch = '\'';
        else if (view == "&lt;")
            escch = '<';
        else if (view == "&gt;")
            escch = '>';
        else if (view == "&amp;")
            escch = '&';
        else
            throw parse_error("encountered invalid esc sequence: '" + str + "'", ParseError::InvalidEscSeq);

        str.erase(str.size() - i, str.size());
        str += escch;
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

    Text read_rawtext() {
        skip_spaces();

        std::string str;
        while (true) {
            int c = peek_char();
            if (c == EOF) {
                throw parse_error("reached the end of the stream while parsing raw text", ParseError::EndOfStream);
            }

            if (c == '&') {
                read_escseq(str);
            }
            else if (c == '<') {
                if (read_match(0, "<![CDATA[")) {
                    read_cdata(str);
                }
                else {
                    break;
                }
            }
            else {
                str += c;
                read_char();
            }
        }

        trim_spaces(str);
        // str.shrink_to_fit();
        return Text(std::move(str));
    }

    void read_cdata(std::string& str) {
        while (true) {
            int c = read_char();
            if (c == EOF) {
                throw parse_error("reached the end of the stream while parsing cdata", ParseError::EndOfStream);
            }

            // check for a closing cdata tag
            if (c == ']') {
                int c1 = read_char();
                if (c1 == EOF) {
                    throw parse_error("reached the end of the stream while parsing cdata close tag", ParseError::EndOfStream);
                }

                int c2 = read_char();
                if (c2 == EOF) {
                    throw parse_error("reached the end of the stream while parsing cdata close tag", ParseError::EndOfStream);
                }

                if (c1 == ']' && c2 == '>')
                    break;

                str += c1;
                str += c2;
            }
            str += c;
        }
        // str.shrink_to_fit();
    }

    // non unicode compliant character detection
    bool static is_name_start(int c) {
        return isalpha(c) || c == ':' || c == '_';
    }

    bool static is_name(int c) {
        return is_name_start(c) || isdigit(c) || c == '-' || c == '.';
    }

    void read_tagname(std::string& str) {
        int i = 0;
        while (true) {
            int c = peek_char();
            if (c == EOF) {
                break;
            }

            if (c == ' ' || c == '>' || c == '?' || c == '/') {
                break;
            }
            if ((i == 0 && is_name_start(c)) || (i > 0 && is_name(c))) {
                str += c;
                read_char();
            }
            else {
                throw parse_error("invalid character in tag name: " + char_string(c), ParseError::InvalidTagname);
            }
            i += 1;
        }
        // str.shrink_to_fit();
    }

    void read_attrvalue(std::string& str) {
        int open_symbol = read_char();
        if (open_symbol != '"' && open_symbol != '\'') {
            throw parse_error("attr val must begin with single or double quotes, found: " + char_string(open_symbol), ParseError::AttrValBegin);
        }
        int close_symbol = open_symbol;

        while (true) {
            int c = peek_char();
            if (c == EOF) {
                break;
            }

            if (c == '&') {
                read_escseq(str);
            }
            else {
                read_char();
                if (c == close_symbol) {
                    break;
                }
                str += c;
            }
        }
        // str.shrink_to_fit();
    }

    void read_attrname(std::string& str) {
        while (true) {
            int c = peek_char();
            if (c == EOF) {
                break;
            }
            if (!is_name(c)) {
                break;
            }
            str += c;
            read_char();
        }
        // str.shrink_to_fit();
    }

    enum token {
        open_cmt = 0,
        open_decl = 1,
        close_decl = 2,
        open_beg = 3,
        close_beg = 4,
        open_end = 5,
        close_end = 6,
        open_dtd = 7,
        text_tok = 8,
        eof_tok = 9
    };

    token read_open_tok() {
        skip_spaces();

        int c = peek_char();
        if (c == EOF) {
            return eof_tok;
        }

        if (c == '<') {
            int c = peek_ahead(1);
            if (c == EOF) {
                return open_beg;
            }

            switch (c) {
            case '/':
                consume(2);
                return open_end;
            case '?':
                consume(2);
                return open_decl;
            case '!': {
                if (read_match(2, "--")) {
                    return open_cmt;
                }
                if (read_match(2, "DOCTYPE")) {
                    return open_dtd;
                }
                return text_tok;
            }
            default:
                read_char();
                return open_beg;
            }
        }
        else {
            return text_tok;
        }
    }

    token read_close_tok() {
        skip_spaces();

        int c = peek_char();
        if (c == EOF) {
            return eof_tok;
        }

        switch (c) {
        case '/': {
            int c = peek_ahead(1);
            if (c == EOF) {
                return text_tok;
            }
            if (c == '>') {
                consume(2);
                return close_beg;
            }
            return text_tok;
        }
        case '?': {
            int c = peek_ahead(1);
            if (c == EOF) {
                return text_tok;
            }
            if (c == '>') {
                consume(2);
                return close_decl;
            }
            return text_tok;
        }
        case '>':
            read_char();
            return close_end;
        case '<':
            throw parse_error("expected close token: <close-decl>, or <close-tag>, got: " + char_string(c), ParseError::InvalidCloseTok);
        default:
            return text_tok;
        }
    }

    token parse_attrs(std::vector<Attr>& attrs) {
        while (true) {
            Attr attr;

            auto tok = read_close_tok();
            switch (tok) {
            case eof_tok:
                throw parse_error("reached end of stream while parsing attributes", ParseError::EndOfStream);
            case close_end:
            case close_beg:
            case close_decl:
                // attrs.shrink_to_fit();
                // no more attributes in the attr list to parse
                return tok;
            case text_tok:
                read_attrname(attr.name);
                break;
            default:
                // this branch should never be called
                auto m = "expected an attribute name, <close-tag>, or <close-decl> symbols, got " + std::to_string(tok);
                throw parse_error(m, ParseError::InvalidAttrList);
            }

            int c = read_char();
            if (c == EOF) {
                throw parse_error("reached end of stream while parsing attributes", ParseError::EndOfStream);
            }
            if (c != '=') {
                throw parse_error("expected a <equals> symbol between attribute pairs, got " + char_string(c), ParseError::InvalidAttrList);
            }

            read_attrvalue(attr.value);
            attrs.push_back(std::move(attr));
        }
    }

    Elem parse_elem_tree() {
        std::stack<Elem*> stack;

        // Start by parsing the root elem node
        Elem root;

        read_tagname(root.tag);
        auto close_tok = parse_attrs(root.attributes);

        switch (close_tok) {
        case close_end:
            stack.push(&root);
            break;
        case close_beg:
            // The root has no child_nodes
            return root;
        default:
            throw parse_error("unclosed attrs list in tag", ParseError::UnclosedAttrsList);
        }

        std::string actual_tag;

        // Parse until the stack is empty using tokens to decide when to push/pop
        while (!stack.empty()) {
            Elem* top = stack.top();

            auto tok = read_open_tok();
            switch (tok) {
            case eof_tok:
                throw parse_error("reached end of stream while parsing element children", ParseError::EndOfStream);
            case open_end: {
                read_tagname(actual_tag);

                if (actual_tag != top->tag) {
                    std::string m("expected a closing tag to be '");
                    m += top->tag;
                    m += "' symbol, got '";
                    m += actual_tag;
                    m += "'";
                    throw parse_error(m, ParseError::CloseTagMismatch);
                }
                actual_tag.clear();

                auto tok1 = read_close_tok();
                if (tok1 == eof_tok) {
                    throw parse_error("reached end of stream while parsing an end tag", ParseError::EndOfStream);
                }
                if (tok1 != close_end) {
                    throw parse_error("expected a <close-tag> symbol, got " + std::to_string(tok), ParseError::InvalidCloseTok);
                }

                // Reaching the end of this node means we backtrack
                // top->children.shrink_to_fit();
                stack.pop();
                break;
            }
            case open_cmt: {
                auto cmnt = parse_cmnt();
                auto node = std::make_unique<Node>(std::move(cmnt));
                top->children.push_back(std::move(node));
                break;
            }
            case open_beg: {
                // Read the next element to be processed by the parser
                auto node = std::make_unique<Node>(Elem());
                auto elem_ptr = &node.get()->as_elem();

                read_tagname(elem_ptr->tag);
                close_tok = parse_attrs(elem_ptr->attributes);

                if (close_tok == close_end) {
                    // This will be the next node we parse
                    stack.push(elem_ptr);
                }
                else if (close_tok != close_beg) { // close_beg means the node has no children
                    throw parse_error("unclosed attrs list in tag", ParseError::InvalidAttrList);
                }

                top->children.push_back(std::move(node));
                break;
            }
            case text_tok: {
                auto text = read_rawtext();
                auto node = std::make_unique<Node>(std::move(text));
                top->children.push_back(std::move(node));
                break;
            }
            default:
                auto m = "expected tex, <open-tag> or <open-comment> symbol, got " + std::to_string(tok);
                throw parse_error(m, ParseError::InvalidOpenTok);
            }
        }

        return root;
    }

    Cmnt parse_cmnt() {
        skip_spaces();

        Cmnt comm;

        while (true) {
            // check if there are more chars to read
            int c = peek_char();
            if (c == EOF) {
                throw parse_error("reached end of stream while parsing comment", ParseError::EndOfStream);
            }
            // check for a closing comment tag
            if (read_match(0, "-->")) {
                trim_spaces(comm.text);
                return comm;
            }

            comm.text += c;
            read_char();
        }
    }

    Decl parse_decl() {
        Decl decl;

        read_tagname(decl.tag);

        auto tok = parse_attrs(decl.attrs);
        if (tok != close_decl) {
            throw parse_error("expected <close-decl> symbol to close a decl, got " + std::to_string(tok), ParseError::InvalidCloseDecl);
        }

        return decl;
    }

    Dtd parse_dtd() {
        Dtd dtd;

        skip_spaces();

        while (true) {
            int c = read_char();
            if (c == EOF) {
                throw parse_error("reached end of stream while parsing a doctype", ParseError::EndOfStream);
            }

            if (c == '>') {
                trim_spaces(dtd.text);
                return dtd;
            }
            dtd.text += c;
        }
    }

    void parse(Document& document) {
        bool parsed_root = false;

        while (true) {
            auto tok = read_open_tok();
            switch (tok) {
            case eof_tok:
                return;
            case open_dtd: {
                auto dtd = parse_dtd();
                auto node = std::make_unique<RootNode>(std::move(dtd));
                document.children.push_back(std::move(node));
                break;
            }
            case open_decl: {
                auto decl = parse_decl();
                auto node = std::make_unique<RootNode>(std::move(decl));
                document.children.push_back(std::move(node));
                break;
            }
            case open_cmt: {
                auto cmnt = parse_cmnt();
                auto node = std::make_unique<RootNode>(std::move(cmnt));
                document.children.push_back(std::move(node));
                break;
            }
            case open_beg: {
                if (parsed_root) {
                    throw parse_error("expected an xml document to only have a single root node", ParseError::MultipleRoots);
                }
                auto elem = parse_elem_tree();
                document.add_root(std::move(elem));
                parsed_root = true;
                break;
            }
            default:
                auto m = "expected text or a <open-tag>, <open-dtd>, <open-comment> or <open-decl> symbol, got " + std::to_string(tok);
                throw parse_error(m, ParseError::InvalidRootOpenTok);
            }
        }
    }
};

Document Document::from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.good())
        throw std::runtime_error("Could not open file " + path);

    Document document;

    StreamReader reader(file);
    Parser parser(reader);
    parser.parse(document);

    return document;
}

Document Document::from_string(const std::string& str) {
    Document document;

    StringReader reader(str);
    Parser parser(reader);
    parser.parse(document);

    return document;
}

std::string Document::serialize() const {
    std::ostringstream ss;
    ss << (*this);
    return ss.str();
}

Elem::Elem(std::string&& tag, std::vector<Node>&& children) noexcept : tag(std::move(tag)) {
    for (auto& child : children) {
        this->children.push_back(std::make_unique<Node>(child));
    }
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

Attr& Elem::expect_attr(const std::string& attr_name) {
    for (auto& attr: attributes)
        if (attr.name == attr_name)
            return attr;
    throw NodeWalkException("Element does not contain attribute with name " + attr_name);
}

std::optional<Elem> Elem::remove_elem(const std::string& rtag) {
    auto it = children.begin();
    while (it != children.end()) {
        auto& node = *it;
        if (auto elem = std::get_if<Elem>(&node->data)) {
            if (elem->tag == rtag) {
                auto ret = std::move(*elem);
                children.erase(it);
                return ret;
            }
        }
        it++;
    }
    return std::nullopt;
}

std::optional<Attr> Elem::remove_attr(const std::string& name) {
    auto it = attributes.begin();
    while (it != attributes.end()) {
        auto& attr = *it;
        if (attr.name == name) {
            auto ret = std::move(attr);
            attributes.erase(it);
            return ret;
        }
        it++;
    }
    return std::nullopt;
}

std::optional<Decl> Document::remove_decl(const std::string& rtag) {
    auto it = children.begin();
    while (it != children.end()) {
        auto& node = *it;
        if (auto decl = std::get_if<Decl>(&node->data)) {
            if (decl->tag == rtag) {
                auto ret = std::move(*decl);
                children.erase(it);
                return ret;
            }
        }
        it++;
    }
    return std::nullopt;
}

void Elem::remove_elems(const std::string& rtag) {
    int boff = 0;
    for (int i = 0; i < children.size(); i++) {
        auto& node = children[i];
        int next_boff = boff;
        if (node->is_elem() && node->as_elem().tag == rtag)
            next_boff++;
        if (boff != 0)
            children[i - boff] = std::move(node);
        boff = next_boff;
    }
    children.erase(children.end() - boff, children.end());
    children.shrink_to_fit();
}

void Elem::remove_attrs(const std::string& name) {
    int boff = 0;
    for (int i = 0; i < attributes.size(); i++) {
        auto& attr = attributes[i];
        int next_boff = boff;
        if (attr.name == name)
            next_boff++;
        if (boff != 0)
            attributes[i - boff] = attr;
        boff = next_boff;
    }
    attributes.erase(attributes.end() - boff, attributes.end());
    attributes.shrink_to_fit();
}

void Document::remove_decls(const std::string& rtag) {
    int boff = 0;
    for (int i = 0; i < children.size(); i++) {
        auto& node = children[i];
        int next_boff = boff;
        if (node->is_decl() && node->as_decl().tag == rtag)
            next_boff++;
        if (boff != 0)
            children[i - boff] = std::move(node);
        boff = next_boff;
    }
    children.erase(children.end() - boff, children.end());
    children.shrink_to_fit();
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

    for (auto& node: other.children) {
        temp_nodes.push_back(std::make_unique<Node>(*node));
    }

    children.clear();
    for (auto& node: temp_nodes) {
        children.push_back(std::move(node));
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
        else if (child->is_cmnt()) {
            on_cmnt(child->as_cmnt());
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
            else if (child->is_cmnt()) {
                on_cmnt(child->as_cmnt());
            }
        }
    }
}

void Elem::normalize() {
    std::stack<Elem*> stack;
    stack.push(this);

    while (!stack.empty()) {
        Elem* top = stack.top();
        stack.pop();

        Text* prev_text = nullptr;
        auto& curr_children = top->children;

        auto it = curr_children.begin();
        while (it != curr_children.end()) {
            auto& child = *it;
            if (child->is_text()) {
                auto& child_text = child->as_text();
                if (prev_text != nullptr) {
                    prev_text->data += child_text.data;
                    it = curr_children.erase(it);
                }
                else {
                    prev_text = &child_text;
                    it++;
                }
            }
            else {
                if (child->is_elem()) {
                    auto elem = &child->as_elem();
                    if (!elem->children.empty())
                        stack.push(elem);
                }
                prev_text = nullptr;
                it++;
            }
        }
    }
}

std::ostream& xtree::operator<<(std::ostream& os, const Attr& attr) {
    os << attr.name << "=" << "\"";
    for (auto c: attr.value) {
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
    for (auto c: text.data) {
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

std::ostream& xtree::operator<<(std::ostream& os, const Document& document) {
    for (auto& node: document.children)
        os << *node;
    if (document.root != nullptr)
        os << *document.root;
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Cmnt& cmnt) {
    os << "<!-- " << cmnt.text << " --> ";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Dtd& dtd) {
    os << "<!DOCTYPE " << dtd.text << "> ";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Node& node) {
    if (auto elem = std::get_if<Elem>(&node.data))
        os << *elem;
    else if (auto text = std::get_if<Text>(&node.data))
        os << *text;
    else if (auto cmnt = std::get_if<Cmnt>(&node.data))
        os << *cmnt;
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const RootNode& node) {
    if (auto text = std::get_if<Decl>(&node.data))
        os << *text;
    else if (auto decl = std::get_if<Cmnt>(&node.data))
        os << *decl;
    else if (auto dtd = std::get_if<Dtd>(&node.data))
        os << *dtd;
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Elem& elem) {
    struct StackFrame {
        const Elem* ptr;
        size_t i;
    };

    std::stack<StackFrame> stack;
    stack.emplace(&elem, 0);

    while (!stack.empty()) {
        StackFrame& top = stack.top();

        auto curr = top.ptr;
        if (top.i == 0) {
            os << "<" << curr->tag;
            for (size_t i = 0; i < curr->attributes.size(); i++) {
                if (i == 0)
                    os << " ";
                os << curr->attributes[i];
                if (i < curr->attributes.size() - 1)
                    os << " ";
            }
            os << "> ";
        }
        if (top.i < curr->children.size()) {
            auto child = curr->children[top.i++].get();
            if (child->is_elem()) {
                auto elem_child = &child->as_elem();
                stack.emplace(elem_child, 0);
            }
            else if (child->is_text()) {
                os << child->as_text();
            }
            else if (child->is_cmnt()) {
                os << child->as_cmnt();
            }
        }
        else {
            os << "</" << curr->tag << "> ";
            stack.pop();
        }
    }

    return os;
}
