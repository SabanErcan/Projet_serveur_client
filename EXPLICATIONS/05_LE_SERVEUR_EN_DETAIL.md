# 🖥️ LE SERVEUR EN DÉTAIL

## 🎯 Rôle du serveur

Le serveur est le **centre de contrôle** de l'application :
- Il accepte les connexions des clients
- Il reçoit les messages des clients
- Il stocke les messages dans une file d'attente
- Il livre les messages aux destinataires
- Il garde une trace de tout (fichier log)

---

## 📁 Les données globales du serveur

```cpp
// Liste des utilisateurs connectés
vector<ConnectedUser> g_connectedUsers;

// File d'attente des messages à livrer
queue<Message> g_messageQueue;

// Historique de tous les messages
vector<Message> g_messageHistory;

// Fichier de log
ofstream g_logFile;

// Flag d'arrêt du serveur
atomic<bool> g_serverRunning(true);
```

### La structure ConnectedUser :
```cpp
struct ConnectedUser {
    string username;           // Nom de l'utilisateur
    SOCKET socket;             // Socket de communication
    thread* handlerThread;     // Thread dédié à cet utilisateur
};
```

---

## 🚀 Démarrage du serveur (fonction main)

### Étape par étape :

```
╔═══════════════════════════════════════════════════════════════╗
║                    DÉMARRAGE DU SERVEUR                        ║
╠═══════════════════════════════════════════════════════════════╣
║                                                                 ║
║  1. Initialiser Winsock (Windows uniquement)                   ║
║     └─► Préparer la couche réseau                              ║
║                                                                 ║
║  2. Ouvrir le fichier de log                                   ║
║     └─► "server.log" en mode ajout                             ║
║                                                                 ║
║  3. Créer la socket serveur                                    ║
║     └─► socket(AF_INET, SOCK_STREAM, 0)                        ║
║                                                                 ║
║  4. Lier au port 8888                                          ║
║     └─► bind(serverSocket, port 8888)                          ║
║                                                                 ║
║  5. Se mettre en écoute                                        ║
║     └─► listen(serverSocket, 5)                                ║
║                                                                 ║
║  6. Démarrer le thread de livraison                            ║
║     └─► thread(deliveryThread)                                 ║
║                                                                 ║
║  7. Boucle infinie : accepter les connexions                   ║
║     └─► Créer un thread par client                             ║
║                                                                 ║
╚═══════════════════════════════════════════════════════════════╝
```

### Le code :

```cpp
int main() {
    // 1. Initialiser le réseau
    SocketUtils::initializeWinsock();
    
    // 2. Ouvrir le fichier log
    g_logFile.open("server.log", ios::app);
    writeLog("=== SERVEUR DÉMARRÉ ===");
    
    // 3. Créer la socket
    SOCKET serverSocket = SocketUtils::createTCPSocket();
    
    // 4. Lier au port 8888
    SocketUtils::bindSocket(serverSocket, 8888);
    
    // 5. Écouter
    SocketUtils::listenSocket(serverSocket);
    writeLog("En écoute sur le port 8888");
    
    // 6. Démarrer le thread de livraison
    thread deliveryThreadObj(deliveryThread);
    
    // 7. Boucle principale
    while (g_serverRunning) {
        // Attendre une connexion
        if (!SocketUtils::hasData(serverSocket, 1000)) {
            continue;  // Vérifier toutes les secondes
        }
        
        // Accepter la connexion
        string clientIP;
        SOCKET clientSocket = SocketUtils::acceptConnection(serverSocket, clientIP);
        
        // Créer un thread pour ce client
        thread* userThread = new thread(userHandlerThread, clientSocket, clientIP);
        
        // L'ajouter à la liste
        {
            lock_guard<mutex> lock(g_usersMutex);
            ConnectedUser user;
            user.socket = clientSocket;
            user.handlerThread = userThread;
            g_connectedUsers.push_back(user);
        }
    }
    
    // Nettoyage...
}
```

---

## 👤 Thread User Handler (1 par client)

Ce thread s'occupe d'UN SEUL client. Il fait 3 choses :

### 1. Recevoir le nom d'utilisateur

```cpp
// Le client envoie son nom en premier
char buffer[256];
size_t received = SocketUtils::receiveWithLength(clientSocket, buffer, 255);
buffer[received] = '\0';
username = string(buffer);

// Mettre à jour la liste des connectés
{
    lock_guard<mutex> lock(g_usersMutex);
    for (auto& user : g_connectedUsers) {
        if (user.socket == clientSocket) {
            user.username = username;
            break;
        }
    }
}

writeLog("Utilisateur connecté: " + username);
```

### 2. Boucle de réception des commandes

```cpp
while (g_serverRunning) {
    // Attendre des données (timeout 1 seconde)
    if (!SocketUtils::hasData(clientSocket, 1000)) {
        continue;
    }
    
    // Recevoir la commande
    received = SocketUtils::receiveWithLength(clientSocket, buffer, 255);
    if (received == 0) {
        // Client déconnecté
        break;
    }
    
    buffer[received] = '\0';
    string command(buffer);
    
    writeLog("Commande reçue de " + username + ": " + command);
    
    // Traiter la commande
    handleCommand(clientSocket, username, command);
}
```

