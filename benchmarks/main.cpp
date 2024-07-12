#include <iomanip>
#include <chrono>
#include <iostream>
#include <fstream>
#include <stack>
#include <random>
#include "../include/xtree.hpp"

std::ofstream OUT("../outlogs.txt");
//std::ostream& OUT = std::cout;

xtree::Document create_benchmark_file(const std::string& file_path, int node_count) {
    auto start = std::chrono::steady_clock::now();

    std::random_device seed;
    std::mt19937 gen{seed()};

    std::uniform_int_distribution<> type_dist{0, 20};
    std::uniform_int_distribution<> action_dist{0, 3};
    std::uniform_int_distribution<> short_dist{4, 12};
    std::uniform_int_distribution<> long_dist{20, 50};
    std::uniform_int_distribution<> char_dist{97, 122};

    std::ofstream file(file_path);

    xtree::Document document;
    document.add_root(xtree::Elem("Root"));

    std::stack<xtree::Elem*> stack;
    stack.push(document.root.get());

    for (int i = 0; i < node_count; i++) {
        auto top = stack.top();

        auto node_type = type_dist(gen);
        if (node_type >= 0 && node_type <= 12) { // 0-12 (60% chance)
            xtree::Elem elem;

            auto length = short_dist(gen);
            for (int j = 0; j < length; j++) {
                elem.tag += (char) char_dist(gen);
            }

            top->add_node(std::move(elem));
        }
        else if (node_type >= 13 && node_type <= 19) { // 13-19 (35%)
            xtree::Text text;

            auto length = long_dist(gen);
            for (int j = 0; j < length; j++) {
                text.data += (char) char_dist(gen);
            }

            top->add_node(text);
            top->add_node(std::move(text));
        }
        else { // 20 (5% chance)
            xtree::Cmnt cmnt;

            auto length = long_dist(gen);
            for (int j = 0; j < length; j++) {
                cmnt.data += (char) char_dist(gen);
            }

            top->add_node(std::move(cmnt));
        }

        auto& back = top->children.back();
        if (!back.is_elem()) {
            continue;
        }

        auto action = action_dist(gen);
        if (action == 0) {
            stack.push(&back.as_elem());
        }
        else if (action == 1 && stack.size() > 2) {
            stack.pop();
        }
    }

    file << document;

    auto stop = std::chrono::steady_clock::now();
    std::cout << "Took: " << std::chrono::duration<double, std::milli>(stop - start).count() << " ms to create bench file" << std::endl;

    return document;
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

xtree::Docstats stat_document(const std::string& file_path) {
    auto document = xtree::Document::from_file(file_path);
    auto stats = xtree::stat_document(document);
    return stats;
}

void benchmark_parse(const std::string& file_path, int count, bool from_mem) {
    std::string str = string_from_file(file_path);

    auto stats_total = stat_document(file_path);

    double ete_time = 0.0;
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < count; i++) {
        if (from_mem)
            xtree::Document::from_string(str);
        else
            xtree::Document::from_file(file_path);
    }

    auto stop = std::chrono::steady_clock::now();
    ete_time += std::chrono::duration<double, std::milli>(stop - start).count();

    OUT << std::setw(35) << file_path
        << std::setw(10) << (from_mem ? "Memory" : "File")
        << std::setw(15) << std::to_string(count) + " runs"
        << std::setw(20) << to_rounded_string(static_cast<double>(str.size()) / 1e3) + " kb/file"
        << std::setw(15) << to_rounded_string(ete_time) + " ms"
        << std::setw(20) << to_rounded_string(ete_time / count) + " ms/file"
        << std::setw(15) << to_rounded_string(((double) str.size() * (double) count / 1e6) / (ete_time / 1e3)) + " mb/s"
        << std::setw(20) << to_rounded_string(static_cast<double>(stats_total.total_mem) / 1e3) + " kb/file"
        << std::setw(20) << std::to_string(stats_total.nodes_count) + " nodes/file"
        << std::endl;
}

void benchmark_print(const std::string& file_path, int count) {
    auto document = xtree::Document::from_file(file_path);

    auto start = std::chrono::steady_clock::now();

    std::ofstream out_file("temp.out");

    for (int i = 0; i < count; i++) {
        out_file << document;
    }

    auto stop = std::chrono::steady_clock::now();
    double ete_time = std::chrono::duration<double, std::milli>(stop - start).count();

    OUT << std::setw(35) << file_path
        << std::setw(15) << std::to_string(count) + " runs"
        << std::setw(20) << to_rounded_string(ete_time) + " ms"
        << std::setw(20) << to_rounded_string(ete_time / count) + " ms/file"
        << std::endl;

    std::remove("temp.out");
}

