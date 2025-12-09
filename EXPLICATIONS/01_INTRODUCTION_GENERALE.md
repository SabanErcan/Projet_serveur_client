# 📚 INTRODUCTION GÉNÉRALE AU PROJET

## 🎯 C'est quoi ce projet en 1 phrase ?

**C'est une application de messagerie instantanée** (comme WhatsApp ou Messenger) mais en version simple, codée en C++.

---

## 🤔 Le concept de base à comprendre

Imagine que tu veux envoyer un SMS à un ami :

```
TOI (Client) ──────► ANTENNE (Serveur) ──────► TON AMI (Client)
```

Notre projet fonctionne pareil :
- **Le SERVEUR** = le centre de tri des messages (comme une poste)
- **Le CLIENT** = l'application sur ton téléphone (toi qui envoies/reçois)

---

## 📁 Les fichiers du projet expliqués simplement

```
Projet_serveur_client/
│
├── serveur.cpp        ← Le programme "serveur" (le centre de tri)
├── client.cpp         ← Le programme "client" (l'appli utilisateur)
│
├── message.h          ← La définition d'un message (c'est quoi un message)
├── message.cpp        ← Le code pour créer/gérer les messages
│
├── socket_utils.h     ← Les outils pour communiquer sur le réseau
├── socket_utils.cpp   ← Le code des outils réseau
│
└── Makefile           ← Script pour compiler automatiquement
```

---

## 🔑 Les 3 concepts clés à retenir

### 1️⃣ ARCHITECTURE CLIENT-SERVEUR

```
   CLIENT 1 (Alice)  ──┐
                       │
   CLIENT 2 (Bob)    ──┼──────► SERVEUR ◄── Gère tout le monde
                       │
   CLIENT 3 (Charlie)──┘
```

- **Un seul serveur** attend les connexions
- **Plusieurs clients** peuvent se connecter
- Les clients ne parlent JAMAIS directement entre eux !

### 2️⃣ LES SOCKETS (prises réseau)

Une **socket** c'est comme une **prise électrique pour le réseau** :
- Tu "branches" ta socket pour te connecter
- Tu envoies/reçois des données par cette socket
- Tu "débranches" quand tu as fini

### 3️⃣ LES THREADS (fils d'exécution)

Un **thread** c'est comme avoir **plusieurs mains** :
- Tu peux faire plusieurs choses EN MÊME TEMPS
- Exemple : écouter la musique ET écrire un message

---

## 🎮 Comment ça marche concrètement ?

### Étape 1 : Démarrer le serveur
```
Le serveur démarre et dit : "J'attends sur le port 8888"
```

### Étape 2 : Un client se connecte
```
Alice lance le client
Elle tape son nom : "Alice"
→ Le serveur dit : "OK Alice est connectée !"
```

### Étape 3 : Alice envoie un message
```
Alice veut envoyer "Salut" à Bob
→ Le message va dans une FILE D'ATTENTE sur le serveur
```

### Étape 4 : Le serveur livre le message
```
Toutes les 30 secondes, le serveur regarde sa file d'attente
→ "Oh, un message pour Bob !"
→ Il envoie le message à Bob
```

### Étape 5 : Bob reçoit le message
```
Bob voit apparaître : "[NOUVEAU MESSAGE] De: Alice"
```

---

## ⏱️ Pourquoi 30 secondes d'attente ?

C'est un choix de conception ! Le serveur **regroupe les messages** :
- Il attend 30 secondes
- Puis il livre TOUS les messages d'un coup
- C'est plus efficace que d'envoyer message par message

---

## 📋 Les fonctionnalités du projet

### Côté CLIENT (ce que peut faire l'utilisateur) :
| N° | Action | Description |
|----|--------|-------------|
| 1 | Lister les messages | Voir tous les messages reçus |
| 2 | Lire un message | Voir le contenu complet d'un message |
| 3 | Marquer comme lu | Indiquer qu'on a lu le message |
| 4 | Composer un message | Écrire et envoyer un nouveau message |
| 5 | Lister les utilisateurs | Voir qui est connecté |
| 6 | Récupérer le log | Voir l'historique du serveur |
| 7 | Se déconnecter | Quitter proprement |

### Côté SERVEUR (ce que fait le serveur automatiquement) :
- ✅ Accepte les nouvelles connexions
- ✅ Reçoit les messages des clients
- ✅ Stocke les messages dans une file d'attente
- ✅ Livre les messages toutes les 30 secondes
- ✅ Enregistre tout dans un fichier log

---

## 🎯 Résumé pour ton oral

> "Ce projet est une **application de messagerie instantanée** en C++.
> 
> Elle utilise une **architecture client-serveur** : le serveur central gère toutes les connexions et la distribution des messages.
> 
> Les **sockets TCP/IP** permettent la communication réseau, et les **threads** permettent de gérer plusieurs clients simultanément.
> 
> Les messages sont stockés dans une **file d'attente** et livrés **toutes les 30 secondes** aux destinataires."

---

➡️ **Fichier suivant** : `02_LES_SOCKETS_EXPLIQUEES.md` pour comprendre la communication réseau
