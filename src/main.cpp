#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include "../include/blockchain.h"
#include "../include/wallet.h"
#include "../include/memory_storage.h"
#include "../include/utils.h"

namespace fs = std::filesystem;

// Function prototypes
void printHelp();
void createWallet(const std::string& walletName);
void loadWallet(const std::string& walletName);
void viewWalletInfo();
void viewBalance();
void sendCoins(const std::string& recipientAddr, double amount);
void uploadMemory(const std::string& filePath, const std::string& memoryType, const std::string& description);
void mineBlock();
void listMemories();
void viewBlockchain();
void printTransactions();

// Global state
std::unique_ptr<Blockchain> g_blockchain;
std::unique_ptr<Wallet> g_wallet;
std::unique_ptr<MemoryStorage> g_memoryStorage;
bool g_running = true;

int main() {
    std::cout << "===============================================" << std::endl;
    std::cout << "  Ahmiyat Blockchain - Proof of Memories" << std::endl;
    std::cout << "===============================================" << std::endl;

    // Initialize blockchain, wallet, and memory storage
    g_blockchain = std::make_unique<Blockchain>();
    g_memoryStorage = std::make_unique<MemoryStorage>("memories");
    
    // Create wallet directory if it doesn't exist
    if (!fs::exists("wallets")) {
        fs::create_directory("wallets");
    }

    std::string command;
    while (g_running) {
        std::cout << std::endl << "Enter command (type 'help' for command list): ";
        std::cin >> command;
        
        if (command == "help") {
            printHelp();
        }
        else if (command == "create_wallet") {
            std::string walletName;
            std::cout << "Enter wallet name: ";
            std::cin >> walletName;
            createWallet(walletName);
        }
        else if (command == "load_wallet") {
            std::string walletName;
            std::cout << "Enter wallet name: ";
            std::cin >> walletName;
            loadWallet(walletName);
        }
        else if (command == "wallet_info") {
            viewWalletInfo();
        }
        else if (command == "balance") {
            viewBalance();
        }
        else if (command == "send") {
            std::string recipient;
            double amount;
            
            std::cout << "Enter recipient address: ";
            std::cin >> recipient;
            
            std::cout << "Enter amount: ";
            std::cin >> amount;
            
            sendCoins(recipient, amount);
        }
        else if (command == "upload") {
            std::string filePath, typeStr, description;
            
            std::cout << "Enter file path: ";
            std::cin >> filePath;
            
            std::cout << "Enter memory type (IMAGE, VIDEO, MEME, TEXT): ";
            std::cin >> typeStr;
            
            std::cin.ignore();  // Clear the input buffer
            std::cout << "Enter description: ";
            std::getline(std::cin, description);
            
            uploadMemory(filePath, typeStr, description);
        }
        else if (command == "mine") {
            mineBlock();
        }
        else if (command == "memories") {
            listMemories();
        }
        else if (command == "blockchain") {
            viewBlockchain();
        }
        else if (command == "transactions") {
            printTransactions();
        }
        else if (command == "exit") {
            g_running = false;
            std::cout << "Goodbye!" << std::endl;
        }
        else {
            std::cout << "Unknown command. Type 'help' for command list." << std::endl;
        }
    }
    
    return 0;
}

void printHelp() {
    std::cout << "\nAvailable commands:" << std::endl;
    std::cout << "  help - Show this help message" << std::endl;
    std::cout << "  create_wallet <name> - Create a new wallet" << std::endl;
    std::cout << "  load_wallet <name> - Load an existing wallet" << std::endl;
    std::cout << "  wallet_info - View current wallet information" << std::endl;
    std::cout << "  balance - Check your wallet balance" << std::endl;
    std::cout << "  send <recipient> <amount> - Send Ahmiyat coins to another address" << std::endl;
    std::cout << "  upload <file> <type> <description> - Upload a memory to earn coins" << std::endl;
    std::cout << "  mine - Mine a new block and earn rewards" << std::endl;
    std::cout << "  memories - List your uploaded memories" << std::endl;
    std::cout << "  blockchain - View the current blockchain" << std::endl;
    std::cout << "  transactions - View pending transactions" << std::endl;
    std::cout << "  exit - Exit the application" << std::endl;
}

