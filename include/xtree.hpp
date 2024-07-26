// Joseph Prichard
// 4/25/2024
// Xml parsing library for C++

#include <variant>
#include <memory>
#include <stack>
#include <vector>

namespace xtree {

// debug flag to add runtime logging for copy constructors and exception functions
#define DEBUG_XTREE_HD false

struct NodeWalkException : public std::runtime_error {
    explicit NodeWalkException(std::string&& m) : std::runtime_error(m) {
#if DEBUG_XTREE_HD
        printf("Threw a node walk exception %s", m.c_str());
#endif
    }
};

struct Attr {
    std::string name;
    std::string value;

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

    Attr* select_attr(const std::string& attr_name) {
       for (auto& attr: attrs)
            if (attr.name == attr_name)
                return &attr;
        return nullptr;
    }

    Attr& expect_attr(const std::string& attr_name) {
        for (auto& attr: attrs)
            if (attr.name == attr_name)
                return attr;
        throw NodeWalkException("decl does not contain attribute with name " + attr_name);
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

using NodeVariant = std::variant<std::unique_ptr<Elem>, Cmnt, Text>; // invariant: ElemPtr cannot point to a null

struct Node {
    NodeVariant data;

    explicit Node(NodeVariant data) : data(std::move(data)) {}

    Node(Node&&) = default;

    // we use a copy factory method instead of a copy constructor to avoid copy semantics in favor of move semantics
    static Node from_other(const Node& other);

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

    std::string serialize() const;
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

    ~Elem();

    // we use a copy factory method of a copy constructor to avoid copy semantics in favor of move semantics
    static Elem from_other(const Elem& other) noexcept;

    Elem(Elem&&) = default;

    Elem* select_elem(const std::string& ctag);

    Attr* select_attr(const std::string& attr_name);

    Elem& expect_elem(const std::string& ctag);

    Attr& expect_attr(const std::string& attr_name);

    void remove_attrs(const std::string& name);

    void remove_elems(const std::string& rtag);

    std::optional<Attr> remove_attr(const std::string& name);

    std::optional<Elem> remove_elem(const std::string& rtag);

    size_t normalize();

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

    std::string serialize() const;
};

bool operator==(const Elem& elem, const Elem& other);

std::ostream& operator<<(std::ostream& os, const Elem& elem);

using BaseVariant = std::variant<Cmnt, Decl, Dtd>;

struct BaseNode {
    BaseVariant data;

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

    friend bool operator==(const BaseNode& node, const BaseNode& other) {
        return node.data == other.data;
    };
};

std::ostream& operator<<(std::ostream& os, const BaseNode& node);

struct Document {
    std::vector<BaseNode> children;
    std::unique_ptr<Elem> root;

    Document() = default;

    Document(Document&&) = default;

    static Document from_file(const std::string& file_path);

    static Document from_file(std::ifstream& file);

    static Document from_string(const std::string& str);

    static Document from_buffer(const char* buffer, size_t size);

    static Document from_other(const Document& other);

    Decl* select_decl(const std::string& tag) {
        for (auto& child: children)
            if (auto decl = get_if<Decl>(&child.data))
                if (decl->tag == tag)
                    return decl;
        return nullptr;
    }

    void remove_decls(const std::string& rtag);

    std::optional<Decl> remove_decl(const std::string& rtag);

    std::vector<BaseNode>::iterator begin() {
        return children.begin();
    }

    std::vector<BaseNode>::iterator end() {
        return children.end();
    }

    Document& add_node(BaseVariant node) {
        children.emplace_back(std::move(node));
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

    size_t normalize() const {
        if (root != nullptr)
            return root->normalize();
        return 0;
    }

    void clear() {
        children.clear();
        root = nullptr;
    }

    Document& operator=(const Document& other);

    std::string serialize() const;
};

template<typename F1, typename F2>
void walk_document(Document& document, const F1& on_node, const F2& on_base) {
    for (auto& child: document.children) {
        on_base(child);
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

Docstats stat_document(Document& node);

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
    InvalidRootOpenTok,
    InvalidXmlMeta,
};

struct ParseException : public std::runtime_error {
    ParseError code;

    explicit ParseException(std::string& m, ParseError code) : std::runtime_error(m), code(code) {
#if DEBUG_XTREE_HD
        printf("Threw a parse exception %s: %d", m.c_str(), code);
#endif
    }
};

}