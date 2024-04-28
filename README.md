# XML+
Super simple and low-level C++20 library for the hierarchical tree based serialization format called XML. 

XML+ makes extensive usage of modern c++20 features such as polymorphic allocators, allowing the calling to have control 
of where allocations happen - while still having a relatively abstract API.

Deserialize a document from a string using `xmlc::parse_document(std::string, std::pmr::memory_resource)`.
The function will allocate all xml nodes within the memory resource.
```c++
std::pmr::monotonic_buffer_resource resource;
XmlDocument document = xmlc::parse_document(str, resource);
XmlDocument document1 = xmlc::parse_document(str1, resource);
```

Serialize a document into a string using `xmlc::XmlDocument::serialize()`.
```c++
std::string actual_serialized = document.serialize();
```

Nodes on the document tree can be accessed using utility functions.
```c++
const char* text_data = document.first_elem_child("test")->first_child("test1");
const char* attr_value = document.first_elem_child("test")->first_attr("hello123")->get_value();
```