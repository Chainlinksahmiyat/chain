#pragma once

#include <string>
#include <vector>
#include <ctime>
#include <cstdint>
#include "transaction.h"

/**
 * @class Block
 * @brief Represents a block in the Ahmiyat blockchain
 * 
 * Contains block metadata and transactions
 */
class Block {
public:
    /**
     * @brief Constructor for creating a new block
     * @param indexIn Block index in the chain
     * @param dataIn Transactions to include in this block
     * @param previousHashIn Hash of the previous block in the chain
     */
    Block(uint32_t indexIn, const std::vector<Transaction>& dataIn, const std::string& previousHashIn);
    
    /**
     * @brief Default constructor for deserialization
     */
    Block();
    
    /**
     * @brief Constructor for database reconstruction
     * @param previousHashIn Hash of the previous block
     * @param timestampIn Block creation timestamp
     * @param difficultyIn Mining difficulty used
     */
    Block(const std::string& previousHashIn, time_t timestampIn, uint32_t difficultyIn)
        : m_index(0), m_timestamp(timestampIn), m_previousHash(previousHashIn), m_nonce(0) {}
    
    /**
     * @brief Generate block hash based on contents
     * @return SHA-256 hash of block contents
     */
    std::string calculateHash() const;
    
    /**
     * @brief Mine the block using Proof of Memories consensus
     * @param difficulty Mining difficulty (number of leading zeros required)
     * @param minerAddress Address of the miner
     * @return True if mining was successful, false otherwise
     */
    bool mineBlock(int difficulty, const std::string& minerAddress);
    
    // Getters
    uint32_t getIndex() const;
    time_t getTimestamp() const;
    std::string getPreviousHash() const;
    std::string getHash() const;
    std::vector<Transaction> getTransactions() const;
    uint32_t getNonce() const;
    std::string getMinerAddress() const;
    
    // For database operations
    void addTransaction(const Transaction& tx) { m_transactions.push_back(tx); }
    
    // Additional methods for database adapter
    void setHash(const std::string& hash) { m_hash = hash; }
    void setNonce(uint32_t nonce) { m_nonce = nonce; }
    
    // These methods aren't in the current Block implementation
    // but are needed by the database adapter
    void setMerkleRoot(const std::string& merkleRoot) { /* Not used in current implementation */ }
    void setHeight(uint32_t height) { /* Not used in current implementation */ }
    uint32_t getDifficulty() const { return 0; /* Not tracked in current implementation */ }
    std::string getMerkleRoot() const { return ""; /* Not implemented */ }
    uint32_t getHeight() const { return m_index; /* Using index as height */ }
    
    // For JSON serialization
    std::string toJson() const;
    static Block fromJson(const std::string& json);
    
private:
    uint32_t m_index;
    time_t m_timestamp;
    std::vector<Transaction> m_transactions;
    std::string m_previousHash;
    std::string m_hash;
    uint32_t m_nonce;
    std::string m_minerAddress;
};
