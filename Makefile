# =============================================================================
# Makefile - Application de messagerie instantanée client/serveur
# Projet R3.05 - Programmation Système
# =============================================================================

# -----------------------------------------------------------------------------
# Configuration du compilateur
# -----------------------------------------------------------------------------
CXX = g++
CXXFLAGS = -std=c++20 -pthread -Wall -Wextra -O2

# Pas besoin de bibliothèque supplémentaire sous Linux/WSL
# Sous Windows avec MinGW, ajouter : LDFLAGS = -lws2_32
LDFLAGS =

# -----------------------------------------------------------------------------
# Définition des fichiers sources
# -----------------------------------------------------------------------------
COMMON_SRC = message.cpp socket_utils.cpp
SERVER_SRC = serveur.cpp $(COMMON_SRC)
CLIENT_SRC = client.cpp $(COMMON_SRC)

# -----------------------------------------------------------------------------
# Définition des exécutables
# -----------------------------------------------------------------------------
SERVER_EXE = serveur
CLIENT_EXE = client

# -----------------------------------------------------------------------------
# Règle par défaut : compile le serveur et le client
# -----------------------------------------------------------------------------
all: $(SERVER_EXE) $(CLIENT_EXE)

# -----------------------------------------------------------------------------
# Compilation du serveur
# Dépendances : serveur.cpp, message.cpp, socket_utils.cpp
# -----------------------------------------------------------------------------
$(SERVER_EXE): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# -----------------------------------------------------------------------------
# Compilation du client
# Dépendances : client.cpp, message.cpp, socket_utils.cpp
# -----------------------------------------------------------------------------
$(CLIENT_EXE): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# -----------------------------------------------------------------------------
# Nettoyage des fichiers générés
# -----------------------------------------------------------------------------
clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE) server.log *.o

# -----------------------------------------------------------------------------
# Compilation du serveur uniquement
# -----------------------------------------------------------------------------
server: $(SERVER_EXE)

# -----------------------------------------------------------------------------
# Compilation du client uniquement
# -----------------------------------------------------------------------------
client: $(CLIENT_EXE)

# -----------------------------------------------------------------------------
# Affichage de l'aide
# -----------------------------------------------------------------------------
help:
	@echo "=== Makefile - Messagerie Instantanée ==="
	@echo ""
	@echo "Utilisation :"
	@echo "  make          Compiler le serveur et le client"
	@echo "  make server   Compiler uniquement le serveur"
	@echo "  make client   Compiler uniquement le client"
	@echo "  make clean    Supprimer les fichiers générés"
	@echo "  make help     Afficher cette aide"
	@echo ""
	@echo "Exécution :"
	@echo "  ./serveur                    Lancer le serveur (port 8888)"
	@echo "  ./client [IP] [PORT]         Lancer le client"

# Déclaration des cibles qui ne sont pas des fichiers
.PHONY: all clean server client help
