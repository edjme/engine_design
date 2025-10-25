@echo off 
chcp 65001 > nul 
title Расчет кинематики КШМ 
echo ======================================== 
echo    ПРОГРАММА РАСЧЕТА КИНЕМАТИКИ КШМ 
echo ======================================== 
echo. 
echo Автоматически создаются папки: 
echo   - Kinematic_params   (для входных параметров) 
echo   - Kinematic_results  (для результатов) 
echo. 
Kinematic_Engine_Design_v1.102.exe 
pause 
