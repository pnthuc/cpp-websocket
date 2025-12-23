#include "terminal.h"

void executeCommand(const std::string& command, const std::string& path, websocket::stream<tcp::socket>& ws) {
    std::string fullCommand = "cd /d \"" + path + "\" && " + command + " 2>&1 & cd";
    std::array<char, 128> buffer;
    std::string result;

    FILE* pipe = _popen(fullCommand.c_str(), "r");
    if (!pipe) return;

    auto trim_copy = [](const std::string& s) {
        size_t start = 0;
        while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
        size_t end = s.size();
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
        return s.substr(start, end - start);
    };

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) 
        result += buffer.data();

    _pclose(pipe);

    std::vector<std::string> lines;
    size_t pos = 0;
    while (pos < result.size()) {
        size_t next = result.find_first_of("\r\n", pos);
        if (next == std::string::npos) {
            lines.push_back(result.substr(pos));
            break;
        } else {
            lines.push_back(result.substr(pos, next - pos));
            size_t skip = next;
            while (skip < result.size() && (result[skip] == '\r' || result[skip] == '\n')) ++skip;
            pos = skip;
        }
    }

    int lastIdx = -1;
    std::string finalDir;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string t = trim_copy(lines[i]);
        if (!t.empty()) {
            lastIdx = static_cast<int>(i);
            finalDir = t;
        }
    }

    std::string infoResult;
    if (lastIdx >= 0) {
        for (int i = 0; i < lastIdx; ++i) {
            if (!infoResult.empty()) infoResult += "\n";
            infoResult += lines[i];
        }
    } else 
        infoResult = result; 

    if (!finalDir.empty()) 
        try {
            std::filesystem::current_path(finalDir);
        } catch (...) {}

    sendMsg(ws, "text", "Info", infoResult);
    sendMsg(ws, "text", "PATH", std::filesystem::current_path().string());
    file_logger->info("Executed command: {}", command);
    std::cerr << "Executed command: " << command << std::endl;
    return;
}