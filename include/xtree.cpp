// Joseph Prichard
// 4/27/2024
// An implementation for a xml document parser.

#include <stack>
#include <sstream>
#include <fstream>
#include <cassert>
#include <optional>
#include "xtree.hpp"

using namespace xtree;

typedef int i32;

struct BaseReader {
    int row = 1;
    int col = 1;

    // get a character from the stream, and advance the row and column counters
    // used without the return value to consume a single character in the stream
    i32 read_char() {
        i32 c = get_char();
        if (c == EOF)
            return EOF;

        if (c == '\n') {
            col = 1;
            row += 1;
        }
        else {
            col += 1;
        }
        return c;
    }

    ParseException parse_error(const std::string& message, ParseError code) const {
        std::string error_message = message + " at row: " + std::to_string(row) + ", col: " + std::to_string(col);
        return ParseException(error_message, code);
    }

    // consumes ahead len characters in the stream by "read_char"-ing each one
    // intended to be used after peeking multiple characters ahead
    void consume(size_t len) {
        for (size_t i = 0; i < len; i++)
            read_char();
    }

    // get a character from the stream without advancing row and column counters
    virtual i32 get_char() { return 0; }

    // peek at the "current" character in the stream WITHOUT consuming
    virtual i32 peek_char() { return 0; }

    // peek ahead index characters at the "current" character in the stream WITHOUT consuming
    // if we call peek_ahead(2) and we have not peeked ahead 0 or 1 yet, they will be automatically "got" from the stream
    virtual i32 peek_ahead(int index) { return 0; }

    // perform a peek ahead match against a target string - consuming ahead ONLY if the peek ahead matches the target string
    // skip can be used to start the peek match skip characters ahead - for example if we have already checked that they match
    // skip exists for the sake of efficiency only
    virtual bool read_match(const std::string& str, int skip) { return false; }
};

struct StringReader : BaseReader {
    const char* data;
    const size_t size;
    size_t position = 0;

    StringReader(const char* data, size_t size) : data(data), size(size) {}

    i32 get_char() override {
        if (position >= size) {
            return EOF;
        }
        return data[position++];
    }

    i32 peek_char() override {
        if (position >= size) {
            return EOF;
        }
        return data[position];
    }

    i32 peek_ahead(int index) override {
        if (position + index >= size) {
            return EOF;
        }
        return data[position + index];
    }

    bool read_match(const std::string& str, int skip) override {
        for (size_t i = 0; i < str.size(); i++) {
            auto index = position + i + skip;
            if (index >= size) {
                return false;
            }
            char c = data[index];
            if (c != str[i]) {
                return false;
            }
        }

        consume(skip + str.size());
        return true;
    }
};

struct StreamReader : BaseReader {
    static constexpr int LB_SIZ = 12; // maximum number of characters we can look ahead

    std::ifstream& file;
    i32 lbuf[LB_SIZ] = {0}; // lookahead ring buffer
    int head = 0;
    int tail = 0;
    int size = 0;

    explicit StreamReader(std::ifstream& file) : file(file) {}

    void push_lbuf(int c) {
        assert(size != LB_SIZ);

        lbuf[tail] = c;
        tail = (tail + 1) % LB_SIZ;
        size++;
    }

    i32 scan_lbuf(int index) {
        return lbuf[(head + index) % LB_SIZ];
    }

    i32 pop_lbuf() {
        i32 c = lbuf[head];
        head = (head + 1) % LB_SIZ;
        size--;
        return c;
    }

    int size_lbuf() const {
        return size;
    }

    i32 get_char() override {
        int c;
        if (size_lbuf() == 0)
            c = file.get();
        else
            c = pop_lbuf();
        return c;
    }

    i32 peek_char() override {
        return peek_ahead(0);
    }

    i32 peek_ahead(int index) override {
        int buf_size = size_lbuf();

        for (int i = 0; i < index - buf_size + 1; i++) {
            int c = file.get();
            if (c == EOF) {
                return EOF;
            }
            push_lbuf(c);
        }

        int c = scan_lbuf(index);
        return c;
    }

