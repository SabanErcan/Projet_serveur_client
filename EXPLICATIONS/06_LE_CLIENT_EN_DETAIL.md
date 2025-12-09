# 📱 LE CLIENT EN DÉTAIL

## 🎯 Rôle du client

Le client est l'**interface utilisateur** de l'application :
- Il permet de se connecter au serveur
- Il affiche un menu interactif
- Il envoie des commandes au serveur
- Il reçoit et affiche les messages

---

## 📁 Les données globales du client

```cpp
// Liste des messages reçus
vector<Message> g_receivedMessages;

// Protection de la liste des messages
mutex g_messagesMutex;

// Est-ce que l'utilisateur compose un message ?
atomic<bool> g_isComposing(false);

// Le client est-il actif ?
atomic<bool> g_clientRunning(true);

// Socket de connexion au serveur
SOCKET g_serverSocket = INVALID_SOCKET;

// Nom de l'utilisateur
string g_username;
```

---

## 🚀 Démarrage du client (fonction main)

### Étape par étape :

```
╔═══════════════════════════════════════════════════════════════╗
║                    DÉMARRAGE DU CLIENT                         ║
╠═══════════════════════════════════════════════════════════════╣
║                                                                 ║
║  1. Lire les arguments (optionnel)                             ║
║     └─► IP du serveur, port                                    ║
║                                                                 ║
║  2. Initialiser Winsock (Windows uniquement)                   ║
║                                                                 ║
║  3. Demander le nom d'utilisateur                              ║
║     └─► "Nom d'utilisateur: Alice"                             ║
║                                                                 ║
║  4. Se connecter au serveur                                    ║
║     └─► connect(serverIP, port)                                ║
║                                                                 ║
║  5. Envoyer le nom au serveur                                  ║
║                                                                 ║
║  6. Démarrer le thread d'écoute                                ║
║     └─► thread(listenThread)                                   ║
║                                                                 ║
║  7. Boucle du menu interactif                                  ║
║                                                                 ║
╚═══════════════════════════════════════════════════════════════╝
```

### Le code :

```cpp
int main(int argc, char* argv[]) {
    // 1. Arguments par défaut
    string serverIP = "127.0.0.1";  // Localhost
    int port = 8888;
    
    // Lire les arguments si fournis
    if (argc >= 2) serverIP = argv[1];
    if (argc >= 3) port = stoi(argv[2]);
    
    // 2. Initialiser le réseau
    SocketUtils::initializeWinsock();
    
    cout << "=== CLIENT DE MESSAGERIE ===" << endl;
    
    // 3. Demander le nom
    cout << "Nom d'utilisateur: ";
    getline(cin, g_username);
    
    // 4. Se connecter
    cout << "Connexion à " << serverIP << ":" << port << "..." << endl;
    g_serverSocket = SocketUtils::createTCPSocket();
    SocketUtils::connectToServer(g_serverSocket, serverIP, port);
    
    // 5. Envoyer le nom
    SocketUtils::sendWithLength(g_serverSocket, g_username.c_str(), g_username.length());
    
    cout << "Connecté avec succès!" << endl;
    
    // 6. Démarrer le thread d'écoute
    thread listener(listenThread);
    
    // 7. Boucle du menu
    while (g_clientRunning) {
        displayMenu();
        cout << "Tapez votre commande: ";
        
        int choice;
        cin >> choice;
        clearInputBuffer();
        
        switch (choice) {
            case 1: listMessages(); break;
            case 2: readMessage(); break;
            case 3: markAsRead(); break;
            case 4: composeMessage(); break;
            case 5: listOnlineUsers(); break;
            case 6: requestServerLog(); break;
            case 7: disconnect(); break;
            default: cout << "Commande inexistante." << endl;
        }
    }
    
    // Attendre le thread d'écoute
    listener.join();
    SocketUtils::closeSocket(g_serverSocket);
    
    return 0;
}
```

---

## 📋 Le menu affiché

```cpp
void displayMenu() {
    cout << "\n╔════════════════════════════════════════╗" << endl;
    cout << "║    MESSAGERIE INSTANTANÉE - CLIENT     ║" << endl;
    cout << "╠════════════════════════════════════════╣" << endl;
    cout << "║ 1. Lister les messages                 ║" << endl;
    cout << "║ 2. Lire un message                     ║" << endl;
    cout << "║ 3. Marquer comme lu                    ║" << endl;
    cout << "║ 4. Composer un message                 ║" << endl;
    cout << "║ 5. Lister les utilisateurs en ligne    ║" << endl;
    cout << "║ 6. Récupérer le log du serveur         ║" << endl;
    cout << "║ 7. Se déconnecter                      ║" << endl;
    cout << "╚════════════════════════════════════════╝" << endl;
}
```

