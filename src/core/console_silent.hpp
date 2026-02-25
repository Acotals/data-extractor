#pragma once
#include <string>
namespace Core {
    class Console {
    public:
        Console(bool verbose = false) {}
        // All methods do nothing
        void BrowserHeader(const std::string&, const std::string&) const {}
        void Debug(const std::string&) const {}
        void Warn(const std::string&) const {}
        void Error(const std::string&) const {}
        void ProfileHeader(const std::string&) const {}
        void KeyDecrypted(const std::string&) const {}
        void NoAbeWarning(const std::string&) const {}
        void AsterKeyDecrypted(const std::string&) const {}
        void ExtractionResult(const std::string&, int, int = 0) const {}
        void DataRow(const std::string&, const std::string&) const {}
        void Summary(int, int, int, int, int, int, const std::string&) const {}
    };
}