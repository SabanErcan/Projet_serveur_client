# 📨 LA STRUCTURE MESSAGE EXPLIQUÉE

## 🤔 C'est quoi un Message dans le projet ?

Un **Message** c'est comme une **lettre** :

```
┌─────────────────────────────────────┐
│  LETTRE                             │
├─────────────────────────────────────┤
│  De : Alice          (expéditeur)   │
│  À  : Bob            (destinataire) │
│  Objet : Salut !     (sujet)        │
├─────────────────────────────────────┤
│                                     │
│  Coucou Bob, comment ça va ?        │
│  (corps du message)                 │
│                                     │
└─────────────────────────────────────┘
```

---

## 📝 La structure Message dans le code

Voici comment le message est défini dans `message.h` :

```cpp
struct Message {
    char from[50];      // Expéditeur (max 50 caractères)
    char to[50];        // Destinataire (max 50 caractères)
    char subject[100];  // Sujet (max 100 caractères)
    char body[500];     // Corps du message (max 500 caractères)
    bool isRead;        // Est-ce que le message a été lu ?
    time_t receivedAt;  // Date/heure de réception
};
```

---

## 📏 Pourquoi des tailles fixes ?

### Le problème avec les tailles variables :

```
Message 1 : "Salut"           → 5 caractères
Message 2 : "Bonjour à tous"  → 14 caractères
```

Si on envoie ça sur le réseau, comment le récepteur sait où finit le message ?

### La solution : Tailles fixes !

```cpp
char body[500];  // TOUJOURS 500 caractères en mémoire
```

Même si tu écris seulement "Salut" (5 caractères), on réserve quand même 500 caractères.

### Avantage :
```cpp
sizeof(Message)  // TOUJOURS la même taille !
```

On peut envoyer la structure **en bloc** sur le réseau, c'est simple et efficace.

---

## 🔢 Taille totale d'un Message

```
from[50]      =  50 octets
to[50]        =  50 octets
subject[100]  = 100 octets
body[500]     = 500 octets
isRead        =   1 octet
receivedAt    =   8 octets (sur systèmes 64 bits)
─────────────────────────────
TOTAL         ≈ 709 octets
```

---

## 🏗️ Les constructeurs du Message

### Constructeur par défaut (vide) :

```cpp
Message msg;  // Crée un message vide
```

Le code :
```cpp
Message::Message() : isRead(false), receivedAt(0) {
    memset(from, 0, MAX_FROM_SIZE);      // Remplir de zéros
    memset(to, 0, MAX_TO_SIZE);          // Remplir de zéros
    memset(subject, 0, MAX_SUBJECT_SIZE);// Remplir de zéros
    memset(body, 0, MAX_BODY_SIZE);      // Remplir de zéros
}
```

`memset` remplit toute la mémoire avec des `\0` (caractère nul).

### Constructeur avec paramètres :

```cpp
Message msg("Alice", "Bob", "Salut !", "Comment ça va ?");
```

Le code :
```cpp
Message::Message(const string& fromStr, const string& toStr, 
                 const string& subjectStr, const string& bodyStr) {
    
    // D'abord, on vérifie les tailles
    validateField(fromStr, MAX_FROM_SIZE - 1, "From");
    validateField(toStr, MAX_TO_SIZE - 1, "To");
    validateField(subjectStr, MAX_SUBJECT_SIZE - 1, "Subject");
    validateField(bodyStr, MAX_BODY_SIZE - 1, "Body");
    
    // Puis on copie les données
    strncpy(from, fromStr.c_str(), MAX_FROM_SIZE - 1);
    strncpy(to, toStr.c_str(), MAX_TO_SIZE - 1);
    strncpy(subject, subjectStr.c_str(), MAX_SUBJECT_SIZE - 1);
    strncpy(body, bodyStr.c_str(), MAX_BODY_SIZE - 1);
    
    isRead = false;
    receivedAt = 0;
}
```

---

## ✅ Validation des champs

