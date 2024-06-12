// Joseph Prichard
// 4/25/2024
// Xml parsing library for C++

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

        [[nodiscard]] const std::string& get_name() const {
            return name;
        }

        [[nodiscard]] const std::string& get_value() const {
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

        explicit Elem(std::string&& tag): tag(std::move(tag)) {}

        Elem(std::string&& tag, std::vector<Attr>&& attributes)
            : tag(std::move(tag)), attributes(std::move(attributes)) {}

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

        // ensures that an elem can be moved when possible and not copied
        Elem(Elem&& _) = default;

        [[nodiscard]] const std::string& get_tag() const {
            return tag;
        }

        [[nodiscard]] Elem* select_element(const std::string& ctag);

        [[nodiscard]] Attr* select_attr(const std::string& attr_name);

        void add_attribute(std::string&& name, std::string&& value) {
            attributes.emplace_back(std::move(name), std::move(value));
        }

        void add_node(NodeVariant&& node) {
            children.emplace_back(std::make_unique<Node>(std::move(node)));
        }

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

        [[nodiscard]] const char* what() const noexcept override {
            return message.c_str();
        }
    };

    struct Node {
        NodeVariant data;

        [[nodiscard]] bool is_comment() const {
            return holds_alternative<Comment>(data);
        }

        [[nodiscard]] bool is_text() const {
            return holds_alternative<Text>(data);
        }

        [[nodiscard]] bool is_elem() const {
            return holds_alternative<Elem>(data);
        }

        [[nodiscard]] Comment& as_comment() {
            if (auto node = get_if<Comment>(&data))
                return *node;
            throw NodeTypeException("node is not a comment type node");
        }

        [[nodiscard]] const std::string& as_text() const {
            if (auto node = get_if<Text>(&data))
                return *node;
            throw NodeTypeException("node is not a text type node");
        }

        [[nodiscard]] Elem& as_elem() {
            if (auto elem = get_if<Elem>(&data))
                return *elem;
            throw NodeTypeException("node is not an elem type node");
        }

        [[nodiscard]] Elem* find_element(const char* tag) {
            if (auto elem = get_if<Elem>(&data))
                return elem->select_element(tag);
            return nullptr;
        }

        friend std::ostream& operator<<(std::ostream& os, const Node& node);

        [[nodiscard]] std::string serialize() const {
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

        [[nodiscard]] bool is_comment() const {
            return std::holds_alternative<Comment>(data);
        }

        [[nodiscard]] bool is_decl() const {
            return std::holds_alternative<Decl>(data);
        }

        [[nodiscard]] Comment& as_comment() {
            if (auto node = std::get_if<Comment>(&data))
                return *node;
            throw NodeTypeException("node is not a comment type node");
        }

        [[nodiscard]] const Decl& as_decl() const {
            if (auto node = std::get_if<Decl>(&data))
                return *node;
            throw NodeTypeException("node is not a decl type node");
        }

        friend std::ostream& operator<<(std::ostream& os, const RootNode& node);

        [[nodiscard]] std::string serialize() const {
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

        [[nodiscard]] Decl* select_decl(const std::string& tag) {
            for (auto& child: children)
                if (auto decl = get_if<Decl>(&child->data))
                    if (decl->tag == tag)
                        return decl;
            return nullptr;
        }

        void add_node(RootVariant&& node) {
            children.emplace_back(std::make_unique<RootNode>(std::move(node)));
        }

        [[nodiscard]] Elem& get_root() const {
            return *root;
        }

        void set_root(Elem&& elem) {
            root = std::make_unique<Elem>(std::move(elem));
        }

        [[nodiscard]] std::string serialize() const {
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
        int row;
        int col;
    public:
        explicit TokenException(std::string&& m, int row, int col) : message(std::move(m)), row(row), col(col) {
        }

        [[nodiscard]] int get_row() const {
            return row;
        }

        [[nodiscard]] int get_col() const {
            return col;
        }

        [[nodiscard]] const char* what() const noexcept override {
            return message.c_str();
        }
    };

}
