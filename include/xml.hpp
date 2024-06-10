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

        friend bool operator==(const Decl& decl, const Decl& other) {
            return decl.tag == other.tag && decl.attributes == other.attributes;
        }
    };

    struct Comment {
        std::string text;

        Comment(const Comment& other) = default;

        explicit Comment(std::string&& text) : text(std::move(text)) {}

        friend bool operator==(const Comment& comment, const Comment& other) {
            return comment.text == other.text;
        }
    };

    struct Node;

    bool operator==(const Node& node, const Node& other);

    struct Elem {
        std::string tag;
        std::vector<Attr> attributes;
        std::vector<std::unique_ptr<Node>> children;

        [[nodiscard]] const std::string& get_tag() const {
            return tag;
        }

        [[nodiscard]] Elem* select_element(const std::string& ctag);

        [[nodiscard]] Attr* select_attr(const std::string& attr_name);

        void add_attribute(std::string&& name, std::string&& value) {
            attributes.emplace_back(std::move(name), std::move(value));
        }

        void add_node(Node&& node) {
            children.emplace_back(std::make_unique<Node>(std::move(node)));
        }

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
        explicit NodeTypeException(const std::string& m) : message(m) {
            printf("%s\n", m.c_str());
        }

        [[nodiscard]] const char* what() const noexcept override {
            return message.c_str();
        }
    };

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
            throw NodeTypeException("node is not a text type node");
        }

        [[nodiscard]] Elem* as_elem() {
            if (auto elem = get_if<Elem>(&data))
                return elem;
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

    struct Document {
        std::vector<std::unique_ptr<Node>> children;

        Document() = default;

        explicit Document(const std::string& docstr);

        [[nodiscard]] Elem* select_element(const std::string& tag) {
            for (auto& child: children)
                if (auto elem = get_if<Elem>(&child->data))
                    if (elem->tag == tag)
                        return elem;
            return nullptr;
        }

        void add_node(Node&& node) {
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