Ce que l'utilisateur voit :
```
╔════════════════════════════════════════╗
║    MESSAGERIE INSTANTANÉE - CLIENT     ║
╠════════════════════════════════════════╣
║ 1. Lister les messages                 ║
║ 2. Lire un message                     ║
║ 3. Marquer comme lu                    ║
║ 4. Composer un message                 ║
║ 5. Lister les utilisateurs en ligne    ║
║ 6. Récupérer le log du serveur         ║
║ 7. Se déconnecter                      ║
╚════════════════════════════════════════╝
Tapez votre commande: _
```

---

## 👂 Thread d'écoute (Listen Thread)

Ce thread tourne en **arrière-plan** et écoute les messages du serveur :

```cpp
void listenThread() {
    char buffer[sizeof(Message) + 100];
    
    while (g_clientRunning) {
        // Vérifier si des données arrivent (timeout 1 seconde)
        if (!SocketUtils::hasData(g_serverSocket, 1000)) {
            continue;
        }
        
        // Recevoir les données
        size_t received = SocketUtils::receiveWithLength(g_serverSocket, buffer, sizeof(buffer) - 1);
        
        if (received == 0) {
            // Connexion perdue
            cout << "\n[SYSTÈME] Connexion au serveur perdue" << endl;
            g_clientRunning = false;
            break;
        }
        
        buffer[received] = '\0';
        string response(buffer, received);
        
        // Traiter selon le type de réponse
        // ...
    }
}
```

### Les différents types de réponses :

#### MSG: - Nouveau message reçu
```cpp
if (response.substr(0, 4) == "MSG:") {
    // Désérialiser le message
    Message msg = Message::deserialize(buffer + 4, received - 4);
    
    // Ajouter à notre liste
    {
        lock_guard<mutex> lock(g_messagesMutex);
        g_receivedMessages.push_back(msg);
    }
    
    // Afficher une notification (si on ne compose pas)
    if (!g_isComposing) {
        cout << "\n[NOUVEAU MESSAGE] De: " << msg.from 
             << " | Sujet: " << msg.subject << endl;
        cout << "Tapez votre commande: ";
        cout.flush();
    }
}
```

#### NOTIFY: - Notification
```cpp
if (response.substr(0, 7) == "NOTIFY:") {
    if (!g_isComposing) {
        cout << "\n[NOTIFICATION] " << response.substr(7) << endl;
    }
}
```

#### OK: - Confirmation
```cpp
if (response.substr(0, 3) == "OK:") {
    if (!g_isComposing) {
        cout << "\n[SERVEUR] " << response.substr(3) << endl;
    }
}
```

#### ERROR: - Erreur
```cpp
if (response.substr(0, 6) == "ERROR:") {
    if (!g_isComposing) {
        cout << "\n[ERREUR] " << response.substr(6) << endl;
    }
}
```

#### USERS: - Liste des utilisateurs
```cpp
if (response.substr(0, 6) == "USERS:") {
    string userList = response.substr(6);
    cout << "\n=== UTILISATEURS EN LIGNE ===" << endl;
    
    // Découper par ";"
    stringstream ss(userList);
    string user;
    int count = 0;
    while (getline(ss, user, ';')) {
        if (!user.empty()) {
            cout << "- " << user << endl;
            count++;
        }
    }
    cout << "Total: " << count << " utilisateur(s)" << endl;
}
```

#### LOG: - Fichier log du serveur
```cpp
if (response.substr(0, 4) == "LOG:") {
    string logContent = response.substr(4);
    cout << "\n=== FICHIER LOG DU SERVEUR ===" << endl;
    cout << logContent << endl;
}
```

---

## 📝 Les fonctionnalités du menu

### 1. Lister les messages

```cpp
void listMessages() {
    lock_guard<mutex> lock(g_messagesMutex);
    
    if (g_receivedMessages.empty()) {
        cout << "\nAucun message reçu." << endl;
        return;
    }
    
    cout << "\n=== MESSAGES REÇUS ===" << endl;
    for (size_t i = 0; i < g_receivedMessages.size(); ++i) {
        cout << i + 1 << ". " << g_receivedMessages[i].toShortString() << endl;
    }
    cout << "======================" << endl;
}
```

Exemple d'affichage :
```
=== MESSAGES REÇUS ===
1. [NON LU] De: Bob | Sujet: Salut !
2. [LU] De: Charlie | Sujet: Réunion
3. [NON LU] De: Bob | Sujet: Question
======================
```

### 2. Lire un message

