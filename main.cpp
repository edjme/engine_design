#include <iostream>
#include <windows.h>

using namespace std;
int input();

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    system("chcp 65001 > nul");

    cout << "Hello \n";
    input();
    return 0;
}