# XTree
XTree is a C++20 library for the hierarchical tree based serialization format called XML, that tries to be easy to use yet flexible and performant.

## What it Does

XTree uses a Document Object Model (DOM), meaning the XML data is parsed into a C++ objects that can be analyzed and modified, and then written to disk or another output stream. You can also construct an XML document from scratch with C++ objects and write it to disk or another output stream. You can also to stream XML from an input stream without creating a document first. 

XTree uses an iterative variant of the recursive descent algorithm to parse a set of mutually recursive structures into DOM-like tree structure. XTree provides a myriad of utility functions - making it effective at creating XML documents or modifying documents once they are parsed.

Ideally, XTree is used when your you need to parse one or many XML documents, modify them, and serialize them back to a file.

### Utilities
XTree supports utility methods to search for nodes, remove nodes, normalize the tree, print to an output stream, or copy the tree.
XTree uses move constructors to provide move semantics as a default way of passing element trees around.

### Encodings
Supports UTF-8 encodings for XML documents. All XML documents are assumed to be UTF-8.

### Memory Model
`Node` structures own their data, including strings and children nodes. Ownership to child nodes is enforced using `std::unique_ptr`. Data for the node structures is allocated on the heap. Since node structures are only destroyed when their parents are destroyed - you can move nodes to and from different `Document` instances, or out of one. `Node` structures store the different cases using `std::variant` - a type-safe tagged union from C++17.

### Error Handling
XTree uses the `ParseException` and `NodeWalkException` to report errors while parsing or walking the tree.

## Usage

Clone from this github repository.
```shell
$ git clone https://github.com/JosephPrichard/xtree.git
```

Build and run tests to ensure you have a working version of the project.
```shell
$ cd xtree
$ cd tests
$ cmake .
$ cmake --build .
```

Include the correct headers in the files where you need to use the library.
```c++
#include "../xtree/include/xtree.hpp"
```

## Examples

Load and parse an XML file.

```c++
// parses into a tree without modifying the input string
xtree::Document first_document = xtree::Document::from_string("<Root> <Child> Hello World! </Child> </Root>");

// parses the file into a tree without reading the file into a string
xtree::Document second_document = xtree::Document::from_file("file.txt");
```

Send the XML document to an output stream or a string.
```c++
// spits the document out to an in memory string
std::string str = document.serialize();

// alternatively, spit the document out to an output stream
std::ofstream ofs;
ofs.open("file.xml", std::ofstream::out | std::ofstream::trunc);
ofs << document;
```

Lookup or modify some information, then spit back to the file.
```c++
// open the document from a file
xtree::Document document = xtree::Document::from_file("file.txt");

// lookup some data from the document
xtree::Elem& child = document.expect_root().expect_elem("Manager").expect_elem("Programmer");
std::string& value = document.expect_root().expect_elem("Employee").expect_attr("name").value;

// lookup some data then modify it
document.expect_elem("Boss").add_attr("name", child.nth_attr(0).value);

// remove some data and retrieve whatever was removed
std::optional<xtree::Attr> removed_attr = document.expect_root().remove_attr("name");
std::optional<xtree::Elem> removed_elem = document.expect_root().remove_elem("Child");

// move the root out of the document
xtree::Elem root = std::move(document.root);

// insert a new root element
xtree::Elem root = xtree::Elem("Son", {{"name", "John"}, {"age", "22"}})
    .add_node(xtree::Elem("Dad", {{"name", "Bill"}, {"age", "57"}}))
    .add_node(xtree::Elem("Mom", {{"name", "Mary"}, {"age", "54"}}));
document.add_root(std::move(root));

// then spit the modified document to a file
std::ofstream ofs;
ofs.open("file.xml", std::ofstream::out | std::ofstream::trunc);
ofs << document;
```

Normalize to merge adjacent text nodes.
```c++
xtree::Document document;

// create an element tree structure
xtree::Elem root = xtree::Elem("Root")
    .add_node(xtree::Text("Hello"))
    .add_node(xtree::Text("World"));
document.add_root(std::move(root));

// normalization will merge the "Hello" and" World" text nodes into "Hello World"
document.normalize();
```

Iterate over the children of documents and elements.
```c++
xtree::Document document;

// a document's iterator can be accessed using Document::iterator
for (xtree::BaseNode& child : document) {
    if (child->is_decl())
        std::cout << child->as_decl().tag << std::endl;
    else if (child->is_cmnt())
        std::cout << child->as_cmnt().text << std::endl;
    else
        std::cout << child->as_dtd().text << std::endl;
}

// an elem's iterator can be accessed using Elem::iterator
for (xtree::Node& child : document.expect_root()) {
    if (child->is_elem())
        std::cout << child->as_elem().tag << std::endl;
    else if (child->is_text())
        std::cout << child->as_text() << std::endl;
    else
        std::cout << child->as_comment().text << std::endl;
}
```
