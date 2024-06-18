// Joseph Prichard
// 4/28/2024
// Tests for parser

#include <memory_resource>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "../include/xml.hpp"

void test_comment() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<!-- This is a comment -->"
        "<Name>"
        "</Name>"
        "</Test>";

    xtree::Document document(docstr);

    xtree::Document expected;
    auto root = xtree::Elem("Test", {{"TestId",   "0001"},
                                     {"TestType", "CMD"}});
    root.add_node(xtree::Comment("This is a comment"));
    root.add_node(xtree::Elem("Name"));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_small_document\nExpected: \n" << expected.serialize() <<
                  "\n but got \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_dashed_comment() {
    auto docstr =
        "<Tests> "
        "<!-- This is -a- comment -->"
        "</Tests> ";

    xtree::Document document(docstr);

    xtree::Document expected;
    auto root = xtree::Elem("Tests");
    root.add_node(xtree::Comment("This is -a- comment"));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_comment_document\nExpected: \n" <<
                  expected.serialize() << "\n but got \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_unopened_tag() {
    auto docstr =
        "<Test>"
        "--> /> ?> >"
        "</Test>";

    xtree::Document document(docstr);

    xtree::Document expected;
    auto root = xtree::Elem("Test");
    root.add_node(xtree::Text("--> /> ?> >"));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_small_document\nExpected: \n" << expected.serialize() <<
                  "\n but got \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_cdata() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name>"
        "Testing <![CDATA[Xml Text <Txt> </Txt>]]>"
        "</Name>"
        "</Test>";

    xtree::Document document(docstr);

    xtree::Document expected;
    auto root = xtree::Elem("Test", {{"TestId",   "0001"},
                                     {"TestType", "CMD"}});
    auto inner = xtree::Elem("Name");
    inner.add_node(xtree::Text("Testing Xml Text <Txt> </Txt>"));
    root.add_node(std::move(inner));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_small_document\nExpected: \n" << expected.serialize() <<
                  "\n but got \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_unclosed() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name/>";

    try {
        xtree::Document document(docstr);
    } catch (std::exception& ex) {
        auto expected_msg = "reached the end of the stream while parsing at row 0 at col 42";
        if (strcmp(ex.what(), expected_msg) != 0) {
            std::cerr << "Expected message to read\n" << expected_msg << "\n but got \n" << ex.what() << "\n" << std::endl;
        }
        return;
    }
    std::cerr << "Expected document parse to throw an exception";
}

void test_unequal_tags() {
    auto docstr = "<Test> <Name/> </Test1>";

    try {
        xtree::Document document(docstr);
    } catch (std::exception& ex) {
        auto expected_msg = "encountered invalid token in stream: Test1 but expected closing tag 'Test' at row 0 at col 22";
        if (strcmp(ex.what(), expected_msg) != 0) {
            std::cerr << "Expected message to read\n" << expected_msg << "\n but got \n" << ex.what() << "\n" << std::endl;
        }
        return;
    }
    std::cerr << "Expected document parse to throw an exception";
}

void test_multiple_roots() {
    auto docstr = "<Test> </Test> <Test1> </Test>";
    try {
        xtree::Document document(docstr);
    } catch (std::exception& ex) {
        auto expected_msg = "expected an xml document to only have a single root node at row 0 at col 16";
        if (strcmp(ex.what(), expected_msg) != 0) {
            std::cerr << "Expected message to read\n" << expected_msg << "\n but got \n" << ex.what() << "\n" << std::endl;
        }
        return;
    }
    std::cerr << "Expected document parse to throw an exception";
}

void test_copy_tree() {
    auto child = xtree::Elem("Child", {{"Name", "Joseph"}});

    // copy the child into the document twice
    auto root = xtree::Elem("Test", {{"TestId", "0001"}});
    root.add_node(child);
    root.add_node(child);
    root.add_node(xtree::Elem("Name"));

    xtree::Document document;
    document.set_root(std::move(root));

    std::string expected_docstr =
        "<Test TestId=\"0001\"> "
        "<Child Name=\"Joseph\"> </Child> "
        "<Child Name=\"Joseph\"> </Child> "
        "<Name> </Name> "
        "</Test> ";

    if (document.serialize() != expected_docstr) {
        std::cerr << "Expected serialized to be \n" << expected_docstr << "\nbut got\n" << document.serialize() << std::endl;
    }
}

