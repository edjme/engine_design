#ifndef IND_PALPHA_PATH_MANAGER_H
#define IND_PALPHA_PATH_MANAGER_H

#include <string>

class PathManagerIndUnwrap
{
public:
    static std::string getExecutablePath();
    static std::string getInputDirectory();
    static std::string getOutputDirectory();
    static bool ensureDirectoriesExist();
};

#endif
