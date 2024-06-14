// Joseph Prichard
// 4/25/2024
// Xml parsing library for C++

#pragma clang diagnostic push
#pragma ide diagnostic ignored "modernize-use-nodiscard"

#include <string>
#include <utility>
#include <vector>
#include <variant>
#include <memory>
#include <optional>
#include <sstream>
#include <cstring>

namespace xtree {
    struct Attr {
        std::string name;
        std::string value;

        const std::string& get_name() const {
            return name;
        }

        const std::string& get_value() const {
            return value;
        }

        friend std::ostream& operator<<(std::ostream& os, const Attr& attr);

        friend bool operator==(const Attr& attr, const Attr& other) {
            return attr.name == other.name && attr.value == other.value;
        }
    };

    using Text = std::string;

    struct Decl {
        std::string tag;
        std::vector<Attr> attributes;

        void add_attribute(std::string&& name, std::string&& value) {
            attributes.emplace_back(std::move(name), std::move(value));
        }

        friend std::ostream& operator<<(std::ostream& os, const Decl& decl);

        friend bool operator==(const Decl& decl, const Decl& other) {
            return decl.tag == other.tag && decl.attributes == other.attributes;
        }
    };

    struct Comment {
        std::string text;

        Comment(const Comment& other) = default;

        explicit Comment(std::string text) : text(std::move(text)) {}

        friend std::ostream& operator<<(std::ostream& os, const Comment& comment);

        friend bool operator==(const Comment& comment, const Comment& other) {
            return comment.text == other.text;
        }
    };

    struct Elem;

    using NodeVariant = std::variant<Elem, Comment, Text>;

    using RootVariant = std::variant<Comment, Decl>;

    struct Node;

    bool operator==(const Node& node, const Node& other);

    struct Elem {
        std::string tag;
        std::vector<Attr> attributes;
        std::vector<std::unique_ptr<Node>> children;

        Elem() = default;

        explicit Elem(std::string&& tag): Elem(std::move(tag), {}, {}) {}

        Elem(std::string&& tag, std::vector<Attr>&& attributes)
            : Elem(Elem(std::move(tag), std::move(attributes), {})) {}

        Elem(std::string&& tag, std::vector<Attr>&& attributes, std::vector<std::unique_ptr<Node>>&& children)
            : tag(std::move(tag)), attributes(std::move(attributes)), children(std::move(children)) {}

        Elem(const Elem& other) {
            tag = other.tag;
            attributes = other.attributes;
            children.clear();
            for (auto& child: other.children) {
                children.emplace_back(std::make_unique<Node>(*child));
            }
        }

        Elem(Elem&&) = default;

        const std::string& get_tag() const {
            return tag;
        }

        Elem* select_elem(const std::string& ctag);

        Attr* select_attr(const std::string& attr_name);

        void add_attr(std::string&& name, std::string&& value) {
            attributes.emplace_back(std::move(name), std::move(value));
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

        void add_node(NodeVariant&& node) {
            children.emplace_back(std::make_unique<Node>(std::move(node)));
        }

        void remove_node(const std::string& tag);

        friend std::ostream& operator<<(std::ostream& os, const Elem& elem);

        friend bool operator==(const Elem& elem, const Elem& other) {
            if (elem.tag != other.tag || elem.attributes != other.attributes) {
                return false;
            }

            if (elem.children.size() != other.children.size()) {
                return false;
            }
            for (int i = 0; i < elem.children.size(); i++) {
                if (*elem.children[i] != *other.children[i])
                    return false;
            }
            return true;
        }
    };

    class NodeTypeException : public std::exception {
    private:
        std::string message;
    public:
        explicit NodeTypeException(std::string&& m) : message(std::move(m)) {}

        const char* what() const noexcept override {
            return message.c_str();
        }
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
            throw NodeTypeException("node is not a comment type node");
        }

        const std::string& as_text() const {
            if (auto node = get_if<Text>(&data))
                return *node;
            throw NodeTypeException("node is not a text type node");
        }

        Elem& as_elem() {
            if (auto elem = get_if<Elem>(&data))
                return *elem;
            throw NodeTypeException("node is not an elem type node");
        }

        Elem* find_element(const char* tag) {
            if (auto elem = get_if<Elem>(&data))
                return elem->select_elem(tag);
            return nullptr;
        }

        friend std::ostream& operator<<(std::ostream& os, const Node& node);

        std::string serialize() const {
            std::stringstream ss;
            ss << (*this);
            return ss.str();
        }

        friend bool operator==(const Node& node, const Node& other) {
            return node.data == other.data;
        }
    };

    struct RootNode {
        RootVariant data;

        bool is_comment() const {
            return std::holds_alternative<Comment>(data);
        }

        bool is_decl() const {
            return std::holds_alternative<Decl>(data);
        }

        Comment& as_comment() {
            if (auto node = std::get_if<Comment>(&data))
                return *node;
            throw NodeTypeException("node is not a comment type node");
        }

        const Decl& as_decl() const {
            if (auto node = std::get_if<Decl>(&data))
                return *node;
            throw NodeTypeException("node is not a decl type node");
        }

        friend std::ostream& operator<<(std::ostream& os, const RootNode& node);

        std::string serialize() const {
            std::stringstream ss;
            ss << (*this);
            return ss.str();
        }

        friend bool operator==(const RootNode& node, const RootNode& other) {
            return node.data == other.data;
        }
    };

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

        void add_node(RootVariant&& node) {
            children.emplace_back(std::make_unique<RootNode>(std::move(node)));
        }

        std::unique_ptr<Elem>& get_root() {
            return root;
        }

        void set_root(Elem&& elem) {
            root = std::make_unique<Elem>(std::move(elem));
        }

        std::string serialize() const {
            std::stringstream ss;
            ss << (*this);
            return ss.str();
        }

        friend std::ostream& operator<<(std::ostream& os, const Document& document) {
            for (auto& node: document.children)
                os << *node;
            os << *document.root;
            return os;
        }

        friend bool operator==(const Document& document, const Document& other) {
            if (document.children.size() != other.children.size()) {
                return false;
            }
            for (int i = 0; i < document.children.size(); i++) {
                if (*document.children[i] != *other.children[i])
                    return false;
            }
            return true;
        }
    };

    class TokenException : public std::exception {
    private:
        std::string message;
    public:
        int row;
        int col;

        explicit TokenException(std::string&& m, int row, int col) : message(std::move(m)), row(row), col(col) {
        }

        const char* what() const noexcept override {
            return message.c_str();
        }
    };

}

#pragma clang diagnostic pop