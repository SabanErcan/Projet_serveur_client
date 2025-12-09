# 🔌 LES SOCKETS EXPLIQUÉES SIMPLEMENT

## 🤔 C'est quoi une socket ?

Imagine que tu veux appeler quelqu'un au téléphone :

```
TOI ────► TÉLÉPHONE ────► RÉSEAU ────► TÉLÉPHONE ────► TON AMI
```

La **socket** c'est ton **téléphone** dans le monde informatique !
- C'est le "point de connexion" entre deux programmes
- Elle permet d'envoyer et recevoir des données sur le réseau

---

## 📞 Analogie complète : Le téléphone

| Action téléphone | Action socket | Fonction en C++ |
|-----------------|---------------|-----------------|
| Acheter un téléphone | Créer une socket | `socket()` |
| Avoir un numéro | Lier à un port | `bind()` |
| Attendre qu'on t'appelle | Écouter | `listen()` |
| Décrocher | Accepter la connexion | `accept()` |
| Appeler quelqu'un | Se connecter | `connect()` |
| Parler | Envoyer des données | `send()` |
| Écouter | Recevoir des données | `recv()` |
| Raccrocher | Fermer la socket | `close()` |

---

## 🏠 Comprendre l'adresse IP et le port

### L'adresse IP = L'adresse de la maison
```
Exemple : 192.168.1.10
C'est l'adresse unique de l'ordinateur sur le réseau
```

### Le port = Le numéro de l'appartement
```
Exemple : 8888
C'est le "numéro de porte" du programme sur cet ordinateur
```

### Ensemble :
```
192.168.1.10:8888 = "Ordinateur 192.168.1.10, programme sur le port 8888"
```

---

## 🖥️ Côté SERVEUR : Comment ça marche ?

### Étape par étape :

```
1. CRÉER LA SOCKET
   ┌─────────────┐
   │   socket()  │  → "J'achète un téléphone"
   └─────────────┘

2. BIND (lier au port)
   ┌─────────────────────┐
   │  bind(port 8888)    │  → "Mon numéro c'est le 8888"
   └─────────────────────┘

3. LISTEN (écouter)
   ┌─────────────┐
   │   listen()  │  → "J'attends qu'on m'appelle"
   └─────────────┘

4. ACCEPT (accepter)
   ┌─────────────┐
   │   accept()  │  → "Quelqu'un appelle ! Je décroche !"
   └─────────────┘
   
   ↓ Retourne une NOUVELLE socket pour parler avec ce client
```

### Le code simplifié :
```cpp
// 1. Créer la socket serveur
SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

// 2. Lier au port 8888
sockaddr_in addr;
addr.sin_port = htons(8888);  // Port 8888
bind(serverSocket, (sockaddr*)&addr, sizeof(addr));

// 3. Écouter (max 5 connexions en attente)
listen(serverSocket, 5);

// 4. Accepter une connexion
SOCKET clientSocket = accept(serverSocket, ...);
// Maintenant on peut parler avec ce client !
```

---

## 👤 Côté CLIENT : Comment ça marche ?

### Étape par étape :

```
1. CRÉER LA SOCKET
   ┌─────────────┐
   │   socket()  │  → "J'achète un téléphone"
   └─────────────┘

2. CONNECT (se connecter)
   ┌─────────────────────────────────┐
   │  connect(192.168.1.10, 8888)   │  → "J'appelle le serveur"
   └─────────────────────────────────┘

   ↓ Si le serveur accepte, on est connecté !
```

### Le code simplifié :
```cpp
// 1. Créer la socket client
SOCKET mySocket = socket(AF_INET, SOCK_STREAM, 0);

// 2. Se connecter au serveur
sockaddr_in serverAddr;
serverAddr.sin_addr.s_addr = inet_addr("192.168.1.10");
serverAddr.sin_port = htons(8888);
connect(mySocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

// Maintenant on peut parler au serveur !
```

---

## 📤 Envoyer et recevoir des données

### Envoyer : `send()`
```cpp
// Envoyer "Bonjour" au serveur
char message[] = "Bonjour";
send(socket, message, strlen(message), 0);
```

### Recevoir : `recv()`
```cpp
// Recevoir des données
char buffer[256];
int octetsRecus = recv(socket, buffer, sizeof(buffer), 0);
// buffer contient maintenant les données reçues
```

---

## 🔧 Le fichier socket_utils.h du projet

