# Application de Messagerie Instantanée en C++

Application client/serveur de messagerie instantanée développée en C++ avec sockets TCP/IP et multi-threading.

## 📋 Fonctionnalités

### Serveur
- **Architecture multi-threads** :
  - Thread principal : accepte les connexions clients
  - Threads User Handler : un par client connecté
  - Thread Delivery : livraison asynchrone des messages (toutes les 30 secondes)
- **Système de logs** : enregistrement de toutes les opérations dans `server.log`
- **Gestion des messages** :
  - File d'attente pour les messages à délivrer
  - Historique de tous les messages délivrés
  - Support du broadcast (envoi à tous avec "all")
- **Synchronisation** : utilisation de mutex pour protéger les données partagées
- **Arrêt automatique** : le serveur s'arrête quand le dernier client se déconnecte

### Client
- **Architecture multi-threads** :
  - Thread principal : interface utilisateur et commandes
  - Thread Listen : écoute constante des messages du serveur
- **Fonctionnalités** :
  - Lister les messages reçus avec statut lu/non lu
  - Lire un message par indice ou par sujet
  - Marquer des messages comme lus
  - Composer et envoyer des messages
  - Broadcast vers tous les utilisateurs
  - Lister les utilisateurs en ligne
  - Récupérer le fichier log du serveur
  - Déconnexion propre

## 🏗️ Architecture

```
Projet_serveur_client/
├── message.h              # Structure Message avec validation
├── message.cpp            # Implémentation de Message
├── socket_utils.h         # Utilitaires sockets cross-platform
├── socket_utils.cpp       # Implémentation des utilitaires
├── serveur.cpp            # Serveur multi-threads
├── client.cpp             # Client multi-threads
└── README.md              # Ce fichier
```

## 📦 Structure Message

Chaque message contient :
- **From** : expéditeur (max 50 caractères)
- **To** : destinataire (max 50 caractères, "all" pour broadcast)
- **Subject** : sujet (max 100 caractères)
- **Body** : corps du message (max 500 caractères)
- **isRead** : statut de lecture

## 🔧 Compilation

### Windows (MinGW/MSYS2)
```powershell
# Compiler le serveur
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur.exe -lws2_32

# Compiler le client
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client.exe -lws2_32
```

### Linux
```bash
# Compiler le serveur
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur

# Compiler le client
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client
```

## 🚀 Utilisation

### Démarrer le serveur

```powershell
# Windows
.\serveur.exe

# Linux
./serveur
```

Le serveur démarre sur le port **8888** par défaut.

### Démarrer un client

```powershell
# Windows - Connexion locale
.\client.exe

# Windows - Connexion à un serveur distant
.\client.exe 192.168.1.100 8888

# Linux
./client
./client 192.168.1.100 8888
```

Lors du démarrage, le client demande un nom d'utilisateur.

## 📝 Commandes Client

Une fois connecté, le menu suivant s'affiche :

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
```

### 1. Lister les messages
Affiche tous les messages reçus avec leur statut (lu/non lu), expéditeur et sujet.

### 2. Lire un message
Deux options :
- Par **indice** : entrer le numéro du message
- Par **sujet** : entrer le texte exact du sujet

### 3. Marquer comme lu
Marque un message comme lu en entrant son indice.

### 4. Composer un message
Étapes :
1. Entrer le destinataire (nom d'utilisateur ou "all" pour broadcast)
2. Entrer le sujet (max 100 caractères)
3. Entrer le corps du message (max 500 caractères)

**Note** : Le message sera livré lors du prochain cycle de livraison (max 30 secondes).

### 5. Lister les utilisateurs en ligne
Affiche la liste de tous les utilisateurs actuellement connectés.

### 6. Récupérer le log du serveur
Télécharge et affiche le contenu complet du fichier `server.log`.

### 7. Se déconnecter
Ferme proprement la connexion au serveur.

## 🧪 Scénarios de Test

### Test 1 : Communication simple entre deux clients

**Terminal 1 - Serveur**
```powershell
.\serveur.exe
```

**Terminal 2 - Client Alice**
```powershell
.\client.exe
# Nom: Alice
# Commande: 4 (Composer un message)
# Destinataire: Bob
# Sujet: Test
# Corps: Bonjour Bob!
```

**Terminal 3 - Client Bob**
```powershell
.\client.exe
# Nom: Bob
# Attendre la notification du message (max 30s)
# Commande: 1 (Lister les messages)
# Commande: 2 (Lire le message)
```

### Test 2 : Broadcast à tous les utilisateurs

**Terminal 1 - Serveur**
```powershell
.\serveur.exe
```

**Terminaux 2, 3, 4 - Clients Alice, Bob, Charlie**
```powershell
.\client.exe
# Nom: Alice / Bob / Charlie
```

**Client Alice**
```powershell
# Commande: 4 (Composer un message)
# Destinataire: all
# Sujet: Annonce générale
# Corps: Réunion demain à 10h!
```

**Clients Bob et Charlie**
```powershell
# Recevoir automatiquement la notification
# Commande: 1 (Lister les messages)
```

### Test 3 : Gestion des erreurs

**Test 3.1 : Destinataire inexistant**
```powershell
# Client Alice
# Commande: 4
# Destinataire: UserInexistant
# Sujet: Test
# Corps: Message test
# Le serveur logge l'erreur dans server.log
```

**Test 3.2 : Message trop long**
```powershell
# Client Alice
# Commande: 4
# Destinataire: Bob
# Sujet: Un sujet beaucoup trop long qui dépasse largement la limite autorisée de cent caractères exactement!
# Résultat: Erreur de validation
```

**Test 3.3 : Commande inexistante**
```powershell
# Client Alice
# Commande: 99
# Résultat: "Commande inexistante."
```

### Test 4 : Fonctionnalités avancées

**Test 4.1 : Lister les utilisateurs en ligne**
```powershell
# 3 clients connectés: Alice, Bob, Charlie
# Client Alice
# Commande: 5
# Résultat: Liste affichant Alice, Bob, Charlie
```

**Test 4.2 : Récupérer le log du serveur**
```powershell
# Client Alice
# Commande: 6
# Résultat: Affichage complet de server.log
```

**Test 4.3 : Marquer comme lu**
```powershell
# Client Bob (a reçu 2 messages)
# Commande: 1 (Liste: 2 messages non lus)
# Commande: 3 (Marquer comme lu)
# Indice: 1
# Commande: 1 (Liste: 1 message lu, 1 non lu)
```

### Test 5 : Arrêt du serveur

```powershell
# Lancer le serveur
.\serveur.exe

