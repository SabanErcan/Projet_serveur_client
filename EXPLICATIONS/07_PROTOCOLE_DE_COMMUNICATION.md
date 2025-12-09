# 📡 PROTOCOLE DE COMMUNICATION

## 🤔 C'est quoi un protocole ?

Un **protocole** c'est comme un **langage commun** entre le client et le serveur.

Exemple dans la vie réelle :
```
Toi     : "Bonjour"      ← Salutation
Vendeur : "Bonjour !"    ← Réponse
Toi     : "Je voudrais un café"  ← Demande
Vendeur : "Voilà, 2€"    ← Réponse
Toi     : "Merci, au revoir"  ← Fin
```

Si tu parles japonais et le vendeur français... ça ne marche pas !

**Le protocole définit :**
- Quels **messages** on peut envoyer
- Dans quel **format**
- Quelles **réponses** on attend

---

## 📋 Les commandes du protocole

### Du CLIENT vers le SERVEUR :

| Commande | Description | Format |
|----------|-------------|--------|
| `SEND:` | Envoyer un message | `SEND:` + Message sérialisé |
| `LIST_USERS` | Demander la liste des utilisateurs | Texte simple |
| `GET_LOG` | Demander le fichier log | Texte simple |
| `DISCONNECT` | Se déconnecter | Texte simple |

### Du SERVEUR vers le CLIENT :

| Préfixe | Description | Format |
|---------|-------------|--------|
| `MSG:` | Nouveau message | `MSG:` + Message sérialisé |
| `OK:` | Confirmation | `OK:` + texte |
| `ERROR:` | Erreur | `ERROR:` + texte |
| `NOTIFY:` | Notification | `NOTIFY:` + texte |
| `USERS:` | Liste des utilisateurs | `USERS:` + noms séparés par `;` |
| `LOG:` | Contenu du log | `LOG:` + contenu du fichier |

---

## 📦 Le protocole à préfixe de longueur

### Le problème de TCP

TCP est un protocole de **flux** (stream), pas de **messages** :

```
Tu envoies : "Bonjour" puis "Comment vas-tu ?"

TCP peut livrer :
  Paquet 1 : "BonjourComment"
  Paquet 2 : " vas-tu ?"

Ou :
  Paquet 1 : "Bonj"
  Paquet 2 : "our"
  Paquet 3 : "Comment vas-tu ?"
```

**Comment savoir où finit un message ?**

### La solution : Préfixe de longueur

On envoie **d'abord la taille**, puis les données :

```
┌───────────────────┬────────────────────────────┐
│  TAILLE (4 octets)│       DONNÉES              │
├───────────────────┼────────────────────────────┤
│  00 00 00 07      │  B o n j o u r             │
│  (7 en binaire)   │  (7 caractères)            │
└───────────────────┴────────────────────────────┘
```

Le récepteur :
1. Lit les 4 premiers octets → "Ah, il y a 7 octets de données"
2. Lit exactement 7 octets → "Bonjour"
3. C'est fini !

---

## 🔄 Schéma d'un échange complet

### Exemple : Alice envoie "Salut" à Bob

```
╔══════════════════════════════════════════════════════════════════╗
║                    ENVOI D'UN MESSAGE                             ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                    ║
║  ALICE (Client)                              SERVEUR               ║
║  ─────────────                               ───────               ║
║                                                                    ║
║  1. Compose le message                                            ║
║     Message("Alice", "Bob", "Hello", "Salut!")                    ║
║                                                                    ║
║  2. Envoie la commande                                            ║
║     ┌────────────────────────────┐                                ║
║     │ [5] S E N D :              │ ─────────────────────►        ║
║     │ (longueur=5, "SEND:")     │                                ║
║     └────────────────────────────┘                                ║
║                                                                    ║
║  3. Envoie le message sérialisé                                   ║
║     ┌────────────────────────────┐                                ║
║     │ [709] <données binaires>  │ ─────────────────────►        ║
║     │ (longueur=709, Message)   │                                ║
║     └────────────────────────────┘                                ║
║                                                                    ║
║                                  4. Le serveur reçoit              ║
║                                     - Désérialise le message      ║
║                                     - L'ajoute à la queue         ║
║                                                                    ║
║  5. Reçoit la confirmation                                        ║
║     ┌────────────────────────────┐                                ║
║     │ [24] O K : M e s s ...    │ ◄─────────────────────         ║
║     │ "OK:Message en file"      │                                ║
║     └────────────────────────────┘                                ║
║                                                                    ║
╚══════════════════════════════════════════════════════════════════╝
```

