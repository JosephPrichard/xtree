// Joseph Prichard
// 4/25/2024
// Xml parsing library for C++

#include <string>
#include <utility>
#include <vector>
#include <variant>
#include <memory>
#include <iosfwd>
#include <cstring>
#include <optional>

namespace xtree {

#define DEBUG_XTREE_HD false

struct Attr {
    std::string name;
    std::string value;

#if DEBUG_XTREE_HD
    Attr(Attr&&) = default;

    Attr(const Attr& other) : name(other.name), value(other.value) {
        printf("Copied attr: %s\n", name.c_str());
    }

    Attr() = default;

    Attr(std::string&& name, std::string&& value) : name(std::move(name)), value(std::move(value)) {}

    Attr& operator=(const Attr&) = default;
#endif

    friend bool operator==(const Attr& attr, const Attr& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Attr& attr);

struct Text {
    std::string data;

#if DEBUG_XTREE_HD
    Text(Text&&) = default;

    Text(const Text& other) : data(other.data) {
        printf("Copied text: %s\n", data.c_str());
    }

    Text() = default;

    explicit Text(std::string&& data) : data(std::move(data)) {}

    Text& operator=(const Text&) = default;
#endif

    friend bool operator==(const Text& lhs, const Text& rhs) = default;
};

std::ostream& operator<<(std::ostream& os, const Text& text);

struct Decl {
    std::string tag;
    std::vector<Attr> attrs;

#if DEBUG_XTREE_HD
    Decl(Decl&&) = default;

    Decl(const Decl& other) : tag(other.tag), attrs(other.attrs) {
        printf("Copied decl: %s\n", tag.c_str());
    }

    Decl() = default;

    explicit Decl(std::string&& tag) : tag(std::move(tag)) {}

    Decl(std::string&& tag, std::vector<Attr>&& attrs) : tag(std::move(tag)), attrs(std::move(attrs)) {}

    Decl& operator=(const Decl&) = default;
#endif

    Decl& add_attr(std::string&& name, std::string&& value) {
        attrs.emplace_back(std::move(name), std::move(value));
        return *this;
    }

    friend bool operator==(const Decl& decl, const Decl& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Decl& decl);

struct Cmnt {
    std::string text;

#if DEBUG_XTREE_HD
    Cmnt(Cmnt&&) = default;

    Cmnt(const Cmnt& other) : text(other.text) {
        printf("Copied comment: %s\n", text.c_str());
    }

    Cmnt() = default;

    explicit Cmnt(std::string&& text) : text(std::move(text)) {}

    Cmnt& operator=(const Cmnt&) = default;
#endif

    friend bool operator==(const Cmnt& cmnt, const Cmnt& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Cmnt& cmnt);

struct Dtd {
    std::string text;

#if DEBUG_XTREE_HD
    Dtd(Dtd&&) = default;

    Dtd(const Dtd& other) : text(other.text) {
        printf("Copied DTD: %s\n", text.c_str());
    }

    Dtd() = default;

    explicit Dtd(std::string&& text) : text(std::move(text)) {}

    Dtd& operator=(const Dtd&) = default;
#endif

    friend bool operator==(const Dtd& dtd, const Dtd& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Dtd& dtd);

struct Elem;

using NodeVariant = std::variant<Elem, Cmnt, Text>;

using RootVariant = std::variant<Cmnt, Decl, Dtd>;

struct Node;

bool operator==(const Node& node, const Node& other);

struct Elem {
    std::string tag;
    std::vector<Attr> attributes;
    std::vector<std::unique_ptr<Node>> children;

    Elem() = default;

    explicit Elem(std::string&& tag) noexcept: Elem(std::move(tag), {}, {}) {}

    Elem(std::string&& tag, std::vector<Attr>&& attributes) noexcept
        : Elem(Elem(std::move(tag), std::move(attributes), {})) {}

    Elem(std::string&& tag, std::vector<Attr>&& attributes, std::vector<std::unique_ptr<Node>>&& children) noexcept
        : tag(std::move(tag)), attributes(std::move(attributes)), children(std::move(children)) {}

    Elem(std::string&& tag, std::vector<Node>&& children) noexcept;

    Elem(const Elem& other) noexcept : tag(other.tag), attributes(other.attributes) {
#if DEBUG_XTREE_HD
        printf("Copied elem: %s\n", tag.c_str());
#endif
        children.clear();
        for (auto& child: other.children) {
            children.push_back(std::make_unique<Node>(*child)); // within recursive call-chain
        }
    }

    Elem(Elem&&) = default;

    Elem* select_elem(const std::string& ctag);

    Attr* select_attr(const std::string& attr_name);

    Elem& expect_elem(const std::string& ctag);

    Attr& expect_attr(const std::string& attr_name);

    void remove_attrs(const std::string& name);

    void remove_elems(const std::string& rtag);

    std::optional<Attr> remove_attr(const std::string& name);

    std::optional<Elem> remove_elem(const std::string& rtag);

    void normalize();

    std::vector<std::unique_ptr<Node>>::iterator begin() {
        return children.begin();
    }

    std::vector<std::unique_ptr<Node>>::iterator end() {
        return children.end();
    }

    Elem&& add_attr(std::string&& name, std::string&& value) && {
        attributes.emplace_back(std::move(name), std::move(value));
        return std::move(*this);
    }

    Elem& add_attr(std::string&& name, std::string&& value) & {
        attributes.emplace_back(std::move(name), std::move(value));
        return *this;
    }

