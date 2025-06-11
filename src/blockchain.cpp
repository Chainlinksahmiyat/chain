#include "../include/blockchain.h"
#include "../include/utils.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>

Blockchain::Blockchain() : m_miningReward(50.0) {
    // Create the genesis block
    m_chain.push_back(createGenesisBlock());
}

Block Blockchain::createGenesisBlock() {
    // The first block has no previous hash
    return Block(0, std::vector<Transaction>(), "0");
}

Block Blockchain::getLatestBlock() const {
    std::lock_guard<std::mutex> lock(m_chainMutex);
    return m_chain.back();
}

bool Blockchain::addBlock(const std::string& minerAddress) {
    std::lock_guard<std::mutex> lock(m_chainMutex);
    
    // Check if miner has enough memories to mine
    if (!hasEnoughMemoriesForMining(minerAddress)) {
        std::cerr << "Miner doesn't have enough memories to mine a block" << std::endl;
        return false;
    }
    
    Block latestBlock = m_chain.back();
    Block newBlock(latestBlock.getIndex() + 1, m_pendingTransactions, latestBlock.getHash());
    
    // Try to mine the block (performs proof of memories)
    if (!newBlock.mineBlock(4, minerAddress)) { // Difficulty level of 4 (can be adjusted)
        std::cerr << "Failed to mine block" << std::endl;
        return false;
    }
    
    // Validate the new block
    if (!isValidNewBlock(newBlock, latestBlock)) {
        std::cerr << "Invalid new block" << std::endl;
        return false;
    }
    
    // Add the block to the chain
    m_chain.push_back(newBlock);
    
    // Clear pending transactions and create mining reward
    m_pendingTransactions.clear();
    
    // Create a reward transaction for the miner
    Transaction rewardTx("", minerAddress, m_miningReward);
    m_pendingTransactions.push_back(rewardTx);
    
    return true;
}

bool Blockchain::isValidNewBlock(const Block& newBlock, const Block& previousBlock) const {
    // Check index continuity
    if (newBlock.getIndex() != previousBlock.getIndex() + 1) {
        std::cerr << "Invalid block index" << std::endl;
        return false;
    }
    
    // Check previous hash link
    if (newBlock.getPreviousHash() != previousBlock.getHash()) {
        std::cerr << "Invalid previous block hash" << std::endl;
        return false;
    }
    
    // Verify block hash
    if (newBlock.calculateHash() != newBlock.getHash()) {
        std::cerr << "Invalid block hash" << std::endl;
        return false;
    }
    
    return true;
}

bool Blockchain::hasEnoughMemoriesForMining(const std::string& address) const {
    // Check if the miner has uploaded at least some memories to be eligible for mining
    // Simplified version - in a real implementation, this would check the quality and
    // quantity of memories according to the consensus rules
    auto it = m_memoryProofs.find(address);
    if (it == m_memoryProofs.end()) {
        return false;
    }
    
    // Require at least 3 memories to be eligible for mining
    return it->second.size() >= 3;
}

std::vector<Block> Blockchain::getChain() const {
    std::lock_guard<std::mutex> lock(m_chainMutex);
    return m_chain;
}

bool Blockchain::isChainValid() const {
    std::lock_guard<std::mutex> lock(m_chainMutex);
    
    // Start from index 1 since we can't validate the genesis block
    for (size_t i = 1; i < m_chain.size(); ++i) {
        const Block& currentBlock = m_chain[i];
        const Block& previousBlock = m_chain[i - 1];
        
        // Validate the block
        if (!isValidNewBlock(currentBlock, previousBlock)) {
            return false;
        }
    }
    
    return true;
}

bool Blockchain::addTransaction(const Transaction& transaction) {
    // Validate the transaction
    if (!transaction.isValid()) {
        std::cerr << "Invalid transaction signature" << std::endl;
        return false;
    }
    
    // For non-reward transactions, check if sender has enough balance
    if (transaction.getType() == Transaction::TransactionType::COIN_TRANSFER) {
        if (getBalance(transaction.getFromAddress()) < transaction.getAmount()) {
            std::cerr << "Not enough balance for transaction" << std::endl;
            return false;
        }
    }
    
    // Add to pending transactions
    m_pendingTransactions.push_back(transaction);
    return true;
}

std::vector<Transaction> Blockchain::getPendingTransactions() const {
    return m_pendingTransactions;
}

