#include <iomanip>
#include <chrono>
#include <iostream>
#include <fstream>
#include <stack>
#include "../include/xtree.hpp"

#define rand_alpha() (char) (rand() % (122 - 97) + 97)

void create_benchmark_file(const std::string& file_path, int node_count) {
    auto start = std::chrono::steady_clock::now();

    srand((unsigned) time(NULL));

    std::ofstream file(file_path);

    xtree::Document document;
    document.add_root(std::move(xtree::Elem("Root")));

    std::stack<xtree::Elem*> stack;
    stack.push(document.root.get());

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

            top->add_node(std::move(elem));
            break;
        }
        case 1: {
            xtree::Text text;

            auto length = rand() % 50 + 10;
            for (int j = 0; j < length; j++) {
                text.data += rand_alpha();
            }

            top->add_node(std::move(text));
            break;
        }
        default:
            xtree::Cmnt cmnt;

            auto length = rand() % 50 + 10;
            for (int j = 0; j < length; j++) {
                cmnt.data += rand_alpha();
            }

            top->add_node(std::move(cmnt));
            break;
        }

        auto& back = top->children.back();
        if (!back.is_elem()) {
            continue;
        }

        auto action = rand() % 3;
        if (action == 0) {
            stack.push(&back.as_elem());
        }
        else if (action == 1 && stack.size() > 2) {
            stack.pop();
        }
    }

    file << document;

    auto stop = std::chrono::steady_clock::now();
    std::cout << "Took: " << std::chrono::duration<double, std::milli>(stop - start).count() << " ms to create file" << std::endl;
}

std::string to_rounded_string(double num) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << num;
    return ss.str();
}

std::string string_from_file(const std::string& file_path) {
    std::string docstr;

    std::ifstream temp_file(file_path);
    if (temp_file.is_open()) {
        std::string line;
        while (std::getline(temp_file, line)) {
            docstr += line + "\n";
        }
        temp_file.close();
    }
    else {
        std::cerr << "Failed to read test file: " << file_path << std::endl;
        exit(1);
    }

    return docstr;
}

void benchmark_from_file(const std::string& file_path, int count, bool from_mem) {
    std::string str = string_from_file(file_path);

    double ete_time = 0.0;
    auto start = std::chrono::steady_clock::now();

    xtree::Docstats stats_total;

    for (int i = 0; i < count; i++) {
        auto document = from_mem
            ? xtree::Document::from_string(str) 
            : xtree::Document::from_file(file_path);

        auto stats = xtree::doc_stat(document);
        stats_total.nodes_count += stats.nodes_count;
        stats_total.total_mem += stats.total_mem;
    }

    auto stop = std::chrono::steady_clock::now();
    ete_time += std::chrono::duration<double, std::milli>(stop - start).count();

    std::cout
        << std::setw(40) << file_path
        << std::setw(10) << (from_mem ? "Memory" : "File")
        << std::setw(15) << std::to_string(count) + " runs"
        << std::setw(20) << to_rounded_string(str.size() / 1e3) + " kb"
        << std::setw(20) << to_rounded_string(ete_time) + " ms" 
        << std::setw(20) << to_rounded_string(ete_time / count) + " ms/file"
        << std::setw(15) << to_rounded_string(((double) str.size() * (double) count / 1e6) / (ete_time / 1e3)) + " mb/s"
        << std::setw(15) << to_rounded_string(stats_total.total_mem / 1e3) + " kb"
        << std::setw(15) << std::to_string(stats_total.nodes_count) + " nodes"
        << std::endl;
}

int main() {
    create_benchmark_file("../input/random_dump.xml", 2000000);

    std::cout 
        << std::setw(40) << "File path" 
        << std::setw(10) << "Mode"
        << std::setw(15) << "Runs count"
        << std::setw(20) << "File size"
        << std::setw(20) << "Total time"
        << std::setw(20) << "Average time"
        << std::setw(15) << "Throughput"
        << std::setw(15) << "Allocated"
        << std::setw(15) << "Node count"
        << std::endl;

    try {
        benchmark_from_file("../input/employee_records.xml", 100, false);
        benchmark_from_file("../input/plant_catalog.xml", 100, false);
        benchmark_from_file("../input/books_catalog.xml", 1000, true);
        benchmark_from_file("../input/employee_hierarchy.xml", 1000, true);
        benchmark_from_file("../input/book_store.xml", 1000, true);

        benchmark_from_file("../input/gie_file.xml", 10, true);
        benchmark_from_file("../input/gie_file2.xml", 10, true);
        // benchmark_from_file("../input/kml_manager_export_waypoints.xml", 10, true);
        // benchmark_from_file("../input/kml_manager_export.xml", 10, true);
        
        benchmark_from_file("../input/random_dump.xml", 1, false);
        benchmark_from_file("../input/random_dump.xml", 1, true);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}