    Elem&& add_node(NodeVariant&& node) && {
        children.push_back(std::make_unique<Node>(std::move(node)));
        return std::move(*this);
    }

    Elem& add_node(NodeVariant&& node) & {
        children.push_back(std::make_unique<Node>(std::move(node)));
        return *this;
    }

    friend bool operator==(const Elem& elem, const Elem& other) {
        if (elem.tag != other.tag || elem.attributes != other.attributes) {
            return false;
        }

        if (elem.children.size() != other.children.size()) {
            return false;
        }
        for (size_t i = 0; i < elem.children.size(); i++) {
            if (*elem.children[i] != *other.children[i])
                return false;
        }
        return true;
    }

    Elem& operator=(const Elem& other);
};

std::ostream& operator<<(std::ostream& os, const Elem& elem);

struct NodeWalkException : public std::runtime_error {
    explicit NodeWalkException(std::string&& m) : std::runtime_error(m) {}
};

struct Node {
    NodeVariant data;

    bool is_cmnt() const {
        return holds_alternative<Cmnt>(data);
    }

    bool is_text() const {
        return holds_alternative<Text>(data);
    }

    bool is_elem() const {
        return holds_alternative<Elem>(data);
    }

    Cmnt& as_cmnt() {
        if (auto node = get_if<Cmnt>(&data))
            return *node;
        throw NodeWalkException("node is not a comment type node");
    }

    Text& as_text() {
        if (auto node = get_if<Text>(&data))
            return *node;
        throw NodeWalkException("node is not a text type node");
    }

    Elem& as_elem() {
        if (auto elem = get_if<Elem>(&data))
            return *elem;
        throw NodeWalkException("node is not an elem type node");
    }

    Elem* find_element(const char* tag) {
        if (auto elem = get_if<Elem>(&data))
            return elem->select_elem(tag);
        return nullptr;
    }

    friend bool operator==(const Node& node, const Node& other) {
        return node.data == other.data;
    }
};

std::ostream& operator<<(std::ostream& os, const Node& node);

struct RootNode {
    RootVariant data;

    bool is_cmnt() const {
        return std::holds_alternative<Cmnt>(data);
    }

    bool is_decl() const {
        return std::holds_alternative<Decl>(data);
    }

    bool is_dtd() const {
        return std::holds_alternative<Dtd>(data);
    }

    Cmnt& as_cmnt() {
        if (auto node = std::get_if<Cmnt>(&data))
            return *node;
        throw NodeWalkException("node is not a comment type node");
    }

    Decl& as_decl() {
        if (auto node = std::get_if<Decl>(&data))
            return *node;
        throw NodeWalkException("node is not a decl type node");
    }

    Dtd& as_dtd() {
        if (auto node = std::get_if<Dtd>(&data))
            return *node;
        throw NodeWalkException("node is not a decl type node");
    }

    friend bool operator==(const RootNode& node, const RootNode& other) {
        return node.data == other.data;
    }
};

std::ostream& operator<<(std::ostream& os, const RootNode& node);

struct Document {
    std::vector<std::unique_ptr<RootNode>> children;
    std::unique_ptr<Elem> root;

    Document() = default;

    static Document from_file(const std::string&);

    static Document from_string(const std::string& str);

    Decl* select_decl(const std::string& tag) {
        for (auto& child: children)
            if (auto decl = get_if<Decl>(&child->data))
                if (decl->tag == tag)
                    return decl;
        return nullptr;
    }

    void remove_decls(const std::string& rtag);

    std::optional<Decl> remove_decl(const std::string& rtag);

    std::vector<std::unique_ptr<RootNode>>::iterator begin() {
        return children.begin();
    }

    std::vector<std::unique_ptr<RootNode>>::iterator end() {
        return children.end();
    }

    Document& add_node(RootVariant&& node) {
        children.push_back(std::make_unique<RootNode>(std::move(node)));
        return *this;
    }

    Elem& expect_root() const {
        if (root != nullptr) {
            return *root;
        }
        throw NodeWalkException("document does not contain a root element");
    }

    Document& add_root(Elem&& elem) {
        root = std::make_unique<Elem>(std::move(elem));
        return *this;
    }

    std::string serialize() const;

    friend bool operator==(const Document& document, const Document& other) {
        if (document.root != nullptr && other.root != nullptr) {
            if (*document.root != *other.root) {
                return false;
            }
        }
        else if (document.root != nullptr || other.root != nullptr) {
            return false;
        }
        if (document.children.size() != other.children.size()) {
            return false;
        }
        for (size_t i = 0; i < document.children.size(); i++) {
            if (*document.children[i] != *other.children[i])
                return false;
        }
        return true;
    }
};

std::ostream& operator<<(std::ostream& os, const Document& document);

struct DocumentWalker {
    virtual void on_elem(Elem& elem) {};

    virtual void on_cmnt(Cmnt& cmnt) {};

    virtual void on_dtd(Dtd& dtd) {};

    virtual void on_decl(Decl& decl) {};

    virtual void on_text(Text& text) {};

    void walk_document(Document& document);
};

enum class ParseError {
    EndOfStream,
    InvalidEscSeq,
    InvalidTagname,
    InvalidCloseTok,
    InvalidOpenTok,
    InvalidAttrList,
    InvalidCloseDecl,
    AttrValBegin,
    UnclosedAttrsList,
    CloseTagMismatch,
    MultipleRoots,
    InvalidRootOpenTok
};

struct ParseException : public std::runtime_error {
    ParseError code;

    explicit ParseException(std::string& m, ParseError code) : std::runtime_error(m), code(code) {}
};

}