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
#include <stack>
#include <functional>

namespace xtree {

#define DEBUG_XTREE_HD false

struct Attr {
    std::string name;
    std::string value;

#if DEBUG_XTREE_HD
//    Attr(Attr&&) = default;
//
//    Attr(const Attr& other) : name(other.name), value(other.value) {
//        printf("Copied attr: %s\n", name.c_str());
//    }
//
//    Attr() = default;
//
//    Attr(std::string name, std::string value) : name(std::move(name)), value(std::move(value)) {}
//
//    Attr& operator=(const Attr&) = default;
#endif

    friend bool operator==(const Attr& attr, const Attr& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Attr& attr);

struct Text {
    std::string data;

#if DEBUG_XTREE_HD
    Text() = default;

    Text(Text&&) = default;

    Text(const Text& other) : data(other.data) {
        printf("Copied data: %s\n", data.c_str());
    }

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
    Decl() = default;

    Decl(Decl&&) = default;

    Decl(const Decl& other) : tag(other.tag), attrs(other.attrs) {
        printf("Copied decl: %s\n", tag.c_str());
    }

    explicit Decl(std::string tag) : tag(std::move(tag)) {}

    Decl(std::string tag, std::vector<Attr> attrs) : tag(std::move(tag)), attrs(std::move(attrs)) {}

    Decl& operator=(const Decl&) = default;
#endif

    Decl& add_attr(std::string name, std::string value) {
        attrs.emplace_back(std::move(name), std::move(value));
        return *this;
    }

    friend bool operator==(const Decl& decl, const Decl& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Decl& decl);

struct Cmnt {
    std::string data;

#if DEBUG_XTREE_HD
    Cmnt() = default;

    Cmnt(Cmnt&&) = default;

    Cmnt(const Cmnt& other) : data(other.data) {
        printf("Copied comment: %s\n", data.c_str());
    }

    explicit Cmnt(std::string text) : data(std::move(text)) {}

    Cmnt& operator=(const Cmnt&) = default;
#endif

    friend bool operator==(const Cmnt& cmnt, const Cmnt& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Cmnt& cmnt);

struct Dtd {
    std::string data;

#if DEBUG_XTREE_HD
    Dtd() = default;

    Dtd(Dtd&&) = default;

    Dtd(const Dtd& other) : data(other.data) {
        printf("Copied DTD: %s\n", data.c_str());
    }

    explicit Dtd(std::string text) : data(std::move(text)) {}

    Dtd& operator=(const Dtd&) = default;
#endif

    friend bool operator==(const Dtd& dtd, const Dtd& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Dtd& dtd);

struct Elem;

struct NodeWalkException : public std::runtime_error {
    explicit NodeWalkException(std::string&& m) : std::runtime_error(m) {}
};

using NodeVariant = std::variant<std::unique_ptr<Elem>, Cmnt, Text>; // invariant: ElemPtr cannot point to a null

struct Node {
    NodeVariant data;

    explicit Node(NodeVariant data) : data(std::move(data)) {}

    Node(Node&&) = default;

    Node(const Node& other) {
        if (auto elem = std::get_if<std::unique_ptr<Elem>>(&other.data))
            data = std::make_unique<Elem>(**elem);
        else if (auto cmnt = std::get_if<Cmnt>(&other.data))
            data = *cmnt;
        else if (auto text = std::get_if<Text>(&other.data))
            data = *text;
    }

    bool is_cmnt() const {
        return holds_alternative<Cmnt>(data);
    }

    bool is_text() const {
        return holds_alternative<Text>(data);
    }

    bool is_elem() const {
        return holds_alternative<std::unique_ptr<Elem>>(data);
    }

    Cmnt& as_cmnt() {
        if (auto node = std::get_if<Cmnt>(&data))
            return *node;
        throw NodeWalkException("node is not a comment type node");
    }

    Text& as_text() {
        if (auto node = std::get_if<Text>(&data))
            return *node;
        throw NodeWalkException("node is not a data type node");
    }

    Elem& as_elem() {
        if (auto elem_ptr = std::get_if<std::unique_ptr<Elem>>(&data)) {
            auto& elem = *elem_ptr;
            if (elem == nullptr)
                throw NodeWalkException("elem type node is nullptr");
            return *elem;
        }
        throw NodeWalkException("node is not an elem type node");
    }