### 3. Nettoyage à la déconnexion

```cpp
removeUser(clientSocket);
SocketUtils::closeSocket(clientSocket);
```

---

## 📋 Les commandes traitées par handleCommand

### SEND: - Envoyer un message

```cpp
if (command.substr(0, 5) == "SEND:") {
    // Recevoir les données du message
    char buffer[sizeof(Message)];
    size_t received = SocketUtils::receiveWithLength(clientSocket, buffer, sizeof(buffer));
    
    // Reconstruire le message
    Message msg = Message::deserialize(buffer, received);
    
    // L'ajouter à la file d'attente
    {
        lock_guard<mutex> lock(g_queueMutex);
        g_messageQueue.push(msg);
    }
    
    // Confirmer au client
    string response = "OK:Message en file d'attente";
    SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
}
```

### LIST_USERS - Lister les utilisateurs

```cpp
if (command == "LIST_USERS") {
    string userList;
    
    // Construire la liste
    {
        lock_guard<mutex> lock(g_usersMutex);
        for (const auto& user : g_connectedUsers) {
            userList += user.username + ";";
        }
    }
    
    // Envoyer la réponse
    string response = "USERS:" + userList;
    SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
}
```

### GET_LOG - Récupérer le fichier log

```cpp
if (command == "GET_LOG") {
    // Lire le fichier
    ifstream logFile("server.log");
    string logContent((istreambuf_iterator<char>(logFile)),
                       istreambuf_iterator<char>());
    
    // Envoyer
    string response = "LOG:" + logContent;
    SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
}
```

### DISCONNECT - Se déconnecter

```cpp
if (command == "DISCONNECT") {
    string response = "OK:Déconnexion";
    SocketUtils::sendWithLength(clientSocket, response.c_str(), response.length());
    // Le client va se déconnecter, on sort de la boucle
}
```

---

## 📬 Thread de livraison (Delivery Thread)

Ce thread se réveille **toutes les 30 secondes** et livre les messages :

```cpp
void deliveryThread() {
    writeLog("Thread de livraison démarré");
    
    while (g_serverRunning) {
        // Dormir 30 secondes
        this_thread::sleep_for(chrono::seconds(30));
        
        lock_guard<mutex> queueLock(g_queueMutex);
        
        if (g_messageQueue.empty()) {
            continue;  // Rien à livrer
        }
        
        writeLog("Livraison de " + to_string(g_messageQueue.size()) + " message(s)");
        
        // Traiter tous les messages
        while (!g_messageQueue.empty()) {
            Message msg = g_messageQueue.front();
            g_messageQueue.pop();
            
            // Horodater
            msg.receivedAt = time(nullptr);
            
            // Livrer
            if (string(msg.to) == "all") {
                broadcastMessage(msg);  // À tout le monde
            } else {
                if (isUserConnected(msg.to)) {
                    sendMessageToUser(msg.to, msg);  // À une personne
                } else {
                    // Notifier l'expéditeur de l'échec
                    sendNotificationToSender(msg.from, 
                        "NOTIFY:Échec - " + string(msg.to) + " non connecté");
                }
            }
            
            // Archiver
            {
                lock_guard<mutex> historyLock(g_historyMutex);
                g_messageHistory.push_back(msg);
            }
        }
    }
}
```

---

## 📤 Envoi des messages

### Envoyer à un utilisateur précis :

```cpp
void sendMessageToUser(const string& username, const Message& msg) {
    lock_guard<mutex> lock(g_usersMutex);
    
    // Trouver le socket de cet utilisateur
    for (const auto& user : g_connectedUsers) {
        if (user.username == username) {
            // Sérialiser le message
            char buffer[sizeof(Message) + 10];
            size_t size;
            msg.serialize(buffer, size);
            
            // Préparer le paquet : "MSG:" + données
            string header = "MSG:";
            vector<char> fullMessage(header.begin(), header.end());
            fullMessage.insert(fullMessage.end(), buffer, buffer + size);
            
            // Envoyer
            SocketUtils::sendWithLength(user.socket, fullMessage.data(), fullMessage.size());
            return;
        }
    }
}
```

### Broadcast (envoyer à tous) :

```cpp
void broadcastMessage(const Message& msg) {
    lock_guard<mutex> lock(g_usersMutex);
    
    for (const auto& user : g_connectedUsers) {
        // Exclure l'expéditeur (il n'a pas besoin de recevoir son propre message)
        if (user.username != string(msg.from)) {
            // Envoyer le message...
            // (même code que sendMessageToUser)
        }
    }
}
```

---

## 📝 Système de log

Toutes les actions sont enregistrées dans `server.log` :

