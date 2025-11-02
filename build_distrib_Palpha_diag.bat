@echo off
chcp 65001 > nul
title Создание дистрибутива программы
echo ========================================================
echo    СОЗДАНИЕ ДИСТРИБУТИВА РАЗВЕРТКИ РАСЧЕТА ИНДИКАТОРНОЙ ДИАГРАММЫ
echo ========================================================
echo.

if exist "Palpha_Diagr_v1.00" (
    echo Удаление старой версии...
    rmdir /s /q "Palpha_Diagr_v1.00"
)

echo Создание папки дистрибутива...
mkdir "Palpha_Diagr_v1.00"

echo.
echo Компиляция программы...
echo -----------------------
g++ -std=c++20 -O2 -s -mconsole ^
core/Diag_Palpha/Calculations/main_Palpha.cpp ^
core/Diag_Palpha/Input_data/Palpha_input.cpp ^
core/Diag_Palpha/Output_data/Palpha_output.cpp ^
core/Diag_Palpha/Calculations/calc_Palpha.cpp ^
core/common/common_types.cpp ^
ind_Palpha_path_manager.cpp ^
-I. -ICalculations -o "Palpha_Diagr_v1.00\Palpha_Diagr_v1.00.exe" ^
-DUNICODE -D_UNICODE ^
-finput-charset=UTF-8 -fexec-charset=UTF-8 ^
-static -static-libgcc -static-libstdc++

if %errorlevel% neq 0 (
    echo.
    echo ОШИБКА: Компиляция не удалась!
    echo Проверьте наличие всех исходных файлов.
    pause
    exit /b 1
)

echo ✓ Программа успешно скомпилирована

echo Создание вспомогательных файлов...
echo ----------------------------------

:: run.bat - скрипт запуска
echo @echo off > "Palpha_Diagr_v1.00\run.bat"
echo chcp 65001 ^> nul >> "Palpha_Diagr_v1.00\run.bat"
echo title Расчет развертки индикаторной диаграммы >> "Palpha_Diagr_v1.00\run.bat"
echo echo ======================================== >> "Palpha_Diagr_v1.00\run.bat"
echo echo    ПРОГРАММА РАСЧЕТА РАЗВЕРТКИ ИНДИКАТОРНОЙ ДИАГРАММЫ >> "Palpha_Diagr_v1.00\run.bat"
echo echo ======================================== >> "Palpha_Diagr_v1.00\run.bat"
echo echo. >> "Palpha_Diagr_v1.00\run.bat"
echo echo Автоматически создаются папки: >> "Palpha_Diagr_v1.00\run.bat"
echo echo   - Palpha_params   (для входных параметров) >> "Palpha_Diagr_v1.00\run.bat"
echo echo   - Palpha_results  (для результатов) >> "Palpha_Diagr_v1.00\run.bat"
echo echo. >> "Palpha_Diagr_v1.00\run.bat"
echo Palpha_Diagr_v1.00.exe >> "Palpha_Diagr_v1.00\run.bat"
echo pause >> "Palpha_Diagr_v1.00\run.bat"
echo ✓ Создан run.bat

:: README.txt - расширенная инструкция
echo ПРОГРАММА ДЛЯ РАСЧЕТА РАЗВЕРТКИ ИНДИКАТОРНОЙ ДИАГРАММЫ > "Palpha_Diagr_v1.00\README.txt"
echo Версия 1.00 >> "Palpha_Diagr_v1.00\README.txt"
echo. >> "Palpha_Diagr_v1.00\README.txt"
echo ================================= >> "Palpha_Diagr_v1.00\README.txt"
echo КАК ИСПОЛЬЗОВАТЬ: >> "Palpha_Diagr_v1.00\README.txt"
echo ================================= >> "Palpha_Diagr_v1.00\README.txt"
echo 1. Запустите run.bat или сразу Palpha_Diagr_v1.00.exe>> "Palpha_Diagr_v1.00\README.txt"
echo 2. Следуйте инструкциям в программе (run.bat) или в help >> "Palpha_Diagr_v1.00\README.txt"
echo 3. Результаты сохранятся в папку Palpha_results >> "Palpha_Diagr_v1.00\README.txt"
echo. >> "Palpha_Diagr_v1.00\README.txt"
echo ================================ >> "Palpha_Diagr_v1.00\README.txt"
echo ================================ >> "Palpha_Diagr_v1.00\README.txt"
echo ФАЙЛЫ И ПАПКИ: >> "Palpha_Diagr_v1.00\README.txt"
echo ================================ >> "Palpha_Diagr_v1.00\README.txt"
echo Программа автоматически создает: >> "Palpha_Diagr_v1.00\README.txt"
echo - Palpha_params/   для входных параметров >> "Palpha_Diagr_v1.00\README.txt"
echo - Palpha_results/  для результатов расчетов >> "Palpha_Diagr_v1.00\README.txt"
echo. >> "Palpha_Diagr_v1.00\README.txt"
echo ================================ >> "Palpha_Diagr_v1.00\README.txt"
echo СИСТЕМНЫЕ ТРЕБОВАНИЯ: >> "Palpha_Diagr_v1.00\README.txt"
echo ================================ >> "Palpha_Diagr_v1.00\README.txt"
echo - Windows 10 или новее >> "Palpha_Diagr_v1.00\README.txt"
echo - Не требует установки дополнительных библиотек >> "Palpha_Diagr_v1.00\README.txt"
echo - Автономный исполняемый файл >> "Palpha_Diagr_v1.00\README.txt"
echo. >> "Palpha_Diagr_v1.00\README.txt"
echo ✓ Создан README.txt


echo.
echo ========================================
echo ДИСТРИБУТИВ УСПЕШНО СОЗДАН!
echo ========================================
echo.
echo Содержимое папки Palpha_Diagr_v1.00:
echo.
echo   📁 Palpha_Diagr_v1.00/
echo   ├── 🚀 Palpha_Diagr_v1.00.exe     (основная программа)
echo   ├── ⚡ run.bat           (скрипт запуска)
echo   └── 📖 README.txt        (подробная инструкция)
echo.
echo КАК ИСПОЛЬЗОВАТЬ:
echo   1. Скопируйте папку Palpha_Diagr_v1.00
echo   2. Запустите run.bat
echo   3. Следуйте инструкциям в программе
echo.
echo ✅ Программа готова к распространению!
echo ✅ Работает на Windows 10+ без зависимостей
echo.
echo Нажмите любую клавишу для выхода...
pause >nul