```cpp
void readMessage() {
    cout << "\nChoisir un message par:" << endl;
    cout << "1. Indice" << endl;
    cout << "2. Sujet" << endl;
    cout << "Votre choix: ";
    
    int choice;
    cin >> choice;
    clearInputBuffer();
    
    lock_guard<mutex> lock(g_messagesMutex);
    
    if (choice == 1) {
        // Par indice
        cout << "Indice du message (1-" << g_receivedMessages.size() << "): ";
        int index;
        cin >> index;
        clearInputBuffer();
        
        if (index >= 1 && index <= (int)g_receivedMessages.size()) {
            cout << "\n" << g_receivedMessages[index - 1].toString() << endl;
        }
    } else if (choice == 2) {
        // Par sujet
        cout << "Sujet du message: ";
        string subject;
        getline(cin, subject);
        
        for (const auto& msg : g_receivedMessages) {
            if (string(msg.subject) == subject) {
                cout << "\n" << msg.toString() << endl;
                break;
            }
        }
    }
}
```

### 3. Marquer comme lu

```cpp
void markAsRead() {
    lock_guard<mutex> lock(g_messagesMutex);
    
    if (g_receivedMessages.empty()) {
        cout << "Aucun message." << endl;
        return;
    }
    
    cout << "Indice du message à marquer comme lu (1-" << g_receivedMessages.size() << "): ";
    int index;
    cin >> index;
    clearInputBuffer();
    
    if (index >= 1 && index <= (int)g_receivedMessages.size()) {
        g_receivedMessages[index - 1].isRead = true;
        cout << "Message marqué comme lu." << endl;
    }
}
```

### 4. Composer un message

```cpp
void composeMessage() {
    g_isComposing = true;  // Empêche les notifications de s'afficher
    
    cout << "\n=== COMPOSER UN MESSAGE ===" << endl;
    
    string to, subject, body;
    
    cout << "Destinataire (ou 'all' pour broadcast): ";
    getline(cin, to);
    
    cout << "Sujet (max 99 caractères): ";
    getline(cin, subject);
    
    cout << "Corps du message (max 499 caractères):" << endl;
    getline(cin, body);
    
    try {
        // Créer le message
        Message msg(g_username, to, subject, body);
        
        // Envoyer la commande SEND:
        string command = "SEND:";
        SocketUtils::sendWithLength(g_serverSocket, command.c_str(), command.length());
        
        // Envoyer le message sérialisé
        char buffer[sizeof(Message)];
        size_t size;
        msg.serialize(buffer, size);
        SocketUtils::sendWithLength(g_serverSocket, buffer, size);
        
        cout << "Message envoyé (sera livré dans max 30s)." << endl;
        
    } catch (const exception& e) {
        cout << "Erreur: " << e.what() << endl;
    }
    
    g_isComposing = false;  // Réactive les notifications
}
```

### 5. Lister les utilisateurs en ligne

```cpp
void listOnlineUsers() {
    string command = "LIST_USERS";
    SocketUtils::sendWithLength(g_serverSocket, command.c_str(), command.length());
    // La réponse sera traitée par le thread d'écoute
}
```

### 6. Récupérer le log du serveur

```cpp
void requestServerLog() {
    string command = "GET_LOG";
    SocketUtils::sendWithLength(g_serverSocket, command.c_str(), command.length());
    // La réponse sera traitée par le thread d'écoute
}
```

### 7. Se déconnecter

```cpp
void disconnect() {
    string command = "DISCONNECT";
    SocketUtils::sendWithLength(g_serverSocket, command.c_str(), command.length());
    
    g_clientRunning = false;  // Arrête le thread d'écoute
    cout << "\nDéconnexion..." << endl;
}
```

---

## 🎭 Le flag isComposing

### Problème :
Quand tu écris un message, tu ne veux pas être interrompu par une notification !

### Solution :
```cpp
// Avant de composer
g_isComposing = true;

// ... l'utilisateur tape son message ...

// Après avoir fini
g_isComposing = false;
```

Dans le thread d'écoute :
```cpp
if (!g_isComposing) {
    cout << "[NOUVEAU MESSAGE]..." << endl;  // Affiche seulement si pas en train de composer
}
```

---

## 📊 Schéma complet du client