# Connecter un client
.\client.exe
# Nom: Alice

# Se déconnecter
# Commande: 7

# Observer le serveur:
# Message: "Dernier client déconnecté - Arrêt du serveur"
# Le serveur se termine automatiquement
```

### Test 6 : Synchronisation et concurrence

```powershell
# Lancer le serveur
.\serveur.exe

# Connecter 5 clients simultanément (5 terminaux)
.\client.exe # Alice
.\client.exe # Bob
.\client.exe # Charlie
.\client.exe # David
.\client.exe # Eve

# Depuis Alice, envoyer un message broadcast
# Commande: 4
# Destinataire: all
# Sujet: Test concurrent
# Corps: Message de synchronisation

# Vérifier que tous les clients reçoivent le message
# Depuis chaque client:
# Commande: 1 (après 30s max)
```

## 🔒 Synchronisation

### Mutex utilisés
- **g_usersMutex** : protège la liste des utilisateurs connectés
- **g_queueMutex** : protège la file d'attente des messages
- **g_historyMutex** : protège l'historique des messages
- **g_logMutex** : protège l'écriture dans le fichier log
- **g_messagesMutex** (client) : protège la liste des messages reçus

### Variables atomiques
- **g_serverRunning** : contrôle l'arrêt du serveur
- **g_clientRunning** : contrôle l'arrêt du client
- **g_isComposing** : empêche l'affichage de notifications pendant la composition

## ⚠️ Gestion d'Erreurs

L'application gère les erreurs suivantes :

1. **Commande inexistante** : message d'erreur au client
2. **Utilisateur destinataire inexistant** : enregistré dans le log
3. **Message mal formaté** : validation côté client et serveur
4. **Message à lire inexistant** : vérification d'indice
5. **Limites de champs dépassées** : exception avec message explicite
6. **Utilisateur déconnecté pendant envoi** : détection et log
7. **Échec d'envoi** : exceptions catchées et loguées
8. **Perte de connexion** : détection par le thread d'écoute

## 📊 Logs Serveur

Le fichier `server.log` contient :
- Timestamp de chaque opération
- Connexions/déconnexions des utilisateurs
- Messages envoyés et livrés
- Erreurs rencontrées
- Commandes reçues

Exemple :
```
[2025-11-24 14:30:15] === SERVEUR DE MESSAGERIE DÉMARRÉ ===
[2025-11-24 14:30:15] Serveur en écoute sur le port 8888
[2025-11-24 14:30:15] Thread de livraison démarré
[2025-11-24 14:30:20] Utilisateur connecté: Alice depuis 127.0.0.1
[2025-11-24 14:30:25] Utilisateur connecté: Bob depuis 127.0.0.1
[2025-11-24 14:30:30] Commande reçue de Alice: SEND:
[2025-11-24 14:30:30] Message ajouté à la queue de Alice vers Bob
[2025-11-24 14:31:00] Livraison de 1 message(s)
[2025-11-24 14:31:00] Message livré de Alice à Bob
```

## 🔧 Personnalisation

### Changer le port
Modifier la constante dans `serveur.cpp` et `client.cpp` :
```cpp
constexpr int PORT = 8888;  // Votre port
```

### Changer l'intervalle de livraison
Modifier dans `serveur.cpp` :
```cpp
constexpr int DELIVERY_INTERVAL_SECONDS = 30;  // Votre intervalle
```

### Modifier les limites de messages
Modifier dans `message.h` :
```cpp
constexpr size_t MAX_FROM_SIZE = 50;
constexpr size_t MAX_TO_SIZE = 50;
constexpr size_t MAX_SUBJECT_SIZE = 100;
constexpr size_t MAX_BODY_SIZE = 500;
```

## 🐛 Dépannage

### Erreur "Address already in use"
Le port 8888 est déjà utilisé. Attendez quelques secondes ou changez le port.

### Erreur WSAStartup (Windows)
Assurez-vous de compiler avec `-lws2_32`.

### Le serveur ne s'arrête pas
Le serveur attend que tous les clients se déconnectent. Utilisez Ctrl+C en dernier recours.

### Les messages ne sont pas reçus immédiatement
C'est normal ! Les messages sont livrés toutes les 30 secondes par le thread de livraison.

## 📚 Concepts C++ Utilisés

- **std::thread** : création et gestion de threads
- **std::mutex** : synchronisation des accès aux données partagées
- **std::lock_guard** : RAII pour les mutex
- **std::atomic** : variables atomiques pour flags
- **std::vector** : conteneurs dynamiques
- **std::queue** : file FIFO pour les messages
- **std::string** : gestion des chaînes
- **Sockets système** : socket(), bind(), listen(), accept(), connect(), send(), recv()

## 👨‍💻 Auteur

Développé pour le cours R3.05 - BUT Informatique 2A  
Style de programmation système avec C++ moderne

## 📄 Licence

Projet académique - Libre d'utilisation pour l'apprentissage
