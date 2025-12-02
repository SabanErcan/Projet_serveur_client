# Guide de Démarrage Rapide

## ⚡ Compilation Express

### Windows (PowerShell)
```powershell
# Option 1: Script automatique
.\compile_and_test.ps1

# Option 2: Compilation manuelle
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur.exe -lws2_32
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client.exe -lws2_32
```

### Linux
```bash
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client
```

## 🚀 Test Rapide (3 Minutes)

### Étape 1 : Démarrer le serveur
**Terminal 1**
```powershell
.\serveur.exe
```
Vous devriez voir :
```
[2025-11-24 XX:XX:XX] === SERVEUR DE MESSAGERIE DÉMARRÉ ===
[2025-11-24 XX:XX:XX] Serveur en écoute sur le port 8888
```

### Étape 2 : Connecter Alice
**Terminal 2**
```powershell
.\client.exe
```
Entrez : `Alice`

### Étape 3 : Connecter Bob
**Terminal 3**
```powershell
.\client.exe
```
Entrez : `Bob`

### Étape 4 : Alice envoie un message à Bob
**Terminal 2 (Alice)**
```
Tapez: 4 [ENTER]
Destinataire: Bob [ENTER]
Sujet: Premier test [ENTER]
Corps: Bonjour Bob, ça marche! [ENTER]
```

### Étape 5 : Attendre 30 secondes
⏰ Les messages sont livrés toutes les 30 secondes.

### Étape 6 : Bob reçoit et lit le message
**Terminal 3 (Bob)**
```
[Notification apparaît automatiquement]
Tapez: 1 [ENTER]  # Lister les messages
Tapez: 2 [ENTER]  # Lire un message
Tapez: 1 [ENTER]  # Par indice
Tapez: 1 [ENTER]  # Message numéro 1
```

### Étape 7 : Test du broadcast
**Terminal 2 (Alice)**
```
Tapez: 4 [ENTER]
Destinataire: all [ENTER]
Sujet: Annonce générale [ENTER]
Corps: Message pour tout le monde! [ENTER]
```

**Terminal 3 (Bob)**
Après 30 secondes max, reçoit automatiquement le message.

### Étape 8 : Lister les utilisateurs en ligne
**Terminal 2 ou 3**
```
Tapez: 5 [ENTER]
```
Résultat :
```
=== UTILISATEURS EN LIGNE ===
- Alice
- Bob
Total: 2 utilisateur(s)
```

### Étape 9 : Voir le log du serveur
**Terminal 2 ou 3**
```
Tapez: 6 [ENTER]
```

### Étape 10 : Déconnexion
**Terminal 2 (Alice)**
```
Tapez: 7 [ENTER]
```

**Terminal 3 (Bob)**
```
Tapez: 7 [ENTER]
```

**Terminal 1 (Serveur)**
Le serveur s'arrête automatiquement :
```
[...] Dernier client déconnecté - Arrêt du serveur
[...] === SERVEUR ARRÊTÉ ===
```

## 🎯 Tests Avancés

### Test 1 : Multiples clients (5 clients simultanés)
Ouvrez 6 terminaux :
- 1 serveur
- 5 clients : Alice, Bob, Charlie, David, Eve

Alice envoie un broadcast → tous les 4 autres reçoivent.

### Test 2 : Messages simultanés
- Alice → Bob : "Message 1"
- Charlie → Bob : "Message 2"
- David → Bob : "Message 3"

Bob liste ses messages → 3 messages apparaissent.

### Test 3 : Gestion d'erreurs
```
Alice compose un message :
Destinataire: UtilisateurInexistant
Sujet: Test
Corps: Test

Vérifier server.log :
[...] Utilisateur destinataire inexistant: UtilisateurInexistant
```

### Test 4 : Validation des limites
```
Alice compose un message :
Sujet: [Tapez plus de 100 caractères]
→ Erreur: Subject dépasse la limite de 100 caractères
```

## 📋 Checklist de Validation

- [ ] Le serveur démarre sur le port 8888
- [ ] Plusieurs clients peuvent se connecter
- [ ] Les messages unicast fonctionnent (Alice → Bob)
- [ ] Le broadcast fonctionne (Alice → all)
- [ ] Les messages sont livrés dans les 30 secondes
- [ ] La liste des utilisateurs est correcte
- [ ] Le fichier server.log est créé et mis à jour
- [ ] Les limites de champs sont respectées
- [ ] Les erreurs sont bien gérées
- [ ] Le serveur s'arrête quand tous les clients se déconnectent
- [ ] Pas de pollution de l'affichage pendant la composition
- [ ] Les mutex empêchent les accès concurrents
- [ ] Le Listen Thread affiche les notifications

## ⚠️ Problèmes Courants

### Le serveur ne démarre pas
```
Erreur: bind failed
→ Le port 8888 est déjà utilisé
→ Solution: Attendez 1 minute ou changez PORT dans serveur.cpp
```

### Le client ne se connecte pas
```
Erreur: Échec de connexion
→ Le serveur n'est pas démarré
→ Solution: Lancez d'abord .\serveur.exe
```

### Les messages ne sont pas reçus
```
→ C'est normal ! Attendez 30 secondes max
→ Le thread Delivery livre par lots
```

### Erreur de compilation
```
Erreur: ws2_32
→ Windows uniquement
→ Solution: Ajoutez -lws2_32 à la commande g++

Erreur: std::thread
→ Manque -pthread
→ Solution: Ajoutez -pthread à la commande g++
```

## 🎓 Points Clés du Projet

### Architecture Multi-Threads
✅ **Serveur** : 3+ threads (Main, User Handlers, Delivery)  
✅ **Client** : 2 threads (Main, Listen)

### Synchronisation
✅ **Mutex** : g_usersMutex, g_queueMutex, g_historyMutex, g_logMutex  
✅ **lock_guard** : Protection automatique RAII  
✅ **Atomic** : g_serverRunning, g_clientRunning, g_isComposing

### Sockets Système
✅ **Création** : socket()  
✅ **Bind** : bind()  
✅ **Écoute** : listen()  
✅ **Acceptation** : accept()  
✅ **Connexion** : connect()  
✅ **Envoi/Réception** : send(), recv()

### Validation et Erreurs
✅ **Limites** : Validation des tailles de champs  
✅ **Exceptions** : std::invalid_argument, std::runtime_error  
✅ **Logs** : Toutes les opérations enregistrées

## 📞 Support

Pour toute question sur le projet :
- Consultez le README.md complet
- Vérifiez les logs dans server.log
- Testez avec les scénarios fournis

Bon codage ! 🚀
