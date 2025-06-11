#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include "block.h"
#include "transaction.h"
#include "wallet.h"
#include "memory_proof.h"

/**
 * @class Blockchain
 * @brief Core blockchain implementation for the Ahmiyat coin network
 * 
 * This class implements the main blockchain functionality including:
 * - Chain management
 * - Transaction pool
 * - Mining and validation
 * - Consensus mechanism (Proof of Memories)
 */
class Blockchain {
public:
    Blockchain();
    
    // Block operations
    bool addBlock(const std::string& minerAddress);
    Block getLatestBlock() const;
    std::vector<Block> getChain() const;
    bool isChainValid() const;
    
    // Transaction operations
    bool addTransaction(const Transaction& transaction);
    std::vector<Transaction> getPendingTransactions() const;
    bool processTransaction(const Transaction& transaction);
    double getBalance(const std::string& address) const;
    
    // Memory proof operations
    bool verifyMemoryProof(const MemoryProof& proof);
    bool storeMemoryProof(const MemoryProof& proof);
    
    // Mining and rewards
    void minePendingTransactions(const std::string& minerAddress);
    double getMiningReward() const;
    void setMiningReward(double reward);
    
    // Utility functions
    size_t getChainSize() const;
    std::string getChainAsJson() const;
    void saveChain(const std::string& filename) const;
    bool loadChain(const std::string& filename);
    
private:
    std::vector<Block> m_chain;
    std::vector<Transaction> m_pendingTransactions;
    std::unordered_map<std::string, std::vector<MemoryProof>> m_memoryProofs;
    double m_miningReward;
    mutable std::mutex m_chainMutex;
    
    Block createGenesisBlock();
    bool hasEnoughMemoriesForMining(const std::string& address) const;
    bool isValidNewBlock(const Block& newBlock, const Block& previousBlock) const;
};
