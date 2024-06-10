# XML Dom
Super simple C++20 library for the hierarchical tree based serialization format called XML.

XMLDom uses the recursive descent algorithm to parse a set of mutually recursive structures
into DOM-like tree structure. The tree structure enforces ownership using `std::unique_ptr` from parent
to child nodes.

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
xmldom::Document document(docstr);
```

Serialize a document into a string or file using `xmldom::Document::serialize()` or the `<<` operator.
```c++
auto str = document.serialize();

std::ofstream ofs;
ofs.open("doc.xml", std::ofstream::out | std::ofstream::trunc);
file << document;
```

Nodes on the document tree can be accessed or modified using utility functions.
```c++
auto text_data = document.find_child("test")->find_child("test1");
auto attr_value = document.find_child("test")->find_attr("hello123")->get_value();

document.add_child(xmldom::Node(xmldom::Elem()));
document.find_child("test")->add_attr("Test1", "Test2");
```

See more examples in `/tests/main.cpp`