# XTree
A C++20 library for the hierarchical tree based serialization format called XML.

XTree uses the recursive descent algorithm to parse a set of mutually recursive structures
into DOM-like tree structure. The tree structure enforces ownership using `std::unique_ptr` from parent
to child nodes. XTree currently only supports ASCII xml documents.

An example XML file:
```
<book id="bk101">
  <author>Gambardella, Matthew</author>
  <title>XML Developer's Guide</title>
  <genre>Computer</genre>
  <price>44.95</price>
  <publish_date>2000-10-01</publish_date>
  <description>An in-depth look at creating applications
  with XML.</description>
</book>
```

Deserialize a document from a string using the `Document` constructor.

```c++
xtree::Document document(docstr);
```

Serialize a document into a string or file using `xtree::Document::serialize()` or the `<<` operator.
```c++
auto str = document.serialize();

std::ofstream ofs;
ofs.open("doc.xml", std::ofstream::out | std::ofstream::trunc);
ofs << document;
```

Nodes on the document tree can be accessed or modified using utility functions.
```c++
auto child = document.find_element("Manager")->select_element("Programmer");
auto value = document.select_element("Employee")->select_attr("name")->get_value();

document.add_node(xtree::Node(xtree::Elem("CEO")));
document.select_element("CEO")->add_attribute("name", "Joseph");
```

```c++
xtree::Document expected;

auto decl = xtree::Node(xtree::Decl("xml", {{"version", "1.0"}}));

auto root = xtree::Elem("Dad", {{"name", "Tom"}, {"age", "54"}});
root.add_node(xtree::Node(xtree::Elem("Son", {{"name", "Joseph"}, {"age", "22"}})));

expected.add_node(std::move(decl));
expected.add_node(xtree::Node(std::move(root)));
```