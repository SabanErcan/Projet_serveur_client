# 🧵 LES THREADS EXPLIQUÉS SIMPLEMENT

## 🤔 C'est quoi un thread ?

Imagine que tu es dans une cuisine :

### Sans threads (1 seule main) :
```
1. Tu coupes les légumes      ⏱️ 5 min
2. Tu attends que l'eau bouille  ⏱️ 10 min (tu fais RIEN)
3. Tu fais cuire les pâtes   ⏱️ 10 min
─────────────────────────────────────────
Total : 25 minutes !
```

### Avec threads (plusieurs mains) :
```
Main 1 : Tu coupes les légumes     ⏱️ 5 min
Main 2 : En même temps, l'eau chauffe  ⏱️ 10 min
Main 1 : Tu fais cuire les pâtes   ⏱️ 10 min
─────────────────────────────────────────
Total : 15 minutes ! (on économise du temps)
```

**Un thread = une "main" qui peut faire une tâche en parallèle**

---

## 🎯 Pourquoi on a besoin de threads dans ce projet ?

### Problème sans threads :

```
SERVEUR avec 1 seul thread :

Client 1 se connecte → On s'occupe de lui
                       ↓
Client 2 veut se connecter → ATTEND ! (bloqué)
                              ↓
Client 3 veut se connecter → ATTEND AUSSI ! (bloqué)
```

😱 **Un seul client peut bloquer tout le monde !**

### Solution avec threads :

```
SERVEUR avec plusieurs threads :

Thread 1 → S'occupe de Client 1
Thread 2 → S'occupe de Client 2  ← En MÊME TEMPS !
Thread 3 → S'occupe de Client 3  ← En MÊME TEMPS !
```

✅ **Chaque client a son propre "employé" dédié !**

---

## 🏗️ Les threads dans le SERVEUR

Le serveur a **3 types de threads** :

```
╔══════════════════════════════════════════════════════════════╗
║                         SERVEUR                               ║
╠══════════════════════════════════════════════════════════════╣
║                                                                ║
║   ┌──────────────────┐                                        ║
║   │ THREAD PRINCIPAL │  ← Accepte les nouvelles connexions    ║
║   │                  │     (comme un réceptionniste)          ║
║   └────────┬─────────┘                                        ║
║            │                                                   ║
║            │ Crée un nouveau thread pour chaque client        ║
║            ▼                                                   ║
║   ┌──────────────────┐ ┌──────────────────┐                  ║
║   │ THREAD CLIENT 1  │ │ THREAD CLIENT 2  │  ← 1 par client  ║
║   │ (User Handler)   │ │ (User Handler)   │                  ║
║   └──────────────────┘ └──────────────────┘                  ║
║                                                                ║
║   ┌──────────────────┐                                        ║
║   │ THREAD LIVRAISON │  ← Livre les messages toutes les 30s  ║
║   │ (Delivery)       │     (comme un facteur)                 ║
║   └──────────────────┘                                        ║
║                                                                ║
╚══════════════════════════════════════════════════════════════╝
```

### Thread Principal (main)
```cpp
// Boucle infinie : accepte les connexions
while (serverRunning) {
    // Attendre une nouvelle connexion
    SOCKET clientSocket = accept(serverSocket, ...);
    
    // Créer un NOUVEAU THREAD pour ce client
    thread* t = new thread(userHandlerThread, clientSocket);
}
```

### Thread User Handler (1 par client)
```cpp
void userHandlerThread(SOCKET clientSocket) {
    // Recevoir le nom d'utilisateur
    recv(clientSocket, username);
    
    // Boucle : écouter les commandes de CE client
    while (running) {
        recv(clientSocket, command);
        
        if (command == "SEND:") {
            // Traiter l'envoi de message
        }
        // ... autres commandes
    }
}
```

