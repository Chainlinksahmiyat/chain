#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

#include "simple_http_server.h"
#include "../../include/blockchain.h"
#include "../../include/wallet.h"
#include "../../include/memory_proof.h"
#include "../../include/memory_storage.h"
#include "../../include/utils.h"

namespace ahmiyat {
namespace web {

class AhmiyatWebApp {
public:
    AhmiyatWebApp(int port = 5000);
    
    void start();
    void stop();
    
private:
    // Web server port
    int m_port;
    
    // HTTP server instance
    std::unique_ptr<SimpleHttpServer> m_server;
    
    // Blockchain instance
    std::shared_ptr<Blockchain> m_blockchain;
    
    // Memory storage instance
    std::shared_ptr<MemoryStorage> m_storage;
    
    // User wallets (address -> wallet)
    std::unordered_map<std::string, Wallet> m_wallets;
    std::mutex m_walletsMutex;
    
    // Active user sessions
    std::unordered_map<std::string, std::string> m_sessions; // token -> address
    std::mutex m_sessionsMutex;
    
    // Setup API routes
    void setupRoutes();
    
    // API route handlers
    HttpResponse handleHome(const HttpRequest& req);
    HttpResponse handleLogin(const HttpRequest& req);
    HttpResponse handleLogout(const HttpRequest& req);
    HttpResponse handleRegister(const HttpRequest& req);
    HttpResponse handleBalance(const HttpRequest& req);
    HttpResponse handleUploadMemory(const HttpRequest& req);
    HttpResponse handleGetMemories(const HttpRequest& req);
    HttpResponse handleGetTransactions(const HttpRequest& req);
    HttpResponse handleMine(const HttpRequest& req);
    HttpResponse handleTransfer(const HttpRequest& req);
    HttpResponse handleGetBlockchain(const HttpRequest& req);
    HttpResponse handleStaticFiles(const HttpRequest& req);
    HttpResponse handleStaticFilesNoPrefixCSS(const HttpRequest& req);
    HttpResponse handleStaticFilesNoPrefixJS(const HttpRequest& req);
    
    // Helper methods
    std::string generateSessionToken();
    bool isAuthenticated(const HttpRequest& req, std::string& address);
    void saveWallets();
    void loadWallets();
    std::string getAuthenticatedAddress(const HttpRequest& req);
    std::string extractStaticFilePath(const std::string& uri);
    std::string getContentType(const std::string& path);
    std::string readFile(const std::string& path);
    std::string parseJson(const std::string& body, const std::string& key);
};

} // namespace web
} // namespace ahmiyat