// Joseph Prichard
// 4/25/2024
// Tests for the xml parsing library

#include <iostream>
#include <memory_resource>
#include "xml.hpp"

void test_read_tokens() {
    std::string str(R"(<Test TestId="0001" TestType="CMD"> <Name></Name> </Test>)");

    std::pmr::monotonic_buffer_resource mbr;
    std::pmr::polymorphic_allocator<XmlNode> node_pa{&mbr };
    Parser parser(str, node_pa);

    auto actual_tokens = parser.read_tokens();

    std::vector<XmlToken> expected_tokens;
    expected_tokens.emplace_back(OpenBegTag{.name = "Test"});
    expected_tokens.emplace_back(AttrName{.name = "TestId"});
    expected_tokens.emplace_back(Assign{});
    expected_tokens.emplace_back(AttrValue{.value = "0001"});
    expected_tokens.emplace_back(AttrName{.name = "TestType"});
    expected_tokens.emplace_back(Assign{});
    expected_tokens.emplace_back(AttrValue{.value = "CMD"});
    expected_tokens.emplace_back(CloseTag{});
    expected_tokens.emplace_back(OpenBegTag{.name = "Name"});
    expected_tokens.emplace_back(CloseTag{});
    expected_tokens.emplace_back(OpenEndTag{.name = "Name"});
    expected_tokens.emplace_back(CloseTag{});
    expected_tokens.emplace_back(OpenEndTag{.name = "Test"});
    expected_tokens.emplace_back(CloseTag{});

    auto actual_serialized = serialize_tokens(actual_tokens);
    auto expected_serialized = serialize_tokens(expected_tokens);

    auto cond = "Pass";
    auto stream = stdout;
    if (actual_serialized != expected_serialized) {
        cond = "Fail";
        stream = stderr;
    }
    fprintf(stream, "%s: expected\n %s\ngot \n %s\n\n", cond, actual_serialized.c_str(), expected_serialized.c_str());
}

void test_parse_only_elems() {
    std::string str(R"(<Test TestId="0001" TestType="CMD"> <Name> </Name> </Test> )");

    std::pmr::monotonic_buffer_resource mbr;
    std::pmr::polymorphic_allocator<XmlNode> node_pa{ &mbr };
    Parser parser(str, node_pa);

    auto tree = parser.parse();

    auto actual_serialized = serialize_xml_tree(tree);

    auto cond = "Pass";
    auto stream = stdout;
    if (actual_serialized != str) {
        cond = "Fail";
        stream = stderr;
    }
    fprintf(stream, "%s: expected\n %s\ngot \n %s\n\n", cond, actual_serialized.c_str(), str.c_str());
}

int main() {
    extern bool DEBUG;
    DEBUG = true;
    test_read_tokens();
    test_parse_only_elems();
}
