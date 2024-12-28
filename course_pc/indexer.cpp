#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <set>
#include <vector>
#include <shared_mutex>
#include <algorithm>
#include <sstream>
#include <mutex>
#include "file_manager.cpp"

using namespace std;

class Indexer {
private:
    static const int N = 16;
    vector<unordered_map<string, set<string>>> sections;
    vector<shared_mutex> sectionMutexes;

    int getSectionIndex(const string& word) {
        return hash<string>{}(word) % N;
    }

public:
    Indexer() : sections(N), sectionMutexes(N) {}

    void addFileToIndex(const string& fileName, const string& content) {
        istringstream stream(content);
        string word;
        while (stream >> word) {
            transform(word.begin(), word.end(), word.begin(), ::tolower);

            int sectionIndex = getSectionIndex(word);

            unique_lock<shared_mutex> lock(sectionMutexes[sectionIndex]);
            sections[sectionIndex][word].insert(fileName);
        }
    }

    vector<string> search(const string& word) {
        string lowerWord = word;
        transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);

        int sectionIndex = getSectionIndex(lowerWord);

        shared_lock<shared_mutex> lock(sectionMutexes[sectionIndex]);

        if (sections[sectionIndex].find(lowerWord) != sections[sectionIndex].end()) {
            const auto& fileSet = sections[sectionIndex][lowerWord];
            return vector<string>(fileSet.begin(), fileSet.end());
        }

        return {};
    }

    void updateIndex(const string& fileName, const string& newContent) {
        if (newContent.empty()) {
            for (int i = 0; i < N; ++i) {
                unique_lock<shared_mutex> lock(sectionMutexes[i]);
                for (auto it = sections[i].begin(); it != sections[i].end();) {
                    it->second.erase(fileName);
                    if (it->second.empty()) {
                        it = sections[i].erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        } else {
            istringstream stream(newContent);
            string word;
            while (stream >> word) {
                transform(word.begin(), word.end(), word.begin(), ::tolower);

                int sectionIndex = getSectionIndex(word);

                unique_lock<shared_mutex> lock(sectionMutexes[sectionIndex]);
                sections[sectionIndex][word].insert(fileName);
            }
        }
    }

    void MonitorDeletions(const wstring& directory) {
        HANDLE hDir = CreateFileW(
                directory.c_str(),
                FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                nullptr
        );

        if (hDir == INVALID_HANDLE_VALUE) {
            std::cerr << "Error: Unable to open directory for monitoring.\n";
            return;
        }

        char buffer[1024];
        DWORD bytesReturned;
        FileManager fileManager;

        while (true) {
            if (ReadDirectoryChangesW(
                    hDir,
                    buffer,
                    sizeof(buffer),
                    TRUE,
                    FILE_NOTIFY_CHANGE_FILE_NAME,
                    &bytesReturned,
                    nullptr,
                    nullptr)) {
                auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);

                do {
                    wstring fileName(info->FileName, info->FileNameLength / sizeof(wchar_t));
                    wstring fullPath = directory + fileName;

                    if (info->Action == FILE_ACTION_ADDED){
                        wcout << L"File added: " << fullPath << L"\n";
                        wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
                        string str = converter.to_bytes(fullPath);
                        string newContent = fileManager.readFileContent(str);
                        updateIndex(str, newContent);
                    }
                    if (info->Action == FILE_ACTION_REMOVED) {
                        wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
                        string str = converter.to_bytes(fullPath);

                        wcout << L"File deleted: " << fullPath << L"\n";
                        updateIndex(str, "");
                    }

                    if (info->NextEntryOffset == 0) break;
                    info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                            reinterpret_cast<char*>(info) + info->NextEntryOffset);
                } while (info);
            } else {
                cerr << "ReadDirectoryChangesW failed for handle." << endl;
                break;
            }
        }

        CloseHandle(hDir);
    }
};
