#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include "common/common_types.h"
#include <locale>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

int main()
{

    SetConsoleOutputCP(CP_UTF8);
    system("chcp 65001 > nul");
}