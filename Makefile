# Makefile pour l'application de messagerie instantanée

# Compilateur et flags
CXX = g++
CXXFLAGS = -std=c++20 -pthread -Wall -Wextra
LDFLAGS = -lws2_32

# Fichiers sources
MESSAGE_SRC = message.cpp
SOCKET_SRC = socket_utils.cpp
SERVER_SRC = serveur.cpp
CLIENT_SRC = client.cpp

# Fichiers objets
MESSAGE_OBJ = message.o
SOCKET_OBJ = socket_utils.o
SERVER_OBJ = serveur.o
CLIENT_OBJ = client.o

# Exécutables
SERVER_EXE = serveur.exe
CLIENT_EXE = client.exe

# Règle par défaut
all: $(SERVER_EXE) $(CLIENT_EXE)

# Compilation du serveur
$(SERVER_EXE): $(SERVER_OBJ) $(MESSAGE_OBJ) $(SOCKET_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compilation du client
$(CLIENT_EXE): $(CLIENT_OBJ) $(MESSAGE_OBJ) $(SOCKET_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Règles pour les fichiers objets
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Dépendances
serveur.o: serveur.cpp message.h socket_utils.h
client.o: client.cpp message.h socket_utils.h
message.o: message.cpp message.h
socket_utils.o: socket_utils.cpp socket_utils.h

# Nettoyage
clean:
	del /Q *.o *.exe server.log 2>nul

# Nettoyage complet (avec log)
cleanall: clean
	del /Q server.log 2>nul

# Compilation serveur uniquement
server: $(SERVER_EXE)

# Compilation client uniquement
client: $(CLIENT_EXE)

# Aide
help:
	@echo Usage:
	@echo   make         - Compiler serveur et client
	@echo   make server  - Compiler uniquement le serveur
	@echo   make client  - Compiler uniquement le client
	@echo   make clean   - Nettoyer les fichiers objets et executables
	@echo   make cleanall- Nettoyer tout (y compris server.log)
	@echo   make help    - Afficher cette aide

.PHONY: all clean cleanall server client help
