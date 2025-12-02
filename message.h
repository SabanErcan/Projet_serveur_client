/*
 * message.h
 * 
 * Définition de la structure Message pour la communication client/serveur.
 * Structure à taille fixe (POD) pour faciliter la sérialisation binaire.
 * 
 * Projet R3.05 - Programmation Système
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <cstring>
#include <stdexcept>
#include <ctime>

/* Constantes définissant les tailles maximales des champs */
constexpr size_t MAX_FROM_SIZE = 50;      /* Taille max du nom expéditeur    */
constexpr size_t MAX_TO_SIZE = 50;        /* Taille max du nom destinataire  */
constexpr size_t MAX_SUBJECT_SIZE = 100;  /* Taille max du sujet             */
constexpr size_t MAX_BODY_SIZE = 500;     /* Taille max du corps du message  */

/*
 * Structure Message
 * 
 * Représente un message échangé entre client et serveur.
 * Utilise des tableaux de char de taille fixe pour permettre
 * une sérialisation binaire simple via memcpy.
 * 
 * La taille fixe garantit que sizeof(Message) est constant,
 * ce qui simplifie la transmission réseau.
 */
struct Message {
    char from[MAX_FROM_SIZE];       /* Nom de l'expéditeur                   */
    char to[MAX_TO_SIZE];           /* Destinataire ("all" = broadcast)      */
    char subject[MAX_SUBJECT_SIZE]; /* Sujet du message                      */
    char body[MAX_BODY_SIZE];       /* Corps du message                      */
    bool isRead;                    /* Indicateur de lecture (côté client)   */
    time_t receivedAt;              /* Horodatage de réception               */
    
    /* Constructeur par défaut : initialise tous les champs à zéro */
    Message();
    
    /* Constructeur avec paramètres (valide les tailles) */
    Message(const std::string& fromStr, const std::string& toStr, 
            const std::string& subjectStr, const std::string& bodyStr);
    
    /* Validation de la taille d'un champ */
    static void validateField(const std::string& field, size_t maxSize, const std::string& fieldName);
    
    /* Sérialisation : convertit la structure en tableau d'octets */
    void serialize(char* buffer, size_t& size) const;
    
    /* Désérialisation : reconstruit la structure depuis un tableau d'octets */
    static Message deserialize(const char* buffer, size_t size);
    
    /* Formatage pour affichage complet */
    std::string toString() const;
    
    /* Formatage pour affichage résumé (liste) */
    std::string toShortString() const;
};

#endif /* MESSAGE_H */
