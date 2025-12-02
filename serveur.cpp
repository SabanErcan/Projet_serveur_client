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

// Constantes de configuration
constexpr int PORT = 8888;
constexpr int DELIVERY_INTERVAL_SECONDS = 30; // Intervalle de livraison des messages

// Structure représentant un utilisateur connecté
struct ConnectedUser {
    std::string username;
    SOCKET socket;
    std::thread* handlerThread; // Thread dédié à la gestion de cet utilisateur
};

// Variables globales (partagées entre les threads)
std::vector<ConnectedUser> g_connectedUsers; // Liste des utilisateurs connectés
std::queue<Message> g_messageQueue;          // File d'attente des messages à livrer
std::vector<Message> g_messageHistory;       // Historique complet des messages
std::mutex g_usersMutex;                     // Mutex pour protéger l'accès à g_connectedUsers
std::mutex g_queueMutex;                     // Mutex pour protéger l'accès à g_messageQueue
std::mutex g_historyMutex;                   // Mutex pour protéger l'accès à g_messageHistory
std::mutex g_logMutex;                       // Mutex pour protéger l'écriture dans le fichier log
std::ofstream g_logFile;                     // Fichier de log
std::atomic<bool> g_serverRunning(true);     // Drapeau atomique pour contrôler la boucle principale

// Prototypes de fonctions
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

/**
 * Écrit un message dans le fichier de log et sur la console.
 * Thread-safe grâce à g_logMutex.
 */
void writeLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::string timestamp = getCurrentTimestamp();
    std::string logEntry = "[" + timestamp + "] " + message;
    g_logFile << logEntry << std::endl;
    g_logFile.flush();
    std::cout << logEntry << std::endl;
}

/**
 * Retourne le timestamp actuel formaté en chaîne de caractères.
 */
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/**
 * Thread dédié à la gestion d'un client spécifique.
 * Reçoit les commandes du client et les traite.
 */
