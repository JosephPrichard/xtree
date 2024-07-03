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

namespace xtree {

struct Attr {
    std::string name;
    std::string value;

    friend bool operator==(const Attr& attr, const Attr& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Attr& attr);

struct Text {
    std::string data;

    std::string& contents() {
        return data;
    }

    friend bool operator==(const Text& attr, const Text& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Text& text);

struct Decl {
    std::string tag;
    std::vector<Attr> attrs;

    std::string& tagname() {
        return tag;
    }

    std::vector<Attr>& attributes() {
        return attrs;
    }

    Decl& add_attr(std::string&& name, std::string&& value) {
        attrs.emplace_back(std::move(name), std::move(value));
        return *this;
    }

    friend bool operator==(const Decl& decl, const Decl& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Decl& decl);

struct Comment {
    std::string text;

    friend bool operator==(const Comment& comment, const Comment& other) = default;
};

std::ostream& operator<<(std::ostream& os, const Comment& comment);

struct DocType {
    std::string text;

    friend bool operator==(const DocType& dtd, const DocType& other) = default;
};

std::ostream& operator<<(std::ostream& os, const DocType& dtd);

struct Elem;

using NodeVariant = std::variant<Elem, Comment, Text>;

using RootVariant = std::variant<Comment, Decl, DocType>;

struct Node;

bool operator==(const Node& node, const Node& other);

struct Elem {
    std::string tag;
    std::vector<Attr> attributes;
    std::vector<std::unique_ptr<Node>> children;

    Elem() = default;

    explicit Elem(std::string&& tag) noexcept : Elem(std::move(tag), {}, {}) {}

    Elem(std::string&& tag, std::vector<Attr>&& attributes) noexcept
        : Elem(Elem(std::move(tag), std::move(attributes), {})) {}

    Elem(std::string&& tag, std::vector<Attr>&& attributes, std::vector<std::unique_ptr<Node>>&& children) noexcept
        : tag(std::move(tag)), attributes(std::move(attributes)), children(std::move(children)) {}

    Elem(const Elem& other) noexcept {
        tag = other.tag;
        attributes = other.attributes;
        children.clear();
        for (auto& child: other.children) {
            children.emplace_back(std::make_unique<Node>(*child));
        }
    }

    Elem(Elem&&) = default;

    Elem* select_elem(const std::string& ctag);

    Attr* select_attr(const std::string& attr_name);

    Elem& expect_elem(const std::string& ctag);

    Attr& select_attr_ex(const std::string& attr_name);

    std::vector<std::unique_ptr<Node>>::iterator begin() {
        return children.begin();
    }

    std::vector<std::unique_ptr<Node>>::iterator end() {
        return children.end();
    }

    Elem& add_attr(std::string&& name, std::string&& value) {
        attributes.emplace_back(std::move(name), std::move(value));
        return *this;
    }

    void remove_attr(const std::string& name) {
        auto it = attributes.begin();
        while (it != attributes.end()) {
            if (it->name == name) {
                it = attributes.erase(it);
            } else {
                it++;
            }
        }
    }

    Elem& add_node(NodeVariant&& node) {
        children.emplace_back(std::make_unique<Node>(std::move(node)));
        return *this;
    }

    void remove_node(const std::string& tag);

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

    bool is_comment() const {
        return holds_alternative<Comment>(data);
    }

    bool is_text() const {
        return holds_alternative<Text>(data);
    }

    bool is_elem() const {
        return holds_alternative<Elem>(data);
    }

    Comment& as_comment() {
        if (auto node = get_if<Comment>(&data))
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

    bool is_comment() const {
        return std::holds_alternative<Comment>(data);
    }

    bool is_decl() const {
        return std::holds_alternative<Decl>(data);
    }

    bool is_dtd() const {
        return std::holds_alternative<DocType>(data);
    }

    Comment& as_comment() {
        if (auto node = std::get_if<Comment>(&data))
            return *node;
        throw NodeWalkException("node is not a comment type node");
    }

    Decl& as_decl() {
        if (auto node = std::get_if<Decl>(&data))
            return *node;
        throw NodeWalkException("node is not a decl type node");
    }

    DocType& as_dtd() {
        if (auto node = std::get_if<DocType>(&data))
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

    explicit Document(const std::string& docstr);

    static bool RECURSIVE_PARSER;

    Decl* select_decl(const std::string& tag) {
        for (auto& child: children)
            if (auto decl = get_if<Decl>(&child->data))
                if (decl->tag == tag)
                    return decl;
        return nullptr;
    }

    void remove_decl(const std::string& rtag) {
        auto it = children.begin();
        while (it != children.end()) {
            if ((*it)->is_decl() && (*it)->as_decl().tag == rtag) {
                it = children.erase(it);
            } else {
                it++;
            }
        }
    }

    std::vector<std::unique_ptr<RootNode>>::iterator begin() {
        return children.begin();
    }

    std::vector<std::unique_ptr<RootNode>>::iterator end() {
        return children.end();
    }

    Document& add_node(RootVariant&& node) {
        children.emplace_back(std::make_unique<RootNode>(std::move(node)));
        return *this;
    }

    Elem& expect_root() {
        if (root != nullptr) {
            return *root;
        }
        throw NodeWalkException("Document does not contain a root element");
    }

    Document& add_root(Elem elem) {
        root = std::make_unique<Elem>(std::move(elem));
        return *this;
    }

    std::string serialize() const;

    friend std::ostream& operator<<(std::ostream& os, const Document& document) {
        for (auto& node: document.children)
            os << *node;
        os << *document.root;
        return os;
    }

    friend bool operator==(const Document& document, const Document& other) {
        if (document.root != nullptr && other.root != nullptr) {
            if (*document.root != *other.root) {
                return false;
            }
        } else if (document.root != nullptr || other.root != nullptr) {
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

struct DocumentWalker {
    virtual void on_elem(Elem& elem) {};

    virtual void on_comment(Comment& comment) {};

    virtual void on_dtd(DocType& dtd) {};

    virtual void on_decl(Decl& decl) {};

    virtual void on_text(Text& text) {};

    void walk_document(Document& document);
};

struct ParseException : public std::runtime_error {
    explicit ParseException(std::string& m) : std::runtime_error(m) {}
};

}