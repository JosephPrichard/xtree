// Joseph Prichard
// 4/25/2024
// Xml parsing library for C++

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <optional>
#include <sstream>
#include <cstring>

namespace xmlc {
    bool DEBUG = false;

    struct XmlAttr {
        std::pmr::string name;
        std::pmr::string value;

        [[nodiscard]] const char* get_name() const;
        [[nodiscard]] const char* get_value() const;
    };

    using XmlText = std::pmr::string;

    struct XmlDecl {
        std::pmr::string tag;
        std::pmr::vector<XmlAttr> attrs;
    };

    struct XmlNode;

    struct XmlElem {
        std::pmr::string tag;
        std::pmr::vector<XmlAttr> attrs;
        std::pmr::vector<XmlNode*> children;

        [[nodiscard]] const XmlElem* first_child(const char* ctag) const;
        [[nodiscard]] const XmlAttr* first_attr(const char* attr_name) const;
    };

    struct XmlNode {
        std::variant<XmlElem, XmlText, XmlDecl> data;

        [[nodiscard]] bool is_text() const;
        [[nodiscard]] bool is_elem() const;
        [[nodiscard]] const char* as_text() const;
        [[nodiscard]] const XmlElem* as_elem() const;
        [[nodiscard]] const XmlElem* first_child(const char* tag) const;
        [[nodiscard]] std::string serialize();
    };

    class TokenException : public std::exception {
    private:
        std::string message;
    public:
        explicit TokenException(const std::string& m) : message(m) {
            if (DEBUG) {
                printf("%s\n", m.c_str());
            }
        }

        const char* what() {
            return message.c_str();
        }
    };

    class TypeException : public std::exception {
    private:
        std::string message;
    public:
        explicit TypeException(const std::string& m) : message(m) {
            if (DEBUG) {
                printf("%s\n", m.c_str());
            }
        }

        const char* what() {
            return message.c_str();
        }
    };

    struct XmlDocument {
        std::pmr::vector<XmlNode*> children;

        [[nodiscard]] const XmlDecl* first_decl_child(const char* tag) const;
        [[nodiscard]] const XmlElem* first_elem_child(const char* tag) const;
        std::string serialize();
    };

    XmlDocument parse_document(const std::string& docstr, std::pmr::memory_resource& resource);
}
