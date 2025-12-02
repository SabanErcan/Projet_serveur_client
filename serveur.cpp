/*
 * serveur.cpp
 * 
 * Serveur de messagerie instantanée multi-threadé.
 * 
 * Architecture :
 *   - Thread principal     : accepte les connexions entrantes
 *   - Threads utilisateurs : un par client connecté (réception commandes)
 *   - Thread de livraison  : distribue les messages toutes les 30 secondes
 * 
 * Synchronisation :
 *   - Mutex sur les structures partagées (utilisateurs, file, historique)
 *   - Variable atomique pour le flag d'arrêt
 * 
 * Projet R3.05 - Programmation Système
 */

#include "message.h"
#include "socket_utils.h"
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <fstream>
#include <chrono>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <atomic>

/* ========================================================================== */
/*                          CONFIGURATION                                     */
/* ========================================================================== */

constexpr int PORT = 8888;                        /* Port d'écoute du serveur       */
constexpr int DELIVERY_INTERVAL_SECONDS = 30;     /* Intervalle de livraison (s)    */

/* ========================================================================== */
/*                      STRUCTURES DE DONNÉES                                 */
/* ========================================================================== */

/*
 * Structure représentant un utilisateur connecté.
 * Chaque client dispose de son propre thread de gestion.
 */
struct ConnectedUser {
    std::string username;           /* Nom d'utilisateur                      */
    SOCKET socket;                  /* Socket de communication                */
    std::thread* handlerThread;     /* Thread dédié à ce client               */
};

/* ========================================================================== */
/*                      VARIABLES GLOBALES                                    */
/* ========================================================================== */

/* Données partagées entre les threads */
std::vector<ConnectedUser> g_connectedUsers;  /* Liste des utilisateurs connectés  */
std::queue<Message> g_messageQueue;           /* File d'attente des messages       */
std::vector<Message> g_messageHistory;        /* Historique des messages           */

/* Mutex pour la synchronisation (exclusion mutuelle) */
std::mutex g_usersMutex;                      /* Protection de g_connectedUsers    */
std::mutex g_queueMutex;                      /* Protection de g_messageQueue      */
std::mutex g_historyMutex;                    /* Protection de g_messageHistory    */
std::mutex g_logMutex;                        /* Protection de l'écriture log      */

/* Autres variables globales */
std::ofstream g_logFile;                      /* Fichier de journalisation         */
std::atomic<bool> g_serverRunning(true);      /* Flag d'arrêt (atomique)           */

/* ========================================================================== */
/*                      PROTOTYPES DE FONCTIONS                               */
/* ========================================================================== */

void writeLog(const std::string& message);
std::string getCurrentTimestamp();
void userHandlerThread(SOCKET clientSocket, std::string clientIP);
void deliveryThread();
void sendMessageToUser(const std::string& username, const Message& msg);
void broadcastMessage(const Message& msg);
std::string getUsernameBySocket(SOCKET sock);
void removeUser(SOCKET sock);
void handleCommand(SOCKET clientSocket, const std::string& username, const std::string& command);
bool isUserConnected(const std::string& username);
void sendNotificationToSender(const std::string& senderUsername, const std::string& notification);

/* ========================================================================== */
/*                           JOURNALISATION                                   */
/* ========================================================================== */

/*
 * Écrit un message horodaté dans le fichier de log et sur la console.
 * Thread-safe grâce au mutex g_logMutex.
 */
void writeLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::string timestamp = getCurrentTimestamp();
    std::string logEntry = "[" + timestamp + "] " + message;
    g_logFile << logEntry << std::endl;
    g_logFile.flush();
    std::cout << logEntry << std::endl;
}

/*
 * Retourne l'heure actuelle formatée en chaîne de caractères.
 * Format : YYYY-MM-DD HH:MM:SS
 */
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/* ========================================================================== */
/*                    THREAD DE GESTION UTILISATEUR                           */
/* ========================================================================== */

/*
 * Thread dédié à la gestion d'un client connecté.
 * 
 * Cycle de vie :
 *   1. Réception du nom d'utilisateur
 *   2. Boucle de réception des commandes
 *   3. Nettoyage et fermeture à la déconnexion
 * 
 * Paramètres :
 *   - clientSocket : socket de communication avec ce client
 *   - clientIP     : adresse IP du client (pour le log)
 */