```cpp
void writeLog(const string& message) {
    lock_guard<mutex> lock(g_logMutex);  // Protection multi-thread
    
    string timestamp = getCurrentTimestamp();  // Ex: "2024-12-09 14:30:45"
    string logEntry = "[" + timestamp + "] " + message;
    
    g_logFile << logEntry << endl;  // Écrire dans le fichier
    g_logFile.flush();               // S'assurer que c'est écrit
    cout << logEntry << endl;        // Afficher aussi sur la console
}
```

Exemple de contenu du fichier log :
```
[2024-12-09 14:30:00] === SERVEUR DÉMARRÉ ===
[2024-12-09 14:30:00] En écoute sur le port 8888
[2024-12-09 14:30:15] Utilisateur connecté: Alice
[2024-12-09 14:30:20] Utilisateur connecté: Bob
[2024-12-09 14:30:25] Commande reçue de Alice: SEND:
[2024-12-09 14:30:25] Message ajouté à la queue de Alice vers Bob
[2024-12-09 14:30:45] Livraison de 1 message(s)
[2024-12-09 14:30:45] Message livré de Alice à Bob
```

---

## 🚪 Arrêt du serveur

Le serveur s'arrête automatiquement quand le **dernier client** se déconnecte :

```cpp
void removeUser(SOCKET sock) {
    lock_guard<mutex> lock(g_usersMutex);
    
    // Trouver et supprimer l'utilisateur
    auto it = find_if(g_connectedUsers.begin(), g_connectedUsers.end(),
                      [sock](const ConnectedUser& user) { return user.socket == sock; });
    
    if (it != g_connectedUsers.end()) {
        string username = it->username;
        g_connectedUsers.erase(it);
        
        writeLog("Utilisateur retiré: " + username);
        
        // Si plus personne n'est connecté
        if (g_connectedUsers.empty()) {
            writeLog("Dernier client déconnecté - Arrêt du serveur");
            g_serverRunning = false;  // Arrêter le serveur
        }
    }
}
```

---

## 📊 Schéma complet du serveur

```
╔═════════════════════════════════════════════════════════════════════╗
║                           SERVEUR                                    ║
╠═════════════════════════════════════════════════════════════════════╣
║                                                                       ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │                    THREAD PRINCIPAL                           │   ║
║  │                                                               │   ║
║  │   socket() → bind(8888) → listen() → accept()                │   ║
║  │                                          │                    │   ║
║  │                                          ▼                    │   ║
║  │                              Créer thread par client          │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                       ║
║  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              ║
║  │ Thread User1 │  │ Thread User2 │  │ Thread User3 │   ...        ║
║  │              │  │              │  │              │              ║
║  │ - recv()     │  │ - recv()     │  │ - recv()     │              ║
║  │ - SEND:      │  │ - LIST_USERS │  │ - GET_LOG    │              ║
║  │ - etc.       │  │ - etc.       │  │ - etc.       │              ║
║  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              ║
║         │                 │                 │                       ║
║         └─────────────────┼─────────────────┘                       ║
║                           ▼                                          ║
║  ┌─────────────────────────────────────────────────────────────┐    ║
║  │                DONNÉES PARTAGÉES (protégées par mutex)       │    ║
║  │                                                              │    ║
║  │  g_connectedUsers  ←─────── Liste des utilisateurs          │    ║
║  │  g_messageQueue    ←─────── File d'attente des messages     │    ║
║  │  g_messageHistory  ←─────── Historique des messages         │    ║
║  │  g_logFile         ←─────── Fichier de log                  │    ║
║  └──────────────────────────────┬──────────────────────────────┘    ║
║                                 │                                    ║
║                                 ▼                                    ║
║  ┌──────────────────────────────────────────────────────────────┐   ║
║  │                   THREAD DE LIVRAISON                         │   ║
║  │                                                               │   ║
║  │   while (running) {                                          │   ║
║  │       sleep(30 secondes)                                     │   ║
║  │       pour chaque message en queue:                          │   ║
║  │           si destinataire == "all":                          │   ║
║  │               broadcastMessage()                             │   ║
║  │           sinon:                                             │   ║
║  │               sendMessageToUser()                            │   ║
║  │   }                                                          │   ║
║  └──────────────────────────────────────────────────────────────┘   ║
║                                                                       ║
╚═════════════════════════════════════════════════════════════════════╝
```

---

## 🎯 Ce qu'il faut retenir pour l'oral

> "Le **serveur** est le point central de l'application.
>
> Au démarrage, il crée une socket, se lie au **port 8888**, et se met en **écoute**.
>
> Pour chaque client qui se connecte, il crée un **thread dédié** (User Handler) qui gère les commandes de ce client.
>
> Les messages sont stockés dans une **file d'attente** et livrés par le **thread de livraison** toutes les **30 secondes**.
>
> Toutes les actions sont enregistrées dans un **fichier log** (`server.log`).
>
> Le serveur s'**arrête automatiquement** quand le dernier client se déconnecte."

---

➡️ **Fichier suivant** : `06_LE_CLIENT_EN_DETAIL.md` pour comprendre le fonctionnement du client
