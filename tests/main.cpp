// Joseph Prichard
// 4/28/2024
// Tests for parser

#include <memory_resource>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "../include/xtree.hpp"

void test_small_document() {
    auto str =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<!-- This is a comment -->"
        "<Name>"
        "</Name>"
        "</Test>";

    auto document = xtree::Document::from_string(str);

    xtree::Document expected;
    auto root = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}})
        .add_node(xtree::Cmnt("This is a comment"))
        .add_node(xtree::Elem("Name"));
    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_dashed_comment() {
    auto str = "<Tests> <!-- This is -a- comment --> </Tests> ";

    auto document = xtree::Document::from_string(str);

    xtree::Document expected;
    auto root = xtree::Elem("Tests");
    root.add_node(xtree::Cmnt("This is -a- comment"));
    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_unopened_tag() {
    auto str = "<Test> Hello --> /> ?> > </Test>";

    auto document = xtree::Document::from_string(str);

    xtree::Document expected;
    auto root = xtree::Elem("Test");
    root.add_node(xtree::Text("Hello --> /> ?> >"));
    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_escseq() {
    auto str = "<Test name=\"&quot; &apos; &lt; &gt; &amp;\"> &quot; &apos; &lt; &gt; &amp; </Test>";

    auto document = xtree::Document::from_string(str);

    xtree::Document expected;
    auto root = xtree::Elem("Test")
        .add_attr("name", "\" ' < > &")
        .add_node(xtree::Text("\" ' < > &"));
    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }

    auto serial_document = document.serialize();
    auto serial_expected = "<Test name=\"&quot; &apos; &lt; &gt; &amp;\"> &quot; &apos; &lt; &gt; &amp; </Test> ";

    if (serial_document != serial_expected) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", serial_expected, serial_document.c_str());
    }
}