void userHandlerThread(SOCKET clientSocket, std::string clientIP) {
    std::string username;
    
    try {
        /* Étape 1 : Réception du nom d'utilisateur */
        char buffer[256];
        size_t received = SocketUtils::receiveWithLength(clientSocket, buffer, sizeof(buffer) - 1);
        if (received == 0) {
            throw std::runtime_error("Déconnexion lors de la réception du nom");
        }
        buffer[received] = '\0';
        username = std::string(buffer);
        
        /* Mise à jour du nom dans la structure globale */
        {
            std::lock_guard<std::mutex> lock(g_usersMutex);
            for (auto& user : g_connectedUsers) {
                if (user.socket == clientSocket) {
                    user.username = username;
                    break;
                }
            }
        }
        
        writeLog("Utilisateur connecté: " + username + " depuis " + clientIP);
        
        /* Étape 2 : Boucle principale de réception des commandes */
        while (g_serverRunning) {
            /* Vérification non-bloquante avec timeout de 1 seconde */
            if (!SocketUtils::hasData(clientSocket, 1000)) {
                continue;
            }
            
            /* Réception de la commande */
            received = SocketUtils::receiveWithLength(clientSocket, buffer, sizeof(buffer) - 1);
            if (received == 0) {
                writeLog("Utilisateur déconnecté: " + username);
                break;
            }
            
            buffer[received] = '\0';
            std::string command(buffer);
            
            writeLog("Commande reçue de " + username + ": " + command);
            handleCommand(clientSocket, username, command);
        }
        
    } catch (const std::exception& e) {
        writeLog("Erreur avec " + username + ": " + std::string(e.what()));
    }
    
    /* Étape 3 : Nettoyage */
    removeUser(clientSocket);
    SocketUtils::closeSocket(clientSocket);
}

/* ========================================================================== */
/*                       THREAD DE LIVRAISON                                  */
/* ========================================================================== */

/*
 * Thread de livraison périodique des messages.
 * 
 * Fonctionnement :
 *   - Se réveille toutes les 30 secondes
 *   - Vide la file d'attente des messages
 *   - Route chaque message vers son destinataire (unicast ou broadcast)
 *   - Notifie l'expéditeur en cas d'échec de livraison
 */
void deliveryThread() {
    writeLog("Thread de livraison démarré");
    
    while (g_serverRunning) {
        /* Attente de l'intervalle de livraison */
        std::this_thread::sleep_for(std::chrono::seconds(DELIVERY_INTERVAL_SECONDS));
        
        std::lock_guard<std::mutex> queueLock(g_queueMutex);
        
        if (g_messageQueue.empty()) {
            continue;
        }
        
        writeLog("Livraison de " + std::to_string(g_messageQueue.size()) + " message(s)");
        
        /* Traitement de tous les messages en attente */
        while (!g_messageQueue.empty()) {
            Message msg = g_messageQueue.front();
            g_messageQueue.pop();
            
            /* Horodatage de la livraison */
            msg.receivedAt = std::time(nullptr);
            
            /* Routage : broadcast si "all", unicast sinon */
            bool delivered = false;
            if (std::string(msg.to) == "all") {
                broadcastMessage(msg);
                delivered = true;
            } else {
                /* Vérification de l'existence du destinataire */
                if (isUserConnected(msg.to)) {
                    sendMessageToUser(msg.to, msg);
                    delivered = true;
                } else {
                    /* Notification d'échec à l'expéditeur */
                    std::string notification = "NOTIFY:Échec livraison - Utilisateur '" + std::string(msg.to) + "' non connecté";
                    sendNotificationToSender(msg.from, notification);
                    writeLog("Échec livraison: destinataire '" + std::string(msg.to) + "' non connecté");
                }
            }
            
            /* Archivage dans l'historique */
            {
                std::lock_guard<std::mutex> historyLock(g_historyMutex);
                g_messageHistory.push_back(msg);
            }
            
            if (delivered) {
                writeLog("Message livré de " + std::string(msg.from) + " à " + std::string(msg.to));
            }
        }
    }
    
    writeLog("Thread de livraison terminé");
}

/* ========================================================================== */
/*                     FONCTIONS UTILITAIRES                                  */
/* ========================================================================== */

/*
 * Vérifie si un utilisateur est actuellement connecté.
 */
bool isUserConnected(const std::string& username) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    for (const auto& user : g_connectedUsers) {
        if (user.username == username) {
            return true;
        }
    }
    return false;
}

/*
 * Envoie une notification à un utilisateur (typiquement l'expéditeur).
 * Utilisé pour informer d'un échec de livraison.
 */
void sendNotificationToSender(const std::string& senderUsername, const std::string& notification) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    for (const auto& user : g_connectedUsers) {
        if (user.username == senderUsername) {
            try {
                SocketUtils::sendWithLength(user.socket, notification.c_str(), notification.length());
            } catch (const std::exception& e) {
                writeLog("Échec envoi notification à " + senderUsername + ": " + std::string(e.what()));
            }
            return;
        }
    }
}