---

### Exemple : Le serveur livre le message à Bob

```
╔══════════════════════════════════════════════════════════════════╗
║                    LIVRAISON D'UN MESSAGE                         ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                    ║
║  SERVEUR (Thread Livraison)                BOB (Client)           ║
║  ──────────────────────────                ────────────           ║
║                                                                    ║
║  1. 30 secondes sont passées                                      ║
║                                                                    ║
║  2. Récupère le message de la queue                               ║
║     msg = queue.front()                                           ║
║     queue.pop()                                                   ║
║                                                                    ║
║  3. Prépare le paquet                                             ║
║     "MSG:" + message.serialize()                                  ║
║                                                                    ║
║  4. Envoie à Bob                                                  ║
║     ┌────────────────────────────┐                                ║
║     │ [713] M S G : <données>   │ ─────────────────────►        ║
║     │ (4 + 709 octets)          │                                ║
║     └────────────────────────────┘                                ║
║                                                                    ║
║                                  5. Bob reçoit (Thread Écoute)    ║
║                                     - Lit "MSG:"                  ║
║                                     - Désérialise le message     ║
║                                     - L'ajoute à sa liste        ║
║                                     - Affiche notification       ║
║                                                                    ║
║                                  ┌─────────────────────────────┐  ║
║                                  │ [NOUVEAU MESSAGE]            │  ║
║                                  │ De: Alice | Sujet: Hello     │  ║
║                                  └─────────────────────────────┘  ║
║                                                                    ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## 🔢 Format des différentes réponses

### MSG: (Nouveau message)

```
┌──────────────────────────────────────────────────────────┐
│ Format : "MSG:" + Message sérialisé (709 octets)         │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  Octets 0-3   : Longueur totale (ex: 713)               │
│  Octets 4-7   : "MSG:"                                  │
│  Octets 8-716 : Données du Message                      │
│                 - from (50 octets)                       │
│                 - to (50 octets)                         │
│                 - subject (100 octets)                   │
│                 - body (500 octets)                      │
│                 - isRead (1 octet)                       │
│                 - receivedAt (8 octets)                  │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### USERS: (Liste des utilisateurs)

```
┌──────────────────────────────────────────────────────────┐
│ Format : "USERS:" + noms séparés par ";"                 │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  Exemple : "USERS:Alice;Bob;Charlie;"                    │
│                                                          │
│  Le client découpe avec ";" pour obtenir :              │
│  - "Alice"                                               │
│  - "Bob"                                                 │
│  - "Charlie"                                             │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### OK: / ERROR: / NOTIFY: (Texte simple)

```
┌──────────────────────────────────────────────────────────┐
│ Format : Préfixe + texte explicatif                      │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  "OK:Message en file d'attente"                         │
│  "ERROR:Commande inexistante"                            │
│  "NOTIFY:Échec - Bob non connecté"                       │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### LOG: (Fichier log)

