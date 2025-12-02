# Script de test pour l'application de messagerie
# Pour Windows PowerShell

Write-Host "=== COMPILATION DE L'APPLICATION ===" -ForegroundColor Cyan

# Compilation
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur.exe -lws2_32
if ($LASTEXITCODE -ne 0) {
    Write-Host "Erreur de compilation du serveur" -ForegroundColor Red
    exit 1
}

g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client.exe -lws2_32
if ($LASTEXITCODE -ne 0) {
    Write-Host "Erreur de compilation du client" -ForegroundColor Red
    exit 1
}

Write-Host "Compilation réussie!" -ForegroundColor Green
Write-Host ""

Write-Host "=== INSTRUCTIONS DE TEST ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. Ouvrez un terminal et lancez le serveur:" -ForegroundColor Yellow
Write-Host "   .\serveur.exe" -ForegroundColor White
Write-Host ""
Write-Host "2. Ouvrez 2-3 autres terminaux et lancez des clients:" -ForegroundColor Yellow
Write-Host "   .\client.exe" -ForegroundColor White
Write-Host ""
Write-Host "3. Pour chaque client, entrez un nom d'utilisateur différent:" -ForegroundColor Yellow
Write-Host "   - Alice" -ForegroundColor White
Write-Host "   - Bob" -ForegroundColor White
Write-Host "   - Charlie" -ForegroundColor White
Write-Host ""
Write-Host "4. Testez les fonctionnalités:" -ForegroundColor Yellow
Write-Host "   [1] Lister les messages" -ForegroundColor White
Write-Host "   [4] Composer un message (essayez broadcast avec 'all')" -ForegroundColor White
Write-Host "   [5] Lister les utilisateurs en ligne" -ForegroundColor White
Write-Host "   [6] Récupérer le log du serveur" -ForegroundColor White
Write-Host ""
Write-Host "5. Attendez 30 secondes pour voir la livraison des messages" -ForegroundColor Yellow
Write-Host ""
Write-Host "6. Déconnectez tous les clients pour arrêter le serveur automatiquement" -ForegroundColor Yellow
Write-Host ""
Write-Host "=== FICHIERS GÉNÉRÉS ===" -ForegroundColor Cyan
Write-Host "- serveur.exe : Exécutable du serveur" -ForegroundColor White
Write-Host "- client.exe  : Exécutable du client" -ForegroundColor White
Write-Host "- server.log  : Log du serveur (créé au démarrage)" -ForegroundColor White
Write-Host ""
