# XMLc
Super simple C++20 library for the hierarchical tree based serialization format called XML.

XMLc uses the recursive descent algorithm to parse a set of mutually recursive structures
into DOM-like tree structure. The tree structure enforces ownership using `std::unique_ptr` from parent
to child nodes. Right now, CDATA and comment nodes are parsed into raw-text nodes, but this may
be changed in the future.

An example XML file:
```
<?xml version="1.0" ?>
<?xmlmeta?>
<Tests Id="123">
    <Test TestId="0001" TestType="CMD">
        Testing 123 Testing 123
        <Test TestId="0001" TestType="CMD">
            The Internal Text
        </Test>
    </Test>
</Tests>
```

Deserialize a document from a string using the `XmlDocument` constructor.

```c++
xml::XmlDocument document(docstr);
```

Serialize a document into a string using `xml::XmlDocument::serialize()`.
```c++
std::string str2 = document.serialize();
```

Nodes on the document tree can be accessed using utility functions.
```c++
const char* text_data = document.find_child("test")->find_child("test1");
const char* attr_value = document.find_child("test")->find_attr("hello123")->get_value();
```

See more examples in `xml_test.cpp`