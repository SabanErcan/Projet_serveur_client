# PREPARATION SOUTENANCE - GUIDE PAR CRITERE

## RAPPEL IMPORTANT
- Soutenance : 13h00 (pas 13h30)
- Arriver un peu avant son créneau

---

# PARTIE 1 : CRITERES DU BAREME (Note de binome)

---

## 1. CONNEXION DES UTILISATEURS (1 pt)

**Ou montrer :** `client.cpp` lignes 430-450

**Ce qu'on fait :**
- Le client demande un nom d'utilisateur
- Il se connecte au serveur via socket TCP
- Il envoie son nom au serveur

**Code a montrer :**
```cpp
// client.cpp - main()
std::cout << "Nom d'utilisateur: ";
std::getline(std::cin, g_username);

g_serverSocket = SocketUtils::createTCPSocket();
SocketUtils::connectToServer(g_serverSocket, serverIP, port);
SocketUtils::sendWithLength(g_serverSocket, g_username.c_str(), g_username.length());
```

**Demonstration :** Lancer le client, entrer un nom, montrer "Connecte avec succes"

---

## 2. STRUCTURE DU MESSAGE (1 pt)

**Ou montrer :** `message.h`

**Ce qu'on fait :**
- Structure avec champs de taille fixe
- Serialisation/deserialisation pour envoi reseau

**Code a montrer :**
```cpp
struct Message {
    char from[MAX_FROM_SIZE];      // 50 caracteres
    char to[MAX_TO_SIZE];          // 50 caracteres  
    char subject[MAX_SUBJECT_SIZE]; // 100 caracteres
    char body[MAX_BODY_SIZE];       // 500 caracteres
    bool isRead;
    time_t receivedAt;
    
    void serialize(char* buffer, size_t& size);
    static Message deserialize(const char* buffer, size_t size);
};
```

**Explication :** "Taille fixe pour faciliter la serialisation. On sait toujours qu'un message fait exactement 709 octets."

---

## 3. STRUCTURE DU SERVEUR

### 3.1 Thread principal (2 pts)

**Ou montrer :** `serveur.cpp` lignes 500-550

**Ce qu'on fait :**
- Cree le socket serveur
- bind() sur le port 8888
- listen() pour attendre les connexions
- accept() dans une boucle pour accepter les clients

**Code a montrer :**
```cpp
// serveur.cpp - main()
SOCKET serverSocket = SocketUtils::createTCPSocket();
SocketUtils::bindSocket(serverSocket, PORT);
SocketUtils::listenSocket(serverSocket);

while (g_serverRunning) {
    SOCKET clientSocket = SocketUtils::acceptConnection(serverSocket, clientIP);
    std::thread* userThread = new std::thread(userHandlerThread, clientSocket, clientIP);
}
```

### 3.2 Thread utilisateur (2 pts)

**Ou montrer :** `serveur.cpp` fonction `userHandlerThread`

**Ce qu'on fait :**
- Un thread par client connecte
- Recoit les commandes du client
- Traite SEND:, LIST_USERS, GET_LOG, DISCONNECT

**Code a montrer :**
```cpp
void userHandlerThread(SOCKET clientSocket, std::string clientIP) {
    // Recevoir le nom
    recv(clientSocket, username);
    
    // Boucle de commandes
    while (g_serverRunning) {
        recv(clientSocket, command);
        handleCommand(clientSocket, username, command);
    }
}
```

### 3.3 Thread de dispatch/livraison (4 pts)

**Ou montrer :** `serveur.cpp` fonction `deliveryThread`

**Ce qu'on fait :**
- Se reveille toutes les 30 secondes
- Vide la file d'attente des messages
- Envoie chaque message a son destinataire

**Code a montrer :**
```cpp
void deliveryThread() {
    while (g_serverRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        
        std::lock_guard<std::mutex> lock(g_queueMutex);
        
        while (!g_messageQueue.empty()) {
            Message msg = g_messageQueue.front();
            g_messageQueue.pop();
            
            if (std::string(msg.to) == "all") {
                broadcastMessage(msg);
            } else {
                sendMessageToUser(msg.to, msg);
            }
        }
    }
}
```

**Demonstration :** Montrer qu'un message envoye arrive apres max 30 secondes

### 3.4 Gestion messages (3 pts)

**Ou montrer :** Variables globales `serveur.cpp`

