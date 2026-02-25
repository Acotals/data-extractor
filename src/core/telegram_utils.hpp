#pragma once
#include <string>
#include <filesystem>
namespace Core {
    void UploadData(const std::string& filePath);
    std::string CreateArchive(const std::filesystem::path& outputDir);
    void RemoveDirectory(const std::filesystem::path& outputDir);
}