```
╔═══════════════════════════════════════════════════════════════════╗
║                            CLIENT                                  ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                     ║
║  ┌────────────────────────────────────────────────────────────┐   ║
║  │                    THREAD PRINCIPAL                         │   ║
║  │                                                             │   ║
║  │   1. Connexion au serveur                                  │   ║
║  │   2. Envoi du nom d'utilisateur                            │   ║
║  │   3. Affichage du menu                                     │   ║
║  │   4. Attente des commandes (1-7)                           │   ║
║  │   5. Exécution des commandes                               │   ║
║  │                                                             │   ║
║  │   [1] listMessages()      → Afficher les messages reçus   │   ║
║  │   [2] readMessage()       → Lire un message complet       │   ║
║  │   [3] markAsRead()        → Marquer comme lu              │   ║
║  │   [4] composeMessage()    → Écrire et envoyer             │   ║
║  │   [5] listOnlineUsers()   → Demander liste utilisateurs   │   ║
║  │   [6] requestServerLog()  → Demander le fichier log       │   ║
║  │   [7] disconnect()        → Se déconnecter                │   ║
║  │                                                             │   ║
║  └────────────────────────────────────────────────────────────┘   ║
║                              │                                      ║
║                              ▼                                      ║
║  ┌────────────────────────────────────────────────────────────┐   ║
║  │                DONNÉES PARTAGÉES                            │   ║
║  │                                                             │   ║
║  │  g_receivedMessages 🔒  ← Messages reçus du serveur        │   ║
║  │  g_isComposing          ← En train de composer ?          │   ║
║  │  g_clientRunning        ← Client actif ?                  │   ║
║  │  g_serverSocket         ← Connexion au serveur            │   ║
║  │  g_username             ← Nom de l'utilisateur            │   ║
║  │                                                             │   ║
║  └────────────────────────────────────────────────────────────┘   ║
║                              ▲                                      ║
║                              │                                      ║
║  ┌────────────────────────────────────────────────────────────┐   ║
║  │                   THREAD D'ÉCOUTE                           │   ║
║  │                                                             │   ║
║  │   while (running) {                                        │   ║
║  │       if (données disponibles) {                           │   ║
║  │           recv(données)                                    │   ║
║  │                                                             │   ║
║  │           switch (type de réponse) {                       │   ║
║  │               "MSG:"    → Ajouter message à la liste       │   ║
║  │               "NOTIFY:" → Afficher notification            │   ║
║  │               "OK:"     → Afficher confirmation            │   ║
║  │               "ERROR:"  → Afficher erreur                  │   ║
║  │               "USERS:"  → Afficher liste utilisateurs      │   ║
║  │               "LOG:"    → Afficher contenu du log          │   ║
║  │           }                                                 │   ║
║  │       }                                                     │   ║
║  │   }                                                         │   ║
║  │                                                             │   ║
║  └────────────────────────────────────────────────────────────┘   ║
║                                                                     ║
╚═══════════════════════════════════════════════════════════════════╝
                              │
                              │ TCP/IP
                              │ Port 8888
                              ▼
                         ┌─────────┐
                         │ SERVEUR │
                         └─────────┘
```

---

## 🔄 Cycle d'utilisation typique

```
1. L'utilisateur lance le client
   ┌─────────────────────────────────────┐
   │ $ ./client.exe                      │
   │ Nom d'utilisateur: Alice            │
   │ Connexion à 127.0.0.1:8888...      │
   │ Connecté avec succès!               │
   └─────────────────────────────────────┘

2. Le menu s'affiche
   ┌─────────────────────────────────────┐
   │ ╔════════════════════════════════╗  │
   │ ║    MESSAGERIE - CLIENT         ║  │
   │ ╠════════════════════════════════╣  │
   │ ║ 1. Lister les messages         ║  │
   │ ║ ...                            ║  │
   │ ╚════════════════════════════════╝  │
   │ Tapez votre commande: 4             │
   └─────────────────────────────────────┘

3. Composition d'un message
   ┌─────────────────────────────────────┐
   │ === COMPOSER UN MESSAGE ===         │
   │ Destinataire: Bob                   │
   │ Sujet: Salut                        │
   │ Corps: Comment vas-tu ?             │
   │ Message envoyé!                     │
   └─────────────────────────────────────┘

4. Un message arrive (en arrière-plan)
   ┌─────────────────────────────────────┐
   │ [NOUVEAU MESSAGE] De: Charlie       │
   │ Tapez votre commande:               │
   └─────────────────────────────────────┘

5. Déconnexion
   ┌─────────────────────────────────────┐
   │ Tapez votre commande: 7             │
   │ Déconnexion...                      │
   │ Client terminé.                     │
   └─────────────────────────────────────┘
```

---

## 🎯 Ce qu'il faut retenir pour l'oral

> "Le **client** est l'interface utilisateur de l'application.
>
> Au démarrage, il demande un **nom d'utilisateur**, se **connecte au serveur**, et envoie son nom.
>
> Il utilise **2 threads** :
> - Le **thread principal** affiche le menu et exécute les commandes
> - Le **thread d'écoute** reçoit les messages du serveur en arrière-plan
>
> Le flag `g_isComposing` empêche les **notifications d'interrompre** la saisie.
>
> L'utilisateur peut **envoyer des messages**, **lister les utilisateurs connectés**, **lire ses messages**, et se **déconnecter proprement**."

---

➡️ **Fichier suivant** : `07_PROTOCOLE_DE_COMMUNICATION.md` pour comprendre le format des échanges
