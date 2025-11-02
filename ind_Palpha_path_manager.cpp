#include "ind_Palpha_path_manager.h"
#include <filesystem>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

std::string PathManagerIndUnwrap::getExecutablePath()
{
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    fs::path exe = fs::path(path).parent_path();
    return exe.string();
#else
    return fs::current_path().string();
#endif
}

std::string PathManagerIndUnwrap::getInputDirectory()
{
    return getExecutablePath() + "/Palpha_params";
}

std::string PathManagerIndUnwrap::getOutputDirectory()
{
    return getExecutablePath() + "/Palpha_results";
}

bool PathManagerIndUnwrap::ensureDirectoriesExist()
{
    try
    {
        fs::create_directories(getInputDirectory());
        fs::create_directories(getOutputDirectory());
        return true;
    }
    catch (...)
    {
        std::cerr << "❌ Не удалось создать директории." << std::endl;
        return false;
    }
}