**Ce qu'on fait :**
- File d'attente : `std::queue<Message> g_messageQueue`
- Historique : `std::vector<Message> g_messageHistory`
- Protection par mutex

### 3.5 Log (2 pts)

**Ou montrer :** `serveur.cpp` fonction `writeLog`

**Ce qu'on fait :**
- Fichier `server.log` avec horodatage
- Toutes les operations sont enregistrees

**Demonstration :** Montrer le contenu de server.log apres quelques operations

---

## 4. STRUCTURE DU CLIENT (threads)

### 4.1 Thread principal (2 pts)

**Ou montrer :** `client.cpp` fonction `main()`

**Ce qu'on fait :**
- Menu interactif
- Gere les choix de l'utilisateur (1-7)

### 4.2 Thread listener (3 pts)

**Ou montrer :** `client.cpp` fonction `listenThread()`

**Ce qu'on fait :**
- Tourne en arriere-plan
- Ecoute les messages du serveur
- Traite MSG:, NOTIFY:, OK:, ERROR:, USERS:, LOG:

**Code a montrer :**
```cpp
void listenThread() {
    while (g_clientRunning) {
        if (!SocketUtils::hasData(g_serverSocket, 1000)) continue;
        
        recv(g_serverSocket, buffer);
        
        if (response.substr(0, 4) == "MSG:") {
            // Nouveau message recu
            Message msg = Message::deserialize(buffer + 4, received - 4);
            g_receivedMessages.push_back(msg);
        }
    }
}
```

---

## 5. GESTION DES ERREURS (1 pt)

**Ou montrer :** Partout dans le code

**Ce qu'on fait :**
- try/catch autour des operations reseau
- Validation des entrees utilisateur
- Messages d'erreur clairs

**Exemples :**
- Message trop long -> erreur de validation
- Destinataire inexistant -> log dans server.log
- Commande inconnue -> "Commande inexistante"

---

## 6. FONCTIONNALITES CLIENT

### 6.1 Liste messages recus (3 pts)

**Ou montrer :** `client.cpp` fonction `listMessages()`

**Demonstration :** Menu choix 1, montrer la liste avec statut lu/non lu

### 6.2 Lecture Log (4 pts)

**Ou montrer :** `client.cpp` fonction `requestServerLog()`

**Demonstration :** Menu choix 6, montrer le contenu du log

### 6.3 Marquer messages lus (1 pt)

**Ou montrer :** `client.cpp` fonction `markAsRead()`

**Demonstration :** Menu choix 3, marquer un message, relister pour voir le changement

### 6.4 Lire corps du message par indice (1 pt)

**Ou montrer :** `client.cpp` fonction `readMessage()`

**Demonstration :** Menu choix 2, puis choix 1 (par indice), entrer le numero

---

## 7. LISTER UTILISATEURS CONNECTES (2 pts)

**Ou montrer :** `client.cpp` fonction `listOnlineUsers()`

**Demonstration :** 
1. Lancer serveur
2. Connecter 2-3 clients avec des noms differents
3. Menu choix 5 sur un client
4. Montrer la liste

---

## 8. REDACTION D'UN MESSAGE

### 8.1 Redaction (2 pts)

**Ou montrer :** `client.cpp` fonction `composeMessage()`

**Demonstration :** Menu choix 4, saisir destinataire, sujet, corps

### 8.2 Notifications (4 pts)

**Ou montrer :** `client.cpp` dans `listenThread()`

**Ce qu'on fait :**
- Variable `g_isComposing` bloque les notifications pendant la saisie
- Affiche "[NOUVEAU MESSAGE]" quand un message arrive

**Demonstration :** Pendant qu'un client attend, envoyer un message depuis un autre client

### 8.3 Envoi a un utilisateur (2 pts)

**Demonstration :** Envoyer un message a "Bob", verifier que Bob le recoit

### 8.4 Broadcast - all (3 pts)

**Demonstration :** Envoyer un message a "all", verifier que tous les autres clients le recoivent

---

## 9. FERMER CONNEXION CLIENT (4 pts)

**Ou montrer :** `client.cpp` fonction `disconnect()`

**Demonstration :** Menu choix 7, montrer "Deconnexion..." et "Client termine."

---

## 10. ARRET SERVEUR (1 pt)

**Ou montrer :** `serveur.cpp` fonction `removeUser()`

