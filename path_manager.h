#ifndef PATH_MANAGER_H
#define PATH_MANAGER_H

#include <string>
#include <filesystem>

class PathManager
{
public:
    static std::string getExecutablePath();
    static std::string getInputDirectory();
    static std::string getOutputDirectory();
    static bool ensureDirectoriesExist();

    #ifdef _WIN32
    static std::wstring utf8ToWide(const std::string& utf8_str);
    static std::string wideToUtf8(const std::wstring& wide_str);
#endif
};

#endif