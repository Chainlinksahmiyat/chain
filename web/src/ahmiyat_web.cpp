#include "../include/ahmiyat_web.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace ahmiyat {
namespace web {

using json = nlohmann::json;

AhmiyatWebApp::AhmiyatWebApp(int port) : m_port(port) {
    // Initialize blockchain
    m_blockchain = std::make_shared<Blockchain>();

    // Initialize memory storage
    m_storage = std::make_shared<MemoryStorage>();

    // Initialize HTTP server
    m_server = std::make_unique<SimpleHttpServer>(port);

    // Load existing wallets
    loadWallets();

    // Setup API routes
    setupRoutes();
}

void AhmiyatWebApp::start() {
    std::cout << "Starting Ahmiyat web server on port " << m_port << std::endl;
    m_server->start();
}

void AhmiyatWebApp::stop() {
    std::cout << "Stopping Ahmiyat web server" << std::endl;
    m_server->stop();

    // Save wallets before exit
    saveWallets();
}

void AhmiyatWebApp::setupRoutes() {
    // Home routes
    m_server->addRoute(HttpMethod::GET, "/", std::bind(&AhmiyatWebApp::handleHome, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::GET, "/?", std::bind(&AhmiyatWebApp::handleHome, this, std::placeholders::_1));

    // Authentication routes
    m_server->addRoute(HttpMethod::POST, "/api/login", std::bind(&AhmiyatWebApp::handleLogin, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::POST, "/api/logout", std::bind(&AhmiyatWebApp::handleLogout, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::POST, "/api/register", std::bind(&AhmiyatWebApp::handleRegister, this, std::placeholders::_1));

    // Blockchain interaction routes
    m_server->addRoute(HttpMethod::GET, "/api/balance", std::bind(&AhmiyatWebApp::handleBalance, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::POST, "/api/upload", std::bind(&AhmiyatWebApp::handleUploadMemory, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::GET, "/api/memories", std::bind(&AhmiyatWebApp::handleGetMemories, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::GET, "/api/transactions", std::bind(&AhmiyatWebApp::handleGetTransactions, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::POST, "/api/mine", std::bind(&AhmiyatWebApp::handleMine, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::POST, "/api/transfer", std::bind(&AhmiyatWebApp::handleTransfer, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::GET, "/api/blockchain", std::bind(&AhmiyatWebApp::handleGetBlockchain, this, std::placeholders::_1));

    // Static files handlers - with and without /public prefix
    m_server->addRoute(HttpMethod::GET, "/public/css/styles.css", std::bind(&AhmiyatWebApp::handleStaticFiles, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::GET, "/public/js/main.js", std::bind(&AhmiyatWebApp::handleStaticFiles, this, std::placeholders::_1));

    // Direct access routes without /public prefix
    m_server->addRoute(HttpMethod::GET, "/css/styles.css", std::bind(&AhmiyatWebApp::handleStaticFilesNoPrefixCSS, this, std::placeholders::_1));
    m_server->addRoute(HttpMethod::GET, "/js/main.js", std::bind(&AhmiyatWebApp::handleStaticFilesNoPrefixJS, this, std::placeholders::_1));

    // Generic static file handler for all other static assets
    m_server->addRoute(HttpMethod::GET, "/public", std::bind(&AhmiyatWebApp::handleStaticFiles, this, std::placeholders::_1));
}

HttpResponse AhmiyatWebApp::handleHome(const HttpRequest& req) {
    // Serve the main HTML page
    // Try the build directory first (for packaged app)
    std::string content = readFile("public/index.html");

    // If not found, try the source directory
    if (content.empty()) {
        content = readFile("web/public/index.html");
    }

    // If still not found, return 404
    if (content.empty()) {
        return HttpResponse(404, "text/plain", "Not Found - Index Page Missing");
    }

    return HttpResponse(200, "text/html", content);
}

HttpResponse AhmiyatWebApp::handleLogin(const HttpRequest& req) {
    // Parse JSON from request body
    try {
        json body = json::parse(req.body);

        std::string address = body["address"];
        std::string privateKey = body["privateKey"];

        // Check if the address exists and verify the private key
        {
            std::lock_guard<std::mutex> lock(m_walletsMutex);
            if (m_wallets.find(address) == m_wallets.end()) {
                return HttpResponse(404, "application/json", "{\"error\":\"Wallet not found\"}");
            }

            const Wallet& wallet = m_wallets[address];
            if (wallet.getPrivateKey() != privateKey) {
                return HttpResponse(401, "application/json", "{\"error\":\"Invalid private key\"}");
            }
        }

        // Create a session token
        std::string token = generateSessionToken();

        std::cerr << "Generated token for login: " << token << " for address: " << address << std::endl;

        // Store the session
        {
            std::lock_guard<std::mutex> lock(m_sessionsMutex);
            m_sessions[token] = address;

            // Debug: List all sessions
            std::cerr << "Active sessions after login:" << std::endl;
            for (const auto& session : m_sessions) {
                std::cerr << "  Token: " << session.first << " -> Address: " << session.second << std::endl;
            }
        }

        // Return the token
        json result;
        result["token"] = token;
        result["address"] = address;

        return HttpResponse(200, "application/json", result.dump());
    } catch (const std::exception& e) {
        return HttpResponse(400, "application/json", "{\"error\":\"Invalid JSON: " + std::string(e.what()) + "\"}");
    }
}

HttpResponse AhmiyatWebApp::handleLogout(const HttpRequest& req) {
    std::string token = req.getHeader("Authorization");
    if (!token.empty()) {
        // Remove the session
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        m_sessions.erase(token);
    }

    return HttpResponse(200, "application/json", "{\"success\":true}");
}

HttpResponse AhmiyatWebApp::handleRegister(const HttpRequest& req) {
    // Create a new wallet
    Wallet wallet;
    std::string address = wallet.getAddress();
    std::string privateKey = wallet.getPrivateKey();

    // Store the wallet
    {
        std::lock_guard<std::mutex> lock(m_walletsMutex);
        m_wallets[address] = wallet;
    }

    // Save wallets to disk
    saveWallets();

    // Return the wallet details
    json result;
    result["address"] = address;
    result["privateKey"] = privateKey;

    return HttpResponse(200, "application/json", result.dump());
}

HttpResponse AhmiyatWebApp::handleBalance(const HttpRequest& req) {
    std::string address = getAuthenticatedAddress(req);
    if (address.empty()) {
        return HttpResponse(401, "application/json", "{\"error\":\"Unauthorized\"}");
    }

    // Get the wallet balance
    double balance = m_blockchain->getBalance(address);

    // Return the balance
    json result;
    result["balance"] = balance;
    result["address"] = address;

    return HttpResponse(200, "application/json", result.dump());
}

HttpResponse AhmiyatWebApp::handleUploadMemory(const HttpRequest& req) {
    std::cerr << "=== MEMORY UPLOAD REQUEST START ===" << std::endl;
    std::cerr << "Content-Type: " << req.getHeader("Content-Type") << std::endl;
    std::cerr << "Body length: " << req.body.length() << std::endl;

    std::string address = getAuthenticatedAddress(req);
    if (address.empty()) {
        std::cerr << "Authentication failed - no address found" << std::endl;
        return HttpResponse(401, "application/json", "{\"error\":\"Unauthorized\"}");
    }

    std::cerr << "Authenticated address: " << address << std::endl;

    // Parse JSON from request body
    try {
        std::cerr << "Attempting to parse request body as JSON" << std::endl;
        // Log request details
        std::cerr << "Request body length: " << req.body.length() << std::endl;
        std::cerr << "Content-Type: " << req.getHeader("Content-Type") << std::endl;

        if (req.body.empty()) {
            return HttpResponse(400, "application/json", "{\"error\":\"Empty request body\"}");
        }

        // Log the first 100 chars of request body to debug
        std::cerr << "Request body preview: " << req.body.substr(0, 100) << "..." << std::endl;

        json body = json::parse(req.body);

        // Validate required fields
        if (!body.contains("type") || !body.contains("description") || 
            !body.contains("fileData") || !body.contains("fileName")) {
            return HttpResponse(400, "application/json", 
                "{\"error\":\"Missing required fields: type, description, fileData, fileName\"}");
        }

        std::string typeStr = body["type"];
        std::string description = body["description"];

        std::cerr << "Memory type: " << typeStr << std::endl;
        std::cerr << "Description: " << description << std::endl;

        MemoryProof::MemoryType type;
        if (typeStr == "image") {
            type = MemoryProof::MemoryType::IMAGE;
        } else if (typeStr == "video") {
            type = MemoryProof::MemoryType::VIDEO;
        } else if (typeStr == "meme") {
            type = MemoryProof::MemoryType::MEME;
        } else {
            type = MemoryProof::MemoryType::TEXT;
        }

        // Check if the request has file data
        std::string fileData;
        std::string fileName;
        std::string fileType;

        // Improved file data extraction with detailed error handling
        if (body.contains("fileData") && body.contains("fileName")) {
            std::cerr << "Found fileData and fileName in request" << std::endl;

            try {
                // Extract fileData with type checking
                if (body["fileData"].is_string()) {
                    fileData = body["fileData"].get<std::string>();
                    std::cerr << "Successfully extracted fileData as string" << std::endl;
                } else {
                    std::cerr << "Error: fileData is not a string type: " << body["fileData"].type_name() << std::endl;
                    return HttpResponse(400, "application/json", "{\"error\":\"fileData must be a base64 encoded string\"}");
                }

                // Extract fileName with type checking
                if (body["fileName"].is_string()) {
                    fileName = body["fileName"].get<std::string>();
                    std::cerr << "Successfully extracted fileName: " << fileName << std::endl;
                } else {
                    std::cerr << "Error: fileName is not a string type" << std::endl;
                    return HttpResponse(400, "application/json", "{\"error\":\"fileName must be a string\"}");
                }

                // Extract optional fileType with type checking
                if (body.contains("fileType")) {
                    if (body["fileType"].is_string()) {
                        fileType = body["fileType"].get<std::string>();
                        std::cerr << "File type: " << fileType << std::endl;
                    } else {
                        std::cerr << "Warning: fileType is not a string, ignoring" << std::endl;
                    }
                }

                // Validate fileData
                if (fileData.empty()) {
                    std::cerr << "Error: fileData is empty" << std::endl;
                    return HttpResponse(400, "application/json", "{\"error\":\"fileData cannot be empty\"}");
                }

                // Log that we received file data
                std::cerr << "Received file: " << fileName << " (size: " << fileData.size() << " bytes)" << std::endl;

            } catch (const std::exception& e) {
                // Handle any JSON exceptions during extraction
                std::cerr << "Exception while processing file data: " << e.what() << std::endl;
                return HttpResponse(400, "application/json", "{\"error\":\"Invalid file data format: " + std::string(e.what()) + "\"}");
            }
        } else {
            std::cerr << "Warning: No file data or file name found in request" << std::endl;
            std::cerr << "Body keys: ";
            for (auto& item : body.items()) {
                std::cerr << item.key() << " ";
            }
            std::cerr << std::endl;
        }

        // Generate a timestamp for the filename
        std::string timestamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());

        // Create a unique filename including the original extension if available
        std::string extension = ".bin";
        size_t dotPos = fileName.find_last_of(".");
        if (dotPos != std::string::npos) {
            extension = fileName.substr(dotPos);
        }

        std::string filename = "memories/" + typeStr + "/" + address + "_" + timestamp + extension;

        // Ensure directory exists
        std::filesystem::create_directories("memories/" + typeStr);

        // Decode base64 file data and write to file if present, otherwise write placeholder
        std::ofstream file(filename, std::ios::binary);

        if (!fileData.empty()) {
            // Check if fileData starts with a data URL prefix
            std::string base64Data = fileData;
            size_t commaPos = fileData.find(",");
            if (commaPos != std::string::npos) {
                std::cerr << "Found data URL format, extracting actual base64 data" << std::endl;
                base64Data = fileData.substr(commaPos + 1);
            }

            std::cerr << "Base64 data length: " << base64Data.length() << std::endl;

            // Use the existing base64Decode function in utils
            try {
                std::cerr << "Attempting to decode base64 data..." << std::endl;
                std::vector<uint8_t> decodedData = utils::base64Decode(base64Data);
                std::cerr << "Base64 decoded successfully. Decoded size: " << decodedData.size() << " bytes" << std::endl;

                if (decodedData.empty()) {
                    throw std::runtime_error("Decoded data is empty");
                }

                file.write(reinterpret_cast<const char*>(decodedData.data()), decodedData.size());

                if (!file.good()) {
                    std::cerr << "Error writing to file: " << filename << std::endl;
                    return HttpResponse(500, "application/json", "{\"error\":\"Failed to write file to disk\"}");
                }

                std::cerr << "File successfully written to: " << filename << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to decode base64 data: " << e.what() << std::endl;

                // Return error to client
                return HttpResponse(400, "application/json", "{\"error\":\"Failed to decode file data: " + std::string(e.what()) + "\"}");
            }
        } else {
            // Write placeholder content
            std::cerr << "No file data provided, writing placeholder content" << std::endl;
            file << "Memory uploaded by " << address << " at " << utils::getCurrentTimestamp() << "\n";
            file << "Description: " << description << "\n";
            file << "File name: " << fileName << "\n";
        }

        file.close();

        // Create a memory proof
        MemoryProof proof(filename, type, address, description);

        // Get the wallet for signing
        std::string privateKey;
        {
            std::lock_guard<std::mutex> lock(m_walletsMutex);
            if (m_wallets.find(address) == m_wallets.end()) {
                return HttpResponse(404, "application/json", "{\"error\":\"Wallet not found\"}");
            }
            privateKey = m_wallets[address].getPrivateKey();
        }

        // Sign the memory proof
        proof.signMemory(privateKey);

        // Store the memory proof and add a reward transaction
        bool success = m_blockchain->storeMemoryProof(proof);
        if (!success) {
            return HttpResponse(500, "application/json", "{\"error\":\"Failed to store memory proof\"}");
        }

        // Add to memory storage
        if (!m_storage->storeMemory(address, proof)) {
            std::cerr << "Warning: Memory proof was stored in blockchain but failed in storage index" << std::endl;
        }

        // Return the proof details
        json result;
        result["success"] = true;
        result["proofHash"] = proof.getProofHash();
        result["timestamp"] = utils::timeToString(proof.getTimestamp());

        return HttpResponse(200, "application/json", result.dump());
    } catch (const std::exception& e) {
        return HttpResponse(400, "application/json", "{\"error\":\"Invalid request: " + std::string(e.what()) + "\"}");
    }
}

HttpResponse AhmiyatWebApp::handleGetMemories(const HttpRequest& req) {
    std::string address = getAuthenticatedAddress(req);
    if (address.empty()) {
        return HttpResponse(401, "application/json", "{\"error\":\"Unauthorized\"}");
    }

    // Get all memories for the user
    auto memories = m_storage->getMemoriesByAddress(address);

    // Convert to JSON
    json result;
    json memoryList = json::array();

    for (const auto& proof : memories) {
        json memory;
        memory["proofHash"] = proof.getProofHash();
        memory["fileHash"] = proof.getFileHash();
        memory["type"] = MemoryProof::memoryTypeToString(proof.getType());
        memory["description"] = proof.getDescription();
        memory["timestamp"] = utils::timeToString(proof.getTimestamp());
        memory["signature"] = proof.getSignature();

        memoryList.push_back(memory);
    }

    result["memories"] = memoryList;

    return HttpResponse(200, "application/json", result.dump());
}

HttpResponse AhmiyatWebApp::handleGetTransactions(const HttpRequest& req) {
    std::string address = getAuthenticatedAddress(req);
    if (address.empty()) {
        return HttpResponse(401, "application/json", "{\"error\":\"Unauthorized\"}");
    }

    // Get all blocks in the chain
    auto chain = m_blockchain->getChain();

    // Extract transactions for the user
    json result;
    json txList = json::array();

    for (const auto& block : chain) {
        for (const auto& tx : block.getTransactions()) {
            if (tx.getFromAddress() == address || tx.getToAddress() == address) {
                json txJson;
                txJson["fromAddress"] = tx.getFromAddress();
                txJson["toAddress"] = tx.getToAddress();
                txJson["amount"] = tx.getAmount();
                txJson["timestamp"] = utils::timeToString(tx.getTimestamp());
                txJson["type"] = static_cast<int>(tx.getType());

                txList.push_back(txJson);
            }
        }
    }

    result["transactions"] = txList;

    return HttpResponse(200, "application/json", result.dump());
}

HttpResponse AhmiyatWebApp::handleMine(const HttpRequest& req) {
    std::string address = getAuthenticatedAddress(req);
    if (address.empty()) {
        return HttpResponse(401, "application/json", "{\"error\":\"Unauthorized\"}");
    }

    // Mine pending transactions
    m_blockchain->minePendingTransactions(address);

    // Get the new balance
    double balance = m_blockchain->getBalance(address);

    // Return the result
    json result;
    result["success"] = true;
    result["message"] = "Mining successful";
    result["balance"] = balance;

    return HttpResponse(200, "application/json", result.dump());
}

HttpResponse AhmiyatWebApp::handleTransfer(const HttpRequest& req) {
    std::string address = getAuthenticatedAddress(req);
    if (address.empty()) {
        return HttpResponse(401, "application/json", "{\"error\":\"Unauthorized\"}");
    }

    // Parse JSON from request body
    try {
        json body = json::parse(req.body);

        if (!body.contains("toAddress") || !body.contains("amount")) {
            return HttpResponse(400, "application/json", "{\"error\":\"Missing required fields\"}");
        }

        std::string toAddress = body["toAddress"];
        double amount = body["amount"];

        if (amount <= 0) {
            return HttpResponse(400, "application/json", "{\"error\":\"Amount must be positive\"}");
        }

        // Get the wallet for signing
        std::string privateKey;
        {
            std::lock_guard<std::mutex> lock(m_walletsMutex);
            if (m_wallets.find(address) == m_wallets.end()) {
                return HttpResponse(404, "application/json", "{\"error\":\"Wallet not found\"}");
            }
            privateKey = m_wallets[address].getPrivateKey();
        }

        // Create and process the transaction
        Transaction tx(address, toAddress, amount);
        tx.signTransaction(privateKey);

        bool success = m_blockchain->processTransaction(tx);
        if (!success) {
            return HttpResponse(400, "application/json", "{\"error\":\"Failed to process transaction\"}");
        }

        // Get the new balance
        double balance = m_blockchain->getBalance(address);

        // Return the result
        json result;
        result["success"] = true;
        result["message"] = "Transaction processed successfully";
        result["balance"] = balance;

        return HttpResponse(200, "application/json", result.dump());
    } catch (const std::exception& e) {
        return HttpResponse(400, "application/json", "{\"error\":\"Invalid request: " + std::string(e.what()) + "\"}");
    }
}

HttpResponse AhmiyatWebApp::handleGetBlockchain(const HttpRequest& req) {
    // This could be a heavy operation, so we might want to limit it or cache it
    std::string json = m_blockchain->getChainAsJson();

    return HttpResponse(200, "application/json", json);
}

HttpResponse AhmiyatWebApp::handleStaticFiles(const HttpRequest& req) {
    std::string path = extractStaticFilePath(req.uri);

    // First try the build directory
    std::string content = readFile(path.substr(1)); // Remove leading slash

    // If not found, try the source directory
    if (content.empty()) {
        std::string fullPath = "web" + path;
        content = readFile(fullPath);
    }

    // If still not found, return 404
    if (content.empty()) {
        std::cerr << "Static file not found: " << path << std::endl;
        return HttpResponse(404, "text/plain", "Not Found: " + path);
    }

    std::string contentType = getContentType(path);
    return HttpResponse(200, contentType, content);
}

std::string AhmiyatWebApp::generateSessionToken() {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    std::string token;
    for (int i = 0; i < 32; ++i) {
        token += chars[distribution(generator)];
    }

    return token;
}

bool AhmiyatWebApp::isAuthenticated(const HttpRequest& req, std::string& address) {
    std::string token = req.getHeader("Authorization");

    // Trim any whitespace from the token
    token.erase(0, token.find_first_not_of(" \n\r\t"));
    token.erase(token.find_last_not_of(" \n\r\t") + 1);

    if (token.empty()) {
        std::cerr << "Authentication failed: Empty token" << std::endl;
        return false;
    }

    std::cerr << "Checking authentication for token: '" << token << "'" << std::endl;

    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    auto it = m_sessions.find(token);
    if (it == m_sessions.end()) {
        std::cerr << "Authentication failed: Token not found in sessions" << std::endl;
        return false;
    }

    address = it->second;
    std::cerr << "Authentication successful for address: " << address << std::endl;
    return true;
}

void AhmiyatWebApp::saveWallets() {
    std::lock_guard<std::mutex> lock(m_walletsMutex);

    std::ofstream file("wallets.dat");
    if (!file.is_open()) {
        std::cerr << "Failed to save wallets" << std::endl;
        return;
    }

    for (const auto& pair : m_wallets) {
        file << pair.first << ":" << pair.second.getPrivateKey() << "\n";
    }
}

void AhmiyatWebApp::loadWallets() {
    std::ifstream file("wallets.dat");
    if (!file.is_open()) {
        std::cerr << "No wallet file found, starting fresh" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(m_walletsMutex);

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }

        std::string address = line.substr(0, pos);
        std::string privateKey = line.substr(pos + 1);

        // Create wallet directly from private key
        // The public key and address will be derived in the constructor
        Wallet wallet(privateKey);

        m_wallets[address] = wallet;
    }
}

std::string AhmiyatWebApp::getAuthenticatedAddress(const HttpRequest& req) {
    std::string address;
    std::string authHeader = req.getHeader("Authorization");

    std::cerr << "Auth header: '" << authHeader << "'" << std::endl;

    if (isAuthenticated(req, address)) {
        std::cerr << "Authentication successful for address: " << address << std::endl;
        return address;
    }

    std::cerr << "Authentication failed!" << std::endl;
    if (authHeader.empty()) {
        std::cerr << "Reason: Missing Authorization header" << std::endl;
    } else {
        std::cerr << "Reason: Invalid session token" << std::endl;

        // Debug: List active sessions
        std::cerr << "Active sessions:" << std::endl;
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        for (const auto& session : m_sessions) {
            std::cerr << "  Token: " << session.first << " -> Address: " << session.second << std::endl;
        }
    }
    return "";
}

std::string AhmiyatWebApp::extractStaticFilePath(const std::string& uri) {
    // Handle paths like /public/css/styles.css
    if (uri.find("/public/") == 0) {
        return uri;
    }

    return "/public/index.html"; // Default to index.html
}

std::string AhmiyatWebApp::getContentType(const std::string& path) {
    std::string ext;
    size_t pos = path.find_last_of('.');
    if (pos != std::string::npos) {
        ext = path.substr(pos + 1);
    }

    if (ext == "html") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";

    return "text/plain";
}

std::string AhmiyatWebApp::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string AhmiyatWebApp::parseJson(const std::string& body, const std::string& key) {
    try {
        json j = json::parse(body);
        return j[key];
    } catch (...) {
        return "";
    }
}

HttpResponse AhmiyatWebApp::handleStaticFilesNoPrefixCSS(const HttpRequest& req) {
    // First try the build directory
    std::string content = readFile("public/css/styles.css");

    // If not found, try the source directory
    if (content.empty()) {
        content = readFile("web/public/css/styles.css");
    }

    // If still not found, return 404
    if (content.empty()) {
        std::cerr << "CSS file not found" << std::endl;
        return HttpResponse(404, "text/plain", "Not Found: CSS file");
    }

    return HttpResponse(200, "text/css", content);
}

HttpResponse AhmiyatWebApp::handleStaticFilesNoPrefixJS(const HttpRequest& req) {
    // First try the build directory
    std::string content = readFile("public/js/main.js");

    // If not found, try the source directory
    if (content.empty()) {
        content = readFile("web/public/js/main.js");
    }

    // If still not found, return 404
    if (content.empty()) {
        std::cerr << "JS file not found" << std::endl;
        return HttpResponse(404, "text/plain", "Not Found: JS file");
    }

    return HttpResponse(200, "application/javascript", content);
}

} // namespace web
} // namespace ahmiyat