void test_dtd() {
    auto str =
        "<!DOCTYPE hello testing123 hello  >"
        "<Test>"
        "</Test>";

    auto document = xtree::Document::from_string(str);

    xtree::Document expected;
    expected.add_node(xtree::Dtd("hello testing123 hello"));
    auto root = xtree::Elem("Test");
    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_cdata() {
    auto str =
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name>"
        "Testing <![CDATA[Xml Text <Txt> </Txt>]]>"
        "</Name>"
        "</Test>";

    auto document = xtree::Document::from_string(str);

    xtree::Document expected;
    auto root = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}});
    auto inner = xtree::Elem("Name")
        .add_node(xtree::Text("Testing Xml Text <Txt> </Txt>"));
    root.add_node(std::move(inner));
    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_begin_cdata() {
    auto str =
        "<description>\n"
        "<![CDATA[<html> <html/>]]>\n"
        "</description>";

    auto document = xtree::Document::from_string(str);

    xtree::Document expected;

    auto root = xtree::Elem("description");
    root.add_node(xtree::Text("<html> <html/>"));
    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_decl() {
    auto str =
        "<?xml version=\"1.0\" ?>"
        "<Test TestId=\"0001\" TestType=\"CMD\">"
        "<Name/>"
        "</Test>";

    auto document = xtree::Document::from_string(str);

    xtree::Document expected;
    auto decl = xtree::Decl("xml", {{"version", "1.0"}});
    auto root = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}});
    root.add_node(xtree::Elem("Name"));
    expected.add_node(std::move(decl));
    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_larger_doc() {
    auto str =
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

    auto document = xtree::Document::from_string(str);

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

    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_complex_doc() {
    auto str =
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

    auto document = xtree::Document::from_string(str);

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

    expected.add_root(std::move(root));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_unclosed() {
    try {
        auto str = R"(<Test TestId="0001" TestType="CMD"> <Name/>)";
        auto document = xtree::Document::from_string(str);
        printf("Expected document parse to throw an exception\n");
    }
    catch (xtree::ParseException& ex) {
        if (ex.code != xtree::ParseError::EndOfStream) {
            printf("Expected error EndOfStream, got: %s\n", ex.what());
        }
        return;
    }
    catch (std::exception& ex) {
        printf("Threw %s\n", ex.what());
    }
}

void test_unequal_tags() {
    try {
        auto str = "<Test> <Name/> </Test1>";
        auto document = xtree::Document::from_string(str);
        printf("Expected document parse to throw an exception\n");
    }
    catch (xtree::ParseException& ex) {
        if (ex.code != xtree::ParseError::CloseTagMismatch) {
            printf("Expected error CloseTagMismatch, got: %s\n", ex.what());
        }
        return;
    }
    catch (std::exception& ex) {
        printf("Threw %s\n", ex.what());
    }
}

void test_multiple_roots() {
    try {
        auto str = "<Test> </Test> <Test1> </Test>";
        auto document = xtree::Document::from_string(str);
        printf("Expected document parse to throw an exception\n");
    }
    catch (xtree::ParseException& ex) {
        if (ex.code != xtree::ParseError::MultipleRoots) {
            printf("Expected error MultipleRoots, got: %s\n", ex.what());
        }
        return;
    }
    catch (std::exception& ex) {
        printf("Threw %s\n", ex.what());
    }
}

void test_copy_node() {
    auto child = xtree::Elem("Child", {{"Name", "Joseph"}});

    // copy the child into the document twice
    auto root = xtree::Elem("Test", {{"TestId", "0001"}});
    root.add_node(child);
    root.add_node(child);
    root.add_node(xtree::Elem("Name"));

    xtree::Document document;
    document.add_root(std::move(root));

    std::string expected_str =
        "<Test TestId=\"0001\"> "
        "<Child Name=\"Joseph\"> </Child> "
        "<Child Name=\"Joseph\"> </Child> "
        "<Name> </Name> "
        "</Test> ";

    if (document.serialize() != expected_str) {
        std::cerr << "Expected serialized to be \n" << expected_str << "\nGot:\n" << document.serialize() << std::endl;
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

std::string attrs_to_string(std::vector<xtree::Attr>& attrs) {
    std::string str = "{ ";
    for (auto& attr: attrs) {
        str += "{" + attr.name + " " + attr.value + "} ";
    }
    str += "}";
    return str;
}

void test_remove_many_nodes() {
    xtree::Document document;

    auto root = xtree::Elem("Test", {{"TestId","0001"}, {"TestType", "CMD"}});
    root.add_node(xtree::Elem("Name1")
        .add_node(xtree::Elem("Name2"))
        .add_node(xtree::Elem("Name3"))
        .add_node(xtree::Elem("Name3"))
        .add_node(xtree::Elem("Name4"))
        .add_node(xtree::Elem("Name3")));
    root.add_node(xtree::Elem("Name"));

    document.add_node(xtree::Decl("xml", {{"version", "1.0"}}));
    document.add_root(std::move(root));

    document.remove_decls("xml");
    document.expect_root().remove_elems("Name");
    document.expect_root().expect_elem("Name1").remove_elems("Name3");
    document.expect_root().remove_attrs("TestId");

    std::vector<std::string> tags;
    for (auto& node: document.expect_root())
        if (node.is_elem())
            tags.push_back(node.as_elem().tag);

    std::vector<std::string> tags1;
    for (auto& node: document.expect_root().expect_elem("Name1"))
        if (node.is_elem())
            tags1.push_back(node.as_elem().tag);

    auto& attrs = document.expect_root().attrs;

    std::vector<std::string> expected_tags = {"Name1"};
    if (tags != expected_tags) {
        printf("Failed test\nExpected:\n%s\nGot:\n%s\n\n", vecstr_to_string(expected_tags).c_str(), vecstr_to_string(tags).c_str());
    }

    std::vector<std::string> expected_tags1 = {"Name2", "Name4"};
    if (tags1 != expected_tags1) {
        printf("Failed test\nExpected:\n%s\nGot:\n%s\n\n", vecstr_to_string(expected_tags1).c_str(), vecstr_to_string(tags1).c_str());
    }

    std::vector<xtree::Attr> expected_attrs = {{"TestType", "CMD"}};
    if (attrs != expected_attrs) {
        printf("Failed test\nExpected:\n%s\nGot:\n%s\n\n", attrs_to_string(expected_attrs).c_str(), attrs_to_string(attrs).c_str());
    }
}

void test_remove_nodes() {
    xtree::Document document;

    auto root = xtree::Elem("Test", {{"TestId", "0001"}, {"TestType", "CMD"}});
    root.add_node(xtree::Elem("Name1"))
        .add_node(xtree::Elem("Name3"))
        .add_node(xtree::Elem("Name3"))
        .add_node(xtree::Elem("Name2"))
        .add_node(xtree::Elem("Name3"));
    document.add_root(std::move(root));

    auto removed_elem = document.expect_root().remove_elem("Name3");
    auto removed_attr = document.expect_root().remove_attr("TestId");

    std::vector<std::string> tags;
    for (auto& node: document.expect_root())
        if (node.is_elem())
            tags.push_back(node.as_elem().tag);

    auto& attrs = document.expect_root().attrs;

    std::vector<std::string> expected_tags = {"Name1", "Name3", "Name2", "Name3"};
    if (tags != expected_tags) {
        printf("Failed test\nExpected:\n%s\nGot:\n%s\n\n", vecstr_to_string(expected_tags).c_str(), vecstr_to_string(tags).c_str());
    }
    xtree::Elem expected_elem("Name3");
    if (removed_elem.has_value()) {
        if (removed_elem.value() != expected_elem) {
            printf("Failed test\nExpected:\n%s\nGot:\n%s\n\n", expected_elem.tag.c_str(), removed_elem.value().tag.c_str());
        }
    }
    else {
        printf("Failed test\nExpected: non null value\n Got: null value");
    }

    std::vector<xtree::Attr> expected_attrs = {{"TestType", "CMD"}};
    if (attrs != expected_attrs) {
        printf("Failed test\nExpected:\n%s\nGot:\n%s\n\n", attrs_to_string(expected_attrs).c_str(), attrs_to_string(attrs).c_str());
    }
    xtree::Attr expected_attr("TestId", "0001");
    if (removed_attr.has_value()) {
        if (removed_attr.value() != expected_attr) {
            printf("Failed test\nExpected:\n%s\nGot:\n%s\n\n", expected_attr.name.c_str(), removed_attr.value().name.c_str());
        }
    }
    else {
        printf("Failed test\nExpected: non null value\n Got: null value");
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
    document1.add_root(std::move(xtree::Elem("One").add_node(root1)));

    xtree::Document document2;
    auto root2 = xtree::Elem("Two")
        .add_node(xtree::Text("Two.One"));
    document2.add_root(std::move(xtree::Elem("One").add_node(root2)));

    auto& root_ref1 = document1.expect_root().expect_elem("Two");
    auto& roof_ref2 = document2.expect_root().expect_elem("Two");
    auto& elem_ref1 = root_ref1.expect_elem("Four").expect_elem("Five");

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
    expected.add_root(std::move(xtree::Elem("One").add_node(root3)));

    if (expected != document2) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document2.serialize().c_str());
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
    document.add_root(std::move(xtree::Elem("One").add_node(root)));

    auto& elem_ref = document.expect_root().expect_elem("Two");
    auto& roof_ref = document.expect_root();
    roof_ref = elem_ref;

    xtree::Document expected;
    auto root2 = xtree::Elem("Two")
        .add_node(xtree::Text("Two.One"))
        .add_node(xtree::Elem("Three")
                      .add_node(xtree::Text("Three.One")))
        .add_node(xtree::Elem("Four")
                      .add_node(xtree::Elem("Five")));
    expected.add_root(std::move(root2));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_normalize() {
    xtree::Document document;
    auto root = xtree::Elem("One")
        .add_node(xtree::Text("One.One"))
        .add_node(xtree::Text("One.Two"))
        .add_node(xtree::Elem("Two")
            .add_node(xtree::Text("Two.One"))
            .add_node(xtree::Text("Two.Two"))
            .add_node(xtree::Elem("Five")))
        .add_node(xtree::Elem("Three"))
        .add_node(xtree::Text("One.Three"))
        .add_node(xtree::Text("One.Four"))
        .add_node(xtree::Elem("Four"));

    document.add_root(std::move(root));
    document.root->normalize();

    xtree::Document expected;
    auto root2 = xtree::Elem("One")
        .add_node(xtree::Text("One.OneOne.Two"))
        .add_node(xtree::Elem("Two")
            .add_node(xtree::Text("Two.OneTwo.Two"))
            .add_node(xtree::Elem("Five")))
        .add_node(xtree::Elem("Three"))
        .add_node(xtree::Text("One.ThreeOne.Four"))
        .add_node(xtree::Elem("Four"));
    expected.add_root(std::move(root2));

    if (expected != document) {
        printf("Failed test\nExpected: \n %s\n but got \n %s \n\n", expected.serialize().c_str(), document.serialize().c_str());
    }
}

void test_walk_doc() {
    auto str =
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

    auto document = xtree::Document::from_string(str);

    auto& attr_value = document.root->select_attr("Id")->value;
    if (attr_value != "123") {
        printf("Expected '123' for attr but got %s\n", attr_value.c_str());
    }

    auto& tag = document.root->select_elem("Test")->tag;
    if (tag != "Test") {
        printf("Expected 'Test' for tag but got %s\n", tag.c_str());
    }

    auto attr2 = document.root->select_attr("Nope");
    if (attr2 != nullptr) {
        printf("Expected nullptr for attr but got %s\n", attr2->value.c_str());
    }

    std::vector tags = {"xml", "xmlmeta"};
    int i = 0;
    for (auto& child: document) {
        if (!child->is_decl()) {
            printf("Expected nodes should be decl nodes\n");
        }
        if (child->as_decl().tag != tags[i]) {
            printf("Expected %s for decl tag but got %s\n", tags[i], child->as_decl().tag.c_str());
        }
        i++;
    }
}

void test_walk_doc_root() {
    auto str =
        "<?xml version=\"1.0\"?> "
        "<?xmlmeta?> "
        "<Tests Id=\"123\"> "
        "<Test TestId=\"0001\" TestType=\"CMD\"> "
        "<xsd:Test TestId=\"0001\" TestType=\"CMD\"> "
        "The Internal Text"
        "</xsd:Test> "
        "</Test> "
        "</Tests> ";

    auto document = xtree::Document::from_string(str);

    std::vector tags = {"Test", "xsd:Test"};
    int i = 0;
    for (auto& child: document.expect_root()) {
        if (!child.is_elem()) {
            printf("Expected nodes should be elem nodes\n");
        }
        if (child.as_elem().tag != tags[i]) {
            printf("Expected %s for elem tag but got %s\n", tags[i], child.as_elem().tag.c_str());
        }
        i++;
    }
}

void test_walk_tree() {
    auto str = "<?xml ?> <Test> <Test1/> <Name/> Testing Text </Test>";
    auto document = xtree::Document::from_string(str);

    int ecnt = 0;
    std::vector<std::string> elem_tags = {"Test1", "Name"};

    walk_document(document,
        [&elem_tags, &ecnt](xtree::Node& node) {
            if (node.is_elem()) {
                auto& elem = node.as_elem();
                auto& etag = elem_tags[ecnt++];
                if (elem.tag != etag)
                    printf("Expected %s for elem tag but got %s\n", etag.c_str(), elem.tag.c_str());
            }
        },
        [](xtree::Root& root) {
            if (root.is_decl()) {
                auto& decl = root.as_decl();
                if (root.as_decl().tag != "xml")
                    printf("Expected xml for decl tag but got %s\n", decl.tag.c_str());
            }
        });
}

void test_stat_tree() {
    auto str = "<?xml ?> <Test> <Test1/> <Name/> Testing Text </Test>";
    auto document = xtree::Document::from_string(str);

    auto stats = xtree::doc_stat(document);

    xtree::Docstats expected{5, 454};
    if (memcmp(&stats, &expected, sizeof(stats)) != 0) {
        printf("Expected doc stats to be nodes: %zu, mem: %zu but got nodes: %zu, mem: %zu\n",
            expected.nodes_count, expected.total_mem, stats.nodes_count, stats.total_mem);
    }
}

int main() {
    auto start = std::chrono::steady_clock::now();

    try {
        test_small_document();
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
        test_remove_many_nodes();
        test_remove_nodes();
        test_walk_doc();
        test_walk_doc_root();
        test_walk_tree();
        test_copy_node();
        test_copy_assign();
        test_copy_assign_self();
        test_stat_tree();
        test_normalize();
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    auto stop = std::chrono::steady_clock::now();
    auto eteTime = std::chrono::duration<double, std::milli>(stop - start).count();

    std::cout << "Finished executing all tests in " << std::fixed << std::setprecision(2) << eteTime << " ms" << std::endl;
}