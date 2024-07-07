# XTree
An idiomatic C++20 library for the hierarchical tree based serialization format called XML.

XTree uses the recursive descent algorithm to parse a set of mutually recursive structures
into DOM-like tree structure. The tree structure enforces ownership using `std::unique_ptr` from parent
to child nodes. XTree parses but does not validate DTD nodes. Only ASCII xml documents are supported.

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

Deserialize a document from a string or a file using `Document` static methods.

```c++
xtree::Document document(str);
xtree::Document document2();
```

Serialize a document into a string or file using `xtree::Document::serialize()` or the `<<` operator.
```c++
auto str = document.serialize();

std::ofstream ofs;
ofs.open("gie_file.xml", std::ofstream::out | std::ofstream::trunc);
ofs << document;
```

Nodes on the document tree can be accessed or modified using utility functions.
```c++
xtree::Elem* child = document.select_elem("Manager")->select_elem("Programmer");
std::string& value = document.select_elem("Employee")->select_attr("name")->value;
document.add_node(xtree::Elem("Ceo"));
document.select_elem("Ceo")->add_attribute("name", "Joseph");
```

```c++
xtree::Document document;

xtree::Decl decl("xml", {{"version", "1.0"}});
xtre::Elem root("Dad", {{"name", "Tom"}, {"age", "54"}});
root.add_node(xtree::Elem("Son", {{"name", "Joseph"}, {"age", "22"}}));
document.add_node(decl);
document.add_root(root);
```

Child nodes of elements and documents can be iterated over.
```c++
xtree::Document document;

for (std::unique_ptr<xtree::RootNode>& child : document) {
    if (child->is_decl()) {
        std::cout << child->as_decl().tag << std::endl;
    } else if (child->is_cmnt()) {
        std::cout << child->as_cmnt().text << std::endl;
    } else {
        std::cout << child->as_dtd().text << std::endl;
    }
}

for (std::unique_ptr<xtree::Node>& child : *document.root) {
    if (child->is_elem()) {
        std::cout << child->as_elem().tag << std::endl;
    } else if (child->is_text()) {
        std::cout << child->as_text() << std::endl;
    } else {
        std::cout << child->as_comment().text << std::endl;
    }
}
```

Copy or move fragments of documents to and from each other.
```c++
xtree::Document document("employees1.xml");
xtree::Document document2("employees2.xml");

xtree::Elem& employees = document.root->expect_elem("Employees");
xtree::Elem& employees2 = document.root->expect_elem("Employees");

employees = employees;
```