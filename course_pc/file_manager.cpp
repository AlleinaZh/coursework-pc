#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace std;

class FileManager {
public:
    vector<string> readFiles(const string& directory, int startIndex, int endIndex) {
        vector<string> files;
        try {
            int index = 0;
            for (const auto& entry : filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    if (directory.find("custom") != string::npos || (index >= startIndex && index < endIndex)) {
                        files.push_back(entry.path().string());
                    }
                    index++;
                }
            }
        } catch (const filesystem::filesystem_error& e) {
            cout << "Error reading directory: " << e.what() << endl;
        }
        return files;
    }

    string readFileContent(const string& fileName) {
        ifstream file(fileName);
        if (!file.is_open()) {
            cout << "Error opening file: " << fileName << endl;
            return "";
        }

        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void saveFileContent(const string& fileName, const string& content) {
        string outputDirectory = "D:/kpi/coursework-pc/course_pc/aclImdb/custom";
        try {
            filesystem::create_directories(outputDirectory);
            ofstream outFile(outputDirectory + "/" + fileName);
            if (!outFile.is_open()) {
                cout << "Error opening file for writing: " << outputDirectory + "/" + fileName << endl;
                return;
            }
            outFile << content;
            outFile.close();
        } catch (const filesystem::filesystem_error& e) {
            cout << "Error saving file: " << e.what() << endl;
        }
    }
};