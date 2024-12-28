#undef byte

#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <regex>
#include "file_manager.cpp"
#include "indexer.cpp"
#include "thread_pool.cpp"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 8080


SOCKET createServerSocket() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed." << endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        cerr << "Listen failed." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    cout << "Server is running on http://localhost:" << PORT << endl;
    return serverSocket;
}

string parseRequestParam(const string& request, const string& paramName) {
    size_t startPos = request.find(paramName + "=");
    if (startPos == string::npos) return "";
    startPos += paramName.size() + 1;
    size_t endPos = request.find('&', startPos);
    if (endPos == string::npos) endPos = request.find(' ', startPos);
    return request.substr(startPos, endPos - startPos);
}

pair<string, string> parsePostData(const string& request) {
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos == string::npos) return {"", ""};
    string body = request.substr(bodyPos + 4);

    size_t fileNamePos = body.find("file_name=");
    size_t fileContentPos = body.find("file_content=");
    if (fileNamePos == string::npos || fileContentPos == string::npos) return {"", ""};

    fileNamePos += 10;
    size_t fileNameEnd = body.find('&', fileNamePos);
    string fileName = body.substr(fileNamePos, fileNameEnd - fileNamePos);

    fileContentPos += 13;
    string fileContent = body.substr(fileContentPos);

    return {fileName, fileContent};
}

void handleRequest(SOCKET clientSocket, FileManager& fileManager, Indexer& indexer) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, 4096, 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }

    string request(buffer, bytesReceived);
    string httpResponse;

    bool isConsoleRequest = request.find("User-Agent") == string::npos;

    if (request.find("GET / ") != string::npos || request.find("GET /index.html") != string::npos) {
        string htmlContent = R"(
<!DOCTYPE html>
<html>
<head>
    <title>File Manager</title>
    <link href="https://fonts.googleapis.com/css2?family=Quicksand:wght@300;400;500;600&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Quicksand', sans-serif;
            background-color: #ffe4e1;
            color: #444;
            margin: 0;
            padding: 20px;
        }
        h1 {
            text-align: center;
            color: #c71585;
            font-size: 2.5em;
        }
        form {
            max-width: 500px;
            margin: 20px auto;
            padding: 20px;
            border: 2px solid #ffa07a;
            border-radius: 10px;
            background-color: #fffafa;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
        }
        label {
            font-weight: bold;
            display: block;
            margin-top: 15px;
            color: #8b008b;
        }
        input[type="text"], textarea {
            width: 100%;
            padding: 12px;
            margin-top: 8px;
            margin-bottom: 20px;
            border: 1px solid #dcdcdc;
            border-radius: 8px;
            background-color: #fffaf0;
            box-sizing: border-box;
        }
        input[type="submit"] {
            background-color: #db7093;
            color: white;
            padding: 12px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 1em;
            width: 100%;
        }
        input[type="submit"]:hover {
            background-color: #c71585;
        }
        hr {
            margin: 20px 0;
            border: 0;
            border-top: 1px solid transparent;
        }
    </style>
</head>
<body>
    <h1>Welcome to File Manager</h1>
    <form id="search-form" method="GET" action="/search">
        <label for="word">Search Word:</label>
        <input type="text" id="word" name="word">
        <input type="submit" value="Search">
    </form>
    <hr>
    <form id="add-file-form" method="POST" action="/add_file">
        <label for="file_name">File Name:</label>
        <input type="text" id="file_name" name="file_name">
        <label for="file_content">File Content:</label>
        <textarea id="file_content" name="file_content" rows="4"></textarea>
        <input type="submit" value="Add File">
    </form>
    <script>
        document.getElementById('search-form').onsubmit = function(event) {
            setTimeout(() => {
                document.getElementById('word').value = '';
            }, 10);
        };
        document.getElementById('add-file-form').onsubmit = function(event) {
            setTimeout(() => {
                document.getElementById('file_name').value = '';
                document.getElementById('file_content').value = '';
            }, 10);
        };
    </script>
