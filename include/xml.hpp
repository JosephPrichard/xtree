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

namespace xmldom {

// an attribute in a xmldom element tag
    struct Attr {
        std::string name;
        std::string value;

        [[nodiscard]] const std::string& get_name() const {
            return name;
        }

        [[nodiscard]] const std::string& get_value() const {
            return value;
        }

        friend bool operator==(const Attr& attr, const Attr& other) {
            return attr.name == other.name && attr.value == other.value;
        }
    };

// raw text node in the xmldom hierarchy
    using Text = std::string;

// a decl node to represent xmldom document metadata
    struct Decl {
        std::string tag;
        std::vector<Attr> attrs;

        void add_attr(const std::string& name, const std::string& value) {
            attrs.emplace_back(name, value);
        }

        friend bool operator==(const Decl& decl, const Decl& other) {
            return decl.tag == other.tag && decl.attrs == other.attrs;
        }
    };

// a comment node is a used to represent xmldom comments
    struct Comment {
        std::string text;

        friend bool operator==(const Comment& comment, const Comment& other) {
            return comment.text == other.text;
        }
    };

    struct Node;

    bool operator==(const Node& node, const Node& other);

// an elem in the document hierarchy with a tag, attributes, and other children as nodes
    struct Elem {
        std::string tag;
        std::vector<Attr> attrs;
        std::vector<std::unique_ptr<Node>> children;

        [[nodiscard]] const std::string& get_tag() const {
            return tag;
        }

        [[nodiscard]] const Elem* find_child(const std::string& ctag) const;

        [[nodiscard]] const Attr* find_attr(const std::string& attr_name) const;

        void add_attr(const std::string& name, const std::string& value) {
            attrs.emplace_back(name, value);
        }

        void add_child(Node&& node) {
            children.emplace_back(std::make_unique<Node>(std::move(node)));
        }

        friend bool operator==(const Elem& elem, const Elem& other) {
            if (elem.tag != other.tag || elem.attrs != other.attrs) {
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

// thrown when a node is not the expected type
    class NodeTypeException : public std::exception {
    private:
        std::string message;
    public:
        explicit NodeTypeException(const std::string& m) : message(m) {
            printf("%s\n", m.c_str());
        }

        [[nodiscard]] const char* what() const noexcept override {
            return message.c_str();
        }
    };

// a xmldom node in the document hierarchy
    struct Node {
        std::variant<Elem, Comment, Text, Decl> data;

        [[nodiscard]] bool is_text() const {
            return holds_alternative<Text>(data);
        }

        [[nodiscard]] bool is_elem() const {
            return holds_alternative<Elem>(data);
        }

        [[nodiscard]] const std::string& as_text() const {
            if (auto text = get_if<Text>(&data))
                return *text;
            throw NodeTypeException("XML Node is not a text type node");
        }

        [[nodiscard]] const Elem* as_elem() const {
            if (auto elem = get_if<Elem>(&data))
                return elem;
            throw NodeTypeException("XML Node is not an elem type node");
        }

        [[nodiscard]] const Elem* find_child(const char* tag) const {
            if (auto elem = get_if<Elem>(&data))
                return elem->find_child(tag);
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

// represents an entire parsed document
    struct Document {
        std::vector<std::unique_ptr<Node>> children;

        explicit Document(const std::string& docstr);

        explicit Document(std::vector<std::unique_ptr<Node>> children) : children(std::move(children)) {};

        [[nodiscard]] const Elem* find_child(const std::string& tag) const {
            for (auto& child: children)
                if (auto elem = get_if<Elem>(&child->data))
                    if (elem->tag == tag)
                        return elem;
            return nullptr;
        }

        void add_child(Node&& node) {
            children.emplace_back(std::make_unique<Node>(std::move(node)));
        }

        std::string serialize() {
            std::string str;
            for (auto& node: children)
                str += node->serialize();
            return str;
        }

        friend std::ostream& operator<<(std::ostream& os, const Document& document) {
            for (auto& node: document.children)
                os << node;
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

// thrown when an exception is found during parsing
    class TokenException : public std::exception {
    private:
        std::string message;
    public:
        explicit TokenException(const std::string& m) : message(m) {
            printf("%s\n", m.c_str());
        }

        [[nodiscard]] const char* what() const noexcept override {
            return message.c_str();
        }
    };

}
