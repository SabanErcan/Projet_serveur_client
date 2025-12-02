/**
 * EXEMPLES DE CODE COMMENTÉS
 * Guide pédagogique pour comprendre l'architecture du projet
 */

// ============================================================================
// 1. STRUCTURE MESSAGE - Validation et Sérialisation
// ============================================================================

/*
 * Exemple de création d'un message avec validation
 */
void exemple_creation_message() {
    try {
        // Message valide
        Message msg("Alice", "Bob", "Réunion", "RDV demain à 10h");
        // ✅ Tous les champs respectent les limites
        
        // Message invalide - sujet trop long
        Message msg2("Alice", "Bob", 
                     "Ce sujet est beaucoup trop long et dépasse largement la limite autorisée de cent caractères exactement!",
                     "Corps");
        // ❌ Exception levée : Subject dépasse la limite
        
    } catch (const std::invalid_argument& e) {
        // Gestion de l'erreur de validation
        std::cout << "Erreur: " << e.what() << std::endl;
    }
}

/*
 * Exemple de sérialisation/désérialisation
 */
void exemple_serialisation() {
    // Créer un message
    Message msg("Alice", "Bob", "Test", "Message de test");
    
    // Sérialiser pour envoi sur socket
    char buffer[sizeof(Message)];
    size_t size;
    msg.serialize(buffer, size);
    // Le message est maintenant dans buffer, prêt à être envoyé
    
    // Côté réception - désérialiser
    Message receivedMsg = Message::deserialize(buffer, size);
    // Le message est reconstruit avec toutes ses données
    
    std::cout << receivedMsg.toString() << std::endl;
}

// ============================================================================
// 2. SYNCHRONISATION AVEC MUTEX
// ============================================================================

/*
 * Exemple d'utilisation de mutex pour protéger une liste partagée
 */
std::vector<Message> g_messages;
std::mutex g_messagesMutex;

void thread_ajouter_message(const Message& msg) {
    // MAUVAIS : Accès sans protection
    // g_messages.push_back(msg); // ❌ Race condition possible!
    
    // BON : Utilisation de lock_guard (RAII)
    {
        std::lock_guard<std::mutex> lock(g_messagesMutex);
        g_messages.push_back(msg);
        // Le mutex est automatiquement libéré à la fin du bloc
    } // ✅ Libération automatique
}

void thread_lire_messages() {
    std::lock_guard<std::mutex> lock(g_messagesMutex);
    
    for (const auto& msg : g_messages) {
        std::cout << msg.toShortString() << std::endl;
    }
    // Mutex libéré automatiquement ici
}

/*
 * Exemple avec plusieurs mutex - attention au deadlock!
 */
std::mutex g_mutex1;
std::mutex g_mutex2;

void fonction_securisee() {
    // BON : Toujours verrouiller dans le même ordre
    std::lock_guard<std::mutex> lock1(g_mutex1);
    std::lock_guard<std::mutex> lock2(g_mutex2);
    
    // Opérations protégées
}

// ============================================================================
// 3. THREADS - Création et Gestion
// ============================================================================

/*
 * Exemple de création de thread avec fonction
 */
void fonction_thread(int id, const std::string& nom) {
    std::cout << "Thread " << id << " : " << nom << std::endl;
}

void exemple_creation_threads() {
    // Créer un thread
    std::thread t1(fonction_thread, 1, "Alice");
    
    // Créer plusieurs threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(fonction_thread, i, "User" + std::to_string(i));
    }
    
    // Attendre la fin du premier thread
    if (t1.joinable()) {
        t1.join(); // Bloque jusqu'à ce que t1 se termine
    }
    
    // Attendre tous les threads du vecteur
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

/*
 * Exemple de thread avec lambda
 */
void exemple_thread_lambda() {
    int compteur = 0;
    std::mutex mtx;
    
    auto fonction = [&compteur, &mtx]() {
        for (int i = 0; i < 1000; ++i) {
            std::lock_guard<std::mutex> lock(mtx);
            compteur++;
        }
    };
    
    std::thread t1(fonction);
    std::thread t2(fonction);
    
    t1.join();
    t2.join();
    
    std::cout << "Compteur final: " << compteur << std::endl; // 2000
}