</body>
</html>
)";



        httpResponse = "HTTP/1.1 200 OK\r\nContent-Length: " + to_string(htmlContent.size()) +
                       "\r\nContent-Type: text/html\r\n\r\n" + htmlContent;
    } else if(request.find("GET /search") != string::npos) {
        string searchWord = parseRequestParam(request, "word");
        auto results = indexer.search(searchWord);

        if (isConsoleRequest) {
            stringstream response;
            response << "Results for '" << searchWord << "':\n";
            if (results.empty()) {
                response << "No results found.\n";
            } else {
                for (const auto &file: results) {
                    filesystem::path filePath(file);
                    response << "- " << filePath.filename().string() << "\n";
                }
            }
            httpResponse = response.str();
        } else {
            stringstream resultHtml;
            resultHtml << "<html><head><title>Search Results</title><style>\n"
                          "body { font-family: 'Quicksand', sans-serif; background-color: #ffe4e1; margin: 0; padding: 20px; color: #444; }\n"
                          "h1 { text-align: center; color: #c71585; }\n"
                          "ul { list-style: none; padding: 0; }\n"
                          "li { background: #fffafa; margin: 5px 0; padding: 10px; border: 1px solid #ffa07a; border-radius: 8px; }\n"
                          "button { position: absolute; top: 20px; left: 20px; background-color: #db7093; color: white; padding: 10px 20px; border: none; border-radius: 8px; cursor: pointer; font-size: 1em; }\n"
                          "button:hover { background-color: #c71585; }\n"
                          "</style></head><body>"
                          "<button onclick=\"window.location.href='/'\">To Main</button>"
                          "<h1>Results for '" << searchWord << "':</h1><ul>";

            if (results.empty()) {
                resultHtml << "<li>No results found.</li>";
            } else {
                for (const auto &file: results) {
                    filesystem::path filePath(file);
                    resultHtml << "<li>" << filePath.filename().string() << "</li>";
                }
            }
            resultHtml << "</ul></body></html>";

            httpResponse = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(resultHtml.str().size()) +
                           "\r\nContent-Type: text/html\r\n\r\n" + resultHtml.str();
        }
    }
    else if (request.find("POST /add_file") != string::npos) {
        auto [fileName, fileContent] = parsePostData(request);
        replace(fileName.begin(), fileName.end(), '+', ' ');
        replace(fileContent.begin(), fileContent.end(), '+', ' ');

        if (fileName.empty() || fileContent.empty()) {
            if (isConsoleRequest) {
                httpResponse = "Error: Invalid file data. Please provide a valid file name and content.\n";
            }else {
                httpResponse = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n"
                               "<html><head><title>Error</title><style>\n"
                               "body { font-family: 'Quicksand', sans-serif; background-color: #ffe4e1; color: #444; margin: 0; padding: 20px; }\n"
                               "h1 { color: #c71585; text-align: center; }\n"
                               "p { text-align: center; font-size: 1.2em; }\n"
                               "button { position: absolute; top: 20px; left: 20px; background-color: #db7093; color: white; padding: 10px 20px; border: none; border-radius: 8px; cursor: pointer; font-size: 1em; }\n"
                               "button:hover { background-color: #c71585; }\n"
                               "</style></head><body>"
                               "<button onclick=\"window.location.href='/'\">To Main</button>"
                               "</style></head><body><h1>Invalid File Data</h1><p>Please provide valid file name and content.</p></body></html>";
            }
        } else {
            if (fileName.find(".txt") == string::npos) {
                fileName += ".txt";
            }

            fileManager.saveFileContent(fileName, fileContent);
            if (isConsoleRequest){
                httpResponse = "File '" + fileName + "' added successfully with content:\n" + fileContent + "\n";
            }
            else {
                httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
                               "<html><head><title>Success</title><style>\n"
                               "body { font-family: 'Quicksand', sans-serif; background-color: #ffe4e1; color: #444; margin: 0; padding: 20px; }\n"
                               "h1 { color: #c71585; text-align: center; }\n"
                               "p { text-align: center; font-size: 1.2em; }\n"
                               "button { position: absolute; top: 20px; left: 20px; background-color: #db7093; color: white; padding: 10px 20px; border: none; border-radius: 8px; cursor: pointer; font-size: 1em; }\n"
                               "button:hover { background-color: #c71585; }\n"
                               "</style></head><body>"
                               "<button onclick=\"window.location.href='/'\">To Main</button>"
                               "</style></head><body><h1>File Added</h1><p>File '" + fileName +
                               "' has been successfully added and the index updated.</p></body></html>";
            }
        }
    }
    else {
        httpResponse = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nPage not found";
    }

    send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0);
    closesocket(clientSocket);
}

int main() {
    FileManager fileManager;
    Indexer indexer;
    ThreadPool pool(4);

    vector<string> directories = {
            "D:/kpi/coursework-pc/course_pc/aclImdb/custom",
            "D:/kpi/coursework-pc/course_pc/aclImdb/test/neg",
            "D:/kpi/coursework-pc/course_pc/aclImdb/test/pos",
            "D:/kpi/coursework-pc/course_pc/aclImdb/train/neg",
            "D:/kpi/coursework-pc/course_pc/aclImdb/train/pos",
            "D:/kpi/coursework-pc/course_pc/aclImdb/train/unsup"
    };
    const wstring &directory = L"D:/kpi/coursework-pc/course_pc/aclImdb/";

    vector<pair<int, int>> ranges = {
            {1041, 1250}, {1041, 1250}, {1041, 1250}, {1041, 1250}, {4166, 5000}
    };

    for (size_t i = 0; i < directories.size(); ++i) {
        pool.enqueue([&, i] {
            auto files = fileManager.readFiles(directories[i], ranges[i].first, ranges[i].second);
            if (files.empty()) {
                cout << "No files found in directory: " << directories[i] << endl;
                return;
            }

            for (const auto& file : files) {
                if (regex_match(file, regex(".*\\.txt$", regex::icase))) {
                    string content = fileManager.readFileContent(file);
                    indexer.addFileToIndex(file, content);
                } else {
                    cout << "Skipped non-txt file: " << file << endl;
                }
            }
        });
    }

    thread directoryMonitorThread([&indexer, directory]() {
        indexer.MonitorDeletions(directory);
    });
    directoryMonitorThread.detach();

    SOCKET serverSocket = createServerSocket();
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Failed to start server." << endl;
        return 1;
    }

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            cout << "Accept failed\n";
            continue;
        }
        pool.enqueue([clientSocket, &fileManager, &indexer]() {
            handleRequest(clientSocket, fileManager, indexer);
        });
    }

    pool.~ThreadPool();
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