    const Cmnt& as_cmnt() const {
        if (auto node = std::get_if<Cmnt>(&data))
            return *node;
        throw NodeWalkException("node is not a comment type node");
    }

    const Text& as_text() const {
        if (auto node = std::get_if<Text>(&data))
            return *node;
        throw NodeWalkException("node is not a data type node");
    }

    const Elem& as_elem() const {
        if (auto elem_ptr = std::get_if<std::unique_ptr<Elem>>(&data)) {
            auto& elem = *elem_ptr;
            if (elem == nullptr)
                throw NodeWalkException("elem type node is nullptr");
            return *elem;
        }
        throw NodeWalkException("node is not an elem type node");
    }

    Node& operator=(Node&& other) noexcept;
};

bool operator==(const Node& node, const Node& other);

std::ostream& operator<<(std::ostream& os, const Node& node);

struct Elem {
    std::string tag;
    std::vector<Attr> attrs;
    std::vector<Node> children;

    Elem() = default;

    explicit Elem(std::string tag) noexcept: Elem(std::move(tag), {}, {}) {}

    Elem(std::string tag, std::vector<Attr> attributes) noexcept
        : Elem(Elem(std::move(tag), std::move(attributes), {})) {}

    Elem(std::string tag, std::vector<Attr> attributes, std::vector<Node> children) noexcept
        : tag(std::move(tag)), attrs(std::move(attributes)), children(std::move(children)) {}

    Elem(const Elem& other) noexcept : tag(other.tag), attrs(other.attrs) {
#if DEBUG_XTREE_HD
        printf("Copied elem: %s\n", tag.c_str());
#endif
        children.clear();
        for (auto& child: other.children) {
            children.emplace_back(child); // copy constructs the node in the container, potentially starting recursive copy-call-chain
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

    std::vector<Node>::iterator begin() {
        return children.begin();
    }

    std::vector<Node>::iterator end() {
        return children.end();
    }

    Elem&& add_attr(std::string name, std::string value) && {
        attrs.emplace_back(std::move(name), std::move(value));
        return std::move(*this);
    }

    Elem& add_attr(std::string name, std::string value) & {
        attrs.emplace_back(std::move(name), std::move(value));
        return *this;
    }

    Elem&& add_node(NodeVariant data) && {
        children.emplace_back(std::move(data));
        return std::move(*this);
    }

    Elem& add_node(NodeVariant data) & {
        children.emplace_back(std::move(data));
        return *this;
    }

    Elem&& add_node(Elem data) && {
        children.emplace_back(std::make_unique<Elem>(std::move(data)));
        return std::move(*this);
    }

    Elem& add_node(Elem data) & {
        children.emplace_back(std::make_unique<Elem>(std::move(data)));
        return *this;
    }

    Elem& operator=(const Elem& other);
};

bool operator==(const Elem& elem, const Elem& other);

std::ostream& operator<<(std::ostream& os, const Elem& elem);

using RootVariant = std::variant<Cmnt, Decl, Dtd>;

struct Root {
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

    friend bool operator==(const Root& node, const Root& other) {
        return node.data == other.data;
    };
};

std::ostream& operator<<(std::ostream& os, const Root& node);

struct Document {
    std::vector<std::unique_ptr<Root>> children;
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

    std::vector<std::unique_ptr<Root>>::iterator begin() {
        return children.begin();
    }

    std::vector<std::unique_ptr<Root>>::iterator end() {
        return children.end();
    }

    Document& add_node(RootVariant node) {
        children.push_back(std::make_unique<Root>(std::move(node)));
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
};

template<typename F1, typename F2>
void walk_document(Document& document, const F1& on_node, const F2& on_root) {
    for (auto& child_ptr: document.children) {
        Root& child = *child_ptr;
        on_root(child);
    }

    std::stack<Elem*> stack;
    if (document.root != nullptr)
        stack.push(document.root.get());

    while (!stack.empty()) {
        Elem* top = stack.top();
        stack.pop();

        for (Node& child: top->children) {
            on_node(child);
            if (child.is_elem())
                stack.push(&child.as_elem());
        }
    }
}

struct Docstats {
    size_t nodes_count = 0;
    size_t total_mem = 0;
};

Docstats doc_stat(Document& document);

bool operator==(const Document& document, const Document& other);

std::ostream& operator<<(std::ostream& os, const Document& document);

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