# 🎤 RÉSUMÉ POUR TON ORAL

## 📌 Fiche de révision rapide

Ce fichier te donne **tout ce qu'il faut savoir** pour présenter le projet à l'oral.

---

## 🎯 Présentation en 30 secondes

> "Ce projet est une **application de messagerie instantanée** en C++.
>
> Elle utilise une **architecture client-serveur** : plusieurs clients se connectent à un serveur central qui gère la distribution des messages.
>
> La communication se fait via des **sockets TCP/IP**, et le **multi-threading** permet de gérer plusieurs clients simultanément."

---

## 📁 Les fichiers du projet

| Fichier | Rôle |
|---------|------|
| `serveur.cpp` | Le programme serveur (central) |
| `client.cpp` | Le programme client (utilisateur) |
| `message.h/cpp` | Structure du message |
| `socket_utils.h/cpp` | Utilitaires réseau |

---

## 🏗️ Architecture globale

```
┌──────────┐     ┌──────────┐     ┌──────────┐
│ Client 1 │     │ Client 2 │     │ Client 3 │
│ (Alice)  │     │ (Bob)    │     │(Charlie) │
└────┬─────┘     └────┬─────┘     └────┬─────┘
     │                │                │
     └────────────────┼────────────────┘
                      │
                      ▼
              ┌───────────────┐
              │    SERVEUR    │
              │  (Port 8888)  │
              └───────────────┘
```

---

## 🧵 Les threads du projet

### Serveur (3 types) :
1. **Thread principal** → Accepte les connexions
2. **Thread User Handler** (1 par client) → Gère les commandes
3. **Thread Livraison** → Livre les messages toutes les 30s

### Client (2 types) :
1. **Thread principal** → Menu interactif
2. **Thread d'écoute** → Reçoit les messages en arrière-plan

---

## 🔒 Synchronisation

**Pourquoi ?** Éviter les conflits quand plusieurs threads accèdent aux mêmes données.

**Comment ?** Avec des **mutex** (verrous) :

```cpp
mutex g_usersMutex;    // Protège la liste des utilisateurs
mutex g_queueMutex;    // Protège la file de messages
mutex g_logMutex;      // Protège le fichier log
```

---

## 📨 Structure d'un Message

```cpp
struct Message {
    char from[50];       // Expéditeur
    char to[50];         // Destinataire (ou "all")
    char subject[100];   // Sujet
    char body[500];      // Contenu
    bool isRead;         // Lu ou non
    time_t receivedAt;   // Horodatage
};
```

**Taille fixe** → Facilite la sérialisation (transformation en octets)

---

## 🔌 Les sockets

| Étape | Serveur | Client |
|-------|---------|--------|
| 1 | `socket()` - Créer | `socket()` - Créer |
| 2 | `bind()` - Lier au port | - |
| 3 | `listen()` - Écouter | - |
| 4 | `accept()` - Accepter | `connect()` - Se connecter |
| 5 | `send()`/`recv()` | `send()`/`recv()` |
| 6 | `close()` | `close()` |

---

## 📡 Protocole de communication

### Commandes client → serveur :
- `SEND:` + message → Envoyer un message
- `LIST_USERS` → Demander la liste des connectés
- `GET_LOG` → Demander le fichier log
- `DISCONNECT` → Se déconnecter

### Réponses serveur → client :
- `MSG:` + message → Nouveau message reçu
- `OK:` + texte → Confirmation
- `ERROR:` + texte → Erreur
- `USERS:` + liste → Liste des utilisateurs
- `LOG:` + contenu → Fichier log

---

## 📏 Protocole à préfixe de longueur

**Problème** : TCP peut fragmenter les messages.

**Solution** : Envoyer la taille avant les données.

```
┌──────────────┬─────────────────────┐
│ Taille (4o)  │     Données         │
├──────────────┼─────────────────────┤
│   00 00 00 07│  B o n j o u r      │
└──────────────┴─────────────────────┘
```

---

## 🔄 Cycle de vie d'un message

```
1. Alice compose un message pour Bob
2. Le client envoie "SEND:" + message au serveur
3. Le serveur ajoute le message à la FILE D'ATTENTE
4. Le serveur répond "OK:Message en file"
5. (30 secondes passent...)
6. Le thread de livraison récupère le message
7. Il l'envoie au client de Bob ("MSG:" + message)
8. Bob reçoit une notification
```

---

## 📋 Fonctionnalités du client

