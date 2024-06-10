// Joseph Prichard
// 4/28/2024
// Tests for parser

#include <memory_resource>
#include <fstream>
#include "../include/xml.hpp"

void test_small_document_commented() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
            "<Name>"
            "</Name>"
        "</Test>";

    xmldom::Document document(docstr);

    auto node = std::make_unique<xmldom::Node>(xmldom::Elem("Name"));


    std::vector<std::unique_ptr<xmldom::Node>> children;
    children.push_back(std::make_unique<xmldom::Node>(xmldom::Elem("Name")));

    auto root = std::make_unique<xmldom::Node>(xmldom::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}}, std::move(children)));

    std::vector<std::unique_ptr<xmldom::Node>> root_children;
    root_children.emplace_back(std::move(root));

    xmldom::Document exp_document(std::move(root_children));

    if (exp_document != document) {
        fprintf(stderr, "Failed test_small_document\nExpected: \n%s\n but got \n%s\n",
                exp_document.serialize().c_str(), document.serialize().c_str());
    }
}

void test_small_decl_document() {
    auto docstr =
        "<?xmldom version=\"1.0\" ?>"
        "<Test TestId=\"0001\" TestType=\"CMD\">"
            "<Name/>"
        "</Test>";

    xmldom::Document document(docstr);

    auto decl_node = std::make_unique<xmldom::Node>(xmldom::Decl("xmldom", {{"version", "1.0"}}));

    auto node = std::make_unique<xmldom::Node>(xmldom::Elem("Name"));

    std::vector<std::unique_ptr<xmldom::Node>> children;
    children.emplace_back(std::move(node));

    auto root = std::make_unique<xmldom::Node>(xmldom::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}}, std::move(children)));

    std::vector<std::unique_ptr<xmldom::Node>> root_children;
    root_children.emplace_back(std::move(decl_node));
    root_children.emplace_back(std::move(root));

    xmldom::Document exp_document(std::move(root_children));

    if (exp_document != document) {
        fprintf(stderr, "Failed test_small_document\nExpected: \n%s\n but got \n%s\n",
                exp_document.serialize().c_str(), document.serialize().c_str());
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
        "<?xmldom version=\"1.0\" ?> "
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

    xmldom::Document document(docstr);

    auto ser_docstr = flatten_spacing(document.serialize().c_str());

    if (docstr_in != ser_docstr) {
        fprintf(stderr, "Failed test_document_with_formatting\n\nExpected: \n%s\n\nBut got \n%s\n",
                docstr_in.c_str(), ser_docstr.c_str());
    }
}

void test_document_complex() {
    auto docstr =
        "<?xmldom version=\"1.0\" ?> "
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

    xmldom::Document document(docstr);

    auto ser_docstr = flatten_spacing(document.serialize().c_str());

    if (docstr_in != ser_docstr) {
        fprintf(stderr, "Failed test_document_with_formatting\n\nExpected: \n%s\n\nBut got \n%s\n",
                docstr_in.c_str(), ser_docstr.c_str());
    }
}

void test_document_walk() {
    auto docstr =
        "<?xmldom version=\"1.0\" ?> "
        "<?xmlmeta?> "
        "<Tests Id=\"123\"> "
            "<Test TestId=\"0001\" TestType=\"CMD\"> "
                "Testing 123 Testing 123"
                "<Test TestId=\"0001\" TestType=\"CMD\"> "
                    "The Internal Text"
                "</Test> "
            "</Test> "
        "</Tests> ";

    xmldom::Document document(docstr);

    auto attr_value = document.find_child("Tests")->find_attr("Id")->get_value();
    if (attr_value != "123") {
        fprintf(stderr, "Expected '123' for attr but got '%s'\n", attr_value.c_str());
    }

    auto tag = document.find_child("Tests")->find_child("Test")->get_tag();
    if (tag != "Test") {
        fprintf(stderr, "Expected 'Test' for attr but got '%s'\n", tag.c_str());
    }
}

void test_document_comments() {
    auto docstr =
        "<Tests> "
        "<!-- This is a comment -->"
        "</Tests> ";

    xmldom::Document document(docstr);

    auto node = std::make_unique<xmldom::Node>(xmldom::Comment("This is a comment"));

    std::vector<std::unique_ptr<xmldom::Node>> children;
    children.emplace_back(std::move(node));

    auto root = std::make_unique<xmldom::Node>(xmldom::Elem("Tests", {}, std::move(children)));

    std::vector<std::unique_ptr<xmldom::Node>> root_children;
    root_children.emplace_back(std::move(root));

    xmldom::Document exp_document(std::move(root_children));

    if (exp_document != document) {
        fprintf(stderr, "Failed test_comment_document\nExpected: \n%s\n but got \n%s\n",
                exp_document.serialize().c_str(), document.serialize().c_str());
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
    } else {
        fprintf(stderr, "Failed to read test file: %s\n", file_path.c_str());
        exit(1);
    }

    xmldom::Document first_document(docstr);
//    printf("\nDocument for path %s: \n%s\n\n", file_path.c_str(), first_document.serialize().c_str());

    for (int i = 0; i < 100; i++) {
        xmldom::Document document(docstr);
    }

    auto stop = std::chrono::steady_clock::now();
    auto eteTime = std::chrono::duration<double, std::milli>(stop - start);

    printf("Took %.2f ms to parse all files\n", eteTime.count());
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
}