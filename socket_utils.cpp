/*
 * socket_utils.cpp
 * 
 * Implémentation des fonctions utilitaires pour les sockets TCP.
 * 
 * Projet R3.05 - Programmation Système
 */

#include "socket_utils.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/select.h>
    #include <fcntl.h>
#endif

/* ========================================================================== */
/*                      INITIALISATION / NETTOYAGE                            */
/* ========================================================================== */

/*
 * Initialise la bibliothèque Winsock sous Windows.
 * Demande la version 2.2 qui supporte TCP/IP.
 * Ne fait rien sous Linux (sockets disponibles nativement).
 */
void SocketUtils::initializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        throw std::runtime_error("WSAStartup échoué avec erreur: " + std::to_string(result));
    }
#endif
}

/*
 * Libère les ressources Winsock sous Windows.
 * À appeler avant la fin du programme.
 */
void SocketUtils::cleanupWinsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

/* ========================================================================== */
/*                      CRÉATION / FERMETURE DE SOCKET                        */
/* ========================================================================== */

/*
 * Crée un socket TCP/IPv4.
 * 
 * Paramètres socket() :
 *   - AF_INET     : Famille d'adresses IPv4
 *   - SOCK_STREAM : Socket orienté connexion (TCP)
 *   - IPPROTO_TCP : Protocole TCP explicite
 * 
 * Active l'option SO_REUSEADDR pour pouvoir réutiliser le port
 * immédiatement après fermeture (évite "Address already in use").
 */
SOCKET SocketUtils::createTCPSocket() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        throw std::runtime_error("Échec de création du socket");
    }
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    return sock;
}

/*
 * Ferme un socket de manière portable.
 * Utilise closesocket() sous Windows, close() sous Linux.
 */
void SocketUtils::closeSocket(SOCKET sock) {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
}

/* ========================================================================== */
/*                           CÔTÉ SERVEUR                                     */
/* ========================================================================== */

/*
 * Associe le socket à une adresse locale (bind).
 * 
 * Configuration sockaddr_in :
 *   - sin_family      : AF_INET (IPv4)
 *   - sin_addr.s_addr : INADDR_ANY (écoute sur toutes les interfaces)
 *   - sin_port        : Port converti en network byte order (htons)
 */
void SocketUtils::bindSocket(SOCKET sock, int port) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        throw std::runtime_error("Échec du bind sur le port " + std::to_string(port));
    }
}

/*
 * Passe le socket en mode écoute passive.
 * 
 * Le paramètre backlog définit la taille de la file d'attente
 * des connexions en attente d'être acceptées.
 */
void SocketUtils::listenSocket(SOCKET sock, int backlog) {
    if (listen(sock, backlog) == SOCKET_ERROR) {
        throw std::runtime_error("Échec du listen");
    }
}

/*
 * Accepte une connexion entrante (bloquant).
 * 
 * Retourne le socket client créé pour cette connexion.
 * Récupère l'adresse IP du client via inet_ntoa().
 */
SOCKET SocketUtils::acceptConnection(SOCKET serverSocket, std::string& clientIP) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) {
        throw std::runtime_error("Échec d'accept");
    }
    
    clientIP = inet_ntoa(clientAddr.sin_addr);
    return clientSocket;
}

/* ========================================================================== */
/*                            CÔTÉ CLIENT                                     */
/* ========================================================================== */

/*
 * Connecte le socket à un serveur distant.
 * 
 * Conversion de l'adresse IP :
 *   - Windows : inet_addr() (IPv4 uniquement)
 *   - Linux   : inet_pton() (supporte IPv4 et IPv6)
 */
void SocketUtils::connectToServer(SOCKET sock, const std::string& serverIP, int port) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
#ifdef _WIN32
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
#else
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        throw std::runtime_error("Adresse IP invalide");
    }
#endif
    
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        throw std::runtime_error("Échec de connexion au serveur " + serverIP + ":" + std::to_string(port));
    }
}

/* ========================================================================== */
/*                      ENVOI / RÉCEPTION DE DONNÉES                          */
/* ========================================================================== */

