# XTree
An idiomatic C++20 library for the hierarchical tree based serialization format called XML.

XTree uses an iterative variant of the recursive descent algorithm to parse a set of mutually recursive structures
into DOM-like tree structure. The tree structure enforces ownership using `std::unique_ptr` from parent
to child nodes. XTree provides a myriad of utility functions to modify and analyze an XML document.

An example XML file:
```xml
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

### Usage

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

// ... your code after here
```

### Library

Deserialize a document from a string or a file using factory methods methods.

```c++
xtree::Document document1 = xtree::Document::from_string(str);
xtree::Document document2 = xtree::Document::from_file(file_path);
xtree::Document document3;
```

Serialize a document into a string or file using `xtree::Document::serialize()` or the `<<` operator.
```c++
std::string str = document.serialize();

std::ofstream ofs;
ofs.open("file.xml", std::ofstream::out | std::ofstream::trunc);
ofs << document;
```

Nodes on the document tree can be accessed or modified using utility functions.
```c++
xtree::Elem* child = document.expect_elem("Manager").expect_elem("Programmer");
std::string& value = document.expect_elem("Employee").expect_attr("name").value;

document.add_node(xtree::Elem("Ceo"));
document.select_elem("Ceo")->add_attribute("name", "Joseph");
```

```c++
xtree::Document document;

xtree::Decl decl("xml", {{"version", "1.0"}});
xtree::Elem root = xtree::Elem("Dad", {{"name", "Tom"}, {"age", "54"}})
    .add_node(xtree::Elem("Son", {{"name", "Joseph"}, {"age", "22"}}));
    .add_node(xtree::Elem("Uncle", {{"name", "James"}, {"age", "57"}}));

document.add_node(decl);
document.add_root(root);
```

You can remove elems and attributes from the tree using utility functions.

```c++
xtree::Document document;

// ... add some nodes

std::optional<xtree::Attr> removed_attr = document.expect_root().remove_attr("Age");
std::optional<xtree::Elem> removed_elem = document.expect_root().remove_elem("Son");
```

Copy fragments or the entirety of a document using factory methods.

```c++
xtree::Document document1 = xtree::Document::from_file("employees1.xml");
xtree::Document document2 = xtree::Document::from_other(document1);

xtree::Elem elem2 = xtree::Elem::from_other(document1.expect_root());
```

```c++
xtree::Document document1 = xtree::Document::from_file("employees1.xml");
xtree::Document document2 = xtree::Document::from_file("employees2.xml");

xtree::Elem& employees1 = document1.root->expect_elem("Employees");
xtree::Elem& employees2 = document2.root->expect_elem("Employees");

// copy assignment operator is called
employees1 = employees2;
```

Child nodes of elements and documents can be iterated over.
```c++
xtree::Document document;

for (xtree::BaseNode& child : document) {
    if (child->is_decl()) {
        std::cout << child->as_decl().tag << std::endl;
    } 
    else if (child->is_cmnt()) {
        std::cout << child->as_cmnt().text << std::endl;
    } 
    else {
        std::cout << child->as_dtd().text << std::endl;
    }
}

for (xtree::Node& child : document.expect_root()) {
    if (child->is_elem()) {
        std::cout << child->as_elem().tag << std::endl;
    } 
    else if (child->is_text()) {
        std::cout << child->as_text() << std::endl;
    } 
    else {
        std::cout << child->as_comment().text << std::endl;
    }
}
```

### Benchmarks
The results for the benchmarks can also be found in `./benchmarks/outlogs.txt`.

### Feature List

* Efficient and correct XML parsing ✅
* Support for parsing from both a file and a buffer ✅
* Utility library ✅
    * Text node normalization ✅
    * Node and attr removal ✅
    * Node appendage and modification ✅
    * Serialization and ostream support ✅
    * Copy factory methods and assignment operators ✅
    * Comparison operations ✅
* UTF-8 and UTF-16 support ❌
* xPath parsing and evaluation ❌
* DTD validation support ❌
* SIMD parsing using C++ intrinsics ❌ 
* Parse directly to a struct or class using macros ❌