### Thread de Livraison (Delivery)
```cpp
void deliveryThread() {
    while (running) {
        // Dormir 30 secondes
        sleep_for(seconds(30));
        
        // Vider la file d'attente
        while (!messageQueue.empty()) {
            Message msg = messageQueue.front();
            messageQueue.pop();
            
            // Envoyer au destinataire
            sendMessageToUser(msg.to, msg);
        }
    }
}
```

---

## 🏗️ Les threads dans le CLIENT

Le client a **2 threads** :

```
╔══════════════════════════════════════════════════════════════╗
║                          CLIENT                               ║
╠══════════════════════════════════════════════════════════════╣
║                                                                ║
║   ┌──────────────────┐                                        ║
║   │ THREAD PRINCIPAL │  ← Affiche le menu                    ║
║   │                  │     Attend tes commandes              ║
║   │                  │     (choix 1, 2, 3...)                ║
║   └──────────────────┘                                        ║
║                                                                ║
║   ┌──────────────────┐                                        ║
║   │ THREAD D'ÉCOUTE  │  ← Écoute en PERMANENCE               ║
║   │ (Listen Thread)  │     les messages du serveur           ║
║   │                  │     (en arrière-plan)                 ║
║   └──────────────────┘                                        ║
║                                                                ║
╚══════════════════════════════════════════════════════════════╝
```

### Pourquoi c'est nécessaire ?

**Sans thread d'écoute :**
```
Tu es en train de taper un message...
→ Un message arrive !
→ Tu ne le vois pas car tu es bloqué sur la saisie !
```

**Avec thread d'écoute :**
```
Thread 1 : Tu tapes ton message...
Thread 2 : EN MÊME TEMPS, écoute les messages entrants
→ Un message arrive !
→ Thread 2 l'affiche immédiatement : "[NOUVEAU MESSAGE]"
```

---

## 🔒 Le problème des données partagées

### Le danger :

Imagine deux threads qui modifient la MÊME liste :

```
LISTE DE MESSAGES : [msg1, msg2]

Thread 1 : "Je vais ajouter msg3"
Thread 2 : "Je vais ajouter msg4"

Si les deux font ça EN MÊME TEMPS :
→ 💥 CORRUPTION DE DONNÉES ! 💥
```

### La solution : Les MUTEX

Un **mutex** c'est comme une **clé de salle de bain** :
- Quand tu entres, tu **verrouilles** (lock)
- Personne d'autre ne peut entrer
- Quand tu sors, tu **déverrouilles** (unlock)

```cpp
mutex monMutex;  // La "clé"

// Thread 1
{
    lock_guard<mutex> lock(monMutex);  // Je prends la clé
    liste.push_back(msg3);              // Je modifie la liste
}   // Automatiquement, je rends la clé

// Thread 2 (doit attendre que Thread 1 ait fini)
{
    lock_guard<mutex> lock(monMutex);  // J'attends la clé...
    liste.push_back(msg4);              // Je modifie la liste
}
```

---

## 🔐 Les mutex dans le projet

Le projet utilise **4 mutex** pour protéger les données :

```cpp
// Protection de la liste des utilisateurs connectés
mutex g_usersMutex;

// Protection de la file d'attente des messages
mutex g_queueMutex;

// Protection de l'historique des messages
mutex g_historyMutex;

// Protection du fichier de log
mutex g_logMutex;
```

### Exemple concret dans le code :

```cpp
// Ajouter un message à la file d'attente
{
    lock_guard<mutex> lock(g_queueMutex);  // Verrouiller
    g_messageQueue.push(message);           // Modifier
}  // Déverrouiller automatiquement

// Lire la liste des utilisateurs
{
    lock_guard<mutex> lock(g_usersMutex);  // Verrouiller
    for (auto& user : g_connectedUsers) {
        // Lire les utilisateurs en sécurité
    }
}  // Déverrouiller automatiquement
```

---

## ⚛️ Les variables atomiques

Parfois, un mutex c'est "trop lourd" pour une simple variable.

### Solution : `atomic`