Le projet utilise une **classe utilitaire** pour simplifier tout ça :

```cpp
class SocketUtils {
    // Créer une socket TCP
    static SOCKET createTCPSocket();
    
    // Lier au port
    static void bindSocket(SOCKET sock, int port);
    
    // Écouter
    static void listenSocket(SOCKET sock);
    
    // Accepter une connexion
    static SOCKET acceptConnection(SOCKET serverSocket);
    
    // Se connecter à un serveur
    static void connectToServer(SOCKET sock, string ip, int port);
    
    // Envoyer avec la longueur (protocole à préfixe)
    static void sendWithLength(SOCKET sock, char* data, size_t size);
    
    // Recevoir avec la longueur
    static size_t receiveWithLength(SOCKET sock, char* buffer, size_t maxSize);
};
```

---

## 📏 Le problème de la fragmentation TCP

### Le problème :
TCP peut **couper** les messages en morceaux !

```
Tu envoies : "Bonjour comment vas-tu ?"
TCP peut livrer :
  - Morceau 1 : "Bonj"
  - Morceau 2 : "our comment v"
  - Morceau 3 : "as-tu ?"
```

### La solution du projet : Préfixe de longueur

```
AVANT :
  [Bonjour]  ← Comment savoir où ça finit ?

APRÈS (avec préfixe) :
  [7][Bonjour]
   ↑
   "Les 4 prochains caractères disent la taille : 7"
```

Le code :
```cpp
// Envoi : on envoie d'abord la taille, puis les données
void sendWithLength(SOCKET sock, char* data, size_t size) {
    uint32_t netSize = htonl(size);      // Taille en "network byte order"
    send(sock, &netSize, 4, 0);          // Envoyer la taille (4 octets)
    send(sock, data, size, 0);           // Envoyer les données
}

// Réception : on lit d'abord la taille, puis les données
size_t receiveWithLength(SOCKET sock, char* buffer, size_t maxSize) {
    uint32_t netSize;
    recv(sock, &netSize, 4, 0);          // Lire la taille
    size_t size = ntohl(netSize);         // Convertir
    recv(sock, buffer, size, 0);          // Lire les données
    return size;
}
```

---

## 🪟 Différence Windows vs Linux

### Le problème :
- Windows utilise **Winsock** (bibliothèque Microsoft)
- Linux utilise **BSD Sockets** (standard POSIX)

### La solution du projet :
```cpp
#ifdef _WIN32
    // Code Windows
    #include <winsock2.h>
    typedef int socklen_t;
#else
    // Code Linux
    #include <sys/socket.h>
    #define SOCKET int
    #define closesocket close
#endif
```

Grâce à ça, le MÊME code fonctionne sur Windows ET Linux !

---

## 📊 Schéma récapitulatif

```
╔══════════════════════════════════════════════════════════════════╗
║                        SERVEUR (port 8888)                       ║
╠══════════════════════════════════════════════════════════════════╣
║  1. socket()   → Crée la socket                                  ║
║  2. bind()     → Associe au port 8888                           ║
║  3. listen()   → Se met en écoute                               ║
║  4. accept()   → Attend une connexion                           ║
║        ↓                                                         ║
║  [Nouvelle socket pour chaque client]                           ║
╚══════════════════════════════════════════════════════════════════╝
                              ▲
                              │ Connexion TCP
                              │ Port 8888
                              ▼
╔══════════════════════════════════════════════════════════════════╗
║                          CLIENT                                   ║
╠══════════════════════════════════════════════════════════════════╣
║  1. socket()   → Crée la socket                                  ║
║  2. connect()  → Se connecte au serveur                         ║
║        ↓                                                         ║
║  [Peut maintenant envoyer/recevoir]                             ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## 🎯 Ce qu'il faut retenir pour l'oral

> "Les **sockets TCP/IP** permettent la communication entre le client et le serveur.
>
> Le serveur **écoute sur le port 8888** et **accepte les connexions** entrantes.
>
> Chaque client **se connecte** à cette adresse.
>
> Pour éviter les problèmes de **fragmentation TCP**, on utilise un **protocole à préfixe de longueur** : chaque message est précédé de sa taille sur 4 octets.
>
> Le code est **portable** grâce à des directives de compilation conditionnelle (Windows/Linux)."

---

➡️ **Fichier suivant** : `03_LES_THREADS_EXPLIQUES.md` pour comprendre le parallélisme
