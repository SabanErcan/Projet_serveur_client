#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

// Gestion des différences entre Windows (Winsock) et Linux (BSD Sockets)
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

/**
 * Classe utilitaire statique pour encapsuler les appels système socket.
 * Masque la complexité des différences OS (Windows/Linux).
 */
class SocketUtils {
public:
    // Initialise la bibliothèque Winsock (nécessaire sous Windows uniquement)
    static void initializeWinsock();
    
    // Libère les ressources Winsock
    static void cleanupWinsock();
    
    // Crée un socket TCP (IPv4, Stream)
    static SOCKET createTCPSocket();
    
    // Ferme proprement un socket
    static void closeSocket(SOCKET sock);
    
    // Associe le socket à un port local
    static void bindSocket(SOCKET sock, int port);
    
    // Met le socket en mode écoute pour accepter les connexions entrantes
    static void listenSocket(SOCKET sock, int backlog = 5);
    
    // Accepte une connexion entrante et retourne le socket client
    static SOCKET acceptConnection(SOCKET serverSocket, std::string& clientIP);
    
    // Connecte un socket client à un serveur distant
    static void connectToServer(SOCKET sock, const std::string& serverIP, int port);
    
    // Envoie des données sur le socket (boucle jusqu'à ce que tout soit envoyé)
    static void sendData(SOCKET sock, const char* data, size_t size);
    
    // Reçoit des données du socket (bloquant)
    static ssize_t receiveData(SOCKET sock, char* buffer, size_t size);
    
    // Vérifie si des données sont disponibles en lecture (non-bloquant avec timeout)
    static bool hasData(SOCKET sock, int timeoutMs = 0);
    
    // Envoie des données avec préfixe de longueur (protocole fiable contre fragmentation TCP)
    static void sendWithLength(SOCKET sock, const char* data, size_t size);
    
    // Reçoit des données avec préfixe de longueur (protocole fiable)
    // Retourne le nombre d'octets reçus, ou 0 si déconnexion
    static size_t receiveWithLength(SOCKET sock, char* buffer, size_t maxSize);
    
    // Reçoit exactement 'size' octets (boucle jusqu'à réception complète)
    static bool receiveExact(SOCKET sock, char* buffer, size_t size);
};

#endif // SOCKET_UTILS_H
