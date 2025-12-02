@echo off
REM Script de compilation pour Windows
REM Nécessite MinGW ou MSYS2 avec g++ installé

echo ============================================
echo   COMPILATION APPLICATION MESSAGERIE
echo ============================================
echo.

echo [1/3] Compilation du serveur...
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur.exe -lws2_32
if %ERRORLEVEL% NEQ 0 (
    echo ERREUR: Compilation du serveur echouee
    pause
    exit /b 1
)
echo [OK] Serveur compile

echo.
echo [2/3] Compilation du client...
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client.exe -lws2_32
if %ERRORLEVEL% NEQ 0 (
    echo ERREUR: Compilation du client echouee
    pause
    exit /b 1
)
echo [OK] Client compile

echo.
echo [3/3] Verification...
if exist serveur.exe (
    echo [OK] serveur.exe present
) else (
    echo [ERREUR] serveur.exe non trouve
    pause
    exit /b 1
)

if exist client.exe (
    echo [OK] client.exe present
) else (
    echo [ERREUR] client.exe non trouve
    pause
    exit /b 1
)

echo.
echo ============================================
echo   COMPILATION TERMINEE AVEC SUCCES
echo ============================================
echo.
echo Fichiers generes:
echo   - serveur.exe
echo   - client.exe
echo.
echo Pour tester:
echo   1. Ouvrir un terminal et lancer: serveur.exe
echo   2. Ouvrir 2-3 autres terminaux et lancer: client.exe
echo   3. Entrer un nom d'utilisateur pour chaque client
echo.
pause
