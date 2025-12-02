#include "message.h"
#include "socket_utils.h"
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <sstream>
#include <algorithm>

// Variables globales
std::vector<Message> g_receivedMessages;
std::mutex g_messagesMutex;
std::atomic<bool> g_isComposing(false);
std::atomic<bool> g_clientRunning(true);
SOCKET g_serverSocket = INVALID_SOCKET;
std::string g_username;

// Prototypes de fonctions
void listenThread();
void displayMenu();
void listMessages();
void readMessage();
void markAsRead();
void listOnlineUsers();
void composeMessage();
void requestServerLog();
void disconnect();
void clearInputBuffer();

// Thread d'écoute des messages du serveur
void listenThread() {
    char buffer[sizeof(Message) + 100];
    
    while (g_clientRunning) {
        try {
            // Vérifier si des données sont disponibles
            if (!SocketUtils::hasData(g_serverSocket, 1000)) {
                continue;
            }
            
            // Réception avec protocole fiable (préfixe de longueur)
            size_t received = SocketUtils::receiveWithLength(g_serverSocket, buffer, sizeof(buffer) - 1);
            if (received == 0) {
                if (g_clientRunning) {
                    std::cout << "\n[SYSTÈME] Connexion au serveur perdue" << std::endl;
                    g_clientRunning = false;
                }
                break;
            }
            
            buffer[received] = '\0';
            std::string response(buffer, received);
            
            // Traiter les différents types de réponses
            if (response.substr(0, 4) == "MSG:") {
                // Nouveau message reçu - les données binaires suivent le header
                if (received > 4) {
                    Message msg = Message::deserialize(buffer + 4, received - 4);
                    // Le timestamp est déjà défini par le serveur lors de la livraison
                    
                    {
                        std::lock_guard<std::mutex> lock(g_messagesMutex);
                        g_receivedMessages.push_back(msg);
                    }
                    
                    // Afficher notification seulement si on n'est pas en train de composer
                    if (!g_isComposing) {
                        std::cout << "\n[NOUVEAU MESSAGE] De: " << msg.from 
                                  << " | Sujet: " << msg.subject << std::endl;
                        std::cout << "Tapez votre commande: ";
                        std::cout.flush();
                    }
                }
                
            } else if (response.substr(0, 7) == "NOTIFY:") {
                // Notification du serveur (ex: échec de livraison)
                if (!g_isComposing) {
                    std::cout << "\n[NOTIFICATION] " << response.substr(7) << std::endl;
                    std::cout << "Tapez votre commande: ";
                    std::cout.flush();
                }
                
            } else if (response.substr(0, 3) == "OK:") {
                if (!g_isComposing) {
                    std::cout << "\n[SERVEUR] " << response.substr(3) << std::endl;
                    std::cout << "Tapez votre commande: ";
                    std::cout.flush();
                }
                
            } else if (response.substr(0, 6) == "ERROR:") {
                if (!g_isComposing) {
                    std::cout << "\n[ERREUR] " << response.substr(6) << std::endl;
                    std::cout << "Tapez votre commande: ";
                    std::cout.flush();
                }
                
            } else if (response.substr(0, 6) == "USERS:") {
                std::string userList = response.substr(6);
                std::cout << "\n=== UTILISATEURS EN LIGNE ===" << std::endl;
                
                std::stringstream ss(userList);
                std::string user;
                int count = 0;
                while (std::getline(ss, user, ';')) {
                    if (!user.empty()) {
                        std::cout << "- " << user << std::endl;
                        count++;
                    }
                }
                std::cout << "Total: " << count << " utilisateur(s)" << std::endl;
                std::cout << "=============================" << std::endl;
                
            } else if (response.substr(0, 4) == "LOG:") {
                std::string logContent = response.substr(4);
                std::cout << "\n=== FICHIER LOG DU SERVEUR ===" << std::endl;
                std::cout << logContent << std::endl;
                std::cout << "===============================" << std::endl;
            }
            
        } catch (const std::exception& e) {
            if (g_clientRunning) {
                std::cout << "\n[ERREUR] Écoute: " << e.what() << std::endl;
            }
        }
    }
}