/* ========================================================================== */
/*                       ENVOI DE MESSAGES                                    */
/* ========================================================================== */

/*
 * Envoie un message à un utilisateur spécifique.
 * 
 * Format du protocole :
 *   [4 octets longueur]["MSG:" + données sérialisées du Message]
 */
void sendMessageToUser(const std::string& username, const Message& msg) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    
    for (const auto& user : g_connectedUsers) {
        if (user.username == username) {
            try {
                char buffer[sizeof(Message) + 10];
                size_t size;
                msg.serialize(buffer, size);
                
                /* Construction du message complet : header + données */
                std::string header = "MSG:";
                std::vector<char> fullMessage(header.begin(), header.end());
                fullMessage.insert(fullMessage.end(), buffer, buffer + size);
                
                SocketUtils::sendWithLength(user.socket, fullMessage.data(), fullMessage.size());
                
            } catch (const std::exception& e) {
                writeLog("Échec d'envoi à " + username + ": " + std::string(e.what()));
            }
            return;
        }
    }
    
    writeLog("Utilisateur destinataire inexistant ou déconnecté: " + username);
}

/*
 * Diffuse un message à tous les utilisateurs connectés (sauf l'expéditeur).
 * Utilisé pour les messages avec destinataire "all".
 */
void broadcastMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    
    for (const auto& user : g_connectedUsers) {
        /* Exclure l'expéditeur de la diffusion */
        if (user.username != std::string(msg.from)) {
            try {
                char buffer[sizeof(Message) + 10];
                size_t size;
                msg.serialize(buffer, size);
                
                std::string header = "MSG:";
                std::vector<char> fullMessage(header.begin(), header.end());
                fullMessage.insert(fullMessage.end(), buffer, buffer + size);
                
                SocketUtils::sendWithLength(user.socket, fullMessage.data(), fullMessage.size());
                
            } catch (const std::exception& e) {
                writeLog("Échec broadcast à " + user.username + ": " + std::string(e.what()));
            }
        }
    }
}

/*
 * Retourne le nom d'utilisateur associé à un socket.
 */
std::string getUsernameBySocket(SOCKET sock) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    for (const auto& user : g_connectedUsers) {
        if (user.socket == sock) {
            return user.username;
        }
    }
    return "";
}

/*
 * Retire un utilisateur de la liste des connectés.
 * Condition d'arrêt du serveur : si c'était le dernier utilisateur.
 */
void removeUser(SOCKET sock) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    
    auto it = std::find_if(g_connectedUsers.begin(), g_connectedUsers.end(),
                          [sock](const ConnectedUser& user) { return user.socket == sock; });
    
    if (it != g_connectedUsers.end()) {
        std::string username = it->username;
        g_connectedUsers.erase(it);
        writeLog("Utilisateur retiré: " + username + " (" + std::to_string(g_connectedUsers.size()) + " restants)");
        
        /* Arrêt du serveur si plus aucun client */
        if (g_connectedUsers.empty()) {
            writeLog("Dernier client déconnecté - Arrêt du serveur");
            g_serverRunning = false;
        }
    }
}

/* ========================================================================== */
/*                    TRAITEMENT DES COMMANDES                                */
/* ========================================================================== */

/*
 * Traite une commande reçue d'un client.
 * 
 * Commandes supportées :
 *   - SEND:       Envoyer un message (suivi des données sérialisées)
 *   - LIST_USERS  Obtenir la liste des utilisateurs connectés
 *   - GET_LOG     Télécharger le fichier de log du serveur
 *   - DISCONNECT  Se déconnecter proprement
 */