void benchmark_copy_normalize(const std::string& file_path, xtree::Document& document1, int count) {
    size_t remove_count = 0;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < count; i++) {
        auto document2 = xtree::Document::from_other(document1);
        remove_count += document2.normalize();
    }

    auto stop = std::chrono::steady_clock::now();
    double ete_time = std::chrono::duration<double, std::milli>(stop - start).count();

    OUT << std::setw(35) << file_path
        << std::setw(15) << std::to_string(count) + " runs"
        << std::setw(20) << to_rounded_string(ete_time) + " ms"
        << std::setw(20) << to_rounded_string(ete_time / count) + " ms/file"
        << std::setw(20) << std::to_string(remove_count) + " nodes"
        << std::endl;
}

void benchmark_copyassign_equality(const std::string& file_path, int count) {
    auto document1 = xtree::Document::from_file(file_path);

    xtree::Document document2;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < count; i++) {
        document2 = document1;

        if (document1 != document2) {
            std::cout << "Document " << i << " copy is not equal" << std::endl;
        }
        document2.clear();
    }

    auto stop = std::chrono::steady_clock::now();
    double ete_time = std::chrono::duration<double, std::milli>(stop - start).count();

    OUT << std::setw(35) << file_path
        << std::setw(15) << std::to_string(count) + " runs"
        << std::setw(20) << to_rounded_string(ete_time) + " ms"
        << std::setw(20) << to_rounded_string(ete_time / count) + " ms/file"
        << std::endl;
}

void benchmark_copy_equality(const std::string& file_path, int count) {
    auto document1 = xtree::Document::from_file(file_path);

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < count; i++) {
        auto document2 = xtree::Document::from_other(document1);

        if (document1 != document2) {
            std::cout << "Document " << i << " copy is not equal" << std::endl;
        }
        document2.clear();
    }

    auto stop = std::chrono::steady_clock::now();
    double ete_time = std::chrono::duration<double, std::milli>(stop - start).count();

    OUT << std::setw(35) << file_path
        << std::setw(15) << std::to_string(count) + " runs"
        << std::setw(20) << to_rounded_string(ete_time) + " ms"
        << std::setw(20) << to_rounded_string(ete_time / count) + " ms/file"
        << std::endl;
}

int main() {
    std::cout << "Running benchmark..." << std::endl;

    auto document = create_benchmark_file("../input/random_dump.xml", 2000000);

    try {
        OUT << "Parsing\n"
            << std::setw(35) << "File path"
            << std::setw(10) << "Mode"
            << std::setw(15) << "Runs count"
            << std::setw(20) << "File size"
            << std::setw(15) << "Total time"
            << std::setw(20) << "Average time"
            << std::setw(15) << "Throughput"
            << std::setw(20) << "Allocated"
            << std::setw(20) << "Node count"
            << std::endl;

        benchmark_parse("../input/employee_records.xml", 100, false);
        benchmark_parse("../input/plant_catalog.xml", 100, false);
        benchmark_parse("../input/books_catalog.xml", 1000, true);
        benchmark_parse("../input/employee_hierarchy.xml", 1000, true);
        benchmark_parse("../input/book_store.xml", 1000, true);
        benchmark_parse("../input/gie_file.xml", 10, true);
        benchmark_parse("../input/gie_file2.xml", 10, true);
        benchmark_parse("../input/random_dump.xml", 1, false);
        benchmark_parse("../input/random_dump.xml", 1, true);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    try {
        OUT << "\nPrinting\n"
            << std::setw(35) << "File path"
            << std::setw(15) << "Runs count"
            << std::setw(20) << "Total time"
            << std::setw(20) << "Average time"
            << std::endl;

        benchmark_print("../input/gie_file.xml", 100);
        benchmark_print("../input/gie_file2.xml", 100);
        benchmark_print("../input/random_dump.xml", 3);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    try {
        OUT << "\nCopy + Normalization\n"
            << std::setw(35) << "File path"
            << std::setw(15) << "Runs count"
            << std::setw(20) << "Total time"
            << std::setw(20) << "Average time"
            << std::setw(20) << "Remove count"
            << std::endl;

        benchmark_copy_normalize("../input/random_dump.xml", document, 3);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    try {
        OUT << "\nCopyAssign + Equality\n"
            << std::setw(35) << "File path"
            << std::setw(15) << "Runs count"
            << std::setw(20) << "Total time"
            << std::setw(20) << "Average time"
            << std::endl;

        benchmark_copyassign_equality("../input/gie_file.xml", 100);
        benchmark_copyassign_equality("../input/gie_file2.xml", 100);
        benchmark_copyassign_equality("../input/random_dump.xml", 3);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    try {
        OUT << "\nCopy + Equality\n"
            << std::setw(35) << "File path"
            << std::setw(15) << "Runs count"
            << std::setw(20) << "Total time"
            << std::setw(20) << "Average time"
            << std::endl;

        benchmark_copy_equality("../input/gie_file.xml", 100);
        benchmark_copy_equality("../input/gie_file2.xml", 100);
        benchmark_copy_equality("../input/random_dump.xml", 3);
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    std::cout << "Finished benchmark." << std::endl;
}