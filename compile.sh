#!/bin/bash
# Script de compilation pour Linux
# Nécessite g++ avec support C++20

echo "============================================"
echo "   COMPILATION APPLICATION MESSAGERIE"
echo "============================================"
echo ""

echo "[1/3] Compilation du serveur..."
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur
if [ $? -ne 0 ]; then
    echo "ERREUR: Compilation du serveur échouée"
    exit 1
fi
echo "[OK] Serveur compilé"

echo ""
echo "[2/3] Compilation du client..."
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client
if [ $? -ne 0 ]; then
    echo "ERREUR: Compilation du client échouée"
    exit 1
fi
echo "[OK] Client compilé"

echo ""
echo "[3/3] Vérification..."
if [ -f serveur ]; then
    echo "[OK] serveur présent"
    chmod +x serveur
else
    echo "[ERREUR] serveur non trouvé"
    exit 1
fi

if [ -f client ]; then
    echo "[OK] client présent"
    chmod +x client
else
    echo "[ERREUR] client non trouvé"
    exit 1
fi

echo ""
echo "============================================"
echo "   COMPILATION TERMINÉE AVEC SUCCÈS"
echo "============================================"
echo ""
echo "Fichiers générés:"
echo "  - serveur"
echo "  - client"
echo ""
echo "Pour tester:"
echo "  1. Ouvrir un terminal et lancer: ./serveur"
echo "  2. Ouvrir 2-3 autres terminaux et lancer: ./client"
echo "  3. Entrer un nom d'utilisateur pour chaque client"
echo ""
