// Joseph Prichard
// 4/28/2024
// Tests for parser

#include <memory_resource>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "../include/xtree.hpp"

void test_comment() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<!-- This is a comment -->"
        "<Name>"
        "</Name>"
        "</Test>";

    xtree::Document document(docstr);

    xtree::Document expected;
    auto root = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}})
        .add_node(xtree::Comment("This is a comment"))
        .add_node(xtree::Elem("Name"));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_small_document\nExpected: \n" << expected.serialize() <<
                  "\nbut got \n" << document.serialize() << "\n" << std::endl;
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
                  expected.serialize() << "\nbut got \n" << document.serialize() << "\n" << std::endl;
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
                  "\nbut got \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_escseq() {
    auto docstr = "<Test name=\"&quot; &apos; &lt; &gt; &amp;\"> &quot; &apos; &lt; &gt; &amp; </Test>";

    xtree::Document document(docstr);

    xtree::Document expected;
    auto root = xtree::Elem("Test")
        .add_attr("name", "\" ' < > &")
        .add_node(xtree::Text("\" ' < > &"));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_escseq\nExpected: \n" << expected.serialize() <<
                  "\nbut got \n" << document.serialize() << "\n" << std::endl;
    }

    auto doc_ser = document.serialize();
    auto expected_ser = "<Test name=\"&quot; &apos; &lt; &gt; &amp;\"> &quot; &apos; &lt; &gt; &amp; </Test> ";
    if (doc_ser != expected_ser) {
        std::cerr << "Failed test_escseq\nExpected: \n" << expected_ser <<
                  "\nbut got \n" << doc_ser << "\n" << std::endl;
    }
}

void test_dtd() {
    auto docstr =
        "<!DOCTYPE hello testing123 hello  >"
        "<Test>"
        "</Test>";

    xtree::Document document(docstr);

    xtree::Document expected;
    expected.add_node(xtree::DocType("hello testing123 hello"));
    auto root = xtree::Elem("Test");
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_small_document\nExpected: \n" << expected.serialize() <<
                  "\nbut got \n" << document.serialize() << "\n" << std::endl;
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
    auto root = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}});
    auto inner = xtree::Elem("Name");
    inner.add_node(xtree::Text("Testing Xml Text <Txt> </Txt>"));
    root.add_node(std::move(inner));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_small_document\nExpected: \n" << expected.serialize() <<
                  "\nGot: \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_unclosed() {
    auto docstr =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name/>";

    try {
        xtree::Document document(docstr);
    } catch (std::exception& ex) {
        auto expected_msg = "reached the end of the stream while parsing at row 1 at col 43";
        if (strcmp(ex.what(), expected_msg) != 0) {
            std::cerr << "Expected message to read\n" << expected_msg << "\nGot: \n" << ex.what() << "\n" << std::endl;
        }
        return;
    }
    std::cerr << "Expected document parse to throw an exception" << std::endl;
}

void test_unequal_tags() {
    auto docstr = "<Test> <Name/> </Test1>";

    try {
        xtree::Document document(docstr);
    } catch (std::exception& ex) {
        auto expected_msg = "encountered invalid token in stream: 'Test1' but expected closing tag 'Test' at row 1 at col 23";
        if (strcmp(ex.what(), expected_msg) != 0) {
            std::cerr << "Expected message to read\n" << expected_msg << "\nGot: \n" << ex.what() << "\n" << std::endl;
        }
        return;
    }
    std::cerr << "Expected document parse to throw an exception" << std::endl;
}

void test_multiple_roots() {
    auto docstr = "<Test> </Test> <Test1> </Test>";
    try {
        xtree::Document document(docstr);
    } catch (std::exception& ex) {
        auto expected_msg = "expected an xml document to only have a single root node at row 1 at col 17";
        if (strcmp(ex.what(), expected_msg) != 0) {
            std::cerr << "Expected message to read\n" << expected_msg << "\nGot: \n" << ex.what() << "\n" << std::endl;
        }
        return;
    }
    std::cerr << "Expected document parse to throw an exception" << std::endl;
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
        std::cerr << "Expected serialized to be \n" << expected_docstr << "\nGot:\n" << document.serialize() << std::endl;
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
    auto root = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}});
    root.add_node(xtree::Elem("Name"));
    expected.add_node(std::move(decl));
    expected.set_root(std::move(root));

    if (expected != document) {
        std::cerr << "Failed test_decl\nExpected: \n" << expected.serialize() <<
                  "\nGot: \n" << document.serialize() << "\n" << std::endl;
    }
}