// ============================================================================
// 4. SOCKETS - Communication Réseau
// ============================================================================

/*
 * Exemple de serveur TCP simple
 */
void exemple_serveur_tcp() {
    // 1. Créer le socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    // 2. Configurer l'adresse
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Toutes les interfaces
    serverAddr.sin_port = htons(8888); // Port 8888
    
    // 3. Bind (associer le socket à l'adresse)
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    // 4. Listen (mettre en mode écoute)
    listen(serverSocket, 5); // Backlog de 5 connexions
    
    // 5. Accept (accepter une connexion)
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    
    // 6. Communiquer
    char buffer[256];
    ssize_t received = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[received] = '\0';
    
    const char* response = "Message reçu!";
    send(clientSocket, response, strlen(response), 0);
    
    // 7. Fermer
    closesocket(clientSocket);
    closesocket(serverSocket);
}

/*
 * Exemple de client TCP simple
 */
void exemple_client_tcp() {
    // 1. Créer le socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    // 2. Configurer l'adresse du serveur
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // 3. Connect (se connecter au serveur)
    connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    // 4. Envoyer des données
    const char* message = "Bonjour serveur!";
    send(clientSocket, message, strlen(message), 0);
    
    // 5. Recevoir la réponse
    char buffer[256];
    ssize_t received = recv(clientSocket, buffer, sizeof(buffer), 0);
    buffer[received] = '\0';
    std::cout << "Réponse: " << buffer << std::endl;
    
    // 6. Fermer
    closesocket(clientSocket);
}

// ============================================================================
// 5. ATOMIC - Variables Atomiques
// ============================================================================

/*
 * Exemple d'utilisation de std::atomic pour flags
 */
std::atomic<bool> g_running(true);
std::atomic<int> g_compteur(0);

void thread_worker() {
    while (g_running) {
        g_compteur++; // Opération atomique, pas besoin de mutex
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void exemple_atomic() {
    std::thread t1(thread_worker);
    std::thread t2(thread_worker);
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    g_running = false; // Arrêter les threads
    
    t1.join();
    t2.join();
    
    std::cout << "Compteur: " << g_compteur << std::endl;
}

// ============================================================================
// 6. QUEUE - File FIFO pour Messages
// ============================================================================

/*
 * Exemple d'utilisation de std::queue pour file de messages
 */
std::queue<Message> g_messageQueue;
std::mutex g_queueMutex;

void producteur_messages() {
    for (int i = 0; i < 10; ++i) {
        Message msg("Producer", "Consumer", 
                   "Sujet " + std::to_string(i),
                   "Corps du message " + std::to_string(i));
        
        {
            std::lock_guard<std::mutex> lock(g_queueMutex);
            g_messageQueue.push(msg);
            std::cout << "Message ajouté à la queue" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void consommateur_messages() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::lock_guard<std::mutex> lock(g_queueMutex);
        
        if (g_messageQueue.empty()) {
            std::cout << "Queue vide" << std::endl;
            continue;
        }
        
        while (!g_messageQueue.empty()) {
            Message msg = g_messageQueue.front();
            g_messageQueue.pop();
            std::cout << "Message traité: " << msg.subject << std::endl;
        }
    }
}

// ============================================================================
// 7. GESTION D'ERREURS - Try/Catch
// ============================================================================

/*
 * Exemple de gestion d'erreurs complète
 */
void fonction_avec_gestion_erreurs() {
    try {
        // Opération qui peut échouer
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            throw std::runtime_error("Échec de création du socket");
        }
        
        // Autre opération risquée
        Message msg("", "Bob", "Test", "Corps");
        // Exception si From est vide
        
    } catch (const std::invalid_argument& e) {
        // Erreur de validation
        std::cout << "Erreur de validation: " << e.what() << std::endl;
        
    } catch (const std::runtime_error& e) {
        // Erreur d'exécution
        std::cout << "Erreur d'exécution: " << e.what() << std::endl;
        
    } catch (const std::exception& e) {
        // Toutes les autres exceptions standard
        std::cout << "Erreur: " << e.what() << std::endl;
        
    } catch (...) {
        // Exceptions inconnues
        std::cout << "Erreur inconnue" << std::endl;
    }
}

// ============================================================================
// 8. PATTERN SERVEUR MULTI-CLIENTS
// ============================================================================

/*
 * Pattern pour gérer plusieurs clients avec un thread par client
 */

std::vector<std::thread*> g_clientThreads;
std::mutex g_threadsMutex;

void handler_client(SOCKET clientSocket, int clientId) {
    std::cout << "Client " << clientId << " connecté" << std::endl;
    
    char buffer[256];
    while (true) {
        ssize_t received = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            break; // Client déconnecté
        }
        
        buffer[received] = '\0';
        std::cout << "Client " << clientId << ": " << buffer << std::endl;
    }
    
    closesocket(clientSocket);
    std::cout << "Client " << clientId << " déconnecté" << std::endl;
}

void serveur_multi_clients() {
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // ... configuration et bind ...
    listen(serverSocket, 10);
    
    int clientId = 0;
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
        
        // Créer un thread pour ce client
        std::thread* clientThread = new std::thread(handler_client, clientSocket, clientId++);
        
        {
            std::lock_guard<std::mutex> lock(g_threadsMutex);
            g_clientThreads.push_back(clientThread);
        }
    }
    
    // Attendre tous les threads
    for (auto* t : g_clientThreads) {
        if (t->joinable()) {
            t->join();
        }
        delete t;
    }
}

