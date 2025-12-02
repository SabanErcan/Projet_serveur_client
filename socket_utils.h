/*
 * socket_utils.h
 * 
 * Couche d'abstraction pour les sockets TCP.
 * Gère la portabilité entre Windows (Winsock2) et Linux (BSD Sockets).
 * 
 * Projet R3.05 - Programmation Système
 */

#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

/*
 * Directives de compilation conditionnelle pour la portabilité.
 * Sous Windows, utilisation de Winsock2.
 * Sous Linux, utilisation des sockets POSIX (BSD).
 */
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#include <string>
#include <stdexcept>

/*
 * Classe SocketUtils
 * 
 * Fournit des méthodes statiques pour manipuler les sockets TCP.
 * Encapsule les appels système et gère les différences entre OS.
 */
class SocketUtils {
public:
    /* Initialisation de Winsock (Windows uniquement) */
    static void initializeWinsock();
    
    /* Libération des ressources Winsock */
    static void cleanupWinsock();
    
    /* Création d'un socket TCP (AF_INET, SOCK_STREAM) */
    static SOCKET createTCPSocket();
    
    /* Fermeture d'un socket */
    static void closeSocket(SOCKET sock);
    
    /* Association du socket à une adresse locale (bind) */
    static void bindSocket(SOCKET sock, int port);
    
    /* Passage en mode écoute (listen) */
    static void listenSocket(SOCKET sock, int backlog = 5);
    
    /* Acceptation d'une connexion entrante (accept) */
    static SOCKET acceptConnection(SOCKET serverSocket, std::string& clientIP);
    
    /* Connexion à un serveur distant (connect) */
    static void connectToServer(SOCKET sock, const std::string& serverIP, int port);
    
    /* Envoi de données brutes (send avec gestion envoi partiel) */
    static void sendData(SOCKET sock, const char* data, size_t size);
    
    /* Réception de données brutes (recv bloquant) */
    static ssize_t receiveData(SOCKET sock, char* buffer, size_t size);
    
    /* Vérification de données disponibles avec select() */
    static bool hasData(SOCKET sock, int timeoutMs = 0);
    
    /* 
     * Protocole avec préfixe de longueur (Length-Prefixed Protocol)
     * Résout le problème de fragmentation TCP en préfixant chaque
     * message par sa longueur sur 4 octets (network byte order).
     */
    static void sendWithLength(SOCKET sock, const char* data, size_t size);
    static size_t receiveWithLength(SOCKET sock, char* buffer, size_t maxSize);
    
    /* Réception exacte de N octets (boucle sur recv) */
    static bool receiveExact(SOCKET sock, char* buffer, size_t size);
};

#endif /* SOCKET_UTILS_H */