// Afficher le menu
void displayMenu() {
    std::cout << "\n╔════════════════════════════════════════╗" << std::endl;
    std::cout << "║    MESSAGERIE INSTANTANÉE - CLIENT     ║" << std::endl;
    std::cout << "╠════════════════════════════════════════╣" << std::endl;
    std::cout << "║ 1. Lister les messages                 ║" << std::endl;
    std::cout << "║ 2. Lire un message                     ║" << std::endl;
    std::cout << "║ 3. Marquer comme lu                    ║" << std::endl;
    std::cout << "║ 4. Composer un message                 ║" << std::endl;
    std::cout << "║ 5. Lister les utilisateurs en ligne    ║" << std::endl;
    std::cout << "║ 6. Récupérer le log du serveur         ║" << std::endl;
    std::cout << "║ 7. Se déconnecter                      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════╝" << std::endl;
}

// Lister les messages reçus
void listMessages() {
    std::lock_guard<std::mutex> lock(g_messagesMutex);
    
    if (g_receivedMessages.empty()) {
        std::cout << "\nAucun message reçu." << std::endl;
        return;
    }
    
    std::cout << "\n=== MESSAGES REÇUS ===" << std::endl;
    for (size_t i = 0; i < g_receivedMessages.size(); ++i) {
        std::cout << i + 1 << ". " << g_receivedMessages[i].toShortString() << std::endl;
    }
    std::cout << "======================" << std::endl;
}

// Lire un message
void readMessage() {
    std::cout << "\nChoisir un message par:" << std::endl;
    std::cout << "1. Indice" << std::endl;
    std::cout << "2. Sujet" << std::endl;
    std::cout << "Votre choix: ";
    
    int choice;
    std::cin >> choice;
    clearInputBuffer();
    
    std::lock_guard<std::mutex> lock(g_messagesMutex);
    
    if (g_receivedMessages.empty()) {
        std::cout << "Aucun message à lire." << std::endl;
        return;
    }
    
    if (choice == 1) {
        std::cout << "Indice du message (1-" << g_receivedMessages.size() << "): ";
        int index;
        std::cin >> index;
        clearInputBuffer();
        
        if (index < 1 || index > static_cast<int>(g_receivedMessages.size())) {
            std::cout << "Message inexistant." << std::endl;
            return;
        }
        
        std::cout << "\n" << g_receivedMessages[index - 1].toString() << std::endl;
        
    } else if (choice == 2) {
        std::cout << "Sujet du message: ";
        std::string subject;
        std::getline(std::cin, subject);
        
        bool found = false;
        for (const auto& msg : g_receivedMessages) {
            if (std::string(msg.subject) == subject) {
                std::cout << "\n" << msg.toString() << std::endl;
                found = true;
                break;
            }
        }
        
        if (!found) {
            std::cout << "Aucun message avec ce sujet." << std::endl;
        }
    } else {
        std::cout << "Choix invalide." << std::endl;
    }
}

// Marquer un message comme lu
void markAsRead() {
    std::lock_guard<std::mutex> lock(g_messagesMutex);
    
    if (g_receivedMessages.empty()) {
        std::cout << "Aucun message." << std::endl;
        return;
    }
    
    std::cout << "Indice du message à marquer comme lu (1-" << g_receivedMessages.size() << "): ";
    int index;
    std::cin >> index;
    clearInputBuffer();
    
    if (index < 1 || index > static_cast<int>(g_receivedMessages.size())) {
        std::cout << "Message inexistant." << std::endl;
        return;
    }
    
    g_receivedMessages[index - 1].isRead = true;
    std::cout << "Message marqué comme lu." << std::endl;
}

// Lister les utilisateurs en ligne
void listOnlineUsers() {
    try {
        std::string command = "LIST_USERS";
        SocketUtils::sendWithLength(g_serverSocket, command.c_str(), command.length());
    } catch (const std::exception& e) {
        std::cout << "Erreur lors de la demande: " << e.what() << std::endl;
    }
}