    bool read_match(const std::string& str, int skip) override {
        int str_size = static_cast<int>(str.size());
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
            int c = scan_lbuf(i + skip);
            if (c == EOF) {
                return false;
            }
            if (c != str[i]) {
                return false;
            }
        }

        consume(skip + str.size());
        return true;
    }
};

template <typename Reader>
struct Parser {
    Reader& reader;

    explicit Parser(Reader& reader) : reader(reader) {
        const auto req = std::is_base_of<BaseReader, Reader>::value;
        static_assert(req, "must be derived of reader");
    }

    i32 peek_char() {
        return reader.peek_char();
    }

    i32 peek_ahead(int index) {
        return reader.peek_ahead(index);
    }

    i32 read_char() {
        return reader.read_char();
    }

    bool read_match(const std::string& str, int skip) {
        return reader.read_match(str, skip);
    }

    bool read_match(const std::string& str) {
        return read_match(str, 0);
    }

    void consume(int len) {
        return reader.consume(len);
    }

    void skip_spaces() {
        while (true) {
            auto c = peek_char();
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
        return reader.parse_error(message, code);
    }

    static std::string char_string(i32 c) {
        return {static_cast<char>(c), 1};
    }

    static void append_symbol(std::string& str, i32 c) {
        str += static_cast<char>(c);
    }

    // reads into the buffer argument to avoid an additional allocation and erases after finished
    void read_escseq(std::string& str) {
        int i = 0;
        while (true) {
            int c = read_char();
            if (c == EOF) {
                throw parse_error("reached the end of the stream while parsing escseq", ParseError::EndOfStream);
            }
            append_symbol(str, c);

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
        int size = static_cast<int>(str.size());
        int i = size - 1;
        while (i >= 0) {
            if (!std::isspace(str[i])) {
                break;
            }
            i -= 1;
        }
        if (i < size) {
            str.erase(i + 1);
        }
    }

    Text read_rawtext() {
        skip_spaces();

        std::string str;
        while (true) {
            i32 c = peek_char();
            if (c == EOF) {
                throw parse_error("reached the end of the stream while parsing raw data", ParseError::EndOfStream);
            }

            if (c == '&') {
                read_escseq(str);
            }
            else if (c == '<') {
                if (read_match("<![CDATA[")) {
                    read_cdata(str);
                }
                else {
                    break;
                }
            }
            else {
                append_symbol(str, c);
                read_char();
            }
        }

        trim_spaces(str);
        // str.shrink_to_fit();
        return Text(std::move(str));
    }

    void read_cdata(std::string& str) {
        while (true) {
            i32 c = read_char();
            if (c == EOF) {
                throw parse_error("reached the end of the stream while parsing cdata", ParseError::EndOfStream);
            }

            // check for a closing cdata tag
            if (c == ']') {
                i32 c1 = read_char();
                if (c1 == EOF) {
                    throw parse_error("reached the end of the stream while parsing cdata close tag", ParseError::EndOfStream);
                }

                i32 c2 = read_char();
                if (c2 == EOF) {
                    throw parse_error("reached the end of the stream while parsing cdata close tag", ParseError::EndOfStream);
                }

                if (c1 == ']' && c2 == '>')
                    break;

                append_symbol(str, c1);
                append_symbol(str, c2);
            }
            append_symbol(str, c);
        }
        // str.shrink_to_fit();
    }

    // non unicode compliant character detection
    bool static is_name_start(i32 c) {
        return isalpha(static_cast<char>(c)) || c == ':' || c == '_';
    }

    bool static is_name(i32 c) {
        return is_name_start(c) || isdigit(static_cast<char>(c)) || c == '-' || c == '.';
    }

    void read_tagname(std::string& str) {
        int i = 0;
        while (true) {
            i32 c = peek_char();
            if (c == EOF) {
                break;
            }

            if (c == ' ' || c == '>' || c == '?' || c == '/') {
                break;
            }
            if ((i == 0 && is_name_start(c)) || (i > 0 && is_name(c))) {
                append_symbol(str, c);
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
        auto open_symbol = read_char();
        if (open_symbol != '"' && open_symbol != '\'') {
            throw parse_error("attr val must begin with single or double quotes, found: " + char_string(open_symbol), ParseError::AttrValBegin);
        }
        auto close_symbol = open_symbol;

        while (true) {
            i32 c = peek_char();
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
                append_symbol(str, c);
            }
        }
        // str.shrink_to_fit();
    }

    void read_attrname(std::string& str) {
        while (true) {
            i32 c = peek_char();
            if (c == EOF) {
                break;
            }
            if (!is_name(c)) {
                break;
            }
            append_symbol(str, c);
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

        i32 c = peek_char();
        if (c == EOF) {
            return eof_tok;
        }

        if (c == '<') {
            i32 c1 = peek_ahead(1);
            if (c1 == EOF) {
                return open_beg;
            }

            switch (c1) {
            case '/':
                consume(2);
                return open_end;
            case '?':
                consume(2);
                return open_decl;
            case '!': {
                if (read_match("--", 2)) {
                    return open_cmt;
                }
                if (read_match("DOCTYPE", 2)) {
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

        i32 c = peek_char();
        if (c == EOF) {
            return eof_tok;
        }

        switch (c) {
        case '/': {
            i32 c1 = peek_ahead(1);
            if (c1 == EOF) {
                return text_tok;
            }
            if (c1 == '>') {
                consume(2);
                return close_beg;
            }
            return text_tok;
        }
        case '?': {
            i32 c1 = peek_ahead(1);
            if (c1 == EOF) {
                return text_tok;
            }
            if (c1 == '>') {
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
                throw parse_error("reached end of stream while parsing attrs", ParseError::EndOfStream);
            case close_end:
            case close_beg:
            case close_decl:
                // attrs.shrink_to_fit();
                // no more attrs in the attr list to parse
                return tok;
            case text_tok:
                read_attrname(attr.name);
                break;
            default:
                // this branch should never be called
                auto m = "expected an attribute name, <close-tag>, or <close-decl> symbols, got " + std::to_string(tok);
                throw parse_error(m, ParseError::InvalidAttrList);
            }

            i32 c = read_char();
            if (c == EOF) {
                throw parse_error("reached end of stream while parsing attrs", ParseError::EndOfStream);
            }
            if (c != '=') {
                throw parse_error("expected an <equals> symbol between attribute pairs, got " + char_string(c), ParseError::InvalidAttrList);
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
        auto close_tok = parse_attrs(root.attrs);

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

            token tok = read_open_tok();
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

                token tok1 = read_close_tok();
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
                top->add_node(std::move(cmnt));
                break;
            }
            case open_beg: {
                // Read the next element to be processed by the parser
                auto elem = std::make_unique<Elem>(Elem());
                auto elem_ptr = elem.get();

                read_tagname(elem_ptr->tag);
                close_tok = parse_attrs(elem_ptr->attrs);

                if (close_tok == close_end) {
                    // This will be the next elem we parse
                    stack.push(elem_ptr);
                }
                else if (close_tok != close_beg) { // close_beg means the elem has no children
                    throw parse_error("unclosed attrs list in tag", ParseError::InvalidAttrList);
                }

                top->add_node(std::move(elem));
                break;
            }
            case text_tok: {
                auto text = read_rawtext();
                top->add_node(std::move(text));
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
        Cmnt cmnt;

        while (true) {
            // check if there are more chars to read
            i32 c = peek_char();
            if (c == EOF) {
                throw parse_error("reached end of stream while parsing comment", ParseError::EndOfStream);
            }
            // check for a closing comment tag
            if (read_match("-->")) {
                trim_spaces(cmnt.data);
                return cmnt;
            }

            append_symbol(cmnt.data, c);
            read_char();
        }
    }

    Decl parse_decl() {
        Decl decl;
        read_tagname(decl.tag);

        token tok = parse_attrs(decl.attrs);
        if (tok != close_decl) {
            throw parse_error("expected <close-decl> symbol to close a decl, got " + std::to_string(tok), ParseError::InvalidCloseDecl);
        }

        return decl;
    }

    Dtd parse_dtd() {
        skip_spaces();
        Dtd dtd;

        while (true) {
            i32 c = read_char();
            if (c == EOF) {
                throw parse_error("reached end of stream while parsing a doctype", ParseError::EndOfStream);
            }

            if (c == '>') {
                trim_spaces(dtd.data);
                return dtd;
            }
            append_symbol(dtd.data, c);
        }
    }

    void parse(Document& document) {
        bool parsed_root = false;
        bool parsed_meta = false;

        while (true) {
            token tok = read_open_tok();
            switch (tok) {
            case eof_tok:
                return;
            case open_dtd: {
                auto dtd = parse_dtd();
                document.children.emplace_back(std::move(dtd));
                break;
            }
            case open_decl: {
                auto decl = parse_decl();

                if (decl.tag == "xml") {
                    if (parsed_meta)
                        throw parse_error("document may only have a single xml meta decl tag", ParseError::InvalidXmlMeta);

                    auto vattr = decl.select_attr("version");
                    if (vattr == nullptr)
                        throw parse_error("expected xml meta tag to have a version field", ParseError::InvalidXmlMeta);
                    if (vattr->value != "1.0")
                        throw parse_error("only supports parsing documents with version 1.0, got " + vattr->value, ParseError::InvalidXmlMeta);

                    auto eattr = decl.select_attr("encoding");
                    if (eattr == nullptr)
                        throw parse_error("expected xml meta tag to have an encoding field", ParseError::InvalidXmlMeta);
                    if (eattr->value != "UTF-8")
                        throw parse_error("only supports UTF-8 encodings, got " + eattr->value, ParseError::InvalidXmlMeta);
                }

                document.children.emplace_back(std::move(decl));
                break;
            }
            case open_cmt: {
                auto cmnt = parse_cmnt();
                document.children.emplace_back(std::move(cmnt));
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
                auto m = "expected data or a <open-tag>, <open-dtd>, <open-comment> or <open-decl> symbol, got " + std::to_string(tok);
                throw parse_error(m, ParseError::InvalidRootOpenTok);
            }
        }
    }
};

Document Document::from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.good())
        throw std::runtime_error("could not open file " + path);

    return from_file(file);
}

Document Document::from_file(std::ifstream& file) {
    Document document;

    StreamReader reader(file);
    Parser<StreamReader> parser(reader);
    parser.parse(document);

    return document;
}

Document Document::from_buffer(const char* buffer, size_t size) {
    Document document;

    StringReader reader(buffer, size);
    Parser<StringReader> parser(reader);
    parser.parse(document);

    return document;
}

Document Document::from_string(const std::string& str) {
    Document document;

    StringReader reader(str.data(), str.size());
    Parser<StringReader> parser(reader);
    parser.parse(document);

    return document;
}

std::string Document::serialize() const {
    std::ostringstream ss;
    ss << (*this);
    return ss.str();
}

std::string Elem::serialize() const {
    std::ostringstream ss;
    ss << (*this);
    return ss.str();
}

std::string Node::serialize() const {
    std::ostringstream ss;
    ss << (*this);
    return ss.str();
}

Elem* Elem::select_elem(const std::string& ctag) {
    for (auto& child: children)
        if (auto elem = get_if<std::unique_ptr<Elem>>(&child.data))
            if ((*elem)->tag == ctag)
                return elem->get();
    return nullptr;
}

Attr* Elem::select_attr(const std::string& attr_name) {
    for (auto& attr: attrs)
        if (attr.name == attr_name)
            return &attr;
    return nullptr;
}

Elem& Elem::expect_elem(const std::string& ctag) {
    for (auto& child: children)
        if (auto elem = get_if<std::unique_ptr<Elem>>(&child.data))
            if ((*elem)->tag == ctag)
                return **elem;
    throw NodeWalkException("elem does not contain child with tag name " + ctag);
}

Attr& Elem::expect_attr(const std::string& attr_name) {
    for (auto& attr: attrs)
        if (attr.name == attr_name)
            return attr;
    throw NodeWalkException("elem does not contain attribute with name " + attr_name);
}

std::optional<Elem> Elem::remove_elem(const std::string& rtag) {
    auto it = children.begin();
    while (it != children.end()) {
        auto& node = *it;
        if (auto elem_ptr = std::get_if<std::unique_ptr<Elem>>(&node.data)) {
            auto& elem = *elem_ptr;
            if (elem->tag == rtag) {
                auto ret = std::move(elem);
                children.erase(it);
                return std::move(*ret);
            }
        }
        it++;
    }
    return std::nullopt;
}

std::optional<Attr> Elem::remove_attr(const std::string& name) {
    auto it = attrs.begin();
    while (it != attrs.end()) {
        auto& attr = *it;
        if (attr.name == name) {
            auto ret = std::move(attr);
            attrs.erase(it);
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
        if (auto decl = std::get_if<Decl>(&node.data)) {
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
    size_t back = 0;
    for (size_t i = 0; i < children.size(); i++) {
        auto& node = children[i];
        size_t next_back = back;
        if (node.is_elem() && node.as_elem().tag == rtag)
            next_back++;
        if (back != 0)
            children[i - back] = std::move(node);
        back = next_back;
    }
    children.erase(children.end() - static_cast<long long>(back), children.end());
    children.shrink_to_fit();
}

void Elem::remove_attrs(const std::string& name) {
    size_t back = 0;
    for (size_t i = 0; i < attrs.size(); i++) {
        auto& attr = attrs[i];
        size_t next_back = back;
        if (attr.name == name)
            next_back++;
        if (back != 0)
            attrs[i - back] = attr;
        back = next_back;
    }
    attrs.erase(attrs.end() - static_cast<long long>(back), attrs.end());
    attrs.shrink_to_fit();
}

void Document::remove_decls(const std::string& rtag) {
    size_t back = 0;
    for (size_t i = 0; i < children.size(); i++) {
        auto& node = children[i];
        size_t next_back = back;
        if (node.is_decl() && node.as_decl().tag == rtag)
            next_back++;
        if (back != 0)
            children[i - back] = std::move(node);
        back = next_back;
    }
    children.erase(children.end() - static_cast<long long>(back), children.end());
    children.shrink_to_fit();
}

size_t Elem::normalize() {
    std::stack<Elem*> stack;
    stack.push(this);

    int remove_count = 0;

    while (!stack.empty()) {
        Elem* top = stack.top();
        stack.pop();

        Text* prev_text = nullptr;
        auto& curr_children = top->children;

        auto it = curr_children.begin();
        while (it != curr_children.end()) {
            auto& child = *it;
            if (child.is_text()) {
                auto& child_text = child.as_text();
                if (prev_text != nullptr) {
                    prev_text->data += child_text.data;
                    it = curr_children.erase(it);
                    remove_count++;
                }
                else {
                    prev_text = &child_text;
                    it++;
                }
            }
            else {
                if (child.is_elem()) {
                    auto elem = &child.as_elem();
                    if (!elem->children.empty())
                        stack.push(elem);
                }
                prev_text = nullptr;
                it++;
            }
        }
    }

    return remove_count;
}

Docstats xtree::stat_document(Document& document) {
    Docstats stats{0, 0};

    if (document.root != nullptr) {
        stats.nodes_count++;

        stats.total_mem += sizeof(Elem); // size of the elem, which is NOT stored in line with the node, and is therefore NOT counted in sizeof(Node)

        stats.total_mem += document.root->attrs.capacity() * sizeof(Attr); // size of the entire attr vector
        for (auto& attr : document.root->attrs) {
            stats.total_mem += attr.name.capacity() + attr.value.capacity(); // strlen of the strings in the attr
        }
    }

    walk_document(document,
        [&stats](Node& node) {
            stats.nodes_count++;
            stats.total_mem += sizeof(Node); // size of the node itself (the largest of all variants which in this case is tie between cmnt and text)

            if (node.is_elem()) {
                auto& elem = node.as_elem();

                stats.total_mem += sizeof(Elem); // size of the elem, which is NOT stored in line with the node, and is therefore NOT counted in sizeof(Node)

                stats.total_mem += elem.attrs.capacity() * sizeof(Attr); // size of the entire attr vector
                for (auto& attr : elem.attrs) {
                    stats.total_mem += attr.name.capacity() + attr.value.capacity(); // strlen of the strings in the attr
                }
            }
            else if (node.is_cmnt()) {
                auto& cmnt = node.as_cmnt();
                stats.total_mem += cmnt.data.capacity(); // strlen of the string
            }
            else if (node.is_text()) {
                auto& text = node.as_text();
                stats.total_mem += text.data.capacity(); // strlen of the string
            } else {
                throw std::runtime_error("unknown variant case");
            }
        },
        [&stats](BaseNode& node) {
            stats.nodes_count++;
            stats.total_mem += sizeof(BaseNode); // size of the node itself (the largest of all variants which in this case is the decl)

            if (node.is_decl()) {
                auto& decl = node.as_decl();
                stats.total_mem += decl.tag.capacity(); // strlen of the string

                stats.total_mem += decl.attrs.capacity() * sizeof(Attr); // size of the entire attr vector
                for (auto& attr : decl.attrs) {
                    stats.total_mem += attr.name.capacity() + attr.value.capacity(); // strlen of the strings in the attr
                }
            }
            else if (node.is_cmnt()) {
                auto& cmnt = node.as_cmnt();
                stats.total_mem += cmnt.data.capacity(); // strlen of the string
            }
            else if (node.is_dtd()) {
                auto& dtd = node.as_dtd();
                stats.total_mem += dtd.data.capacity(); // strlen of the string
            }
            else {
                throw std::runtime_error("unknown variant case");
            }
        });

    return stats;
}

// a stack frame for the copy element function to avoid a recursive-loop
// exists as a top level declaration so multiple elements can be copied reusing the same stack using the internal copy_elem func
struct CopyElemSf {
    const Elem* other_ptr;
    size_t other_i;
    Elem* copy_ptr;
};

// an internal "overload" of the Elem::from_other function that allows us to reuse the same stack when we need to copy a lot of elements at a time
Elem copy_elem(const Elem& other, std::stack<CopyElemSf>& stack) noexcept {
    Elem elem(other.tag, other.attrs);

    stack.emplace(&other, 0, &elem);

    while (!stack.empty()) {
        CopyElemSf& top = stack.top();

        auto& copy = top.copy_ptr;
        auto curr = top.other_ptr;

        if (top.other_i < curr->children.size()) {
            auto& other_child = curr->children[top.other_i++];

            if (other_child.is_elem()) {
                auto other_elem_child = &other_child.as_elem();

                // create a copy of the "other" other_child, and push it (as a node) to the copy's children
                auto copy_elem = std::make_unique<Elem>(other_elem_child->tag, other_elem_child->attrs);
                auto copy_elem_ptr = copy_elem.get();
                copy->children.emplace_back(std::move(copy_elem));

                // push the newly created element node to the stack... we need to copy all of its children before it can be truly considered a "finished" copy
                // imagine that this is a function call - and we're passing these as arguments to a recursive copy call
                stack.emplace(other_elem_child, 0, copy_elem_ptr);
            }
            else if (other_child.is_text()) {
                copy->children.emplace_back(other_child.as_text());
            }
            else if (other_child.is_cmnt()) {
                copy->children.emplace_back(other_child.as_cmnt());
            }
            else {
                throw std::runtime_error("unknown variant case");
            }
        } else {
            // we're done copying this node, pop the stack aka "return" from the copy call stack frame
            stack.pop();
        }
    }

    return elem;
}

// an internal "overload" of the Node::from_other function that allows us to reuse a stack in the case the node is an elem
Node copy_node(const Node& other, std::stack<CopyElemSf>& stack) {
    if (auto elem_ptr = std::get_if<std::unique_ptr<Elem>>(&other.data))
        return Node(std::make_unique<Elem>(copy_elem(**elem_ptr, stack)));
    else if (auto cmnt = std::get_if<Cmnt>(&other.data))
        return Node(*cmnt);
    else if (auto text = std::get_if<Text>(&other.data))
        return Node(*text);
    else
        throw std::runtime_error("unknown variant case");
}

Elem Elem::from_other(const Elem& other) noexcept {
    std::stack<CopyElemSf> stack;
    return copy_elem(other, stack);
}

Node Node::from_other(const Node& other) {
    if (auto elem_ptr = std::get_if<std::unique_ptr<Elem>>(&other.data))
        return Node(std::make_unique<Elem>(Elem::from_other(**elem_ptr)));
    else if (auto cmnt = std::get_if<Cmnt>(&other.data))
        return Node(*cmnt);
    else if (auto text = std::get_if<Text>(&other.data))
        return Node(*text);
    else
        throw std::runtime_error("unknown variant case");
}

Document Document::from_other(const Document& other) {
    Document document;
    document.children = other.children;

    if (other.root != nullptr)
        document.root = std::make_unique<Elem>(Elem::from_other(*other.root));

    return document;
}

Document& Document::operator=(const Document& other) {
    if (this == &other)
        return *this;

    children = other.children;

    if (other.root != nullptr)
        root = std::make_unique<Elem>(Elem::from_other(*other.root));

    return *this;
}

Elem& Elem::operator=(const Elem& other) {
    if (this == &other)
        return *this;

    tag = other.tag;
    attrs = other.attrs;

    // we must store the copied nodes somewhere before we clear and append since other.child_nodes might be a child of child_nodes
    std::vector<Node> temp_nodes;
    temp_nodes.reserve(other.children.size());

    std::stack<CopyElemSf> stack;

    for (const Node& node: other.children) {
        // we use the copy_node function when copying these nodes so that we can reuse the stack in the case we have multiple elements to copy.
        temp_nodes.push_back(copy_node(node, stack));
    }

    children.clear();
    for (auto& node: temp_nodes) {
        children.push_back(std::move(node));
    }

    return *this;
}

Node& Node::operator=(Node&& other) noexcept {
    if (this == &other)
        return *this;

    if (auto elem_ptr = std::get_if<std::unique_ptr<Elem>>(&other.data))
        data = std::move(*elem_ptr);
    else if (auto text = std::get_if<Text>(&other.data))
        data = std::move(*text);
    else if (auto cmnt = std::get_if<Cmnt>(&other.data))
        data = std::move(*cmnt);
    else
        throw std::runtime_error("unknown variant case");
    return *this;
}

bool xtree::operator==(const Node& node, const Node& other) {
    if (node.is_elem() && other.is_elem()) // an elem node uses a unique_ptr, so we need a reference to directly check the equality
        return node.as_elem() == other.as_elem();

    return node.data == other.data;
}

bool xtree::operator==(const Elem& elem, const Elem& other)  {
    // base case: just check if the elem fields themselves are equal - if not we don't even need to check the children
    if (elem.tag != other.tag || elem.attrs != other.attrs)
        return false;
    if (elem.children.size() != other.children.size())
        return false;

    struct Sf {
        const Elem* lhs_ptr;
        const Elem* rhs_ptr;
        size_t i;
    };

    std::stack<Sf> stack;
    stack.emplace(&elem, &other, 0);

    while (!stack.empty()) {
        Sf& top = stack.top();
        auto lhs = top.lhs_ptr;
        auto rhs = top.rhs_ptr;

        if (top.i == 0) {
            // first check if the elem fields of this stack frame are equal - if not we can just stop early
            if (lhs->tag != rhs->tag || lhs->attrs != rhs->attrs)
                return false;
            if (lhs->children.size() != rhs->children.size())
                return false;
        }

        if (top.i < lhs->children.size()) { // invariant: rhs->children.size() == lhs->children.size() is true
            auto& lhs_child = lhs->children[top.i];
            auto& rhs_child = rhs->children[top.i];
            top.i++;

            if (lhs_child.is_elem() && rhs_child.is_elem()) { // we need a special case to handle if the nodes are BOTH elems
                auto elem_lhs_child = &lhs_child.as_elem();
                auto elem_rhs_child = &rhs_child.as_elem();
                stack.emplace(elem_lhs_child, elem_rhs_child, 0); // we'll handle this child in the next stack frame...
            }
            else if (lhs_child.data != rhs_child.data ) { // otherwise just check if the variant itself - we don't care if they are, and we stop early if they're not
                return false;
            }
        } else {
            stack.pop();
        }
    }

    return true;
}

bool xtree::operator==(const Document& document, const Document& other) {
    if (document.root != nullptr && other.root != nullptr) {
        if (*document.root != *other.root)
            return false;
    }
    else if (document.root != nullptr || other.root != nullptr) {
        return false;
    }

    if (document.children.size() != other.children.size())
        return false;

    for (size_t i = 0; i < document.children.size(); i++)
        if (document.children[i] != other.children[i])
            return false;

    return true;
}

std::ostream& xtree::operator<<(std::ostream& os, const Attr& attr) {
    os << attr.name << "=" << "\"";
    for (auto c: attr.value) {
        switch (c) {
        case '"': os << "&quot;"; break;
        case '\'': os << "&apos;"; break;
        case '<': os << "&lt;"; break;
        case '>': os << "&gt;"; break;
        case '&': os << "&amp;"; break;
        default: os << c;
        }
    }
    os << "\"";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Text& text) {
    for (auto c: text.data) {
        switch (c) {
        case '"': os << "&quot;"; break;
        case '\'': os << "&apos;"; break;
        case '<': os << "&lt;"; break;
        case '>': os << "&gt;"; break;
        case '&': os << "&amp;"; break;
        default: os << c;
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
        os << node;
    if (document.root != nullptr)
        os << *document.root;
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Cmnt& cmnt) {
    os << "<!-- " << cmnt.data << " --> ";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Dtd& dtd) {
    os << "<!DOCTYPE " << dtd.data << "> ";
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Node& node) {
    if (auto elem_ptr = std::get_if<std::unique_ptr<Elem>>(&node.data))
        os << **elem_ptr;
    else if (auto text = std::get_if<Text>(&node.data))
        os << *text;
    else if (auto cmnt = std::get_if<Cmnt>(&node.data))
        os << *cmnt;
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const BaseNode& node) {
    if (auto text = std::get_if<Decl>(&node.data))
        os << *text;
    else if (auto decl = std::get_if<Cmnt>(&node.data))
        os << *decl;
    else if (auto dtd = std::get_if<Dtd>(&node.data))
        os << *dtd;
    return os;
}

std::ostream& xtree::operator<<(std::ostream& os, const Elem& elem) {
    struct Sf {
        const Elem* ptr;
        size_t i;
    };

    std::stack<Sf> stack;
    stack.emplace(&elem, 0);

    while (!stack.empty()) {
        Sf& top = stack.top();

        auto curr = top.ptr;
        if (top.i == 0) {
            os << "<" << curr->tag;
            for (size_t i = 0; i < curr->attrs.size(); i++) {
                if (i == 0)
                    os << " ";
                os << curr->attrs[i];
                if (i < curr->attrs.size() - 1)
                    os << " ";
            }
            os << "> ";
        }

        if (top.i < curr->children.size()) {
            auto& child = curr->children[top.i++];
            if (child.is_elem()) {
                auto elem_child = &child.as_elem();
                stack.emplace(elem_child, 0);
            }
            else if (child.is_text()) {
                os << child.as_text();
            }
            else if (child.is_cmnt()) {
                os << child.as_cmnt();
            }
            else {
                throw std::runtime_error("unknown variant case");
            }
        }
        else {
            os << "</" << curr->tag << "> ";
            stack.pop();
        }
    }

    return os;
}
