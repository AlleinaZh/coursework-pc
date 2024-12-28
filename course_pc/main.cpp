#undef byte

#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <regex>
#include "file_manager.cpp"
#include "indexer.cpp"
#include "thread_pool.cpp"

#include <chrono>

using namespace std;

int main() {
    FileManager fileManager;
    Indexer indexer;

    int iter = 25;
    vector<string> directories = {
            "D:/kpi/coursework-pc/course_pc/aclImdb/custom",
            "D:/kpi/coursework-pc/course_pc/aclImdb/test/neg",
            "D:/kpi/coursework-pc/course_pc/aclImdb/test/pos",
            "D:/kpi/coursework-pc/course_pc/aclImdb/train/neg",
            "D:/kpi/coursework-pc/course_pc/aclImdb/train/pos",
            "D:/kpi/coursework-pc/course_pc/aclImdb/train/unsup"
    };

    vector<pair<int, int>> ranges = {
            {1041, 1250},
            {1041, 1250},
            {1041, 1250},
            {1041, 1250},
            {4166, 5000}
    };

    double total_time = 0.0;

    for (int i = 0; i < iter; i++) {
        auto singlethread_start = chrono::high_resolution_clock::now();
        ThreadPool pool(16);
        for (size_t i = 0; i < directories.size(); i++) {
            pool.enqueue([&, i] {
                auto files = fileManager.readFiles(directories[i], ranges[i].first, ranges[i].second);
                if (files.empty()) {
                    return;
                }

                for (const auto &file: files) {
                    if (regex_match(file, regex(".*\\.txt$", regex::icase))) {
                        string content = fileManager.readFileContent(file);
                        indexer.addFileToIndex(file, content);
                    } else {
                        cout << "Skipped non-txt file: " << file << endl;
                    }
                }
            });
        }
        pool.~ThreadPool();
        auto singlethread_end = chrono::high_resolution_clock::now();
        double iteration_time = chrono::duration<double>(singlethread_end - singlethread_start).count();
        total_time += iteration_time;

        cout << "Time for iteration " << i + 1 << ": " << iteration_time << " s" << endl;
    }

    cout << "Average execution time: " << total_time / iter << " s" << endl;
    return 0;
}
