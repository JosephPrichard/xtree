# XMLc
Super simple and low-level C++20 toolkit for the hierarchical tree based serialization format called XML. 

XMLc makes extensive usage of modern c++20 features such as polymorphic allocators, allowing the caller to have control 
of where allocations happen - while still having a relatively abstract API.

Deserialize a document from a string using `xml::parse_document(std::string, std::pmr::memory_resource)`.
The function will allocate all xml nodes within the memory resource.
```c++
std::pmr::monotonic_buffer_resource resource;
XmlDocument document = xml::parse_document(str, resource);
XmlDocument document1 = xml::parse_document(str1, resource);
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