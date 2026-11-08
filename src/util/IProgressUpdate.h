#pragma once

// Original: net/minecraft/util/IProgressUpdate.java

#include <string>

namespace mc {

class IProgressUpdate {
public:
    virtual ~IProgressUpdate() = default;
    virtual void displaySavingString(const std::string& message) = 0;
    virtual void resetProgressAndMessage(const std::string& message) = 0;
    virtual void displayLoadingString(const std::string& message) = 0;
    virtual void setLoadingProgress(int progress) = 0;
    virtual void setDoneWorking() = 0;
};

} // namespace mc
