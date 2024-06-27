#include <iomanip>
#include <chrono>
#include <iostream>
#include <fstream>
#include <stack>
#include "../include/xml.hpp"

#define rand_alpha() (char) (rand() % (122 - 97) + 97)

void create_benchmark_file(const std::string& file_path, int node_count) {
    srand((unsigned) time(NULL));

    std::ofstream file(file_path);

    xtree::Document document;
    document.set_root(xtree::Elem("Root"));

    std::stack<xtree::Elem*> stack;
    stack.push(document.root_elem().get());

    for (int i = 0; i < node_count; i++) {
        auto top = stack.top();

        auto node_type = rand() % 3;
        switch (node_type) {
        case 0: {
            xtree::Elem elem;

            auto length = rand() % 8 + 4;
            for (int j = 0; j < length; j++) {
                elem.tag += rand_alpha();
            }

            top->add_node(elem);
            break;
        }
        case 1: {
            xtree::Text text;

            auto length = rand() % 50 + 10;
            for (int j = 0; j < length; j++) {
                text.data += rand_alpha();
            }

            top->add_node(text);
            break;
        }
        default:
            xtree::Comment comment;

            auto length = rand() % 50 + 10;
            for (int j = 0; j < length; j++) {
                comment.text += rand_alpha();
            }

            top->add_node(comment);
            break;
        }

        auto& back = top->children().back();
        if (!back->is_elem()) {
            continue;
        }

        auto action = rand() % 3;
        if (action == 0) {
            stack.push(&back->as_elem());
        }
        else if (action == 1 && stack.size() > 2) {
            stack.pop();
        }
    }

    file << document;
}

void benchmark_from_file(const std::string& file_path, int count) {
    auto start = std::chrono::steady_clock::now();

    std::ifstream file(file_path);
    std::string docstr;

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            docstr += line + "\n";
        }
        file.close();
    }
    else {
        std::cerr << "Failed to read test file: " << file_path << std::endl;
        exit(1);
    }

    for (int i = 0; i < count; i++) {
        xtree::Document document(docstr);
    }

    auto stop = std::chrono::steady_clock::now();
    auto eteTime = std::chrono::duration<double, std::milli>(stop - start).count();

    std::cout
        << "For file size " << docstr.size() << " bytes"
        << ", took " << std::fixed << std::setprecision(2) << eteTime << " ms to parse " << count << " files in total"
        << ", averaging " << (eteTime / count) << " ms/file"
        << ", processing " << ((double) docstr.size() * count / eteTime) << " bytes/ms"
        << std::endl;
}

int main() {
    xtree::Document::RECURSIVE_PARSER = false;

    benchmark_from_file("../input/employee_records.xml", 100);
    benchmark_from_file("../input/plant_catalog.xml", 100);
    benchmark_from_file("../input/books_catalog.xml", 1000);
    benchmark_from_file("../input/employee_hierarchy.xml", 1000);
    benchmark_from_file("../input/book_store.xml", 1000);

    create_benchmark_file("../input/random_dump.xml", 1000000);
    benchmark_from_file("../input/random_dump.xml", 1);
}