std::string vecstr_to_string(std::vector<std::string>& vecstr) {
    std::string str = "{ ";
    for (auto& s: vecstr) {
        str += "\"" + s + "\" ";
    }
    str += "}";
    return str;
}

void test_remove_attr() {
    xtree::Document document;

    auto root = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}});
    root.add_node(xtree::Elem("Name1"));
    root.add_node(xtree::Elem("Name"));

    document.add_node(xtree::Decl("xml", {{"version", "1.0"}}));
    document.set_root(std::move(root));

    document.remove_decl("xml");
    document.root_elem()->remove_node("Name");
    document.root_elem()->remove_attr("TestId");

    auto tags = document.root_elem()->child_tags();

    std::vector<std::string> expected_tags = {"Name1"};
    if (tags != expected_tags) {
        std::cerr << "Failed test_decl\nExpected: \n" << vecstr_to_string(expected_tags) <<
                  "\nGot: \n" << vecstr_to_string(tags) << "\n" << std::endl;
    }

    std::vector<std::string> attr = {{"TestId", "0001"}, {"TestType", "CMD"}};
    if (tags != expected_tags) {
        std::cerr << "Failed test_decl\nExpected: \n" << vecstr_to_string(expected_tags) <<
                  "\nGot: \n" << vecstr_to_string(tags) << "\n" << std::endl;
    }
}

