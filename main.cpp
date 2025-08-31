#ifdef _WIN32
#include <windows.h>
#include "Interface/gui_win.h"

// точка входа GUI
int wmain()
{ // или int main(), если собираешь как консольное
    SetConsoleOutputCP(65001);
    return run_gui(); // полностью передаём управление GUI
}

#endif
