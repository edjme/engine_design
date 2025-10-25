@echo off
chcp 65001 > nul
title Создание дистрибутива программы
echo ========================================
echo    СОЗДАНИЕ ДИСТРИБУТИВА КИНЕМАТИКИ КШМ
echo ========================================
echo.

if exist "Engine_Design_v1.0" (
    echo Удаление старой версии...
    rmdir /s /q "Engine_Design_v1.0"
)

echo Создание папки дистрибутива...
mkdir "Engine_Design_v1.0"

echo.
echo Компиляция программы...
echo -----------------------
g++ -std=c++20 -O2 -s -mconsole ^
Kinematic/Calculations/kinematic/calc_kinematic.cpp ^
Kinematic/input_data/kinematic_input.cpp ^
Kinematic/output_data/kinematic_output.cpp ^
common/common_types.cpp ^
path_manager.cpp ^
-I. -ICalculations -o "Engine_Design_v1.0\kinematic_release.exe" ^
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
echo @echo off > "Engine_Design_v1.0\run.bat"
echo chcp 65001 ^> nul >> "Engine_Design_v1.0\run.bat"
echo title Расчет кинематики КШМ >> "Engine_Design_v1.0\run.bat"
echo echo ======================================== >> "Engine_Design_v1.0\run.bat"
echo echo    ПРОГРАММА РАСЧЕТА КИНЕМАТИКИ КШМ >> "Engine_Design_v1.0\run.bat"
echo echo ======================================== >> "Engine_Design_v1.0\run.bat"
echo echo. >> "Engine_Design_v1.0\run.bat"
echo echo Автоматически создаются папки: >> "Engine_Design_v1.0\run.bat"
echo echo   - Kinematic_params   (для входных параметров) >> "Engine_Design_v1.0\run.bat"
echo echo   - Kinematic_results  (для результатов) >> "Engine_Design_v1.0\run.bat"
echo echo. >> "Engine_Design_v1.0\run.bat"
echo kinematic_release.exe >> "Engine_Design_v1.0\run.bat"
echo pause >> "Engine_Design_v1.0\run.bat"
echo ✓ Создан run.bat

:: README.txt - расширенная инструкция
echo ПРОГРАММА ДЛЯ РАСЧЕТА КИНЕМАТИКИ КШМ > "Engine_Design_v1.0\README.txt"
echo Версия 1.0 >> "Engine_Design_v1.0\README.txt"
echo. >> "Engine_Design_v1.0\README.txt"
echo ================================= >> "Engine_Design_v1.0\README.txt"
echo КАК ИСПОЛЬЗОВАТЬ: >> "Engine_Design_v1.0\README.txt"
echo ================================= >> "Engine_Design_v1.0\README.txt"
echo 1. Запустите run.bat >> "Engine_Design_v1.0\README.txt"
echo 2. Следуйте инструкциям в программе >> "Engine_Design_v1.0\README.txt"
echo 3. Результаты сохранятся в папку Kinematic_results >> "Engine_Design_v1.0\README.txt"
echo. >> "Engine_Design_v1.0\README.txt"
echo ================================ >> "Engine_Design_v1.0\README.txt"
echo ПОДДЕРЖИВАЕМЫЕ ТИПЫ КШМ: >> "Engine_Design_v1.0\README.txt"
echo ================================ >> "Engine_Design_v1.0\README.txt"
echo 1. Аксиальный КШМ >> "Engine_Design_v1.0\README.txt"
echo 2. Дезаксиальный КШМ >> "Engine_Design_v1.0\README.txt"
echo 3. V-образный с рядом сидящими шатунами >> "Engine_Design_v1.0\README.txt"
echo 4. V-образный с прицепным шатуном >> "Engine_Design_v1.0\README.txt"
echo. >> "Engine_Design_v1.0\README.txt"
echo ================================ >> "Engine_Design_v1.0\README.txt"
echo ФАЙЛЫ И ПАПКИ: >> "Engine_Design_v1.0\README.txt"
echo ================================ >> "Engine_Design_v1.0\README.txt"
echo Программа автоматически создает: >> "Engine_Design_v1.0\README.txt"
echo - Kinematic_params/   для входных параметров >> "Engine_Design_v1.0\README.txt"
echo - Kinematic_results/  для результатов расчетов >> "Engine_Design_v1.0\README.txt"
echo. >> "Engine_Design_v1.0\README.txt"
echo ================================ >> "Engine_Design_v1.0\README.txt"
echo СИСТЕМНЫЕ ТРЕБОВАНИЯ: >> "Engine_Design_v1.0\README.txt"
echo ================================ >> "Engine_Design_v1.0\README.txt"
echo - Windows 10 или новее >> "Engine_Design_v1.0\README.txt"
echo - Не требует установки дополнительных библиотек >> "Engine_Design_v1.0\README.txt"
echo - Автономный исполняемый файл >> "Engine_Design_v1.0\README.txt"
echo. >> "Engine_Design_v1.0\README.txt"
echo ✓ Создан README.txt

:: Examples - только текстовый пример (без CSV)
mkdir "Engine_Design_v1.0\Examples" >nul 2>&1

:: Дополнительный пример - текстовый конфиг
echo ПАРАМЕТРЫ РАСЧЕТА КШМ > "Engine_Design_v1.0\Examples\sample_config.txt"
echo ===================== >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo Шаг угла: 1.0 град. >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo Предел угла: 360.0 град. >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo Ход поршня: 0.085 м >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo λ: 0.3 >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo Частота вращения: 4800 об/мин >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo. >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo Для V-образных двигателей: >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo Угол развала: 90.0 град. >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo Угол прицепного шатуна: 45.0 град. >> "Engine_Design_v1.0\Examples\sample_config.txt"
echo ✓ Создан Examples/sample_config.txt

echo.
echo ========================================
echo ДИСТРИБУТИВ УСПЕШНО СОЗДАН!
echo ========================================
echo.
echo Содержимое папки Engine_Design_v1.0:
echo.
echo   📁 Engine_Design_v1.0/
echo   ├── 🚀 kinematic_release.exe     (основная программа)
echo   ├── ⚡ run.bat           (скрипт запуска)
echo   ├── 📖 README.txt        (подробная инструкция)
echo   └── 📁 Examples/         (примеры файлов)
echo       └── sample_config.txt (пример текстового конфига)
echo.
echo КАК ИСПОЛЬЗОВАТЬ:
echo   1. Скопируйте папку Engine_Design_v1.0
echo   2. Запустите run.bat
echo   3. Следуйте инструкциям в программе
echo.
echo ✅ Программа готова к распространению!
echo ✅ Работает на Windows 10+ без зависимостей
echo.
echo Нажмите любую клавишу для выхода...
pause >nul