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
        std::string nm;
        std::string val;

        std::string& name() {
            return nm;
        }

        std::string& value() {
            return val;
        }

        friend std::ostream& operator<<(std::ostream& os, const Attr& attr) {
            os << attr.nm << "=" << "\"";
            for (auto c : attr.val) {
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

        friend bool operator==(const Attr& attr, const Attr& other) = default;
    };

    struct Text {
        std::string data;

        friend std::ostream& operator<<(std::ostream& os, const Text& text) {
            for (auto c : text.data) {
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

        friend bool operator==(const Text& attr, const Text& other) = default;
    };

    struct Decl {
        std::string tag;
        std::vector<Attr> attributes;

        Decl& add_attr(std::string&& name, std::string&& value) {
            attributes.emplace_back(std::move(name), std::move(value));
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& os, const Decl& decl) {
            os << "<?" << decl.tag;
            for (int i = 0; i < decl.attributes.size(); i++) {
                if (i == 0)
                    os << " ";

                os << decl.attributes[i];

                if (i < decl.attributes.size() - 1)
                    os << " ";
            }
            os << "?> ";
            return os;
        }

        friend bool operator==(const Decl& decl, const Decl& other) = default;
    };

    struct Comment {
        std::string text;

        friend std::ostream& operator<<(std::ostream& os, const Comment& comment) {
            os << "<!-- " << comment.text << " -->";
            return os;
        }

        friend bool operator==(const Comment& comment, const Comment& other) = default;
    };

    struct DocType {
        std::string text;

        friend std::ostream& operator<<(std::ostream& os, const DocType& dtd) {
            os << "<!DOCTYPE " << dtd.text << ">";
            return os;
        }

        friend bool operator==(const DocType& dtd, const DocType& other) = default;
    };

    struct Elem;

    using NodeVariant = std::variant<Elem, Comment, Text>;

    using RootVariant = std::variant<Comment, Decl, DocType>;

    struct Node;

    bool operator==(const Node& node, const Node& other);

    struct Elem {
        std::string tag;
        std::vector<Attr> attrs;
        std::vector<std::unique_ptr<Node>> child_nodes;

        Elem() = default;

        explicit Elem(std::string&& tag) noexcept : Elem(std::move(tag), {}, {}) {}

        Elem(std::string&& tag, std::vector<Attr>&& attributes) noexcept
            : Elem(Elem(std::move(tag), std::move(attributes), {})) {}

        Elem(std::string&& tag, std::vector<Attr>&& attributes, std::vector<std::unique_ptr<Node>>&& children) noexcept
            : tag(std::move(tag)), attrs(std::move(attributes)), child_nodes(std::move(children)) {}

        Elem(const Elem& other) noexcept {
            tag = other.tag;
            attrs = other.attrs;
            child_nodes.clear();
            for (auto& child: other.child_nodes) {
                child_nodes.emplace_back(std::make_unique<Node>(*child));
            }
        }

        Elem(Elem&&) = default;

        Elem* select_elem(const std::string& ctag);

        Attr* select_attr(const std::string& attr_name);

        std::vector<std::unique_ptr<Node>>::iterator begin() {
            return child_nodes.begin();
        }

        std::vector<std::unique_ptr<Node>>::iterator end() {
            return child_nodes.end();
        }

        std::string& tagname() {
            return tag;
        }

        std::vector<Attr>& attributes() {
            return attrs;
        }

        std::vector<std::unique_ptr<Node>>& children() {
            return child_nodes;
        }

        Elem& add_attr(std::string&& name, std::string&& value) {
            attrs.emplace_back(std::move(name), std::move(value));
            return *this;
        }

        void remove_attr(const std::string& name) {
            auto it = attrs.begin();
            while (it != attrs.end()) {
                if (it->nm == name) {
                    it = attrs.erase(it);
                } else {
                    it++;
                }
            }
        }

        Elem& add_node(NodeVariant&& node) {
            child_nodes.emplace_back(std::make_unique<Node>(std::move(node)));
            return *this;
        }

        std::vector<std::string> child_tags();

        void remove_node(const std::string& tag);

        friend std::ostream& operator<<(std::ostream& os, const Elem& elem);

        friend bool operator==(const Elem& elem, const Elem& other) {
            if (elem.tag != other.tag || elem.attrs != other.attrs) {
                return false;
            }

            if (elem.child_nodes.size() != other.child_nodes.size()) {
                return false;
            }
            for (int i = 0; i < elem.child_nodes.size(); i++) {
                if (*elem.child_nodes[i] != *other.child_nodes[i])
                    return false;
            }
            return true;
        }

        Elem& operator=(const Elem& other);
    };

    struct NodeTypeException : public std::exception {
        std::string message;

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

        Text& as_text() {
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

        friend std::ostream& operator<<(std::ostream& os, const Node& node) {
            if (auto elem = std::get_if<Elem>(&node.data)) {
                os << *elem;
            }
            else if (auto text = std::get_if<Text>(&node.data)) {
                os << *text;
            }
            else if (auto comment = std::get_if<Comment>(&node.data)) {
                os << *comment;
            }
            return os;
        }

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

        bool is_dtd() const {
            return std::holds_alternative<DocType>(data);
        }

        Comment& as_comment() {
            if (auto node = std::get_if<Comment>(&data))
                return *node;
            throw NodeTypeException("node is not a comment type node");
        }

        Decl& as_decl() {
            if (auto node = std::get_if<Decl>(&data))
                return *node;
            throw NodeTypeException("node is not a decl type node");
        }

        DocType& as_dtd() {
            if (auto node = std::get_if<DocType>(&data))
                return *node;
            throw NodeTypeException("node is not a decl type node");
        }

        friend std::ostream& operator<<(std::ostream& os, const RootNode& node) {
            if (auto text = std::get_if<Decl>(&node.data)) {
                os << *text;
            }
            else if (auto decl = std::get_if<Comment>(&node.data)) {
                os << *decl;
            }
            else if (auto dtd = std::get_if<DocType>(&node.data)) {
                os << *dtd;
            }
            return os;
        }

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
        std::vector<std::unique_ptr<RootNode>> child_nodes;
        std::unique_ptr<Elem> root;

        Document() = default;

        explicit Document(const std::string& docstr);

        static bool RECURSIVE_PARSER;

        std::vector<std::string> child_tags() {
            std::vector<std::string> tags;
            for (auto& child: child_nodes) {
                if (child->is_decl()) {
                    tags.emplace_back(child->as_decl().tag);
                }
            }
            return tags;
        }

        Decl* select_decl(const std::string& tag) {
            for (auto& child: child_nodes)
                if (auto decl = get_if<Decl>(&child->data))
                    if (decl->tag == tag)
                        return decl;
            return nullptr;
        }

        void remove_decl(const std::string& rtag) {
            auto it = child_nodes.begin();
            while (it != child_nodes.end()) {
                if ((*it)->is_decl() && (*it)->as_decl().tag == rtag) {
                    it = child_nodes.erase(it);
                } else {
                    it++;
                }
            }
        }

        std::vector<std::unique_ptr<RootNode>>::iterator begin() {
            return child_nodes.begin();
        }

        std::vector<std::unique_ptr<RootNode>>::iterator end() {
            return child_nodes.end();
        }

        Document& add_node(RootVariant&& node) {
            child_nodes.emplace_back(std::make_unique<RootNode>(std::move(node)));
            return *this;
        }

        std::vector<std::unique_ptr<RootNode>>& children() {
            return child_nodes;
        }

        std::unique_ptr<Elem>& root_elem() {
            return root;
        }

        Document& set_root(Elem&& elem) {
            root = std::make_unique<Elem>(std::move(elem));
            return *this;
        }

        std::string serialize() const {
            std::stringstream ss;
            ss << (*this);
            return ss.str();
        }

        friend std::ostream& operator<<(std::ostream& os, const Document& document) {
            for (auto& node: document.child_nodes)
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
            if (document.child_nodes.size() != other.child_nodes.size()) {
                return false;
            }
            for (int i = 0; i < document.child_nodes.size(); i++) {
                if (*document.child_nodes[i] != *other.child_nodes[i])
                    return false;
            }
            return true;
        }
    };

    struct DocumentWalker {
        virtual void on_elem(Elem& elem) {};

        virtual void on_decl(Decl& decl) {};

        virtual void on_text(Text& text) {};

        void walk_document(Document& document) {
            for (auto& child: document.child_nodes) {
                if (child->is_decl()) {
                    on_decl(child->as_decl());
                }
            }

            on_elem(*document.root);

            std::vector<Elem*> stack;
            stack.emplace_back(document.root.get());

            while (!stack.empty()) {
                auto top = stack.back();
                stack.pop_back();

                for (auto& child: top->child_nodes) {
                    if (child->is_text()) {
                        on_text(child->as_text());
                    }
                    if (child->is_elem()) {
                        on_elem(child->as_elem());
                        stack.emplace_back(&child->as_elem());
                    }
                }
            }
        }
    };

    struct TokenException : public std::exception {
        std::string message;
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