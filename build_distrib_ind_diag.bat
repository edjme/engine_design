@echo off
chcp 65001 > nul
title Создание дистрибутива программы
echo ========================================================
echo    СОЗДАНИЕ ДИСТРИБУТИВА РАСЧЕТА ИНДИКАТОРНОЙ ДИАГРАММЫ
echo ========================================================
echo.

if exist "Ind_Diagr_v1.00" (
    echo Удаление старой версии...
    rmdir /s /q "Ind_Diagr_v1.00"
)

echo Создание папки дистрибутива...
mkdir "Ind_Diagr_v1.00"

echo.
echo Компиляция программы...
echo -----------------------
g++ -std=c++20 -O2 -s -mconsole ^
core/IND_Diagr/calculations/ind_diagr_main.cpp ^
core/IND_Diagr/calculations/calc_ind_diagr.cpp ^
core/IND_Diagr/input_data/ind_diag_input.cpp ^
core/IND_Diagr/output_data/ind_diag_output.cpp ^
core/common/ind_diag_common_types.cpp ^
ind_path_manager.cpp ^
-I. -ICalculations -o "Ind_Diagr_v1.00\Ind_Diagr_v1.00.exe" ^
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
echo @echo off > "Ind_Diagr_v1.00\run.bat"
echo chcp 65001 ^> nul >> "Ind_Diagr_v1.00\run.bat"
echo title Расчет индикаторной диаграммы >> "Ind_Diagr_v1.00\run.bat"
echo echo ======================================== >> "Ind_Diagr_v1.00\run.bat"
echo echo    ПРОГРАММА РАСЧЕТА ИНДИКАТОРНОЙ ДИАГРАММЫ >> "Ind_Diagr_v1.00\run.bat"
echo echo ======================================== >> "Ind_Diagr_v1.00\run.bat"
echo echo. >> "Ind_Diagr_v1.00\run.bat"
echo echo Автоматически создаются папки: >> "Ind_Diagr_v1.00\run.bat"
echo echo   - ind_params   (для входных параметров) >> "Ind_Diagr_v1.00\run.bat"
echo echo   - ind_results  (для результатов) >> "Ind_Diagr_v1.00\run.bat"
echo echo. >> "Ind_Diagr_v1.00\run.bat"
echo Ind_Diagr_v1.00.exe >> "Ind_Diagr_v1.00\run.bat"
echo pause >> "Ind_Diagr_v1.00\run.bat"
echo ✓ Создан run.bat

:: README.txt - расширенная инструкция
echo ПРОГРАММА ДЛЯ РАСЧЕТА ИНДИКАТОРНОЙ ДИАГРАММЫ > "Ind_Diagr_v1.00\README.txt"
echo Версия 1.00 >> "Ind_Diagr_v1.00\README.txt"
echo. >> "Ind_Diagr_v1.00\README.txt"
echo ================================= >> "Ind_Diagr_v1.00\README.txt"
echo КАК ИСПОЛЬЗОВАТЬ: >> "Ind_Diagr_v1.00\README.txt"
echo ================================= >> "Ind_Diagr_v1.00\README.txt"
echo 1. Запустите run.bat или сразу Ind_Diagr_v1.00.exe>> "Ind_Diagr_v1.00\README.txt"
echo 2. Следуйте инструкциям в программе (run.bat) или в help >> "Ind_Diagr_v1.00\README.txt"
echo 3. Результаты сохранятся в папку ind_results >> "Ind_Diagr_v1.00\README.txt"
echo. >> "Ind_Diagr_v1.00\README.txt"
echo ================================ >> "Ind_Diagr_v1.00\README.txt"
echo ================================ >> "Ind_Diagr_v1.00\README.txt"
echo ФАЙЛЫ И ПАПКИ: >> "Ind_Diagr_v1.00\README.txt"
echo ================================ >> "Ind_Diagr_v1.00\README.txt"
echo Программа автоматически создает: >> "Ind_Diagr_v1.00\README.txt"
echo - ind_params/   для входных параметров >> "Ind_Diagr_v1.00\README.txt"
echo - ind_results/  для результатов расчетов >> "Ind_Diagr_v1.00\README.txt"
echo. >> "Ind_Diagr_v1.00\README.txt"
echo ================================ >> "Ind_Diagr_v1.00\README.txt"
echo СИСТЕМНЫЕ ТРЕБОВАНИЯ: >> "Ind_Diagr_v1.00\README.txt"
echo ================================ >> "Ind_Diagr_v1.00\README.txt"
echo - Windows 10 или новее >> "Ind_Diagr_v1.00\README.txt"
echo - Не требует установки дополнительных библиотек >> "Ind_Diagr_v1.00\README.txt"
echo - Автономный исполняемый файл >> "Ind_Diagr_v1.00\README.txt"
echo. >> "Ind_Diagr_v1.00\README.txt"
echo ✓ Создан README.txt


echo.
echo ========================================
echo ДИСТРИБУТИВ УСПЕШНО СОЗДАН!
echo ========================================
echo.
echo Содержимое папки Ind_Diagr_v1.00:
echo.
echo   📁 Ind_Diagr_v1.00/
echo   ├── 🚀 Ind_Diagr_v1.00.exe     (основная программа)
echo   ├── ⚡ run.bat           (скрипт запуска)
echo   └── 📖 README.txt        (подробная инструкция)
echo.
echo КАК ИСПОЛЬЗОВАТЬ:
echo   1. Скопируйте папку Ind_Diagr_v1.00
echo   2. Запустите run.bat
echo   3. Следуйте инструкциям в программе
echo.
echo ✅ Программа готова к распространению!
echo ✅ Работает на Windows 10+ без зависимостей
echo.
echo Нажмите любую клавишу для выхода...
pause >nul