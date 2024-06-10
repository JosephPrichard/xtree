// Joseph Prichard
// 4/28/2024
// Tests for parser

#include <memory_resource>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "../include/xml.hpp"

void test_small_document_commented() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name>"
        "</Name>"
        "</Test>";

    xtree::Document document(docstr);

    xtree::Document expected;

    auto elem = xtree::Elem("Test", {{"TestId",   "0001"},
                                     {"TestType", "CMD"}});
    elem.add_node(xtree::Node(xtree::Elem("Name")));

    auto root = xtree::Node(std::move(elem));
    expected.add_node(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_small_document\nExpected: \n" << expected.serialize() <<
                  "\n but got \n" << document.serialize() << std::endl;
    }
}

void test_small_decl_document() {
    auto docstr =
        "<?xml version=\"1.0\" ?>"
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name/>"
        "</Test>";

    xtree::Document document(docstr);

    xtree::Document expected;

    auto decl = xtree::Node(xtree::Decl("xml", {{"version", "1.0"}}));

    auto root = xtree::Elem("Test", {{"TestId",   "0001"},
                                     {"TestType", "CMD"}});
    root.add_node(xtree::Node(xtree::Elem("Name")));

    expected.add_node(std::move(decl));
    expected.add_node(xtree::Node(std::move(root)));

    if (expected != document) {
        std::cerr << "Failed test_small_decl_document\nExpected: \n" << expected.serialize() <<
                  "\n but got \n" << document.serialize() << std::endl;
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
    auto docstr =
        "<?xtree version=\"1.0\" ?> "
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

    xtree::Document document(docstr);

    auto ser_docstr = flatten_spacing(document.serialize().c_str());

    if (docstr_in != ser_docstr) {
        std::cerr << "Failed test_document_with_formatting\nExpected: \n" << docstr << "\n but got \n" << ser_docstr << std::endl;
    }
}

void test_document_complex() {
    auto docstr =
        "<?xtree version=\"1.0\" ?> "
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

    xtree::Document document(docstr);

    auto ser_docstr = flatten_spacing(document.serialize().c_str());

    if (docstr_in != ser_docstr) {
        std::cerr << "Failed test_document_complex\nExpected: \n" << docstr << "\n but got \n" << ser_docstr << std::endl;
    }
}

void test_document_walk() {
    auto docstr =
        "<?xtree version=\"1.0\" ?> "
        "<?xmlmeta?> "
        "<Tests Id=\"123\"> "
        "<Test TestId=\"0001\" TestType=\"CMD\"> "
        "Testing 123 Testing 123"
        "<!-- Here is a comment -->"
        "<xsd:Test TestId=\"0001\" TestType=\"CMD\"> "
        "The Internal Text"
        "</xsd:Test> "
        "</Test> "
        "</Tests> ";

    xtree::Document document(docstr);

    auto attr_value = document.select_element("Tests")->select_attr("Id")->get_value();
    if (attr_value != "123") {
        std::cerr << "Expected '123' for attr but got " << attr_value << std::endl;
    }

    auto tag = document.select_element("Tests")->select_element("Test")->get_tag();
    if (tag != "Test") {
        std::cerr << "Expected 'Test' for attr but got " << attr_value << std::endl;
    }
}

void test_document_comments() {
    auto docstr =
        "<Tests> "
        "<!-- This is -a- comment -->"
        "</Tests> ";

    xtree::Document document(docstr);

    xtree::Document expected;

    auto root = xtree::Elem("Tests");
    root.add_node(xtree::Node(xtree::Comment("This is -a- comment")));

    expected.add_node(xtree::Node(std::move(root)));

    if (expected != document) {
        std::cerr << "Failed test_comment_document\nExpected: \n" <<
                  expected.serialize() << "\n but got \n" << document.serialize() << std::endl;
    }
}

void test_document_file(const std::string& file_path) {
    auto start = std::chrono::steady_clock::now();

    std::ifstream file(file_path);
    std::string docstr;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            docstr += line + "\n";
        }
        file.close();
    }
    else {
        std::cerr << "Failed to read test file: " << file_path << std::endl;
        exit(1);
    }

    for (int i = 0; i < 100; i++) {
        xtree::Document document(docstr);
    }

    auto stop = std::chrono::steady_clock::now();
    auto eteTime = std::chrono::duration<double, std::milli>(stop - start);

    std::cout << "Took " << std::fixed << std::setprecision(2) << eteTime.count() << " ms to parse all files" << std::endl;
}

int main() {
    test_small_document_commented();
    test_small_decl_document();
    test_document_with_formatting();
    test_document_complex();
    test_document_walk();
    test_document_comments();

    test_document_file("../input/employee_records.xml");
    test_document_file("../input/plant_catalog.xml");
    test_document_file("../input/books_catalog.xml");
    test_document_file("../input/employee_hierarchy.xml");
    test_document_file("../input/book_store.xml");
}