| N° | Fonction | Description |
|----|----------|-------------|
| 1 | Lister | Voir tous les messages |
| 2 | Lire | Lire un message complet |
| 3 | Marquer lu | Marquer comme lu |
| 4 | Composer | Écrire et envoyer |
| 5 | Utilisateurs | Voir qui est connecté |
| 6 | Log | Récupérer le log serveur |
| 7 | Quitter | Se déconnecter |

---

## 🎯 Points techniques importants

### 1. Portabilité Windows/Linux
```cpp
#ifdef _WIN32
    // Code Windows (Winsock)
#else
    // Code Linux (BSD Sockets)
#endif
```

### 2. Network Byte Order
```cpp
htonl()  // Host TO Network Long
ntohl()  // Network TO Host Long
```

### 3. Arrêt automatique du serveur
Le serveur s'arrête quand le dernier client se déconnecte.

### 4. Broadcast
Destinataire = "all" → Message envoyé à tous

---

## ❓ Questions possibles à l'oral

### Q1 : "Pourquoi utiliser des threads ?"
> "Pour gérer plusieurs clients simultanément. Sans threads, le serveur ne pourrait traiter qu'un seul client à la fois."

### Q2 : "Pourquoi les messages sont livrés toutes les 30 secondes ?"
> "C'est un choix de conception. Le thread de livraison regroupe les messages pour les envoyer par lots, ce qui est plus efficace."

### Q3 : "Pourquoi utiliser des mutex ?"
> "Pour protéger les données partagées entre threads. Sans mutex, deux threads pourraient modifier la même donnée en même temps et la corrompre."

### Q4 : "Pourquoi la structure Message a des tailles fixes ?"
> "Pour faciliter la sérialisation. On sait toujours exactement combien d'octets envoyer, ce qui simplifie la transmission réseau."

### Q5 : "Comment fonctionne le protocole à préfixe de longueur ?"
> "On envoie d'abord la taille du message sur 4 octets, puis les données. Le récepteur lit d'abord la taille pour savoir combien d'octets lire ensuite."

### Q6 : "Différence entre TCP et UDP ?"
> "TCP est **fiable** (accusé de réception, ordre garanti) mais plus lent. UDP est **rapide** mais non fiable. On utilise TCP pour la messagerie car on veut être sûr que les messages arrivent."

---

## 📊 Schéma à dessiner au tableau

```
╔═══════════════════════════════════════════════════════════════╗
║                         SERVEUR                                ║
╠═══════════════════════════════════════════════════════════════╣
║                                                                 ║
║  Thread Principal ─────► accept() ─────► Thread User           ║
║                                          (1 par client)        ║
║                                               │                 ║
║                                               ▼                 ║
║                                      ┌─────────────────┐       ║
║                                      │ DONNÉES PARTAGÉES│       ║
║                                      │   (avec mutex)  │       ║
║                                      │                 │       ║
║                                      │ - Utilisateurs  │       ║
║                                      │ - Queue messages│       ║
║                                      │ - Historique    │       ║
║                                      └────────┬────────┘       ║
║                                               │                 ║
║                                               ▼                 ║
║                                      Thread Livraison          ║
║                                      (toutes les 30s)          ║
║                                                                 ║
╚═══════════════════════════════════════════════════════════════╝
```

---

## 🏆 Mots-clés à placer

- **Architecture client-serveur**
- **Sockets TCP/IP**
- **Multi-threading**
- **Mutex / Synchronisation**
- **Sérialisation / Désérialisation**
- **Protocole à préfixe de longueur**
- **Port 8888**
- **File d'attente (queue)**
- **Broadcast**
- **Portabilité cross-platform**

---

## ✅ Checklist avant l'oral

- [ ] Je sais expliquer ce que fait le projet en 30 secondes
- [ ] Je connais les différents fichiers et leur rôle
- [ ] Je sais dessiner l'architecture au tableau
- [ ] Je comprends pourquoi on utilise des threads
- [ ] Je sais expliquer les mutex
- [ ] Je connais les étapes d'une socket (socket, bind, listen, accept)
- [ ] Je sais expliquer le protocole à préfixe de longueur
- [ ] Je connais les commandes du protocole (SEND, LIST_USERS, etc.)
- [ ] Je sais expliquer pourquoi les messages arrivent en 30 secondes

---

## 🎤 Phrase de conclusion pour l'oral

> "En conclusion, ce projet m'a permis de comprendre les concepts fondamentaux de la **programmation système** : la communication réseau avec les **sockets**, la gestion de la concurrence avec les **threads**, et la protection des données partagées avec les **mutex**. C'est une base solide pour développer des applications réseau plus complexes."

---

**Bonne chance pour ton oral ! 💪**
