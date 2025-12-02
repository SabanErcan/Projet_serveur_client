# 🎮 COMMANDES RAPIDES - AIDE MÉMOIRE

## 🔨 Compilation

### Windows
```powershell
# Rapide
.\compile.bat

# Ou manuel
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur.exe -lws2_32
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client.exe -lws2_32
```

### Linux
```bash
# Rapide
./compile.sh

# Ou make
make

# Ou manuel
g++ -std=c++20 -pthread serveur.cpp message.cpp socket_utils.cpp -o serveur
g++ -std=c++20 -pthread client.cpp message.cpp socket_utils.cpp -o client
```

---

## 🚀 Lancement

### Serveur
```powershell
# Windows
.\serveur.exe

# Linux
./serveur
```

### Client
```powershell
# Windows - Connexion locale
.\client.exe

# Windows - Connexion distante
.\client.exe 192.168.1.100 8888

# Linux
./client
./client 192.168.1.100 8888
```

---

## 📱 Commandes Client

```
╔════════════════════════════════════════╗
║              MENU CLIENT               ║
╠════════════════════════════════════════╣
║ 1 │ Lister les messages               ║
║ 2 │ Lire un message                   ║
║ 3 │ Marquer comme lu                  ║
║ 4 │ Composer un message               ║
║ 5 │ Lister les utilisateurs en ligne  ║
║ 6 │ Récupérer le log du serveur       ║
║ 7 │ Se déconnecter                    ║
╚════════════════════════════════════════╝
```

### Détails des Commandes

#### 1️⃣ Lister les messages
```
Tapez: 1
→ Affiche tous les messages reçus avec statut lu/non lu
```

#### 2️⃣ Lire un message
```
Tapez: 2
  Choix: 1 (par indice) ou 2 (par sujet)
    Si indice: 1, 2, 3...
    Si sujet: "Le sujet exact"
→ Affiche le message complet
```

#### 3️⃣ Marquer comme lu
```
Tapez: 3
  Indice: 1, 2, 3...
→ Change le statut à "Lu"
```

#### 4️⃣ Composer un message
```
Tapez: 4
  Destinataire: Bob          (ou "all" pour broadcast)
  Sujet: Test                (max 100 caractères)
  Corps: Bonjour Bob!        (max 500 caractères)
→ Message envoyé, livraison dans max 30s
```

#### 5️⃣ Lister utilisateurs
```
Tapez: 5
→ Affiche tous les utilisateurs connectés
```

#### 6️⃣ Récupérer le log
```
Tapez: 6
→ Affiche le contenu de server.log
```

#### 7️⃣ Se déconnecter
```
Tapez: 7
→ Déconnexion propre du serveur
```

---

## 🧪 Scénarios de Test Rapides

### Scénario 1 : Test Simple (2 min)
```
Terminal 1: .\serveur.exe
Terminal 2: .\client.exe → Alice
Terminal 3: .\client.exe → Bob

Alice (T2): 4 → Bob → Test → Bonjour
[Attendre 30s]
Bob (T3): 1 → voir message
```

### Scénario 2 : Broadcast (3 min)
```
Terminal 1: .\serveur.exe
Terminal 2-5: .\client.exe → Alice, Bob, Charlie, David

Alice (T2): 4 → all → Annonce → Réunion demain
[Attendre 30s]
Bob/Charlie/David: Reçoivent le message
```

### Scénario 3 : Liste Utilisateurs (1 min)
```
Terminal 1: .\serveur.exe
Terminal 2-4: .\client.exe → Alice, Bob, Charlie

Alice (T2): 5
→ Voir: Alice, Bob, Charlie
```

### Scénario 4 : Erreur (1 min)
```
Terminal 1: .\serveur.exe
Terminal 2: .\client.exe → Alice

Alice: 4 → Zorro → Test → Test
[Attendre 30s]
Terminal 1: Voir log "destinataire inexistant: Zorro"
```

---

## 🐛 Dépannage Rapide

### Erreur : "Address already in use"
```powershell
# Attendre 1 minute ou changer le port
# Dans serveur.cpp ligne 11 : PORT = 8889
```

