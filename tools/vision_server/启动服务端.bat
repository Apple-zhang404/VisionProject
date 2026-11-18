@echo off
chcp 65001 >nul
title vision_server  (port 8888)
cd /d "%~dp0"
echo ================================================
echo   vision_server  -  TCP 8888
echo   type text + Enter  -> broadcast to clients
echo   type quit         -> exit
echo ================================================
echo.
vision_server.exe %*
echo.
echo server exited
pause