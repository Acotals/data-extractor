#pragma once
#include <string>
namespace Core {
class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void Log(const std::string& message) = 0;
    virtual void LogDebug(const std::string& message) = 0;
    virtual bool IsValid() const = 0;
};
} // namespace Core