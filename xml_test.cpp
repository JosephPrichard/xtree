//
// Created by Joseph on 4/28/2024.
//

#include <memory_resource>
#include <fstream>
#include "xml.hpp"

void test_small_document() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
            "<Name>"
            "</Name>"
        "</Test>)";

    xmlc::XmlDocument document(docstr);

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

    xmlc::XmlDocument document(docstr);

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

    xmlc::XmlDocument document(docstr);

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

    xmlc::XmlDocument document(docstr);

    auto serialized_doctstr = flatten_spacing(document.serialize().c_str());

    if (docstr_in != serialized_doctstr) {
        fprintf(stderr, "Failed test_document_with_formatting\n\nExpected: \n%s\n\nBut got \n%s\n",
                docstr_in.c_str(), serialized_doctstr.c_str());
    }
}

void test_document_walk() {
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

    xmlc::XmlDocument document(docstr);

    auto attr_value = document.find_child("Tests")->find_attr("Id")->get_value();
    if (strcmp(attr_value, "123") != 0) {
        fprintf(stderr, "Expected '123' for attr but got '%s'", attr_value);
    }

    auto tag = document.find_child("Tests")->find_child("Test")->get_tag();
    if (strcmp(tag, "Test") != 0) {
        fprintf(stderr, "Expected 'Test' for attr but got '%s'", tag);
    }
}

void benchmark_small_document() {
    auto start = std::chrono::steady_clock::now();

    std::ifstream file("../tests/test1.xml");
    std::string docstr;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            docstr += line + "\n";
        }
        file.close();
    } else {
        fprintf(stderr, "Failed to read test file...");
        exit(1);
    }

    for (int i = 0; i < 100; i++) {
        xmlc::XmlDocument document(docstr);
        printf("");
    }

    auto stop = std::chrono::steady_clock::now();
    auto eteTime = std::chrono::duration<double, std::milli>(stop - start);

    printf("Took %.2f ms to parse all files", eteTime.count());
}

int main() {
    test_small_document();
    test_small_decl_document();
    test_document_with_formatting();
    test_document_complex();
    test_document_walk();
    benchmark_small_document();
}