#include "path_manager.h"
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

std::string PathManager::getExecutablePath() {
    try {
#ifdef _WIN32
        // Получаем путь в широких символах (поддерживает кириллицу)
        wchar_t path[MAX_PATH];
        DWORD result = GetModuleFileNameW(NULL, path, MAX_PATH);
        if (result == 0) {
            std::cerr << "GetModuleFileNameW failed" << std::endl;
            return "";
        }

        // Конвертируем в UTF-8 для кроссплатформенности
        std::string utf8_path = wideToUtf8(path);

        fs::path exe_path = fs::path(utf8_path).parent_path();
        return exe_path.string();

#else
        // Linux реализация
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        if (count == -1) {
            return "";
        }
        return fs::path(std::string(path, count)).parent_path().string();
#endif
    } catch (const std::exception& e) {
        std::cerr << "Error in getExecutablePath: " << e.what() << std::endl;
        return "";
    }
}

std::string PathManager::getInputDirectory() {
    std::string exe_path = getExecutablePath();
    if (exe_path.empty()) {
        return "Kinematic_params";
    }
    return (fs::path(exe_path) / "Kinematic_params").string();
}

std::string PathManager::getOutputDirectory() {
    std::string exe_path = getExecutablePath();
    if (exe_path.empty()) {
        return "Kinematic_results";
    }
    return (fs::path(exe_path) / "Kinematic_results").string();
}

bool PathManager::ensureDirectoriesExist() {
    try {
        std::string input_dir = getInputDirectory();
        std::string output_dir = getOutputDirectory();

        std::cout << "Creating directory: " << input_dir << std::endl;
        std::cout << "Creating directory: " << output_dir << std::endl;

#ifdef _WIN32
        // Для Windows используем широкие символы для создания директорий
        std::wstring win_input_dir = utf8ToWide(input_dir);
        std::wstring win_output_dir = utf8ToWide(output_dir);

        bool input_created = CreateDirectoryW(win_input_dir.c_str(), NULL) ||
                            GetLastError() == ERROR_ALREADY_EXISTS;
        bool output_created = CreateDirectoryW(win_output_dir.c_str(), NULL) ||
                             GetLastError() == ERROR_ALREADY_EXISTS;
#else
        // Для Linux используем стандартные пути
        bool input_created = fs::create_directories(input_dir);
        bool output_created = fs::create_directories(output_dir);
#endif

        if (input_created && output_created) {
            std::cout << "Directories created successfully" << std::endl;
            return true;
        } else {
            std::cerr << "Failed to create directories" << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error creating directories: " << e.what() << std::endl;
        return false;
    }
}

#ifdef _WIN32
std::wstring PathManager::utf8ToWide(const std::string& utf8_str) {
    if (utf8_str.empty()) return L"";

    int wide_size = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wide_size == 0) {
        return L"";
    }

    std::wstring wide_str(wide_size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wide_str[0], wide_size);

    // Убираем нулевой символ
    if (!wide_str.empty() && wide_str.back() == L'\0') {
        wide_str.pop_back();
    }

    return wide_str;
}

std::string PathManager::wideToUtf8(const std::wstring& wide_str) {
    if (wide_str.empty()) return "";

    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_size == 0) {
        return "";
    }

    std::string utf8_str(utf8_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_size, nullptr, nullptr);

    // Убираем нулевой символ
    if (!utf8_str.empty() && utf8_str.back() == '\0') {
        utf8_str.pop_back();
    }

    return utf8_str;
}
#endif