#pragma once

#include <string>
#include <vector>
#include <ctime>

/**
 * @class Transaction
 * @brief Represents a transaction in the Ahmiyat blockchain
 * 
 * Transactions can be coin transfers or rewards for memory uploads
 */
class Transaction {
public:
    enum class TransactionType {
        COIN_TRANSFER,   // Regular coin transfer between addresses
        MEMORY_REWARD    // Reward for uploading memory content
    };
    
    /**
     * @brief Constructor for coin transfer
     * @param fromAddress Address of the sender
     * @param toAddress Address of the recipient
     * @param amount Amount of coins to transfer
     */
    Transaction(const std::string& fromAddress, const std::string& toAddress, double amount);
    
    /**
     * @brief Constructor for memory reward
     * @param toAddress Address of the recipient
     * @param amount Amount of reward
     * @param memoryProofHash Hash of the memory proof
     */
    Transaction(const std::string& toAddress, double amount, const std::string& memoryProofHash);
    
    /**
     * @brief Default constructor for deserialization
     */
    Transaction() : m_amount(0), m_timestamp(0), m_type(TransactionType::COIN_TRANSFER) {}
    
    /**
     * @brief Constructor for database reconstruction
     */
    Transaction(const std::string& fromAddress, const std::string& toAddress, 
               double amount, time_t timestamp, TransactionType type)
        : m_fromAddress(fromAddress), m_toAddress(toAddress),
          m_amount(amount), m_timestamp(timestamp), m_type(type) {}
    
    /**
     * @brief Sign the transaction with sender's key
     * @param privateKey Private key of the sender
     */
    void signTransaction(const std::string& privateKey);
    
    /**
     * @brief Verify transaction signature
     * @return True if the transaction is valid, false otherwise
     */
    bool isValid() const;
    
    // Getters
    std::string getFromAddress() const;
    std::string getToAddress() const;
    double getAmount() const;
    time_t getTimestamp() const;
    std::string getSignature() const;
    TransactionType getType() const;
    std::string getMemoryProofHash() const;
    
    // Setters for database operations
    void setSignature(const std::string& signature) { m_signature = signature; }
    void setHash(const std::string& hash) { /* Set internal hash if needed */ }
    std::string getHash() const { return calculateHash(); }
    
    /**
     * @brief Convert transaction to JSON
     * @return JSON representation of the transaction
     */
    std::string toJson() const;
    
    /**
     * @brief Create transaction from JSON
     * @param json JSON representation of a transaction
     * @return Transaction object
     */
    static Transaction fromJson(const std::string& json);
    
    /**
     * @brief Create hash of transaction data for signing
     * @return SHA-256 hash of transaction data
     */
    std::string calculateHash() const;
    
private:
    std::string m_fromAddress; // Can be empty for memory reward transactions
    std::string m_toAddress;
    double m_amount;
    time_t m_timestamp;
    std::string m_signature;
    TransactionType m_type;
    std::string m_memoryProofHash; // Only for memory reward transactions
};
