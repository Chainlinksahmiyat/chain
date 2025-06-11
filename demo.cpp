#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <random>
#include <map>

// Simple hash function for demonstration
std::string simpleHash(const std::string& input) {
    std::hash<std::string> hasher;
    size_t hash = hasher(input);
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

// Forward declarations
class Block;
class Transaction;
class MemoryProof;
class Blockchain;

// Transaction class
class Transaction {
public:
    enum class Type { COIN_TRANSFER, MEMORY_REWARD };
    
    Transaction(const std::string& from, const std::string& to, double amount, Type type = Type::COIN_TRANSFER)
        : fromAddress(from), toAddress(to), amount(amount), type(type), timestamp(std::time(nullptr)) {}
    
    std::string toString() const {
        std::stringstream ss;
        ss << "Transaction: ";
        if (type == Type::MEMORY_REWARD) {
            ss << "MEMORY_REWARD - " << amount << " Ahmiyat to " << toAddress;
        } else {
            ss << fromAddress << " sent " << amount << " Ahmiyat to " << toAddress;
        }
        return ss.str();
    }
    
    std::string fromAddress;
    std::string toAddress;
    double amount;
    Type type;
    time_t timestamp;
};

// Memory Proof class
class MemoryProof {
public:
    enum class Type { IMAGE, VIDEO, MEME, TEXT };
    
    MemoryProof(const std::string& filePath, Type type, const std::string& uploader, const std::string& description)
        : fileHash(simpleHash(filePath)), type(type), uploader(uploader), description(description), timestamp(std::time(nullptr)) {}
    
    std::string toString() const {
        std::stringstream ss;
        std::string typeStr;
        switch (type) {
            case Type::IMAGE: typeStr = "IMAGE"; break;
            case Type::VIDEO: typeStr = "VIDEO"; break;
            case Type::MEME: typeStr = "MEME"; break;
            case Type::TEXT: typeStr = "TEXT"; break;
        }
        
        ss << "Memory: " << typeStr << " - " << description << " (uploaded by " << uploader << ")";
        return ss.str();
    }
    
    std::string fileHash;
    Type type;
    std::string uploader;
    std::string description;
    time_t timestamp;
};

// Block class
class Block {
public:
    Block(uint32_t index, const std::vector<Transaction>& transactions, const std::string& prevHash)
        : index(index), transactions(transactions), previousHash(prevHash), timestamp(std::time(nullptr)), nonce(0) {
        hash = calculateHash();
    }
    
    std::string calculateHash() const {
        std::stringstream ss;
        ss << index << timestamp << previousHash << nonce;
        for (const auto& tx : transactions) {
            ss << tx.fromAddress << tx.toAddress << tx.amount;
        }
        return simpleHash(ss.str());
    }
    
    bool mineBlock(uint32_t difficulty) {
        std::string target(difficulty, '0');
        
        while (hash.substr(0, difficulty) != target) {
            nonce++;
            hash = calculateHash();
        }
        
        std::cout << "Block mined: " << hash << std::endl;
        return true;
    }
    
    std::string toString() const {
        std::stringstream ss;
        ss << "Block #" << index << "\n";
        ss << "  Hash: " << hash << "\n";
        ss << "  Previous Hash: " << previousHash << "\n";
        ss << "  Timestamp: " << timestamp << "\n";
        ss << "  Nonce: " << nonce << "\n";
        ss << "  Transactions: " << transactions.size() << "\n";
        
        for (size_t i = 0; i < transactions.size(); i++) {
            ss << "    " << i+1 << ". " << transactions[i].toString() << "\n";
        }
        
        return ss.str();
    }
    
    uint32_t index;
    std::vector<Transaction> transactions;
    std::string previousHash;
    std::string hash;
    time_t timestamp;
    uint32_t nonce;
};

// Blockchain class
class Blockchain {
public:
    Blockchain() {
        // Create genesis block
        chain.push_back(createGenesisBlock());
        difficulty = 2;
        miningReward = 100.0;
    }
    
    Block createGenesisBlock() {
        return Block(0, std::vector<Transaction>(), "0");
    }
    
    Block getLatestBlock() const {
        return chain.back();
    }
    
    void addBlock(const std::vector<Transaction>& transactions) {
        Block block(chain.size(), transactions, getLatestBlock().hash);
        block.mineBlock(difficulty);
        chain.push_back(block);
    }
    
    bool isValidChain() const {
        for (size_t i = 1; i < chain.size(); i++) {
            const Block& currentBlock = chain[i];
            const Block& previousBlock = chain[i - 1];
            
            if (currentBlock.hash != currentBlock.calculateHash()) {
                return false;
            }
            
            if (currentBlock.previousHash != previousBlock.hash) {
                return false;
            }
        }
        return true;
    }
    
    void minePendingTransactions(const std::string& minerAddress) {
        // Create reward for the miner
        Transaction rewardTx("", minerAddress, miningReward, Transaction::Type::MEMORY_REWARD);
        pendingTransactions.push_back(rewardTx);
        
        Block newBlock(chain.size(), pendingTransactions, getLatestBlock().hash);
        newBlock.mineBlock(difficulty);
        chain.push_back(newBlock);
        
        // Clear pending transactions and add reward for next block
        pendingTransactions.clear();
    }
    
    void addTransaction(const Transaction& transaction) {
        pendingTransactions.push_back(transaction);
    }
    
    void storeMemoryProof(const MemoryProof& proof) {
        memories[proof.uploader].push_back(proof);
        
        // Create reward transaction for the uploader
        double reward = 10.0;
        Transaction rewardTx("", proof.uploader, reward, Transaction::Type::MEMORY_REWARD);
        pendingTransactions.push_back(rewardTx);
    }
    
    double getBalance(const std::string& address) const {
        double balance = 0;
        
        for (const Block& block : chain) {
            for (const Transaction& tx : block.transactions) {
                if (tx.fromAddress == address) {
                    balance -= tx.amount;
                }
                
                if (tx.toAddress == address) {
                    balance += tx.amount;
                }
            }
        }
        
        return balance;
    }
    
    void printChain() const {
        for (const Block& block : chain) {
            std::cout << block.toString() << std::endl;
        }
    }
    
    void printMemories(const std::string& address) const {
        auto it = memories.find(address);
        if (it == memories.end() || it->second.empty()) {
            std::cout << "No memories found for address: " << address << std::endl;
            return;
        }
        
        std::cout << "Memories for " << address << ":" << std::endl;
        for (size_t i = 0; i < it->second.size(); i++) {
            std::cout << "  " << i+1 << ". " << it->second[i].toString() << std::endl;
        }
    }
    
    std::vector<Block> chain;
    std::vector<Transaction> pendingTransactions;
    std::map<std::string, std::vector<MemoryProof>> memories;
    uint32_t difficulty;
    double miningReward;
};

// Simple wallet class
class Wallet {
public:
    Wallet() {
        // Generate a random address for simplicity
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        
        address = "";
        for (int i = 0; i < 40; i++) {
            address += "0123456789abcdef"[dis(gen)];
        }
    }
    
    Transaction createTransaction(const std::string& recipient, double amount) const {
        return Transaction(address, recipient, amount);
    }
    
    std::string address;
};

int main() {
    std::cout << "===============================================" << std::endl;
    std::cout << "  Ahmiyat Blockchain - Proof of Memories" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    // Create blockchain
    Blockchain ahmiyatChain;
    
    // Create user wallets
    Wallet aliceWallet;
    Wallet bobWallet;
    
    std::cout << "\nWallets created:" << std::endl;
    std::cout << "  Alice: " << aliceWallet.address << std::endl;
    std::cout << "  Bob: " << bobWallet.address << std::endl;
    
    // Simulate Alice uploading memories
    std::cout << "\nAlice is uploading memories..." << std::endl;
    MemoryProof memory1("vacation.jpg", MemoryProof::Type::IMAGE, aliceWallet.address, "My vacation in Paris");
    ahmiyatChain.storeMemoryProof(memory1);
    std::cout << "  Uploaded: " << memory1.toString() << std::endl;
    
    MemoryProof memory2("funny_cat.gif", MemoryProof::Type::MEME, aliceWallet.address, "Funny cat meme");
    ahmiyatChain.storeMemoryProof(memory2);
    std::cout << "  Uploaded: " << memory2.toString() << std::endl;
    
    MemoryProof memory3("concert.mp4", MemoryProof::Type::VIDEO, aliceWallet.address, "Rock concert footage");
    ahmiyatChain.storeMemoryProof(memory3);
    std::cout << "  Uploaded: " << memory3.toString() << std::endl;
    
    // Alice mines a block (Proof of Memories)
    std::cout << "\nAlice is mining a block..." << std::endl;
    ahmiyatChain.minePendingTransactions(aliceWallet.address);
    
    // Check balances
    std::cout << "\nCurrent balances:" << std::endl;
    std::cout << "  Alice: " << ahmiyatChain.getBalance(aliceWallet.address) << " Ahmiyat" << std::endl;
    std::cout << "  Bob: " << ahmiyatChain.getBalance(bobWallet.address) << " Ahmiyat" << std::endl;
    
    // Bob mines the next block to give Alice her rewards
    std::cout << "\nBob is mining a block..." << std::endl;
    ahmiyatChain.minePendingTransactions(bobWallet.address);
    
    // Alice sends coins to Bob
    std::cout << "\nAlice sends 15 Ahmiyat to Bob..." << std::endl;
    Transaction tx = aliceWallet.createTransaction(bobWallet.address, 15);
    ahmiyatChain.addTransaction(tx);
    
    // Bob mines another block (includes Alice's transaction)
    std::cout << "\nBob is mining another block..." << std::endl;
    ahmiyatChain.minePendingTransactions(bobWallet.address);
    
    // Final balances
    std::cout << "\nFinal balances:" << std::endl;
    std::cout << "  Alice: " << ahmiyatChain.getBalance(aliceWallet.address) << " Ahmiyat" << std::endl;
    std::cout << "  Bob: " << ahmiyatChain.getBalance(bobWallet.address) << " Ahmiyat" << std::endl;
    
    // List Alice's memories
    std::cout << "\nAlice's uploaded memories:" << std::endl;
    ahmiyatChain.printMemories(aliceWallet.address);
    
    std::cout << "\nBlockchain validation: " << (ahmiyatChain.isValidChain() ? "VALID" : "INVALID") << std::endl;
    
    std::cout << "\nThis demonstration shows how Ahmiyat's 'Proof of Memories' works:" << std::endl;
    std::cout << "1. Users upload memories (images, videos, memes, text)" << std::endl;
    std::cout << "2. Uploaders are rewarded with Ahmiyat coins" << std::endl;
    std::cout << "3. Users with memories can mine blocks to process transactions" << std::endl;
    std::cout << "4. The blockchain maintains a secure record of all activities" << std::endl;
    
    return 0;
}