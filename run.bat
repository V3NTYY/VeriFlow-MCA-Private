@echo off
set /p args=Enter arguments: 
cd MCA_VeriFlow\out\build\x64-debug
MCA_VeriFlow.exe %args%
pause