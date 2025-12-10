# Application de Messagerie Instantanee Client/Serveur

Application de messagerie instantanee developpee en C++ avec sockets TCP/IP et multi-threading.

Projet R3.05 - Programmation Systeme - BUT Informatique 2A

---

## Fonctionnalites

### Serveur
- Architecture multi-threads :
  - Thread principal : accepte les connexions clients
  - Threads User Handler : un par client connecte
  - Thread Delivery : livraison des messages toutes les 30 secondes
- Systeme de logs (server.log)
- File d'attente et historique des messages
- Support du broadcast (envoi a tous avec "all")
- Arret automatique quand le dernier client se deconnecte

### Client
- Architecture multi-threads :
  - Thread principal : interface utilisateur
  - Thread Listener : reception des messages en arriere-plan
- Lister les messages recus (lu/non lu)
- Lire un message par indice ou par sujet
- Marquer des messages comme lus
- Composer et envoyer des messages
- Lister les utilisateurs en ligne
- Recuperer le fichier log du serveur
- Deconnexion propre

---

## Fichiers du projet

```
Projet_serveur_client/
├── serveur.cpp        # Serveur multi-threads
├── client.cpp         # Client multi-threads
├── message.h          # Structure Message
├── message.cpp        # Implementation Message
├── socket_utils.h     # Utilitaires sockets
├── socket_utils.cpp   # Implementation sockets
├── Makefile           # Compilation Linux
└── README.md          # Ce fichier
```

---

## Compilation (Linux)

```bash
make
```

Cela compile le serveur et le client.

Ou manuellement :

```bash
# Serveur
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur

# Client
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client
```

---

## Utilisation

### Demarrer le serveur

```bash
./serveur
```

Le serveur ecoute sur le port 8888.

### Demarrer un client

```bash
./client                      # Connexion locale (127.0.0.1:8888)
./client 192.168.1.10         # Connexion a une autre IP
./client 192.168.1.10 9000    # IP et port personnalises
```

---

## Menu du client

```
1. Lister les messages
2. Lire un message
3. Marquer comme lu
4. Composer un message
5. Lister les utilisateurs en ligne
6. Recuperer le log du serveur
7. Se deconnecter
```

---

## Protocole de communication

### Commandes client vers serveur
- SEND: + message serialise : envoyer un message
- LIST_USERS : demander la liste des connectes
- GET_LOG : demander le fichier log
- DISCONNECT : se deconnecter

### Reponses serveur vers client
- MSG: + message : nouveau message recu
- OK: + texte : confirmation
- ERROR: + texte : erreur
- USERS: + liste : utilisateurs connectes
- LOG: + contenu : fichier log

---

## Synchronisation

Mutex utilises :
- g_usersMutex : protege la liste des utilisateurs
- g_queueMutex : protege la file de messages
- g_historyMutex : protege l'historique
- g_logMutex : protege l'ecriture dans le log

Variables atomiques :
- g_serverRunning / g_clientRunning : controle de l'arret
- g_isComposing : bloque les notifications pendant la saisie

---

## Structure du message

```cpp
struct Message {
    char from[50];       // Expediteur
    char to[50];         // Destinataire (ou "all")
    char subject[100];   // Sujet
    char body[500];      // Corps
    bool isRead;         // Statut de lecture
    time_t receivedAt;   // Horodatage
};
```

Taille fixe (709 octets) pour faciliter la serialisation.

---

## Exemple de test

Terminal 1 - Serveur :
```bash
./serveur
```

Terminal 2 - Client Alice :
```bash
./client
# Nom: Alice
# Choix 4 : Composer un message
# Destinataire: Bob
# Sujet: Test
# Corps: Bonjour Bob!
```

Terminal 3 - Client Bob :
```bash
./client
# Nom: Bob
# Attendre la notification (max 30s)
# Choix 1 : Lister les messages
# Choix 2 : Lire le message
```

---

## Auteur

josua Akono - Saban Ercan - BUT Informatique 2A
Projet R3.05 Programmation Systeme - 2024/2025