```
┌──────────────────────────────────────────────────────────┐
│ Format : "LOG:" + contenu complet du fichier             │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  "LOG:[2024-12-09 14:30:00] Serveur démarré              │
│  [2024-12-09 14:30:15] Alice connectée                   │
│  [2024-12-09 14:30:20] Bob connecté                      │
│  ..."                                                    │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

---

## 🔧 Le code d'envoi/réception

### Envoi avec préfixe (sendWithLength)

```cpp
void SocketUtils::sendWithLength(SOCKET sock, const char* data, size_t size) {
    // 1. Convertir la taille en "network byte order" (big-endian)
    uint32_t netSize = htonl(static_cast<uint32_t>(size));
    
    // 2. Envoyer la taille (4 octets)
    sendData(sock, reinterpret_cast<char*>(&netSize), sizeof(netSize));
    
    // 3. Envoyer les données
    sendData(sock, data, size);
}
```

### Réception avec préfixe (receiveWithLength)

```cpp
size_t SocketUtils::receiveWithLength(SOCKET sock, char* buffer, size_t maxSize) {
    // 1. Lire la taille (4 octets)
    uint32_t netSize;
    if (!receiveExact(sock, reinterpret_cast<char*>(&netSize), sizeof(netSize))) {
        return 0;  // Déconnexion
    }
    
    // 2. Convertir depuis "network byte order"
    size_t size = ntohl(netSize);
    
    // 3. Vérifier que ça ne dépasse pas le buffer
    if (size > maxSize) {
        throw runtime_error("Message trop grand");
    }
    
    // 4. Lire exactement 'size' octets
    if (!receiveExact(sock, buffer, size)) {
        return 0;
    }
    
    return size;
}
```

---

## 🌐 Network Byte Order (Big-Endian)

### Le problème

Différents ordinateurs stockent les nombres différemment :

```
Le nombre 1000 en hexadécimal = 0x000003E8

Little-Endian (Intel) : E8 03 00 00  ← Octet de poids faible en premier
Big-Endian (Network)  : 00 00 03 E8  ← Octet de poids fort en premier
```

### La solution

On convertit TOUJOURS en "Network Byte Order" (Big-Endian) pour le réseau :

```cpp
// Avant d'envoyer : Host → Network
uint32_t netSize = htonl(size);  // htonl = Host TO Network Long

// Après réception : Network → Host
size_t size = ntohl(netSize);    // ntohl = Network TO Host Long
```

---

## 📊 Schéma du flux de données

```
╔═══════════════════════════════════════════════════════════════════╗
║                    FLUX DE DONNÉES COMPLET                         ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                     ║
║  CLIENT                          RÉSEAU                  SERVEUR   ║
║  ──────                          ──────                  ───────   ║
║                                                                     ║
║  Message msg("A","B","X","Y")                                      ║
║       │                                                            ║
║       ▼                                                            ║
║  msg.serialize(buffer)                                             ║
║       │                                                            ║
║       ▼                                                            ║
║  "SEND:" + buffer                                                  ║
║       │                                                            ║
║       ▼                                                            ║
║  [longueur][données]  ────────►  Paquets TCP  ────────►           ║
║                                                                     ║
║                                                    [longueur]      ║
║                                                        │           ║
║                                                        ▼           ║
║                                                    receiveExact()  ║
║                                                        │           ║
║                                                        ▼           ║
║                                                    [données]       ║
║                                                        │           ║
║                                                        ▼           ║
║                                              Message::deserialize()║
║                                                        │           ║
║                                                        ▼           ║
║                                              g_messageQueue.push() ║
║                                                                     ║
╚═══════════════════════════════════════════════════════════════════╝
```

---

## 🎯 Ce qu'il faut retenir pour l'oral

> "Le **protocole de communication** définit le format des échanges entre client et serveur.
>
> Chaque message utilise un **protocole à préfixe de longueur** : on envoie d'abord la taille sur 4 octets, puis les données. Cela résout le problème de **fragmentation TCP**.
>
> Les **commandes client** sont : `SEND:`, `LIST_USERS`, `GET_LOG`, `DISCONNECT`.
>
> Les **réponses serveur** utilisent des préfixes : `MSG:`, `OK:`, `ERROR:`, `NOTIFY:`, `USERS:`, `LOG:`.
>
> Les tailles sont converties en **Network Byte Order** (big-endian) avec `htonl`/`ntohl` pour garantir la compatibilité entre machines."

---

➡️ **Fichier suivant** : `08_RESUME_POUR_ORAL.md` pour un résumé complet pour ton oral