void test_larger_doc() {
    auto docstr =
        "<?xml version=\"1.0\"?>"
        "<?xmlmeta?>"
        "<Tests Id=\"123\">"
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "Testing 123 Testing 123"
        "<Test TestId=\"0002\" TestType=\"CMD1\">"
        "The Internal Text"
        "</Test>"
        "</Test>"
        "</Tests>";

    xtree::Document document(docstr);

    xtree::Document expected;

    expected.add_node(xtree::Decl("xml", {{"version", "1.0"}}));
    expected.add_node(xtree::Decl("xmlmeta"));

    auto root = xtree::Elem("Tests", {{"Id", "123"}});
    auto elem = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}});
    auto elem1 = xtree::Elem("Test", {{"TestId", "0002"}, {"TestType", "CMD1"}});

    elem.add_node(xtree::Text("Testing 123 Testing 123"));
    elem1.add_node(xtree::Text("The Internal Text"));

    elem.add_node(std::move(elem1));
    root.add_node(std::move(elem));

    expected.set_root(std::move(root));

    if (document != expected) {
        std::cerr << "Failed test_larger_doc\nExpected: \n" <<
                expected.serialize() << "\nGot: \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_begin_cdata() {
    auto docstr = 
        "<description>\n"
        "<![CDATA[<html> <html/>]]>\n"
        "</description>";

    xtree::Document document(docstr);

    xtree::Document expected;

    auto root = xtree::Elem("description");
    root.add_node(xtree::Text("<html> <html/>"));
    expected.set_root(std::move(root));

    if (document != expected) {
        std::cerr << "Failed test_begin_cdata\nExpected: \n" <<
                expected.serialize() << "\nGot: \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_complex_doc() {
    auto docstr =
        "<?xml version=\"1.0\"?>"
        "<Tests Id=\"123\">"
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name> Convert number to string </Name>"
        "<CommandLine> Examp1.EXE </CommandLine>"
        "<Input> 1 </Input>"
        "<Output> One </Output>"
        "</Test>"
        "<Test TestId=\"0002\" TestType=\"CMD\">"
        "<Name> Find succeeding characters </Name>"
        "<CommandLine> Examp2.EXE </CommandLine>"
        "<Input> abc </Input>"
        "<Output> def </Output>"
        "</Test>"
        "</Tests>";

    xtree::Document document(docstr);

    xtree::Document expected;

    expected.add_node(xtree::Decl("xml", {{"version", "1.0"}}));

    auto root = xtree::Elem("Tests", {{"Id", "123"}});

    auto elem1 = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}})
        .add_node(xtree::Elem("Name").add_node(xtree::Text("Convert number to string")))
        .add_node(xtree::Elem("CommandLine").add_node(xtree::Text("Examp1.EXE")))
        .add_node(xtree::Elem("Input").add_node(xtree::Text("1")))
        .add_node(xtree::Elem("Output").add_node(xtree::Text("One")));

    auto elem2 = xtree::Elem("Test", {{"TestId", "0002"}, {"TestType", "CMD"}})
        .add_node(xtree::Elem("Name").add_node(xtree::Text("Find succeeding characters")))
        .add_node(xtree::Elem("CommandLine").add_node(xtree::Text("Examp2.EXE")))
        .add_node(xtree::Elem("Input").add_node(xtree::Text("abc")))
        .add_node(xtree::Elem("Output").add_node(xtree::Text("def")));

    root.add_node(std::move(elem1));
    root.add_node(std::move(elem2));

    expected.set_root(std::move(root));

    if (document != expected) {
        std::cerr << "Failed test_complex_doc\nExpected: \n" <<
                  expected.serialize() << "\nGot: \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_copy_assign() {
    xtree::Document document1;
    auto root1 = xtree::Elem("Two")
        .add_node(xtree::Text("Two.One"))
        .add_node(xtree::Elem("Three")
            .add_node(xtree::Text("Three.One")))
        .add_node(xtree::Elem("Four")
            .add_node(xtree::Elem("Five")));
    document1.set_root(xtree::Elem("One").add_node(root1));

    xtree::Document document2;
    auto root2 = xtree::Elem("Two")
        .add_node(xtree::Text("Two.One"));
    document2.set_root(xtree::Elem("One").add_node(root2));

    auto& root_ref1 = document1.root_elem_ex().select_elem_ex("Two");
    auto& roof_ref2 = document2.root_elem_ex().select_elem_ex("Two");
    auto& elem_ref1 = root_ref1.select_elem_ex("Four").select_elem_ex("Five");

    elem_ref1 = roof_ref2;
    roof_ref2 = root_ref1;

    xtree::Document expected;
    auto root3 = xtree::Elem("Two")
        .add_node(xtree::Text("Two.One"))
        .add_node(xtree::Elem("Three")
            .add_node(xtree::Text("Three.One")))
        .add_node(xtree::Elem("Four")
            .add_node(xtree::Elem("Two")
                .add_node(xtree::Text("Two.One"))));
    expected.set_root(xtree::Elem("One").add_node(root3));

    if (expected != document2) {
        std::cerr << "Failed test_copy_assign\nExpected: \n" <<
                  expected.serialize() << "\nGot: \n" << document1.serialize() << "\n" << std::endl;
    }
}

void test_copy_assign_self() {
    xtree::Document document;
    auto root = xtree::Elem("Two")
        .add_node(xtree::Text("Two.One"))
        .add_node(xtree::Elem("Three")
            .add_node(xtree::Text("Three.One")))
        .add_node(xtree::Elem("Four")
            .add_node(xtree::Elem("Five")));
    document.set_root(xtree::Elem("One").add_node(root));

    auto& elem_ref = document.root_elem_ex().select_elem_ex("Two");
    auto& roof_ref = document.root_elem_ex();
    roof_ref = elem_ref;

    xtree::Document expected;
    auto root2 = xtree::Elem("Two")
        .add_node(xtree::Text("Two.One"))
        .add_node(xtree::Elem("Three")
            .add_node(xtree::Text("Three.One")))
        .add_node(xtree::Elem("Four")
            .add_node(xtree::Elem("Five")));
    expected.set_root(root2);

    if (expected != document) {
        std::cerr << "Failed test_copy_assign\nExpected: \n" <<
                  expected.serialize() << "\nGot: \n" << document.serialize() << "\n" << std::endl;
    }
}