Avant de créer un message, on vérifie que les données ne dépassent pas les limites :

```cpp
void Message::validateField(const string& field, size_t maxSize, const string& fieldName) {
    if (field.length() > maxSize) {
        throw invalid_argument(fieldName + " dépasse la limite de " 
                               + to_string(maxSize) + " caractères");
    }
}
```

### Exemple :
```cpp
// Essayer de mettre un sujet trop long
string sujetTropLong = "Blablabla..."; // 150 caractères

Message msg("Alice", "Bob", sujetTropLong, "Test");
// → ERREUR ! "Subject dépasse la limite de 99 caractères"
```

---

## 📦 Sérialisation : Transformer en octets

### Le problème :
On ne peut pas envoyer un objet C++ directement sur le réseau.
On doit le transformer en **suite d'octets**.

### La solution : `serialize()`

```cpp
void Message::serialize(char* buffer, size_t& size) const {
    size = sizeof(Message);           // La taille totale
    memcpy(buffer, this, size);       // Copier la structure en mémoire
}
```

### Schéma :
```
OBJET MESSAGE                    BUFFER (octets)
┌───────────────────┐            ┌─────────────────────────┐
│ from: "Alice"     │            │ A l i c e \0 \0 ... \0  │
│ to: "Bob"         │  ────────► │ B o b \0 \0 \0 ... \0   │
│ subject: "Test"   │  memcpy()  │ T e s t \0 ... \0       │
│ body: "Contenu"   │            │ C o n t e n u \0 ...    │
│ isRead: false     │            │ 0                       │
│ receivedAt: 0     │            │ 0 0 0 0 0 0 0 0         │
└───────────────────┘            └─────────────────────────┘
```

---

## 📭 Désérialisation : Reconstruire depuis les octets

### C'est l'inverse !

```cpp
Message Message::deserialize(const char* buffer, size_t size) {
    if (size != sizeof(Message)) {
        throw runtime_error("Taille invalide");
    }
    
    Message msg;
    memcpy(&msg, buffer, size);  // Copier les octets dans la structure
    return msg;
}
```

### Schéma :
```
BUFFER (octets)                  OBJET MESSAGE
┌─────────────────────────┐      ┌───────────────────┐
│ A l i c e \0 \0 ... \0  │      │ from: "Alice"     │
│ B o b \0 \0 \0 ... \0   │      │ to: "Bob"         │
│ T e s t \0 ... \0       │ ───► │ subject: "Test"   │
│ C o n t e n u \0 ...    │      │ body: "Contenu"   │
│ 0                       │      │ isRead: false     │
│ 0 0 0 0 0 0 0 0         │      │ receivedAt: 0     │
└─────────────────────────┘      └───────────────────┘
```

---

## 🖨️ Affichage des messages

### Affichage complet : `toString()`
```cpp
string Message::toString() const {
    stringstream ss;
    ss << "══════════════════════════════════\n";
    ss << "De     : " << from << "\n";
    ss << "À      : " << to << "\n";
    ss << "Sujet  : " << subject << "\n";
    ss << "──────────────────────────────────\n";
    ss << body << "\n";
    ss << "══════════════════════════════════";
    return ss.str();
}
```

Résultat :
```
══════════════════════════════════
De     : Alice
À      : Bob
Sujet  : Salut !
──────────────────────────────────
Comment ça va ?
══════════════════════════════════
```

### Affichage résumé : `toShortString()`
```cpp
string Message::toShortString() const {
    string status = isRead ? "[LU]" : "[NON LU]";
    return status + " De: " + from + " | Sujet: " + subject;
}
```

Résultat :
```
[NON LU] De: Alice | Sujet: Salut !
```

---

## 🌐 Le destinataire spécial : "all"

Si tu mets **"all"** comme destinataire, le message est envoyé à **tout le monde** !

```cpp
Message msg("Alice", "all", "Annonce", "Bonjour à tous !");
```