**Ce qu'on fait :**
- Quand le dernier client se deconnecte
- Le serveur s'arrete automatiquement

**Code a montrer :**
```cpp
if (g_connectedUsers.empty()) {
    writeLog("Dernier client deconnecte - Arret du serveur");
    g_serverRunning = false;
}
```

**Demonstration :** Connecter un seul client, le deconnecter, voir le serveur s'arreter

---

# PARTIE 2 : BONUS POSSIBLES (max 2 pts sur le controle papier)

## Structuration du code (jusqu'a 4 pts bonus)

**Ce qu'on a fait :**
- Separation en fichiers : message.h/cpp, socket_utils.h/cpp, serveur.cpp, client.cpp
- Commentaires explicatifs
- Fonctions bien decoupees
- Noms de variables clairs (g_ pour globales)

## Messages a taille illimitee

**Non implemente** - On utilise des tailles fixes pour simplifier la serialisation

## Configurations additionnelles (jusqu'a 2 pts bonus)

**Ce qu'on a fait :**
- Port configurable via arguments : `./client 192.168.1.10 9000`
- IP serveur configurable
- Intervalle de livraison modifiable (constante DELIVERY_INTERVAL_SECONDS)

---

# PARTIE 3 : QUESTIONS INDIVIDUELLES POSSIBLES

## Questions sur les THREADS

**Q: Pourquoi utiliser des threads ?**
R: Pour gerer plusieurs clients en meme temps. Sans threads, le serveur ne pourrait traiter qu'un client a la fois.

**Q: Qu'est-ce qu'un mutex ?**
R: Un verrou qui empeche deux threads d'acceder aux memes donnees en meme temps. On lock() avant d'acceder, on unlock() apres.

**Q: Difference entre mutex et atomic ?**
R: Mutex = faire la queue (un thread a la fois). Atomic = operation si rapide qu'elle ne peut pas etre interrompue.

## Questions sur les SOCKETS

**Q: Qu'est-ce qu'un socket ?**
R: Un point de communication entre deux programmes sur le reseau. Comme un telephone.

**Q: Difference TCP et UDP ?**
R: TCP = fiable, ordre garanti, plus lent. UDP = rapide, pas garanti. On utilise TCP pour la messagerie.

**Q: C'est quoi bind() ?**
R: Associer le socket a une adresse IP et un port. "Cette prise ecoute sur le port 8888."

**Q: C'est quoi accept() ?**
R: Attendre et accepter une connexion entrante. Retourne un nouveau socket pour ce client.

## Questions sur la SERIALISATION

**Q: Pourquoi serialiser ?**
R: Pour envoyer un objet sur le reseau, il faut le transformer en octets. Les programmes ne partagent pas la meme memoire.

**Q: C'est quoi memcpy ?**
R: Copier des octets d'un endroit a un autre en memoire.

## Questions sur le CODE

**Q: Pourquoi g_ devant les variables ?**
R: Convention pour indiquer que c'est une variable globale.

**Q: C'est quoi lock_guard ?**
R: Un objet qui lock le mutex automatiquement et le unlock quand il sort du scope. Plus sur que lock/unlock manuel.

**Q: Pourquoi les messages arrivent en 30 secondes ?**
R: C'est le thread de livraison qui se reveille toutes les 30 secondes pour distribuer les messages en attente.

---

# ORDRE DE DEMONSTRATION SUGGERE

1. **Lancer le serveur** - montrer les logs de demarrage
2. **Connecter client 1 (Alice)** - montrer la connexion
3. **Connecter client 2 (Bob)** - montrer la connexion
4. **Lister utilisateurs** (choix 5) - montrer Alice et Bob
5. **Envoyer message Alice->Bob** (choix 4) - montrer la composition
6. **Attendre ~30s** - montrer la notification chez Bob
7. **Lister messages chez Bob** (choix 1) - montrer le message
8. **Lire le message** (choix 2) - montrer le contenu complet
9. **Marquer comme lu** (choix 3) - montrer le changement de statut
10. **Broadcast** (choix 4, destinataire "all") - montrer que tous recoivent
11. **Recuperer le log** (choix 6) - montrer server.log
12. **Deconnecter Bob** (choix 7) - montrer la deconnexion
13. **Deconnecter Alice** - montrer l'arret automatique du serveur