void userHandlerThread(SOCKET clientSocket, std::string clientIP) {
    std::string username;
    
    try {
        // 1. Réception du nom d'utilisateur avec protocole fiable
        char buffer[256];
        size_t received = SocketUtils::receiveWithLength(clientSocket, buffer, sizeof(buffer) - 1);
        if (received == 0) {
            throw std::runtime_error("Déconnexion lors de la réception du nom");
        }
        buffer[received] = '\0';
        username = std::string(buffer);
        
        // Mise à jour du nom d'utilisateur dans la structure globale
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
        
        // 2. Boucle principale de réception des commandes
        while (g_serverRunning) {
            // Vérifier si des données sont disponibles (timeout 1s pour vérifier g_serverRunning régulièrement)
            if (!SocketUtils::hasData(clientSocket, 1000)) {
                continue;
            }
            
            // Réception avec protocole fiable
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
    
    // Nettoyage à la fin du thread
    removeUser(clientSocket);
    SocketUtils::closeSocket(clientSocket);
}

/**
 * Thread de livraison périodique des messages.
 * Vérifie la file d'attente toutes les 30 secondes et distribue les messages.
 */
void deliveryThread() {
    writeLog("Thread de livraison démarré");
    
    while (g_serverRunning) {
        // Attente de l'intervalle de livraison
        std::this_thread::sleep_for(std::chrono::seconds(DELIVERY_INTERVAL_SECONDS));
        
        std::lock_guard<std::mutex> queueLock(g_queueMutex);
        
        if (g_messageQueue.empty()) {
            continue;
        }
        
        writeLog("Livraison de " + std::to_string(g_messageQueue.size()) + " message(s)");
        
        // Traitement de tous les messages en attente
        while (!g_messageQueue.empty()) {
            Message msg = g_messageQueue.front();
            g_messageQueue.pop();
            
            // Ajouter le timestamp de livraison
            msg.receivedAt = std::time(nullptr);
            
            // Routage du message : Broadcast ou Unicast
            bool delivered = false;
            if (std::string(msg.to) == "all") {
                broadcastMessage(msg);
                delivered = true;
            } else {
                // Vérifier si le destinataire existe et envoyer
                if (isUserConnected(msg.to)) {
                    sendMessageToUser(msg.to, msg);
                    delivered = true;
                } else {
                    // Notifier l'expéditeur de l'échec
                    std::string notification = "NOTIFY:Échec livraison - Utilisateur '" + std::string(msg.to) + "' non connecté";
                    sendNotificationToSender(msg.from, notification);
                    writeLog("Échec livraison: destinataire '" + std::string(msg.to) + "' non connecté");
                }
            }
            
            // Archivage du message dans l'historique
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

/**
 * Vérifie si un utilisateur est connecté.
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

/**
 * Envoie une notification à l'expéditeur d'un message.
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

/**
 * Envoie un message à un utilisateur spécifique s'il est connecté.
 */
void sendMessageToUser(const std::string& username, const Message& msg) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    
    for (const auto& user : g_connectedUsers) {
        if (user.username == username) {
            try {
                char buffer[sizeof(Message) + 10];
                size_t size;
                msg.serialize(buffer, size);
                
                // Protocole: En-tête "MSG:" + données sérialisées, le tout avec préfixe de longueur
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

/**
 * Diffuse un message à tous les utilisateurs connectés (sauf l'expéditeur).
 */
void broadcastMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    
    for (const auto& user : g_connectedUsers) {
        if (user.username != std::string(msg.from)) { // Ne pas envoyer à l'expéditeur
            try {
                char buffer[sizeof(Message) + 10];
                size_t size;
                msg.serialize(buffer, size);
                
                // Protocole unifié avec préfixe de longueur
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

/**
 * Trouve le nom d'utilisateur associé à un socket.
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

/**
 * Retire un utilisateur de la liste des connectés.
 * Arrête le serveur si c'était le dernier utilisateur.
 */
void removeUser(SOCKET sock) {
    std::lock_guard<std::mutex> lock(g_usersMutex);
    
    auto it = std::find_if(g_connectedUsers.begin(), g_connectedUsers.end(),
                          [sock](const ConnectedUser& user) { return user.socket == sock; });
    
    if (it != g_connectedUsers.end()) {
        std::string username = it->username;
        g_connectedUsers.erase(it);
        writeLog("Utilisateur retiré: " + username + " (" + std::to_string(g_connectedUsers.size()) + " restants)");
        
        // Arrêter le serveur si c'est le dernier client (Condition d'arrêt du projet)
        if (g_connectedUsers.empty()) {
            writeLog("Dernier client déconnecté - Arrêt du serveur");
            g_serverRunning = false;
        }
    }
}

/**
 * Traite une commande reçue d'un client.
 * Commandes supportées: SEND, LIST_USERS, GET_LOG, DISCONNECT
 */
void handleCommand(SOCKET clientSocket, const std::string& username, const std::string& command) {
    try {
        if (command.substr(0, 5) == "SEND:") {
            // Commande d'envoi de message
            // Le client envoie d'abord "SEND:", puis la structure Message sérialisée
            
            char buffer[sizeof(Message)];
            size_t received = SocketUtils::receiveWithLength(clientSocket, buffer, sizeof(buffer));
            
            if (received == sizeof(Message)) {
                Message msg = Message::deserialize(buffer, received);
                
                // Ajouter le message à la file d'attente pour le thread de livraison
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
            // Demande de la liste des utilisateurs connectés
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
            // Demande de téléchargement du fichier de log
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
            // Demande de déconnexion explicite
            std::string response = "OK:Déconnexion";
            SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
            writeLog("Déconnexion demandée par " + username);
            
        } else {
            // Commande inconnue
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
            // Ignorer les erreurs d'envoi de la réponse d'erreur
        }
    }
}

int main() {
    try {
        // Initialisation de la couche réseau
        SocketUtils::initializeWinsock();
        
        // Ouverture du fichier de log en mode ajout
        g_logFile.open("server.log", std::ios::app);
        if (!g_logFile) {
            std::cerr << "Impossible d'ouvrir le fichier de log" << std::endl;
            return 1;
        }
        
        writeLog("=== SERVEUR DE MESSAGERIE DÉMARRÉ ===");
        
        // Configuration du socket serveur
        SOCKET serverSocket = SocketUtils::createTCPSocket();
        SocketUtils::bindSocket(serverSocket, PORT);
        SocketUtils::listenSocket(serverSocket);
        
        writeLog("Serveur en écoute sur le port " + std::to_string(PORT));
        
        // Démarrage du thread de livraison des messages
        std::thread deliveryThreadObj(deliveryThread);
        
        // Boucle principale d'acceptation des connexions
        while (g_serverRunning) {
            // Vérifier si une connexion est en attente (non-bloquant)
            if (!SocketUtils::hasData(serverSocket, 1000)) {
                continue;
            }
            
            std::string clientIP;
            SOCKET clientSocket = SocketUtils::acceptConnection(serverSocket, clientIP);
            
            // Création d'un nouveau thread pour gérer ce client
            std::thread* userThread = new std::thread(userHandlerThread, clientSocket, clientIP);
            
            // Enregistrement de l'utilisateur
            {
                std::lock_guard<std::mutex> lock(g_usersMutex);
                ConnectedUser user;
                user.username = ""; // Sera défini par le thread userHandlerThread
                user.socket = clientSocket;
                user.handlerThread = userThread;
                g_connectedUsers.push_back(user);
            }
        }
        
        // Arrêt propre : Attendre la fin du thread de livraison
        if (deliveryThreadObj.joinable()) {
            deliveryThreadObj.join();
        }
        
        // Attendre la fin de tous les threads utilisateurs et libérer la mémoire
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
        
        // Fermeture du socket serveur
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