/*
 * Envoie des données sur le socket.
 * 
 * Gère l'envoi partiel en bouclant jusqu'à ce que toutes
 * les données soient envoyées. C'est nécessaire car send()
 * peut ne pas envoyer tous les octets en une seule fois.
 */
void SocketUtils::sendData(SOCKET sock, const char* data, size_t size) {
    size_t totalSent = 0;
    while (totalSent < size) {
        ssize_t sent = send(sock, data + totalSent, size - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            throw std::runtime_error("Échec d'envoi de données");
        }
        totalSent += sent;
    }
}

/*
 * Reçoit des données du socket (appel bloquant).
 * Retourne le nombre d'octets effectivement reçus.
 */
ssize_t SocketUtils::receiveData(SOCKET sock, char* buffer, size_t size) {
    ssize_t received = recv(sock, buffer, size, 0);
    if (received == SOCKET_ERROR) {
        throw std::runtime_error("Échec de réception de données");
    }
    return received;
}

/*
 * Vérifie si des données sont disponibles en lecture (non-bloquant).
 * 
 * Utilise select() avec un timeout pour éviter de bloquer.
 * Retourne true si le socket a des données prêtes à être lues.
 */
bool SocketUtils::hasData(SOCKET sock, int timeoutMs) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    int result = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
    return result > 0 && FD_ISSET(sock, &readSet);
}

/* ========================================================================== */
/*                PROTOCOLE AVEC PRÉFIXE DE LONGUEUR                          */
/* ========================================================================== */

/*
 * Reçoit exactement N octets en bouclant sur recv().
 * 
 * Résout le problème de fragmentation TCP : un seul appel recv()
 * peut retourner moins d'octets que demandé si les données
 * arrivent en plusieurs paquets.
 * 
 * Retourne true si succès, false si déconnexion ou erreur.
 */
bool SocketUtils::receiveExact(SOCKET sock, char* buffer, size_t size) {
    size_t totalReceived = 0;
    while (totalReceived < size) {
        ssize_t received = recv(sock, buffer + totalReceived, size - totalReceived, 0);
        if (received <= 0) {
            return false;
        }
        totalReceived += received;
    }
    return true;
}

/*
 * Envoie des données avec un préfixe de longueur de 4 octets.
 * 
 * Format du message :
 *   [4 octets : longueur en network byte order][données]
 * 
 * Ce protocole permet au récepteur de savoir exactement
 * combien d'octets il doit lire, résolvant ainsi le problème
 * de délimitation des messages sur un flux TCP.
 */
void SocketUtils::sendWithLength(SOCKET sock, const char* data, size_t size) {
    /* Envoi de la longueur (4 octets, big-endian) */
    uint32_t netLength = htonl(static_cast<uint32_t>(size));
    sendData(sock, reinterpret_cast<const char*>(&netLength), sizeof(netLength));
    
    /* Envoi des données */
    sendData(sock, data, size);
}

/*
 * Reçoit des données avec préfixe de longueur.
 * 
 * 1. Lit les 4 premiers octets pour obtenir la longueur
 * 2. Convertit de network byte order vers host byte order
 * 3. Lit exactement ce nombre d'octets
 * 
 * Retourne le nombre d'octets reçus, 0 si déconnexion.
 */
size_t SocketUtils::receiveWithLength(SOCKET sock, char* buffer, size_t maxSize) {
    /* Lecture de la longueur */
    uint32_t netLength;
    if (!receiveExact(sock, reinterpret_cast<char*>(&netLength), sizeof(netLength))) {
        return 0;
    }
    
    uint32_t dataLength = ntohl(netLength);
    
    /* Vérification de la taille du buffer */
    if (dataLength > maxSize) {
        throw std::runtime_error("Message trop grand: " + std::to_string(dataLength) + " > " + std::to_string(maxSize));
    }
    
    /* Lecture des données */
    if (!receiveExact(sock, buffer, dataLength)) {
        return 0;
    }
    
    return dataLength;
}
