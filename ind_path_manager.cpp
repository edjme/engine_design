#include "ind_path_manager.h"
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

std::string IndPathManager::getExecutablePath()
{
    try
    {
#ifdef _WIN32
        wchar_t path[MAX_PATH];
        DWORD result = GetModuleFileNameW(NULL, path, MAX_PATH);
        if (result == 0)
        {
            std::cerr << "GetModuleFileNameW failed" << std::endl;
            return "";
        }

        std::string utf8_path = wideToUtf8(path);
        fs::path exe_path = fs::path(utf8_path).parent_path();
        return exe_path.string();
#else
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        if (count == -1)
        {
            return "";
        }
        return fs::path(std::string(path, count)).parent_path().string();
#endif
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in getExecutablePath: " << e.what() << std::endl;
        return "";
    }
}

std::string IndPathManager::getInputDirectory()
{
    std::string exe_path = getExecutablePath();
    if (exe_path.empty())
    {
        return "ind_params";
    }
    return (fs::path(exe_path) / "ind_params").string();
}

std::string IndPathManager::getOutputDirectory()
{
    std::string exe_path = getExecutablePath();
    if (exe_path.empty())
    {
        return "ind_results";
    }
    return (fs::path(exe_path) / "ind_results").string();
}

bool IndPathManager::ensureDirectoriesExist()
{
    try
    {
        std::string input_dir = getInputDirectory();
        std::string output_dir = getOutputDirectory();

        std::cout << "Проверка и создание директорий:\n";
        std::cout << "  " << input_dir << "\n";
        std::cout << "  " << output_dir << "\n";

#ifdef _WIN32
        std::wstring win_input_dir = utf8ToWide(input_dir);
        std::wstring win_output_dir = utf8ToWide(output_dir);

        bool input_created = CreateDirectoryW(win_input_dir.c_str(), NULL) ||
                             GetLastError() == ERROR_ALREADY_EXISTS;
        bool output_created = CreateDirectoryW(win_output_dir.c_str(), NULL) ||
                              GetLastError() == ERROR_ALREADY_EXISTS;
#else
        bool input_created = fs::create_directories(input_dir);
        bool output_created = fs::create_directories(output_dir);
#endif

        if (input_created && output_created)
        {
            std::cout << "✅ Папки ind_params и ind_results готовы.\n";
            return true;
        }
        else
        {
            std::cerr << "⚠️ Не удалось создать одну из папок.\n";
            return false;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating directories: " << e.what() << std::endl;
        return false;
    }
}

#ifdef _WIN32
std::wstring IndPathManager::utf8ToWide(const std::string &utf8_str)
{
    if (utf8_str.empty())
        return L"";

    int wide_size = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wide_size == 0)
    {
        return L"";
    }

    std::wstring wide_str(wide_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], wide_size);

    if (!wide_str.empty() && wide_str.back() == L'\0')
    {
        wide_str.pop_back();
    }

    return wide_str;
}

std::string IndPathManager::wideToUtf8(const std::wstring &wide_str)
{
    if (wide_str.empty())
        return "";

    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_size == 0)
    {
        return "";
    }

    std::string utf8_str(utf8_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_size, nullptr, nullptr);

    if (!utf8_str.empty() && utf8_str.back() == '\0')
    {
        utf8_str.pop_back();
    }

    return utf8_str;
}
#endif