Côté serveur :
```cpp
if (string(msg.to) == "all") {
    broadcastMessage(msg);  // Envoyer à tout le monde
} else {
    sendMessageToUser(msg.to, msg);  // Envoyer à une seule personne
}
```

---

## 📊 Schéma récapitulatif

```
╔═══════════════════════════════════════════════════════════════════╗
║                      STRUCTURE MESSAGE                             ║
╠═══════════════════════════════════════════════════════════════════╣
║                                                                    ║
║   ┌────────────────────────────────────────────────────────────┐  ║
║   │                       CHAMPS                                │  ║
║   ├────────────────────────────────────────────────────────────┤  ║
║   │  from[50]       ← Expéditeur (qui envoie)                  │  ║
║   │  to[50]         ← Destinataire (qui reçoit) ou "all"       │  ║
║   │  subject[100]   ← Sujet du message                         │  ║
║   │  body[500]      ← Contenu du message                       │  ║
║   │  isRead         ← Le message a-t-il été lu ?               │  ║
║   │  receivedAt     ← Date/heure de réception                  │  ║
║   └────────────────────────────────────────────────────────────┘  ║
║                                                                    ║
║   ┌────────────────────────────────────────────────────────────┐  ║
║   │                      MÉTHODES                               │  ║
║   ├────────────────────────────────────────────────────────────┤  ║
║   │  serialize()    → Transformer en octets pour le réseau    │  ║
║   │  deserialize()  → Reconstruire depuis les octets           │  ║
║   │  toString()     → Affichage complet                        │  ║
║   │  toShortString()→ Affichage résumé                         │  ║
║   │  validateField()→ Vérifier les tailles                     │  ║
║   └────────────────────────────────────────────────────────────┘  ║
║                                                                    ║
╚═══════════════════════════════════════════════════════════════════╝
```

---

## 🔄 Cycle de vie d'un message

```
1. CRÉATION (Client)
   ┌────────────────────────────────────────────┐
   │ Message msg("Alice", "Bob", "Test", "Hi"); │
   └────────────────────────────────────────────┘
                        │
                        ▼
2. SÉRIALISATION (Client)
   ┌────────────────────────────────────────────┐
   │ msg.serialize(buffer, size);               │
   │ → Transformé en 709 octets                 │
   └────────────────────────────────────────────┘
                        │
                        ▼
3. ENVOI SUR LE RÉSEAU
   ┌────────────────────────────────────────────┐
   │ Client ───── 709 octets ─────► Serveur     │
   └────────────────────────────────────────────┘
                        │
                        ▼
4. DÉSÉRIALISATION (Serveur)
   ┌────────────────────────────────────────────┐
   │ Message msg = Message::deserialize(buffer);│
   │ → Reconstruit l'objet Message              │
   └────────────────────────────────────────────┘
                        │
                        ▼
5. STOCKAGE EN FILE D'ATTENTE
   ┌────────────────────────────────────────────┐
   │ g_messageQueue.push(msg);                  │
   └────────────────────────────────────────────┘
                        │
                        ▼
6. LIVRAISON AU DESTINATAIRE
   ┌────────────────────────────────────────────┐
   │ Serveur ───── Message ─────► Client Bob    │
   └────────────────────────────────────────────┘
```

---

## 🎯 Ce qu'il faut retenir pour l'oral

> "La structure **Message** représente un message échangé entre utilisateurs.
>
> Elle contient des champs de **taille fixe** (expéditeur, destinataire, sujet, corps) pour faciliter la transmission réseau.
>
> La **sérialisation** transforme l'objet en octets pour l'envoi, et la **désérialisation** fait l'inverse.
>
> Le destinataire **"all"** permet d'envoyer un message à tous les utilisateurs connectés (broadcast).
>
> Chaque champ est **validé** pour ne pas dépasser sa taille maximale, ce qui évite les erreurs et les failles de sécurité."

---

➡️ **Fichier suivant** : `05_LE_SERVEUR_EN_DETAIL.md` pour comprendre le fonctionnement complet du serveur
