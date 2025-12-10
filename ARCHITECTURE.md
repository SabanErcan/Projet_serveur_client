# Architecture Technique du Projet

## Diagramme de l'Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         SERVEUR (serveur.cpp)                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌────────────────┐      ┌──────────────────────────────────┐  │
│  │ Thread Principal│◄─────┤  Variables Globales Partagées   │  │
│  │                │      │                                  │  │
│  │ - socket()     │      │  g_connectedUsers  (mutex)      │  │
│  │ - bind()       │      │  g_messageQueue    (mutex)      │  │
│  │ - listen()     │      │  g_messageHistory  (mutex)      │  │
│  │ - accept() ─┐  │      │  g_logFile         (mutex)      │  │
│  └────────────┼───┘      └──────────────────────────────────┘  │
│               │                                                  │
│               ├───► Crée User Handler Thread                    │
│               │                                                  │
│  ┌────────────▼─────────┐        ┌─────────────────────────┐  │
│  │ User Handler Threads │        │  Delivery Thread        │  │
│  │ (1 par client)       │        │                         │  │
│  │                      │        │  Boucle toutes les 30s  │  │
│  │ - recv() commandes   │        │                         │  │
│  │ - SEND: → queue      │        │  while (running) {      │  │
│  │ - LIST_USERS         │◄──────►│    sleep(30s)           │  │
│  │ - GET_LOG            │ mutex  │    traiter_queue()      │  │
│  │ - DISCONNECT         │        │    send() messages      │  │
│  └──────────────────────┘        └─────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ TCP/IP Socket
                              │ Port 8888
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         CLIENT (client.cpp)                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────────┐         ┌──────────────────────────────┐ │
│  │ Thread Principal │◄────────┤  Variables Globales          │ │
│  │                  │         │                              │ │
│  │ - Menu UI        │         │  g_receivedMessages (mutex)  │ │
│  │ - Commandes      │         │  g_isComposing      (atomic) │ │
│  │ - Composer msg   │         │  g_clientRunning    (atomic) │ │
│  │ - send()         │         │  g_serverSocket              │ │
│  └──────────────────┘         └──────────────────────────────┘ │
│                                                                  │
│  ┌──────────────────────────┐                                   │
│  │  Listen Thread           │                                   │
│  │                          │                                   │
│  │  while (running) {       │                                   │
│  │    select() / recv()     │                                   │
│  │    if (MSG:)             │                                   │
│  │      → g_receivedMessages│                                   │
│  │      → afficher notif    │                                   │
│  │    if (USERS:)           │                                   │
│  │      → afficher liste    │                                   │
│  │  }                       │                                   │
│  └──────────────────────────┘                                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Flux de Messages

### Envoi d'un Message (Alice → Bob)

```
1. Alice (Client)
   ├─► Commande 4 (Composer)
   ├─► Saisie: To="Bob", Subject="Test", Body="Bonjour"
   ├─► Validation (limites de taille)
   ├─► Création Message en mémoire
   └─► send("SEND:") + send(Message sérialisé)

2. Serveur (User Handler Thread d'Alice)
   ├─► recv("SEND:")
   ├─► recv(Message)
   ├─► Désérialisation
   ├─► lock(g_queueMutex)
   ├─► g_messageQueue.push(message)
   ├─► unlock(g_queueMutex)
   └─► send("OK:Message en file d'attente")

3. Serveur (Delivery Thread)
   ├─► sleep(30 secondes)
   ├─► lock(g_queueMutex)
   ├─► while (!queue.empty())
   │   ├─► msg = queue.front()
   │   ├─► queue.pop()
   │   ├─► if (to == "all") → broadcastMessage()
   │   │   └─► Pour chaque utilisateur sauf from
   │   │       └─► send("MSG:") + send(Message)
   │   └─► else → sendMessageToUser(to, msg)
   │       ├─► lock(g_usersMutex)
   │       ├─► Trouver socket de "Bob"
   │       ├─► unlock(g_usersMutex)
   │       └─► send("MSG:") + send(Message)
   ├─► unlock(g_queueMutex)
   └─► lock(g_historyMutex)
       └─► g_messageHistory.push_back(msg)

4. Bob (Client - Listen Thread)
   ├─► select() détecte données
   ├─► recv() → "MSG:" + Message
   ├─► Désérialisation du Message
   ├─► lock(g_messagesMutex)
   ├─► g_receivedMessages.push_back(msg)
   ├─► unlock(g_messagesMutex)
   └─► if (!g_isComposing)
       └─► Afficher notification

5. Bob (Client - Thread Principal)
   ├─► Commande 1 (Lister)
   ├─► lock(g_messagesMutex)
   ├─► for (msg in g_receivedMessages)
   │   └─► Afficher msg.toShortString()
   └─► unlock(g_messagesMutex)
```

## Gestion des Threads

### Serveur - Thread Principal
```cpp
int main() {
    // Initialisation
    SOCKET serverSocket = createSocket();
    bind(serverSocket, 8888);
    listen(serverSocket, 5);
    
    // Lancer thread de livraison
    thread deliveryThread(deliveryThread);
    
    // Boucle d'acceptation
    while (g_serverRunning) {
        SOCKET clientSocket = accept(serverSocket);
        
        // Créer thread pour ce client
        thread* userThread = new thread(userHandlerThread, clientSocket);
        
        // Enregistrer dans g_connectedUsers
        lock_guard<mutex> lock(g_usersMutex);
        g_connectedUsers.push_back({username, clientSocket, userThread});
    }
    
    // Attendre la fin
    deliveryThread.join();
    for (user : g_connectedUsers) {
        user.thread->join();
        delete user.thread;
    }
}
```

### Serveur - User Handler Thread
```cpp
void userHandlerThread(SOCKET clientSocket, string clientIP) {
    // Recevoir le nom d'utilisateur
    recv(clientSocket, username);
    
    // Boucle de commandes
    while (g_serverRunning) {
        if (!hasData(clientSocket, 1000))
            continue;
            
        string command;
        recv(clientSocket, command);
        
        if (command == "SEND:") {
            Message msg;
            recv(clientSocket, msg);
            
            lock_guard<mutex> lock(g_queueMutex);
            g_messageQueue.push(msg);
            
            send(clientSocket, "OK");
        }
        // ... autres commandes
    }
    
    // Nettoyage
    removeUser(clientSocket);
    closeSocket(clientSocket);
}
```

### Serveur - Delivery Thread
```cpp
void deliveryThread() {
    while (g_serverRunning) {
        sleep_for(seconds(30));
        
        lock_guard<mutex> queueLock(g_queueMutex);
        
        while (!g_messageQueue.empty()) {
            Message msg = g_messageQueue.front();
            g_messageQueue.pop();
            
            if (msg.to == "all") {
                broadcastMessage(msg);
            } else {
                sendMessageToUser(msg.to, msg);
            }
            
            lock_guard<mutex> historyLock(g_historyMutex);
            g_messageHistory.push_back(msg);
        }
    }
}
```

### Client - Thread Principal
```cpp
int main() {
    // Connexion
    SOCKET serverSocket = createSocket();
    connect(serverSocket, "127.0.0.1", 8888);
    send(serverSocket, username);
    
    // Lancer thread d'écoute
    thread listener(listenThread);
    
    // Boucle menu
    while (g_clientRunning) {
        displayMenu();
        int choice;
        cin >> choice;
        
        switch (choice) {
            case 1: listMessages(); break;
            case 4: composeMessage(); break;
            // ...
        }
    }
    
    listener.join();
    closeSocket(serverSocket);
}
```

### Client - Listen Thread
```cpp
void listenThread() {
    while (g_clientRunning) {
        if (!hasData(g_serverSocket, 1000))
            continue;
            
        string response;
        recv(g_serverSocket, response);
        
        if (response.starts_with("MSG:")) {
            Message msg = deserialize(response.substr(4));
            
            lock_guard<mutex> lock(g_messagesMutex);
            g_receivedMessages.push_back(msg);
            
            if (!g_isComposing) {
                cout << "Nouveau message de " << msg.from << endl;
            }
        }
        // ... autres types de réponses
    }
}
```

## Points de Synchronisation

### 1. g_connectedUsers (serveur)
```
Protégé par: g_usersMutex

Accès en lecture:
- userHandlerThread (envoyer message)
- deliveryThread (broadcast)
- handleCommand (LIST_USERS)

Accès en écriture:
- main (ajout nouveau client)
- removeUser (suppression client)

Danger: Deadlock si verrouillage dans ordre différent
```

### 2. g_messageQueue (serveur)
```
Protégé par: g_queueMutex

Accès en écriture:
- userHandlerThread (commande SEND:)

Accès en lecture + écriture:
- deliveryThread (pop et traitement)

Pattern: Producer-Consumer
```

### 3. g_receivedMessages (client)
```
Protégé par: g_messagesMutex

Accès en écriture:
- listenThread (nouveau message)

Accès en lecture:
- Thread principal (liste, lecture)

Accès en lecture + écriture:
- Thread principal (marquer comme lu)
```

### 4. g_isComposing (client)
```
Type: atomic<bool>

Pas besoin de mutex (opérations atomiques)

Utilisé par:
- Thread principal (set true/false)
- listenThread (read pour décider d'afficher)

Évite: Pollution d'affichage pendant composition
```

## Protocole de Communication

### Format des Messages

#### 1. Connexion Initiale
```
Client → Serveur:
  [USERNAME]\0

Exemple: "Alice"
```

#### 2. Envoi de Message
```
Client → Serveur:
  "SEND:" (5 octets)
  [Structure Message sérialisée] (sizeof(Message) octets)

Serveur → Client:
  "OK:Message en file d'attente"
  ou
  "ERROR:Message mal formaté"
```

#### 3. Réception de Message
```
Serveur → Client:
  "MSG:" (4 octets)
  [Structure Message sérialisée] (sizeof(Message) octets)

Client:
  Désérialise automatiquement
  Ajoute à g_receivedMessages
  Affiche notification
```

#### 4. Liste Utilisateurs
```
Client → Serveur:
  "LIST_USERS"

Serveur → Client:
  "USERS:Alice;Bob;Charlie;"
```

#### 5. Récupération Log
```
Client → Serveur:
  "GET_LOG"

Serveur → Client:
  "LOG:[contenu du fichier server.log]"
```

#### 6. Déconnexion
```
Client → Serveur:
  "DISCONNECT"

Serveur → Client:
  "OK:Déconnexion"

Serveur:
  - Ferme le socket
  - Retire de g_connectedUsers
  - Si dernier client → g_serverRunning = false
```

## Cas d'Usage Complexes

### Cas 1: Trois Clients Envoient Simultanément

```
T=0s:  Alice, Bob, Charlie connectés
       Serveur: 3 User Handler Threads actifs

T=1s:  Alice envoie msg à Bob
       → User Handler d'Alice: lock(queueMutex) → push → unlock

T=2s:  Bob envoie msg broadcast
       → User Handler de Bob: lock(queueMutex) → push → unlock

T=3s:  Charlie envoie msg à Alice
       → User Handler de Charlie: lock(queueMutex) → push → unlock

T=30s: Delivery Thread se réveille
       → lock(queueMutex)
       → Traite les 3 messages dans l'ordre FIFO
       → unlock(queueMutex)
       
Résultat:
- Pas de race condition grâce aux mutex
- Ordre préservé (FIFO)
- Tous les destinataires reçoivent leur message
```

### Cas 2: Client Se Déconnecte Pendant Envoi

```
T=0s:  Alice compose un message pour Bob

T=5s:  Bob se déconnecte brutalement (ferme terminal)
       → recv() dans son User Handler retourne 0
       → removeUser(Bob)
       → lock(usersMutex) → erase Bob → unlock

T=10s: Alice finit de composer et envoie
       → Message ajouté à la queue

T=30s: Delivery Thread traite
       → sendMessageToUser("Bob", msg)
       → lock(usersMutex)
       → Cherche Bob → pas trouvé
       → unlock(usersMutex)
       → writeLog("Utilisateur destinataire inexistant: Bob")
       
Résultat:
- Message perdu (normale, Bob déconnecté)
- Pas de crash
- Log créé
```

## Optimisations Possibles

### 1. Pool de Threads
```cpp
// Au lieu de créer un thread par client
// Créer un pool fixe de threads worker

class ThreadPool {
    queue<function<void()>> tasks;
    vector<thread> workers;
    mutex queueMutex;
    condition_variable condition;
    
    void worker() {
        while (true) {
            unique_lock<mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !tasks.empty(); });
            
            auto task = tasks.front();
            tasks.pop();
            lock.unlock();
            
            task(); // Exécuter la tâche
        }
    }
};
```

### 2. Livraison Immédiate Option
```cpp
// Au lieu d'attendre toujours 30s
// Livrer immédiatement si urgent

void deliveryThread() {
    while (g_serverRunning) {
        unique_lock<mutex> lock(g_queueMutex);
        
        if (g_messageQueue.empty()) {
            g_queueCondition.wait_for(lock, seconds(30));
        }
        
        // Traiter la queue
    }
}

// Lors de SEND:
g_queueCondition.notify_one(); // Réveiller le delivery thread
```

### 3. Messages à Taille Variable (BONUS)
```cpp
struct MessageHeader {
    uint32_t fromSize;
    uint32_t toSize;
    uint32_t subjectSize;
    uint32_t bodySize;
};

void sendVariableMessage(SOCKET sock, const Message& msg) {
    MessageHeader header;
    header.fromSize = strlen(msg.from);
    header.toSize = strlen(msg.to);
    header.subjectSize = strlen(msg.subject);
    header.bodySize = strlen(msg.body);
    
    // Envoyer header
    send(sock, &header, sizeof(header));
    
    // Envoyer données
    send(sock, msg.from, header.fromSize);
    send(sock, msg.to, header.toSize);
    send(sock, msg.subject, header.subjectSize);
    send(sock, msg.body, header.bodySize);
}
```

## Métriques et Performances

### Complexités

**Serveur:**
- Ajout message à queue: O(1)
- Recherche utilisateur: O(n) où n = nombre d'utilisateurs
- Broadcast: O(n) où n = nombre d'utilisateurs

**Client:**
- Ajout message reçu: O(1)
- Liste messages: O(m) où m = nombre de messages
- Recherche par sujet: O(m)

### Limites Actuelles

- **Max utilisateurs**: Limité par le nombre de threads (OS dependent)
- **Taille message**: ~700 octets (structure fixe)
- **Latence livraison**: 0-30 secondes
- **Historique**: Illimité (croissance mémoire)

### Améliorations Futures

1. Limiter taille historique (garder seulement N derniers)
2. Persistance sur disque (sauvegarder messages)
3. Compression des messages
4. Chiffrement TLS/SSL
5. Authentification robuste
6. Heartbeat pour détecter déconnexions
