#include "message.h"
#include <sstream>
#include <iomanip>

/**
 * Constructeur par défaut.
 * Initialise tous les champs à 0 pour éviter les données parasites.
 */
Message::Message() : isRead(false), receivedAt(0) {
    memset(from, 0, MAX_FROM_SIZE);
    memset(to, 0, MAX_TO_SIZE);
    memset(subject, 0, MAX_SUBJECT_SIZE);
    memset(body, 0, MAX_BODY_SIZE);
}

/**
 * Constructeur avec paramètres.
 * Vérifie les tailles avant de copier les données.
 * Lève une exception si une taille est dépassée.
 */
Message::Message(const std::string& fromStr, const std::string& toStr, 
                 const std::string& subjectStr, const std::string& bodyStr) : isRead(false), receivedAt(0) {
    // Vérification des contraintes de taille
    validateField(fromStr, MAX_FROM_SIZE - 1, "From");
    validateField(toStr, MAX_TO_SIZE - 1, "To");
    validateField(subjectStr, MAX_SUBJECT_SIZE - 1, "Subject");
    validateField(bodyStr, MAX_BODY_SIZE - 1, "Body");
    
    // Copie sécurisée des chaînes (strncpy ne met pas toujours le \0 final si taille atteinte)
    strncpy(from, fromStr.c_str(), MAX_FROM_SIZE - 1);
    strncpy(to, toStr.c_str(), MAX_TO_SIZE - 1);
    strncpy(subject, subjectStr.c_str(), MAX_SUBJECT_SIZE - 1);
    strncpy(body, bodyStr.c_str(), MAX_BODY_SIZE - 1);
    
    // Assurance que les chaînes sont bien terminées par \0
    from[MAX_FROM_SIZE - 1] = '\0';
    to[MAX_TO_SIZE - 1] = '\0';
    subject[MAX_SUBJECT_SIZE - 1] = '\0';
    body[MAX_BODY_SIZE - 1] = '\0';
}

/**
 * Valide la taille d'un champ.
 * Lève std::invalid_argument si la taille est excessive.
 */
void Message::validateField(const std::string& field, size_t maxSize, const std::string& fieldName) {
    if (field.length() > maxSize) {
        throw std::invalid_argument(fieldName + " dépasse la limite de " + std::to_string(maxSize) + " caractères");
    }
}

/**
 * Sérialise l'objet Message en un buffer d'octets.
 * Copie brute de la mémoire de la structure.
 */
void Message::serialize(char* buffer, size_t& size) const {
    size = sizeof(Message);
    memcpy(buffer, this, size);
}

/**
 * Désérialise un buffer d'octets en objet Message.
 * Vérifie que la taille correspond bien à la structure attendue.
 */
Message Message::deserialize(const char* buffer, size_t size) {
    if (size != sizeof(Message)) {
        throw std::runtime_error("Taille de message invalide lors de la désérialisation");
    }
    Message msg;
    memcpy(&msg, buffer, size);
    return msg;
}

/**
 * Retourne une représentation textuelle complète du message.
 * Utilisé pour la lecture détaillée.
 */
std::string Message::toString() const {
    std::ostringstream oss;
    oss << "=== MESSAGE ===\n"
        << "De: " << from << "\n"
        << "À: " << to << "\n"
        << "Sujet: " << subject << "\n";
    
    // Afficher le timestamp si défini
    if (receivedAt != 0) {
        oss << "Reçu le: " << std::put_time(std::localtime(&receivedAt), "%Y-%m-%d %H:%M:%S") << "\n";
    }
    
    oss << "Corps:\n" << body << "\n"
        << "Lu: " << (isRead ? "Oui" : "Non") << "\n"
        << "===============";
    return oss.str();
}

/**
 * Retourne une représentation résumée du message.
 * Utilisé pour l'affichage des listes de messages.
 */
std::string Message::toShortString() const {
    std::ostringstream oss;
    oss << "[" << (isRead ? "Lu" : "Non lu") << "] "
        << "De: " << std::setw(15) << std::left << from
        << " | Sujet: " << subject;
    return oss.str();
}
