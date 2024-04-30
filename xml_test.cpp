//
// Created by Joseph on 4/28/2024.
//

#include <memory_resource>
#include "xml.hpp"

void test_small_document() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
            "<Name>"
            "</Name>"
        "</Test>)";

    std::pmr::monotonic_buffer_resource resource;
    auto document = xmlc::parse_document(docstr, resource);

    xmlc::XmlNode node(xmlc::XmlElem("Name"));

    std::pmr::vector<xmlc::XmlAttr> attrs;
    attrs.emplace_back("TestId", "0001");
    attrs.emplace_back("TestType", "CMD");

    std::pmr::vector<xmlc::XmlNode*> children;
    children.emplace_back(&node);

    xmlc::XmlNode root(xmlc::XmlElem("Test", std::move(attrs), std::move(children)));
    std::pmr::vector<xmlc::XmlNode*> root_children;
    root_children.emplace_back(&root);

    xmlc::XmlDocument expected_document(std::move(root_children));

    if (expected_document != document) {
        fprintf(stderr, "Failed test_small_document\nExpected: \n%s\n but got \n%s\n",
                expected_document.serialize().c_str(), document.serialize().c_str());
    }
}

void test_small_decl_document() {
    auto docstr =
        "<?xml version=\"1.0\" ?>"
        "<Test TestId=\"0001\" TestType=\"CMD\">"
            "<Name/>"
        "</Test>";

    std::pmr::monotonic_buffer_resource resource;
    auto document = xmlc::parse_document(docstr, resource);

    std::pmr::vector<xmlc::XmlAttr> decl_attrs;
    decl_attrs.emplace_back("version", "1.0");

    xmlc::XmlNode decl_node(xmlc::XmlDecl("xml", std::move(decl_attrs)));

    std::pmr::vector<xmlc::XmlAttr> node_attrs;
    node_attrs.emplace_back("TestId", "0001");
    node_attrs.emplace_back("TestType", "CMD");

    xmlc::XmlNode node(xmlc::XmlElem("Name"));

    std::pmr::vector<xmlc::XmlNode*> children;
    children.emplace_back(&node);

    xmlc::XmlNode root(xmlc::XmlElem("Test", std::move(node_attrs), std::move(children)));
    std::pmr::vector<xmlc::XmlNode*> root_children;
    root_children.emplace_back(&decl_node);
    root_children.emplace_back(&root);

    xmlc::XmlDocument expected_document(std::move(root_children));

    if (expected_document != document) {
        fprintf(stderr, "Failed test_small_document\nExpected: \n%s\n but got \n%s\n",
                expected_document.serialize().c_str(), document.serialize().c_str());
    }
}

std::string flatten_spacing(const char* str) {
    std::string new_str;
    for (int i = 0; str[i] != 0; i++) {
        char c = str[i];
        if (c == '\n') {
            continue;
        }
        new_str += c;
    }
    return new_str;
}

void test_document_with_formatting() {
    // tests if the document is equal and if the formatting matches - this test WILL fail if the way xml is serialized
    auto docstr =
        "<?xml version=\"1.0\" ?> "
        "<?xmlmeta?> "
        "<Tests Id=\"123\"> "
            "<Test TestId=\"0001\" TestType=\"CMD\"> "
                "Testing 123 Testing 123"
                "<Test TestId=\"0001\" TestType=\"CMD\"> "
                    "The Internal Text"
                "</Test> "
            "</Test> "
        "</Tests> ";

    auto docstr_in = flatten_spacing(docstr);

    std::pmr::monotonic_buffer_resource resource;
    auto document = xmlc::parse_document(docstr_in, resource);

    auto serialized_doctstr = flatten_spacing(document.serialize().c_str());

    if (docstr_in != serialized_doctstr) {
        fprintf(stderr, "Failed test_document_with_formatting\n\nExpected: \n%s\n\nBut got \n%s\n",
                docstr_in.c_str(), serialized_doctstr.c_str());
    }
}

void test_document_complex() {
    auto docstr =
        "<?xml version=\"1.0\" ?> "
        "<Tests Id=\"123\"> "
            "<Test TestId=\"0001\" TestType=\"CMD\"> "
                "<Name> Convert number to string</Name> "
                "<CommandLine> Examp1.EXE</CommandLine> "
                "<Input> 1</Input> "
                "<Output> One</Output> "
            "</Test> "
            "<Test TestId=\"0002\" TestType=\"CMD\"> "
                "<Name> Find succeeding characters</Name> "
                "<CommandLine> Examp2.EXE</CommandLine> "
                "<Input> abc</Input> "
                "<Output> def</Output> "
            "</Test> "
        "</Tests> ";

    auto docstr_in = flatten_spacing(docstr);

    std::pmr::monotonic_buffer_resource resource;
    auto document = xmlc::parse_document(docstr_in, resource);

    auto serialized_doctstr = flatten_spacing(document.serialize().c_str());

    if (docstr_in != serialized_doctstr) {
        fprintf(stderr, "Failed test_document_with_formatting\n\nExpected: \n%s\n\nBut got \n%s\n",
                docstr_in.c_str(), serialized_doctstr.c_str());
    }
}

int main() {
    test_small_document();
    test_small_decl_document();
    test_document_with_formatting();
    test_document_complex();
}