void test_walk_doc() {
    auto docstr =
        "<?xml version=\"1.0\"?>"
        "<?xmlmeta?>"
        "<Tests Id=\"123\"> "
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "Testing 123 Testing 123"
        "<!-- Here is a comment -->"
        "<xsd:Test TestId=\"0001\" TestType=\"CMD\">"
        "The Internal Text"
        "</xsd:Test> "
        "</Test>"
        "</Tests>";

    xtree::Document document(docstr);

    auto attr_value = document.root_elem()->select_attr("Id")->value();
    if (attr_value != "123") {
        std::cerr << "Expected '123' for attr but got " << attr_value << std::endl;
    }

    auto tag = document.root_elem()->select_elem("Test")->tagname();
    if (tag != "Test") {
        std::cerr << "Expected 'Test' for tag but got " << attr_value << std::endl;
    }

    auto attr2 = document.root_elem()->select_attr("Nope");
    if (attr2 != nullptr) {
        std::cerr << "Expected nullptr for attr but got " << attr_value << std::endl;
    }

    std::vector tags = {"xml", "xmlmeta"};
    int i = 0;
    for (auto& child: document) {
        if (!child->is_decl()) {
            std::cerr << "Expected nodes should be decl nodes" << std::endl;
        }
        if (child->as_decl().tag != tags[i]) {
            std::cerr << "Expected " << tags[i] << "for decl tag but got " << child->as_decl().tag << std::endl;
        }
        i++;
    }
}

void test_walk_doc_root() {
    auto docstr =
        "<?xml version=\"1.0\"?> "
        "<?xmlmeta?> "
        "<Tests Id=\"123\"> "
        "<Test TestId=\"0001\" TestType=\"CMD\"> "
        "<xsd:Test TestId=\"0001\" TestType=\"CMD\"> "
        "The Internal Text"
        "</xsd:Test> "
        "</Test> "
        "</Tests> ";

    xtree::Document document(docstr);

    std::vector tags = {"Test", "xsd:Test"};
    int i = 0;
    for (auto& child: *document.root_elem()) {
        if (!child->is_elem()) {
            std::cerr << "Expected nodes should be elem nodes" << std::endl;
        }
        if (child->as_elem().tag != tags[i]) {
            std::cerr << "Expected " << tags[i] << "for elem tag but got " << child->as_elem().tag << std::endl;
        }
        i++;
    }
}

void test_walk_tree() {
    struct TestWalker : xtree::DocumentWalker {
        std::vector<std::string> elem_tags = {"Test", "Name"};
        int ecnt = 0;

        void on_elem(xtree::Elem& elem) override {
            auto& etag = elem_tags[ecnt++];
            if (elem.tag != etag) {
                std::cerr << "Expected " << etag << "for elem tag but got " << elem.tag << std::endl;
            }
        }

        void on_decl(xtree::Decl& decl) override {
            if (decl.tag != "xml") {
                std::cerr << "Expected xml for decl tag but got " << decl.tag << std::endl;
            }
        }

        void on_text(xtree::Text& text) override {
            if (text.data != "Testing Text") {
                std::cerr << "Expected Testing Text for text but got " << text << std::endl;
            }
        }
    };

    auto docstr = "<?xml ?> <Test> <Name/> Testing Text </Test>";
    xtree::Document document(docstr);

    TestWalker walker;
    walker.walk_document(document);
}

int main() {
    xtree::Document::RECURSIVE_PARSER = false;

    auto start = std::chrono::steady_clock::now();

    try {
        test_comment();
        test_unopened_tag();
        test_cdata();
        test_begin_cdata();
        test_dtd();
        test_escseq();
        test_unclosed();
        test_unequal_tags();
        test_multiple_roots();
        test_decl();
        test_larger_doc();
        test_complex_doc();
        test_dashed_comment();
        test_copy_tree();
        test_remove_attr();
        test_walk_doc();
        test_walk_doc_root();
        test_walk_tree();
        test_copy_assign();
        test_copy_assign_self();
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    auto stop = std::chrono::steady_clock::now();
    auto eteTime = std::chrono::duration<double, std::milli>(stop - start).count();

    std::cout << "Finished executing all tests in " << std::fixed << std::setprecision(2) << eteTime << " ms" << std::endl;
}