void createWallet(const std::string& walletName) {
    try {
        std::string walletPath = "wallets/" + walletName + ".wallet";
        
        if (fs::exists(walletPath)) {
            std::cout << "Wallet with this name already exists!" << std::endl;
            return;
        }
        
        g_wallet = std::make_unique<Wallet>();
        if (g_wallet->saveToFile(walletPath)) {
            std::cout << "Wallet created successfully!" << std::endl;
            std::cout << "Your address: " << g_wallet->getAddress() << std::endl;
            std::cout << "IMPORTANT: Keep your private key safe!" << std::endl;
        } else {
            std::cout << "Failed to save wallet." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error creating wallet: " << e.what() << std::endl;
    }
}

void loadWallet(const std::string& walletName) {
    try {
        std::string walletPath = "wallets/" + walletName + ".wallet";
        
        if (!fs::exists(walletPath)) {
            std::cout << "Wallet does not exist!" << std::endl;
            return;
        }
        
        g_wallet = std::make_unique<Wallet>(Wallet::loadFromFile(walletPath));
        std::cout << "Wallet loaded successfully!" << std::endl;
        std::cout << "Your address: " << g_wallet->getAddress() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading wallet: " << e.what() << std::endl;
    }
}

void viewWalletInfo() {
    if (!g_wallet) {
        std::cout << "No wallet loaded. Use 'create_wallet' or 'load_wallet' first." << std::endl;
        return;
    }
    
    std::cout << "Wallet Information:" << std::endl;
    std::cout << "  Address: " << g_wallet->getAddress() << std::endl;
    
    // WARNING: Never display private key in a real implementation!
    // This is only for demonstration purposes
    std::cout << "  WARNING: Never share your private key!" << std::endl;
    std::cout << "  Private Key (first 10 chars): " << g_wallet->getPrivateKey().substr(0, 10) << "..." << std::endl;
    
    double balance = g_blockchain->getBalance(g_wallet->getAddress());
    std::cout << "  Balance: " << balance << " Ahmiyat" << std::endl;
    
    size_t memoryCount = g_memoryStorage->getMemoryCount(g_wallet->getAddress());
    std::cout << "  Uploaded Memories: " << memoryCount << std::endl;
}

void viewBalance() {
    if (!g_wallet) {
        std::cout << "No wallet loaded. Use 'create_wallet' or 'load_wallet' first." << std::endl;
        return;
    }
    
    double balance = g_blockchain->getBalance(g_wallet->getAddress());
    std::cout << "Your balance: " << balance << " Ahmiyat" << std::endl;
}

void sendCoins(const std::string& recipientAddr, double amount) {
    if (!g_wallet) {
        std::cout << "No wallet loaded. Use 'create_wallet' or 'load_wallet' first." << std::endl;
        return;
    }
    
    try {
        if (amount <= 0) {
            std::cout << "Amount must be positive." << std::endl;
            return;
        }
        
        double balance = g_blockchain->getBalance(g_wallet->getAddress());
        if (balance < amount) {
            std::cout << "Insufficient balance. You have " << balance << " Ahmiyat." << std::endl;
            return;
        }
        
        Transaction tx = g_wallet->createTransaction(recipientAddr, amount);
        if (g_blockchain->addTransaction(tx)) {
            std::cout << "Transaction created successfully!" << std::endl;
            std::cout << "Transaction will be included in the next mined block." << std::endl;
        } else {
            std::cout << "Failed to create transaction." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error sending coins: " << e.what() << std::endl;
    }
}

void uploadMemory(const std::string& filePath, const std::string& memoryTypeStr, const std::string& description) {
    if (!g_wallet) {
        std::cout << "No wallet loaded. Use 'create_wallet' or 'load_wallet' first." << std::endl;
        return;
    }
    
    try {
        if (!fs::exists(filePath)) {
            std::cout << "File does not exist: " << filePath << std::endl;
            return;
        }
        
        MemoryProof::MemoryType type;
        if (memoryTypeStr == "IMAGE") {
            type = MemoryProof::MemoryType::IMAGE;
        } else if (memoryTypeStr == "VIDEO") {
            type = MemoryProof::MemoryType::VIDEO;
        } else if (memoryTypeStr == "MEME") {
            type = MemoryProof::MemoryType::MEME;
        } else if (memoryTypeStr == "TEXT") {
            type = MemoryProof::MemoryType::TEXT;
        } else {
            std::cout << "Invalid memory type. Use IMAGE, VIDEO, MEME, or TEXT." << std::endl;
            return;
        }
        
        // Create and store the memory proof
        MemoryProof proof = g_memoryStorage->storeMemory(
            filePath, 
            type, 
            g_wallet->getAddress(), 
            description, 
            g_wallet->getPrivateKey()
        );
        
        // Add the memory proof to the blockchain
        if (g_blockchain->storeMemoryProof(proof)) {
            std::cout << "Memory uploaded successfully!" << std::endl;
            std::cout << "You've earned Ahmiyat coins for your contribution." << std::endl;
            std::cout << "Memory hash: " << proof.getFileHash() << std::endl;
        } else {
            std::cout << "Failed to process memory proof." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error uploading memory: " << e.what() << std::endl;
    }
}

void mineBlock() {
    if (!g_wallet) {
        std::cout << "No wallet loaded. Use 'create_wallet' or 'load_wallet' first." << std::endl;
        return;
    }
    
    try {
        std::cout << "Mining block..." << std::endl;
        g_blockchain->minePendingTransactions(g_wallet->getAddress());
        std::cout << "Block mined successfully!" << std::endl;
        std::cout << "Mining reward will be available after the next block is mined." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error mining block: " << e.what() << std::endl;
    }
}

void listMemories() {
    if (!g_wallet) {
        std::cout << "No wallet loaded. Use 'create_wallet' or 'load_wallet' first." << std::endl;
        return;
    }
    
    try {
        std::vector<MemoryProof> memories = g_memoryStorage->getMemoriesByAddress(g_wallet->getAddress());
        
        if (memories.empty()) {
            std::cout << "You haven't uploaded any memories yet." << std::endl;
            return;
        }
        
        std::cout << "Your uploaded memories:" << std::endl;
        for (size_t i = 0; i < memories.size(); ++i) {
            const auto& memory = memories[i];
            std::cout << i + 1 << ". " 
                     << "Type: " << MemoryProof::memoryTypeToString(memory.getType()) << ", "
                     << "Description: " << memory.getDescription() << ", "
                     << "Hash: " << memory.getFileHash().substr(0, 10) << "..."
                     << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error listing memories: " << e.what() << std::endl;
    }
}

void viewBlockchain() {
    size_t chainSize = g_blockchain->getChainSize();
    
    std::cout << "Blockchain Information:" << std::endl;
    std::cout << "  Chain length: " << chainSize << " blocks" << std::endl;
    std::cout << "  Is valid: " << (g_blockchain->isChainValid() ? "Yes" : "No") << std::endl;
    
    char viewDetails;
    std::cout << "View detailed blocks? (y/n): ";
    std::cin >> viewDetails;
    
    if (viewDetails == 'y' || viewDetails == 'Y') {
        std::vector<Block> chain = g_blockchain->getChain();
        
        for (size_t i = 0; i < chain.size(); ++i) {
            const auto& block = chain[i];
            std::cout << "\nBlock #" << block.getIndex() << ":" << std::endl;
            std::cout << "  Hash: " << block.getHash() << std::endl;
            std::cout << "  Previous Hash: " << block.getPreviousHash() << std::endl;
            std::cout << "  Timestamp: " << block.getTimestamp() << std::endl;
            std::cout << "  Nonce: " << block.getNonce() << std::endl;
            
            const auto& transactions = block.getTransactions();
            std::cout << "  Transactions: " << transactions.size() << std::endl;
            
            for (size_t j = 0; j < transactions.size(); ++j) {
                const auto& tx = transactions[j];
                std::cout << "    Tx #" << j + 1 << ": ";
                if (tx.getType() == Transaction::TransactionType::MEMORY_REWARD) {
                    std::cout << "MEMORY_REWARD: " << tx.getAmount() << " Ahmiyat to " 
                              << tx.getToAddress().substr(0, 10) << "..." << std::endl;
                } else if (tx.getFromAddress().empty()) {
                    std::cout << "MINING_REWARD: " << tx.getAmount() << " Ahmiyat to "
                              << tx.getToAddress().substr(0, 10) << "..." << std::endl;
                } else {
                    std::cout << "TRANSFER: " << tx.getAmount() << " Ahmiyat from "
                              << tx.getFromAddress().substr(0, 10) << "... to "
                              << tx.getToAddress().substr(0, 10) << "..." << std::endl;
                }
            }
        }
    }
}

void printTransactions() {
    std::vector<Transaction> pendingTxs = g_blockchain->getPendingTransactions();
    
    if (pendingTxs.empty()) {
        std::cout << "No pending transactions." << std::endl;
        return;
    }
    
    std::cout << "Pending Transactions:" << std::endl;
    
    for (size_t i = 0; i < pendingTxs.size(); ++i) {
        const auto& tx = pendingTxs[i];
        std::cout << i + 1 << ". ";
        
        if (tx.getType() == Transaction::TransactionType::MEMORY_REWARD) {
            std::cout << "MEMORY_REWARD: " << tx.getAmount() << " Ahmiyat to " 
                      << tx.getToAddress().substr(0, 10) << "..." << std::endl;
        } else if (tx.getFromAddress().empty()) {
            std::cout << "MINING_REWARD: " << tx.getAmount() << " Ahmiyat to "
                      << tx.getToAddress().substr(0, 10) << "..." << std::endl;
        } else {
            std::cout << "TRANSFER: " << tx.getAmount() << " Ahmiyat from "
                      << tx.getFromAddress().substr(0, 10) << "... to "
                      << tx.getToAddress().substr(0, 10) << "..." << std::endl;
        }
    }
}