bool Blockchain::processTransaction(const Transaction& transaction) {
    // For memory reward transactions, verify the memory proof
    if (transaction.getType() == Transaction::TransactionType::MEMORY_REWARD) {
        // Find the memory proof by hash
        bool foundMemoryProof = false;
        
        for (const auto& entry : m_memoryProofs) {
            for (const auto& proof : entry.second) {
                if (proof.getProofHash() == transaction.getMemoryProofHash()) {
                    foundMemoryProof = true;
                    break;
                }
            }
            if (foundMemoryProof) break;
        }
        
        if (!foundMemoryProof) {
            std::cerr << "Memory proof not found for reward transaction" << std::endl;
            return false;
        }
    }
    
    return addTransaction(transaction);
}

double Blockchain::getBalance(const std::string& address) const {
    double balance = 0.0;
    
    // Check all blocks for transactions involving this address
    for (const auto& block : m_chain) {
        for (const auto& tx : block.getTransactions()) {
            if (tx.getFromAddress() == address) {
                balance -= tx.getAmount();
            }
            
            if (tx.getToAddress() == address) {
                balance += tx.getAmount();
            }
        }
    }
    
    // Also check pending transactions
    for (const auto& tx : m_pendingTransactions) {
        if (tx.getFromAddress() == address) {
            balance -= tx.getAmount();
        }
        
        if (tx.getToAddress() == address) {
            balance += tx.getAmount();
        }
    }
    
    return balance;
}

bool Blockchain::verifyMemoryProof(const MemoryProof& proof) {
    if (!proof.isValid()) {
        std::cerr << "Invalid memory proof signature" << std::endl;
        return false;
    }
    
    // Check if this memory already exists (prevent duplicates)
    for (const auto& entry : m_memoryProofs) {
        for (const auto& existingProof : entry.second) {
            if (existingProof.getFileHash() == proof.getFileHash()) {
                std::cerr << "Memory already exists in the blockchain" << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

bool Blockchain::storeMemoryProof(const MemoryProof& proof) {
    if (!verifyMemoryProof(proof)) {
        return false;
    }
    
    // Store the proof
    std::string uploader = proof.getUploader();
    m_memoryProofs[uploader].push_back(proof);
    
    // Create a reward transaction for the uploader
    // The reward amount could depend on memory type, size, etc.
    double reward = 10.0; // Fixed reward for simplicity
    
    Transaction rewardTx(uploader, reward, proof.getProofHash());
    m_pendingTransactions.push_back(rewardTx);
    
    return true;
}

void Blockchain::minePendingTransactions(const std::string& minerAddress) {
    // Add a mining reward transaction (will be included in the next block)
    Transaction rewardTx(minerAddress, m_miningReward, "mining_reward");
    m_pendingTransactions.push_back(rewardTx);
    
    // Try to add a new block containing pending transactions
    if (!addBlock(minerAddress)) {
        // If mining fails, remove the reward transaction
        m_pendingTransactions.pop_back();
    }
}

double Blockchain::getMiningReward() const {
    return m_miningReward;
}

void Blockchain::setMiningReward(double reward) {
    m_miningReward = reward;
}

size_t Blockchain::getChainSize() const {
    std::lock_guard<std::mutex> lock(m_chainMutex);
    return m_chain.size();
}

std::string Blockchain::getChainAsJson() const {
    std::lock_guard<std::mutex> lock(m_chainMutex);
    
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"chain\": [\n";
    
    for (size_t i = 0; i < m_chain.size(); ++i) {
        ss << m_chain[i].toJson();
        if (i < m_chain.size() - 1) {
            ss << ",";
        }
        ss << "\n";
    }
    
    ss << "  ],\n";
    ss << "  \"pendingTransactions\": [\n";
    
    for (size_t i = 0; i < m_pendingTransactions.size(); ++i) {
        ss << m_pendingTransactions[i].toJson();
        if (i < m_pendingTransactions.size() - 1) {
            ss << ",";
        }
        ss << "\n";
    }
    
    ss << "  ]\n";
    ss << "}";
    
    return ss.str();
}

void Blockchain::saveChain(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for saving blockchain");
    }
    
    file << getChainAsJson();
    file.close();
}

bool Blockchain::loadChain(const std::string& filename) {
    // Implementation would deserialize the blockchain from JSON
    // This is a simplified version - would need full JSON parsing
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open blockchain file for loading" << std::endl;
        return false;
    }
    
    // In a real implementation, we would parse the JSON and reconstruct the chain
    // For this example, we'll just say it's not implemented
    std::cerr << "Blockchain loading not fully implemented" << std::endl;
    return false;
}