// ============================================================================
// 9. LOGGING THREAD-SAFE
// ============================================================================

/*
 * Pattern pour un système de log thread-safe
 */
std::ofstream g_logFile;
std::mutex g_logMutex;

void writeLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    
    // Timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    // Écrire dans le fichier
    g_logFile << "[" << std::ctime(&time_t_now) << "] " << message << std::endl;
    g_logFile.flush(); // Force l'écriture immédiate
    
    // Afficher aussi dans la console
    std::cout << message << std::endl;
}

// ============================================================================
// 10. SELECT - I/O Non-Bloquant
// ============================================================================

/*
 * Exemple d'utilisation de select() pour vérifier si des données sont disponibles
 */
bool hasDataAvailable(SOCKET sock, int timeoutMs) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    
    struct timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    
    int result = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
    
    return result > 0 && FD_ISSET(sock, &readSet);
}

void exemple_select() {
    SOCKET sock; // ... socket déjà créé et connecté
    
    // Vérifier sans bloquer
    if (hasDataAvailable(sock, 0)) {
        char buffer[256];
        recv(sock, buffer, sizeof(buffer), 0);
        // Traiter les données
    } else {
        // Pas de données, faire autre chose
    }
}

/**
 * RÉSUMÉ DES BONNES PRATIQUES
 * 
 * 1. THREADS
 *    ✅ Toujours faire join() ou detach()
 *    ✅ Protéger les données partagées avec mutex
 *    ✅ Utiliser lock_guard pour RAII
 * 
 * 2. MUTEX
 *    ✅ Verrouiller dans le même ordre partout
 *    ✅ Minimiser la durée de verrouillage
 *    ✅ Utiliser des blocs {} pour limiter la portée
 * 
 * 3. SOCKETS
 *    ✅ Toujours vérifier les valeurs de retour
 *    ✅ Fermer les sockets avec closesocket()
 *    ✅ Initialiser Winsock sur Windows
 * 
 * 4. EXCEPTIONS
 *    ✅ Utiliser des types spécifiques (invalid_argument, runtime_error)
 *    ✅ Catcher par référence const
 *    ✅ Nettoyer les ressources dans catch
 * 
 * 5. MÉMOIRE
 *    ✅ Utiliser smart pointers (unique_ptr, shared_ptr) quand possible
 *    ✅ Ou gérer manuellement avec new/delete appariés
 *    ✅ Préférer les conteneurs STL aux tableaux C
 */
