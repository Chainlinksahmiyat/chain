#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <iostream>
#include <map>

#include "block.h"
#include "transaction.h"
#include "memory_proof.h"
#include "wallet.h"

namespace ahmiyat {

// Forward declarations for PostgreSQL opaque types
typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

class DatabaseAdapter {
public:
    // Singleton instance
    static DatabaseAdapter& getInstance();
    
    // Delete copy and move constructors and assign operators
    DatabaseAdapter(DatabaseAdapter const&) = delete;
    DatabaseAdapter(DatabaseAdapter&&) = delete;
    DatabaseAdapter& operator=(DatabaseAdapter const&) = delete;
    DatabaseAdapter& operator=(DatabaseAdapter&&) = delete;
    
    // Destructor
    ~DatabaseAdapter();
    
    // Connection management
    bool connect(const std::string& connString);
    bool isConnected() const;
    void disconnect();
    
    // Block operations
    bool saveBlock(const Block& block);
    bool getBlock(const std::string& hash, Block& block);
    bool getBlockByHeight(int height, Block& block);
    std::vector<Block> getBlocks(int limit = 10, int offset = 0);
    int getBlockchainHeight();
    
    // Transaction operations
    bool saveTransaction(const Transaction& tx, const std::string& blockHash = "");
    bool getTransaction(const std::string& hash, Transaction& tx);
    std::vector<Transaction> getTransactionsForAddress(const std::string& address, int limit = 10, int offset = 0);
    std::vector<Transaction> getPendingTransactions();
    double getBalance(const std::string& address);
    
    // Wallet operations
    bool saveWallet(const Wallet& wallet);
    bool getWallet(const std::string& address, Wallet& wallet);
    std::vector<std::string> getAllWalletAddresses();
    
    // Memory proof operations
    bool saveMemoryProof(const MemoryProof& proof, const std::string& txHash = "");
    bool getMemoryProof(const std::string& hash, MemoryProof& proof);
    std::vector<MemoryProof> getMemoryProofsForAddress(const std::string& address, int limit = 10, int offset = 0);
    
private:
    // Private constructor for singleton
    DatabaseAdapter();
    
    // Database connection
    PGconn* m_conn;
    std::mutex m_connMutex;
    
    // Database connection details
    std::string m_connString;
    
    // Helper methods
    std::string escapeString(const std::string& input);
    bool executeQuery(const std::string& query);
    PGresult* executeQueryWithResult(const std::string& query);
};

} // namespace ahmiyat