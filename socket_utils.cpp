#include "socket_utils.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/select.h>
    #include <fcntl.h>
#endif

/**
 * Initialisation de Winsock pour Windows.
 * Doit être appelé au début du programme.
 */
void SocketUtils::initializeWinsock() {
#ifdef _WIN32
    WSADATA wsaData;
    // Demande la version 2.2 de Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        throw std::runtime_error("WSAStartup échoué avec erreur: " + std::to_string(result));
    }
#endif
}

/**
 * Nettoyage de Winsock.
 * À appeler à la fin du programme.
 */
void SocketUtils::cleanupWinsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * Création d'un socket TCP standard.
 * AF_INET : IPv4
 * SOCK_STREAM : TCP
 */
SOCKET SocketUtils::createTCPSocket() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        throw std::runtime_error("Échec de création du socket");
    }
    
    // Permet de réutiliser l'adresse/port immédiatement après arrêt (évite "Address already in use")
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    return sock;
}

/**
 * Fermeture portable du socket.
 */
void SocketUtils::closeSocket(SOCKET sock) {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
}

/**
 * Bind le socket sur toutes les interfaces locales (INADDR_ANY) et le port spécifié.
 */
void SocketUtils::bindSocket(SOCKET sock, int port) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Écoute sur toutes les interfaces
    serverAddr.sin_port = htons(port);       // Conversion Host TO Network Short
    
    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        throw std::runtime_error("Échec du bind sur le port " + std::to_string(port));
    }
}

/**
 * Passe le socket en mode écoute.
 * backlog : nombre max de connexions en attente dans la file système.
 */
void SocketUtils::listenSocket(SOCKET sock, int backlog) {
    if (listen(sock, backlog) == SOCKET_ERROR) {
        throw std::runtime_error("Échec du listen");
    }
}

/**
 * Accepte une nouvelle connexion client.
 * Bloquant par défaut.
 * Remplit clientIP avec l'adresse IP du client connecté.
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

/**
 * Connecte le socket à un serveur distant.
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

/**
 * Envoie des données sur le socket.
 * Gère l'envoi partiel en bouclant jusqu'à ce que tout soit envoyé.
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

/**
 * Reçoit des données du socket.
 * Retourne le nombre d'octets lus.
 */
ssize_t SocketUtils::receiveData(SOCKET sock, char* buffer, size_t size) {
    ssize_t received = recv(sock, buffer, size, 0);
    if (received == SOCKET_ERROR) {
        throw std::runtime_error("Échec de réception de données");
    }
    return received;
}

/**
 * Utilise select() pour vérifier si des données sont disponibles en lecture
 * sans bloquer indéfiniment.
 * timeoutMs : temps d'attente max en millisecondes.
 */
bool SocketUtils::hasData(SOCKET sock, int timeoutMs) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    // select retourne > 0 si le socket est prêt à être lu
    int result = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
    return result > 0 && FD_ISSET(sock, &readSet);
}

/**
 * Reçoit exactement 'size' octets en bouclant sur recv().
 * Gère la fragmentation TCP.
 * Retourne true si succès, false si déconnexion ou erreur.
 */
bool SocketUtils::receiveExact(SOCKET sock, char* buffer, size_t size) {
    size_t totalReceived = 0;
    while (totalReceived < size) {
        ssize_t received = recv(sock, buffer + totalReceived, size - totalReceived, 0);
        if (received <= 0) {
            return false; // Déconnexion ou erreur
        }
        totalReceived += received;
    }
    return true;
}

/**
 * Envoie des données avec un préfixe de 4 octets indiquant la longueur.
 * Format: [4 octets longueur en network byte order][données]
 * Cela permet au récepteur de savoir exactement combien d'octets lire.
 */
void SocketUtils::sendWithLength(SOCKET sock, const char* data, size_t size) {
    // Envoyer la longueur en premier (4 octets, network byte order)
    uint32_t netLength = htonl(static_cast<uint32_t>(size));
    sendData(sock, reinterpret_cast<const char*>(&netLength), sizeof(netLength));
    
    // Envoyer les données
    sendData(sock, data, size);
}

/**
 * Reçoit des données avec préfixe de longueur.
 * Lit d'abord les 4 octets de longueur, puis les données.
 * Retourne le nombre d'octets reçus (0 si déconnexion).
 */
size_t SocketUtils::receiveWithLength(SOCKET sock, char* buffer, size_t maxSize) {
    // Lire la longueur (4 octets)
    uint32_t netLength;
    if (!receiveExact(sock, reinterpret_cast<char*>(&netLength), sizeof(netLength))) {
        return 0; // Déconnexion
    }
    
    uint32_t dataLength = ntohl(netLength);
    
    // Vérifier que le buffer est assez grand
    if (dataLength > maxSize) {
        throw std::runtime_error("Message trop grand: " + std::to_string(dataLength) + " > " + std::to_string(maxSize));
    }
    
    // Lire les données
    if (!receiveExact(sock, buffer, dataLength)) {
        return 0; // Déconnexion pendant la réception
    }
    
    return dataLength;
}
