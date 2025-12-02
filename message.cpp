/*
 * message.cpp
 * 
 * Implémentation des méthodes de la structure Message.
 * Gère la construction, validation, sérialisation et affichage des messages.
 * 
 * Projet R3.05 - Programmation Système
 */

#include "message.h"
#include <sstream>
#include <iomanip>

/* ========================================================================== */
/*                            CONSTRUCTEURS                                   */
/* ========================================================================== */

/*
 * Constructeur par défaut.
 * Initialise tous les champs à zéro pour éviter les données non initialisées.
 * memset garantit que les tableaux de char sont remplis de '\0'.
 */
Message::Message() : isRead(false), receivedAt(0) {
    memset(from, 0, MAX_FROM_SIZE);
    memset(to, 0, MAX_TO_SIZE);
    memset(subject, 0, MAX_SUBJECT_SIZE);
    memset(body, 0, MAX_BODY_SIZE);
}

/*
 * Constructeur avec paramètres.
 * Valide les tailles des champs avant copie.
 * Lève une exception std::invalid_argument si un champ est trop long.
 */
Message::Message(const std::string& fromStr, const std::string& toStr, 
                 const std::string& subjectStr, const std::string& bodyStr) : isRead(false), receivedAt(0) {
    
    /* Validation des contraintes de taille */
    validateField(fromStr, MAX_FROM_SIZE - 1, "From");
    validateField(toStr, MAX_TO_SIZE - 1, "To");
    validateField(subjectStr, MAX_SUBJECT_SIZE - 1, "Subject");
    validateField(bodyStr, MAX_BODY_SIZE - 1, "Body");
    
    /* 
     * Copie sécurisée avec strncpy.
     * Note : strncpy ne garantit pas le '\0' final si la chaîne source
     * atteint exactement la taille limite, d'où l'ajout explicite.
     */
    strncpy(from, fromStr.c_str(), MAX_FROM_SIZE - 1);
    strncpy(to, toStr.c_str(), MAX_TO_SIZE - 1);
    strncpy(subject, subjectStr.c_str(), MAX_SUBJECT_SIZE - 1);
    strncpy(body, bodyStr.c_str(), MAX_BODY_SIZE - 1);
    
    /* Garantie du terminateur nul */
    from[MAX_FROM_SIZE - 1] = '\0';
    to[MAX_TO_SIZE - 1] = '\0';
    subject[MAX_SUBJECT_SIZE - 1] = '\0';
    body[MAX_BODY_SIZE - 1] = '\0';
}

/* ========================================================================== */
/*                             VALIDATION                                     */
/* ========================================================================== */

/*
 * Valide qu'un champ ne dépasse pas sa taille maximale.
 * Lève std::invalid_argument avec un message explicite en cas d'erreur.
 */
void Message::validateField(const std::string& field, size_t maxSize, const std::string& fieldName) {
    if (field.length() > maxSize) {
        throw std::invalid_argument(fieldName + " dépasse la limite de " + std::to_string(maxSize) + " caractères");
    }
}

/* ========================================================================== */
/*                     SÉRIALISATION / DÉSÉRIALISATION                        */
/* ========================================================================== */

/*
 * Sérialise la structure Message en un tableau d'octets.
 * Utilise memcpy pour une copie binaire directe de la structure.
 * 
 * Cette approche fonctionne car Message est une structure POD
 * (Plain Old Data) avec des champs de taille fixe.
 */
void Message::serialize(char* buffer, size_t& size) const {
    size = sizeof(Message);
    memcpy(buffer, this, size);
}

/*
 * Désérialise un tableau d'octets en structure Message.
 * Vérifie que la taille reçue correspond exactement à sizeof(Message).
 */
Message Message::deserialize(const char* buffer, size_t size) {
    if (size != sizeof(Message)) {
        throw std::runtime_error("Taille de message invalide lors de la désérialisation");
    }
    Message msg;
    memcpy(&msg, buffer, size);
    return msg;
}

/* ========================================================================== */
/*                            AFFICHAGE                                       */
/* ========================================================================== */

/*
 * Retourne une représentation complète du message.
 * Utilisé pour la lecture détaillée d'un message.
 * Affiche l'horodatage si celui-ci est défini (non nul).
 */
std::string Message::toString() const {
    std::ostringstream oss;
    oss << "=== MESSAGE ===\n"
        << "De: " << from << "\n"
        << "À: " << to << "\n"
        << "Sujet: " << subject << "\n";
    
    /* Affichage conditionnel de l'horodatage */
    if (receivedAt != 0) {
        oss << "Reçu le: " << std::put_time(std::localtime(&receivedAt), "%Y-%m-%d %H:%M:%S") << "\n";
    }
    
    oss << "Corps:\n" << body << "\n"
        << "Lu: " << (isRead ? "Oui" : "Non") << "\n"
        << "===============";
    return oss.str();
}

/*
 * Retourne une représentation résumée du message.
 * Utilisé pour l'affichage des listes de messages.
 * Format compact : [état] De: expéditeur | Sujet: sujet
 */
std::string Message::toShortString() const {
    std::ostringstream oss;
    oss << "[" << (isRead ? "Lu" : "Non lu") << "] "
        << "De: " << std::setw(15) << std::left << from
        << " | Sujet: " << subject;
    return oss.str();
}