void test_decl() {
    auto docstr =
        "<?xml version=\"1.0\" ?>"
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name/>"
        "</Test>";

    xtree::Document document(docstr);

    xtree::Document expected;
    auto decl = xtree::Decl("xml", {{"version", "1.0"}});
    auto root = xtree::Elem("Test", {{"TestId",   "0001"},
                                     {"TestType", "CMD"}});
    root.add_node(xtree::Elem("Name"));
    expected.add_node(std::move(decl));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_decl\nExpected: \n" << expected.serialize() <<
                  "\n but got \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_remove() {
    xtree::Document document;
    auto decl = xtree::Decl("xml", {{"version", "1.0"}});
    auto root = xtree::Elem("Test", {{"TestId",   "0001"},
                                     {"TestType", "CMD"}});
    root.add_node(xtree::Elem("Name1"));
    root.add_node(xtree::Elem("Name"));
    document.add_node(std::move(decl));
    document.set_root(std::move(root));

    document.remove_decl("xml");
    document.get_root()->remove_node("Name");
    document.get_root()->remove_attr("TestId");

    auto docstr = document.serialize();

    auto expected =
        "<Test TestType=\"CMD\"> "
        "<Name1> </Name1> "
        "</Test> ";

    if (docstr != expected) {
        std::cerr << "Failed test_decl\nExpected: \n" << expected <<
                  "\n but got \n" << docstr << "\n" << std::endl;
    }
}

std::string flatten_spacing(const std::string& str) {
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

void test_larger_doc() {
    auto docstr =
        "<?xml version=\"1.0\"?> "
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

    auto ser_docstr = flatten_spacing(document.serialize());

    if (docstr_in != ser_docstr) {
        std::cerr << "Failed test_larger_doc\nExpected: \n" << docstr << "\n but got \n" << ser_docstr << "\n" << std::endl;
    }
}

void test_complex_doc() {
    auto docstr =
        "<?xml version=\"1.0\"?> "
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

    auto ser_docstr = flatten_spacing(document.serialize());

    if (docstr_in != ser_docstr) {
        std::cerr << "Failed test_complex_doc\nExpected: \n" << docstr << "\n but got \n" << ser_docstr << "\n" << std::endl;
    }
}

void test_walk_doc() {
    auto docstr =
        "<?xml version=\"1.0\"?> "
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

    auto attr_value = document.get_root()->select_attr("Id")->get_value();
    if (attr_value != "123") {
        std::cerr << "Expected '123' for attr but got " << attr_value << std::endl;
    }

    auto tag = document.get_root()->select_elem("Test")->get_tag();
    if (tag != "Test") {
        std::cerr << "Expected 'Test' for tag but got " << attr_value << std::endl;
    }

    auto attr2 = document.get_root()->select_attr("Nope");
    if (attr2 != nullptr) {
        std::cerr << "Expected nullptr for attr but got " << attr_value << std::endl;
    }

    for (auto& child : document) {

    }
}

void test_from_file(const std::string& file_path) {
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
    xtree::Document::RECURSIVE_PARSER = false;

    test_comment();
    test_unopened_tag();
    test_cdata();
    test_unclosed();
    test_unequal_tags();
    test_multiple_roots();
    test_decl();
    test_larger_doc();
    test_complex_doc();
    test_walk_doc();
    test_dashed_comment();
    test_copy_tree();
    test_remove();

    test_from_file("../input/employee_records.xml");
    test_from_file("../input/plant_catalog.xml");
    test_from_file("../input/books_catalog.xml");
    test_from_file("../input/employee_hierarchy.xml");
    test_from_file("../input/book_store.xml");
}