```cpp
atomic<bool> g_serverRunning(true);   // Variable atomique
atomic<bool> g_isComposing(false);    // Variable atomique

// Modifier/lire sans mutex :
g_serverRunning = false;  // Sûr !
if (g_isComposing) { }    // Sûr !
```

C'est comme une variable normale, mais **protégée automatiquement** pour les threads.

---

## 🎭 Création et gestion des threads en C++

### Créer un thread :
```cpp
#include <thread>

void maFonction(int parametre) {
    // Code qui s'exécute dans le thread
}

// Créer et démarrer le thread
thread monThread(maFonction, 42);
```

### Attendre qu'un thread finisse :
```cpp
if (monThread.joinable()) {
    monThread.join();  // Attendre la fin
}
```

### Dans le projet :

```cpp
// Serveur : créer le thread de livraison
thread deliveryThreadObj(deliveryThread);

// Serveur : créer un thread par client
thread* userThread = new thread(userHandlerThread, clientSocket);

// Client : créer le thread d'écoute
thread listener(listenThread);

// À la fin : attendre tous les threads
deliveryThreadObj.join();
listener.join();
```

---

## 📊 Schéma complet des threads

```
                    ╔═══════════════════════════════════════════╗
                    ║               SERVEUR                      ║
                    ╠═══════════════════════════════════════════╣
                    ║                                            ║
                    ║  [Thread Principal]                        ║
                    ║       │                                    ║
                    ║       │ accept()                           ║
                    ║       ▼                                    ║
┌──────────────┐    ║  ┌─────────────┐  ┌─────────────┐         ║
│   Client 1   │◄───║──│ Thread U1   │  │ Thread U2   │◄────────║───┐
│ (2 threads)  │    ║  │(userHandler)│  │(userHandler)│         ║   │
│ - Principal  │    ║  └──────┬──────┘  └──────┬──────┘         ║   │
│ - Listen     │    ║         │                │                ║   │
└──────────────┘    ║         ▼                ▼                ║   │
                    ║  ┌─────────────────────────────┐          ║   │
                    ║  │    DONNÉES PARTAGÉES        │          ║   │
                    ║  │    (protégées par mutex)    │          ║   │
                    ║  │                             │          ║   │
                    ║  │  g_connectedUsers 🔒        │          ║   │
                    ║  │  g_messageQueue   🔒        │          ║   │
                    ║  │  g_messageHistory 🔒        │          ║   │
                    ║  └──────────────┬──────────────┘          ║   │
                    ║                 │                         ║   │
                    ║                 ▼                         ║   │
                    ║  [Thread Delivery]                        ║   │
                    ║  - Toutes les 30s                         ║   │
                    ║  - Vide la queue                          ║   │
                    ║  - Envoie les messages                    ║   │
                    ║                                            ║   │
                    ╚═══════════════════════════════════════════╝   │
                                                                     │
                                                            ┌────────┘
                                                            │
                                                    ┌───────▼──────┐
                                                    │   Client 2   │
                                                    │ (2 threads)  │
                                                    │ - Principal  │
                                                    │ - Listen     │
                                                    └──────────────┘
```

---

## 🎯 Ce qu'il faut retenir pour l'oral

> "Le projet utilise une architecture **multi-threads** pour gérer plusieurs clients simultanément.
>
> **Côté serveur**, il y a :
> - Un **thread principal** qui accepte les connexions
> - Un **thread par client** (User Handler) qui traite ses commandes
> - Un **thread de livraison** qui distribue les messages toutes les 30 secondes
>
> **Côté client**, il y a :
> - Un **thread principal** pour l'interface utilisateur
> - Un **thread d'écoute** qui reçoit les messages en arrière-plan
>
> Pour éviter les **conflits d'accès** aux données partagées, on utilise des **mutex** qui garantissent qu'un seul thread modifie les données à la fois."

---

➡️ **Fichier suivant** : `04_LA_STRUCTURE_MESSAGE.md` pour comprendre comment sont construits les messages