void handleCommand(SOCKET clientSocket, const std::string& username, const std::string& command) {
    try {
        if (command.substr(0, 5) == "SEND:") {
            /* Commande d'envoi de message */
            char buffer[sizeof(Message)];
            size_t received = SocketUtils::receiveWithLength(clientSocket, buffer, sizeof(buffer));
            
            if (received == sizeof(Message)) {
                Message msg = Message::deserialize(buffer, received);
                
                /* Ajout à la file d'attente pour le thread de livraison */
                {
                    std::lock_guard<std::mutex> lock(g_queueMutex);
                    g_messageQueue.push(msg);
                }
                
                std::string response = "OK:Message en file d'attente";
                SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
                writeLog("Message ajouté à la queue de " + std::string(msg.from) + " vers " + std::string(msg.to));
            } else {
                std::string response = "ERROR:Message mal formaté";
                SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
            }
            
        } else if (command == "LIST_USERS") {
            /* Liste des utilisateurs connectés */
            std::string userList;
            {
                std::lock_guard<std::mutex> lock(g_usersMutex);
                for (const auto& user : g_connectedUsers) {
                    userList += user.username + ";";
                }
            }
            
            std::string response = "USERS:" + userList;
            SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
            
        } else if (command == "GET_LOG") {
            /* Téléchargement du fichier de log */
            std::ifstream logFile("server.log", std::ios::binary);
            if (logFile) {
                std::string logContent((std::istreambuf_iterator<char>(logFile)),
                                      std::istreambuf_iterator<char>());
                
                std::string response = "LOG:" + logContent;
                SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
            } else {
                std::string response = "ERROR:Impossible de lire le fichier log";
                SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
            }
            
        } else if (command == "DISCONNECT") {
            /* Déconnexion explicite */
            std::string response = "OK:Déconnexion";
            SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
            writeLog("Déconnexion demandée par " + username);
            
        } else {
            /* Commande inconnue */
            std::string response = "ERROR:Commande inexistante";
            SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
            writeLog("Commande invalide de " + username + ": " + command);
        }
        
    } catch (const std::exception& e) {
        writeLog("Erreur traitement commande: " + std::string(e.what()));
        try {
            std::string response = "ERROR:" + std::string(e.what());
            SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
        } catch (...) {
            /* Ignorer les erreurs d'envoi de la réponse d'erreur */
        }
    }
}

/* ========================================================================== */
/*                         FONCTION PRINCIPALE                                */
/* ========================================================================== */

/*
 * Point d'entrée du serveur.
 * 
 * Séquence de démarrage :
 *   1. Initialisation de la couche réseau
 *   2. Ouverture du fichier de log
 *   3. Création et configuration du socket serveur
 *   4. Démarrage du thread de livraison
 *   5. Boucle d'acceptation des connexions
 * 
 * Séquence d'arrêt :
 *   1. Attente de la fin du thread de livraison
 *   2. Attente et nettoyage des threads utilisateurs
 *   3. Fermeture du socket serveur
 *   4. Libération des ressources
 */
int main() {
    try {
        /* Initialisation de la couche réseau (Winsock sous Windows) */
        SocketUtils::initializeWinsock();
        
        /* Ouverture du fichier de log en mode ajout */
        g_logFile.open("server.log", std::ios::app);
        if (!g_logFile) {
            std::cerr << "Impossible d'ouvrir le fichier de log" << std::endl;
            return 1;
        }
        
        writeLog("=== SERVEUR DE MESSAGERIE DÉMARRÉ ===");
        
        /* Configuration du socket serveur */
        SOCKET serverSocket = SocketUtils::createTCPSocket();
        SocketUtils::bindSocket(serverSocket, PORT);
        SocketUtils::listenSocket(serverSocket);
        
        writeLog("Serveur en écoute sur le port " + std::to_string(PORT));
        
        /* Démarrage du thread de livraison */
        std::thread deliveryThreadObj(deliveryThread);
        
        /* Boucle principale : acceptation des connexions entrantes */
        while (g_serverRunning) {
            /* Vérification non-bloquante d'une connexion en attente */
            if (!SocketUtils::hasData(serverSocket, 1000)) {
                continue;
            }
            
            std::string clientIP;
            SOCKET clientSocket = SocketUtils::acceptConnection(serverSocket, clientIP);
            
            /* Création d'un thread dédié pour ce client */
            std::thread* userThread = new std::thread(userHandlerThread, clientSocket, clientIP);
            
            /* Enregistrement dans la liste des utilisateurs */
            {
                std::lock_guard<std::mutex> lock(g_usersMutex);
                ConnectedUser user;
                user.username = "";  /* Sera défini par le thread */
                user.socket = clientSocket;
                user.handlerThread = userThread;
                g_connectedUsers.push_back(user);
            }
        }
        
        /* Arrêt propre : attente du thread de livraison */
        if (deliveryThreadObj.joinable()) {
            deliveryThreadObj.join();
        }
        
        /* Attente et nettoyage des threads utilisateurs */
        {
            std::lock_guard<std::mutex> lock(g_usersMutex);
            for (auto& user : g_connectedUsers) {
                if (user.handlerThread) {
                    if (user.handlerThread->joinable()) {
                        user.handlerThread->join();
                    }
                    delete user.handlerThread;
                    user.handlerThread = nullptr;
                }
            }
        }
        
        /* Fermeture du socket serveur */
        SocketUtils::closeSocket(serverSocket);
        
        writeLog("=== SERVEUR ARRÊTÉ ===");
        g_logFile.close();
        
        SocketUtils::cleanupWinsock();
        
    } catch (const std::exception& e) {
        std::cerr << "Erreur fatale: " << e.what() << std::endl;
        g_logFile.close();
        SocketUtils::cleanupWinsock();
        return 1;
    }
    
    return 0;
}
