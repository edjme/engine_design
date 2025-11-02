#ifndef IND_PATH_MANAGER_H
#define IND_PATH_MANAGER_H

#include <string>
#include <filesystem>

class IndPathManager
{
public:
    // Получить путь к исполняемому файлу
    static std::string getExecutablePath();

    // Папки для входных и выходных данных
    static std::string getInputDirectory();
    static std::string getOutputDirectory();

    // Проверка и автоматическое создание директорий
    static bool ensureDirectoriesExist();

#ifdef _WIN32
    // Преобразование кодировок (UTF-8 <-> Wide)
    static std::wstring utf8ToWide(const std::string &utf8_str);
    static std::string wideToUtf8(const std::wstring &wide_str);
#endif
};

#endif