// Composer un nouveau message
void composeMessage() {
    g_isComposing = true;
    
    std::cout << "\n=== COMPOSER UN MESSAGE ===" << std::endl;
    
    std::string to, subject, body;
    
    std::cout << "Destinataire (ou 'all' pour broadcast): ";
    std::getline(std::cin, to);
    
    std::cout << "Sujet (max " << MAX_SUBJECT_SIZE - 1 << " caractères): ";
    std::getline(std::cin, subject);
    
    std::cout << "Corps du message (max " << MAX_BODY_SIZE - 1 << " caractères):" << std::endl;
    std::getline(std::cin, body);
    
    try {
        // Valider et créer le message
        Message msg(g_username, to, subject, body);
        
        // Envoyer la commande au serveur avec protocole fiable
        std::string command = "SEND:";
        SocketUtils::sendWithLength(g_serverSocket, command.c_str(), command.length());
        
        // Envoyer le message sérialisé avec protocole fiable
        char buffer[sizeof(Message)];
        size_t size;
        msg.serialize(buffer, size);
        SocketUtils::sendWithLength(g_serverSocket, buffer, size);
        
        std::cout << "Message envoyé (sera livré dans max 30s)." << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Erreur: " << e.what() << std::endl;
    }
    
    g_isComposing = false;
}

// Demander le fichier log du serveur
void requestServerLog() {
    try {
        std::string command = "GET_LOG";
        SocketUtils::sendWithLength(g_serverSocket, command.c_str(), command.length());
    } catch (const std::exception& e) {
        std::cout << "Erreur lors de la demande: " << e.what() << std::endl;
    }
}

// Se déconnecter
void disconnect() {
    try {
        std::string command = "DISCONNECT";
        SocketUtils::sendWithLength(g_serverSocket, command.c_str(), command.length());
    } catch (const std::exception& e) {
        // Ignorer les erreurs
    }
    
    g_clientRunning = false;
    std::cout << "\nDéconnexion..." << std::endl;
}

// Nettoyer le buffer d'entrée
void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int main(int argc, char* argv[]) {
    std::string serverIP = "127.0.0.1";
    int port = 8888;
    
    // Paramètres optionnels
    if (argc >= 2) {
        serverIP = argv[1];
    }
    if (argc >= 3) {
        port = std::stoi(argv[2]);
    }
    
    try {
        // Initialisation
        SocketUtils::initializeWinsock();
        
        std::cout << "=== CLIENT DE MESSAGERIE INSTANTANÉE ===" << std::endl;
        
        // Demander le nom d'utilisateur
        std::cout << "Nom d'utilisateur: ";
        std::getline(std::cin, g_username);
        
        if (g_username.empty() || g_username.length() >= MAX_FROM_SIZE) {
            std::cout << "Nom d'utilisateur invalide (max " << MAX_FROM_SIZE - 1 << " caractères)" << std::endl;
            return 1;
        }
        
        // Connexion au serveur
        std::cout << "Connexion à " << serverIP << ":" << port << "..." << std::endl;
        g_serverSocket = SocketUtils::createTCPSocket();
        SocketUtils::connectToServer(g_serverSocket, serverIP, port);
        
        // Envoyer le nom d'utilisateur avec protocole fiable
        SocketUtils::sendWithLength(g_serverSocket, g_username.c_str(), g_username.length());
        
        std::cout << "Connecté avec succès!" << std::endl;
        
        // Lancer le thread d'écoute
        std::thread listener(listenThread);
        
        // Boucle principale
        while (g_clientRunning) {
            displayMenu();
            std::cout << "Tapez votre commande: ";
            
            int choice;
            if (!(std::cin >> choice)) {
                clearInputBuffer();
                std::cout << "Entrée invalide." << std::endl;
                continue;
            }
            clearInputBuffer();
            
            switch (choice) {
                case 1:
                    listMessages();
                    break;
                case 2:
                    readMessage();
                    break;
                case 3:
                    markAsRead();
                    break;
                case 4:
                    composeMessage();
                    break;
                case 5:
                    listOnlineUsers();
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    break;
                case 6:
                    requestServerLog();
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    break;
                case 7:
                    disconnect();
                    break;
                default:
                    std::cout << "Commande inexistante." << std::endl;
            }
        }
        
        // Attendre la fin du thread d'écoute
        if (listener.joinable()) {
            listener.join();
        }
        
        // Fermer le socket
        SocketUtils::closeSocket(g_serverSocket);
        
        std::cout << "Client terminé." << std::endl;
        
        SocketUtils::cleanupWinsock();
        
    } catch (const std::exception& e) {
        std::cerr << "Erreur fatale: " << e.what() << std::endl;
        SocketUtils::cleanupWinsock();
        return 1;
    }
    
    return 0;
}