### Erreur : "WSAStartup failed"
```powershell
# Recompiler avec -lws2_32
g++ ... -lws2_32
```

### Erreur : "No such file or directory"
```powershell
# Vérifier que vous êtes dans le bon dossier
cd "C:\Users\saban\OneDrive\Documents\Cours IUT\BUT Info - 2A\Semestre 1\R3.05\Projet_serveur_client"
```

### Le serveur ne s'arrête pas
```powershell
# Déconnecter tous les clients (commande 7)
# Ou Ctrl+C en dernier recours
```

### Messages pas reçus immédiatement
```
C'est NORMAL ! Les messages sont livrés toutes les 30 secondes.
```

---

## 📊 Limites et Contraintes

| Champ | Limite | Erreur si dépassé |
|-------|--------|-------------------|
| From | 50 caractères | Exception |
| To | 50 caractères | Exception |
| Subject | 100 caractères | Exception |
| Body | 500 caractères | Exception |

---

## 🔑 Raccourcis Clavier

### Dans le terminal client
- **Ctrl+C** : Forcer l'arrêt
- **Enter** : Valider une commande
- **Backspace** : Effacer

### Dans le terminal serveur
- **Ctrl+C** : Arrêt forcé (non recommandé)

---

## 📝 Messages d'Erreur Courants

| Message | Signification | Solution |
|---------|---------------|----------|
| "Commande inexistante" | Numéro invalide | Tapez 1-7 |
| "Subject dépasse la limite" | Sujet trop long | Max 100 car |
| "Body dépasse la limite" | Corps trop long | Max 500 car |
| "Message inexistant" | Indice invalide | Vérifier avec cmd 1 |
| "Utilisateur destinataire inexistant" | User pas connecté | Vérifier avec cmd 5 |
| "Échec de connexion" | Serveur pas lancé | Lancer serveur d'abord |

---

## 🎯 Checklist Démonstration

Avant la démo :
- [ ] Compiler sans erreur
- [ ] Tester avec 2 clients minimum
- [ ] Vérifier que server.log est créé
- [ ] Tester le broadcast
- [ ] Tester la liste utilisateurs
- [ ] Vérifier déconnexion propre

Pendant la démo :
- [ ] Montrer la compilation
- [ ] Lancer le serveur
- [ ] Connecter 3 clients
- [ ] Envoyer message unicast
- [ ] Attendre livraison (30s)
- [ ] Afficher liste utilisateurs
- [ ] Envoyer broadcast
- [ ] Montrer le log serveur
- [ ] Déconnecter tous les clients
- [ ] Montrer arrêt auto du serveur

---

## 🔄 Cycle de Vie Typique

```
1. Compilation
   ↓
2. Lancement serveur (port 8888)
   ↓
3. Connexion clients (envoient username)
   ↓
4. Clients envoient messages (cmd 4)
   ↓
5. Messages ajoutés à la queue
   ↓
6. Delivery thread traite toutes les 30s
   ↓
7. Messages livrés aux destinataires
   ↓
8. Listen thread affiche notifications
   ↓
9. Clients se déconnectent (cmd 7)
   ↓
10. Serveur s'arrête (dernier client parti)
```

---

## 💾 Fichiers Générés

Lors de l'exécution :
- `server.log` - Log du serveur (créé automatiquement)
- `serveur.exe` / `serveur` - Exécutable serveur
- `client.exe` / `client` - Exécutable client

---

## 📚 Ressources

| Document | Usage |
|----------|-------|
| [README.md](README.md) | Documentation complète |
| [QUICKSTART.md](QUICKSTART.md) | Démarrage rapide |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Architecture technique |
| [VALIDATION.md](VALIDATION.md) | Tests et validation |
| [INDEX.md](INDEX.md) | Navigation docs |
| [PROJET.md](PROJET.md) | Résumé académique |

---

**Commandes essentielles en 1 minute** ⏱️

```powershell
# 1. Compiler
.\compile.bat

# 2. Terminal 1
.\serveur.exe

# 3. Terminal 2
.\client.exe
Alice
4
Bob
Test
Bonjour

# 4. Terminal 3
.\client.exe
Bob
[Attendre 30s]
1
```

**C'est parti !** 🚀
