#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <cstring>
#include <stdexcept>

/**
 * Définition des constantes de taille pour les champs du message.
 * Ces limites sont fixes pour simplifier la sérialisation (structure POD).
 */
constexpr size_t MAX_FROM_SIZE = 50;      // Taille max pour l'expéditeur
constexpr size_t MAX_TO_SIZE = 50;        // Taille max pour le destinataire
constexpr size_t MAX_SUBJECT_SIZE = 100;  // Taille max pour le sujet
constexpr size_t MAX_BODY_SIZE = 500;     // Taille max pour le corps du message

/**
 * Structure représentant un message échangé entre client et serveur.
 * Utilise des tableaux de char fixes pour faciliter l'envoi sur le réseau.
 */
struct Message {
    char from[MAX_FROM_SIZE];       // Nom de l'expéditeur
    char to[MAX_TO_SIZE];           // Nom du destinataire ("all" pour broadcast)
    char subject[MAX_SUBJECT_SIZE]; // Sujet du message
    char body[MAX_BODY_SIZE];       // Contenu du message
    bool isRead;                    // État de lecture (pour le client)
    
    // Constructeur par défaut (initialise tout à zéro)
    Message();
    
    // Constructeur paramétré avec validation des tailles
    Message(const std::string& fromStr, const std::string& toStr, 
            const std::string& subjectStr, const std::string& bodyStr);
    
    // Méthode statique pour vérifier si un champ respecte la taille max
    static void validateField(const std::string& field, size_t maxSize, const std::string& fieldName);
    
    // Convertit la structure en tableau d'octets pour l'envoi réseau
    void serialize(char* buffer, size_t& size) const;
    
    // Reconstruit la structure à partir d'un tableau d'octets reçu
    static Message deserialize(const char* buffer, size_t size);
    
    // Formate le message complet pour l'affichage
    std::string toString() const;
    
    // Formate un résumé du message pour les listes
    std::string toShortString() const;
};

#endif // MESSAGE_H
