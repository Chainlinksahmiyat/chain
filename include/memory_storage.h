#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include "memory_proof.h"

namespace fs = std::filesystem;

/**
 * @class MemoryStorage
 * @brief Manages storage of uploaded memory files
 * 
 * Handles file I/O for uploaded memories and their metadata
 */
class MemoryStorage {
public:
    /**
     * @brief Initialize storage with base directory
     * @param baseDir Base directory for storing memory files
     */
    MemoryStorage(const std::string& baseDir = "memories");
    
    /**
     * @brief Store a memory file and generate proof
     * @param filePath Path to the memory file to store
     * @param type Type of memory (image, video, meme, text)
     * @param uploader Address of the uploader
     * @param description User description of the memory
     * @param privateKey Private key of the uploader for signing
     * @return Memory proof for the stored file
     */
    MemoryProof storeMemory(const std::string& filePath, 
                          MemoryProof::MemoryType type, 
                          const std::string& uploader, 
                          const std::string& description,
                          const std::string& privateKey);
                          
    /**
     * @brief Store an existing memory proof
     * @param uploader Address of the uploader
     * @param proof The memory proof to store
     * @return True if successful, false otherwise
     */
    bool storeMemory(const std::string& uploader, const MemoryProof& proof);
    
    /**
     * @brief Retrieve a memory file by its hash
     * @param fileHash Hash of the file to retrieve
     * @return Path to the stored file
     */
    std::string retrieveMemory(const std::string& fileHash) const;
    
    /**
     * @brief Get all memories uploaded by a specific address
     * @param address Address of the uploader
     * @return Vector of memory proofs
     */
    std::vector<MemoryProof> getMemoriesByAddress(const std::string& address) const;
    
    /**
     * @brief Get memory count for an address (used for mining eligibility)
     * @param address Address to check
     * @return Number of memories uploaded by the address
     */
    size_t getMemoryCount(const std::string& address) const;
    
    /**
     * @brief Verify if a memory exists
     * @param fileHash Hash of the file to check
     * @return True if the memory exists, false otherwise
     */
    bool memoryExists(const std::string& fileHash) const;
    
    /**
     * @brief Get list of all addresses that have uploaded memories
     * @return Vector of uploader addresses
     */
    std::vector<std::string> getAllUploaderAddresses() const;
    
    /**
     * @brief Save the memory index to disk
     * @return True if saving was successful, false otherwise
     */
    bool saveIndex() const;
    
    /**
     * @brief Load the memory index from disk
     * @return True if loading was successful, false otherwise
     */
    bool loadIndex();
    
private:
    std::string m_baseDir;
    std::unordered_map<std::string, MemoryProof> m_memoryIndex;
    std::unordered_map<std::string, std::vector<std::string>> m_addressToMemories;
    std::mutex m_storageMutex;
    
    /**
     * @brief Create storage directories
     */
    void initializeStorage();
    
    /**
     * @brief Calculate hash of a file
     * @param filePath Path to the file
     * @return SHA-256 hash of the file
     */
    std::string calculateFileHash(const std::string& filePath) const;
    
    /**
     * @brief Generate storage path for a file
     * @param fileHash Hash of the file
     * @return Path where the file should be stored
     */
    std::string getStoragePath(const std::string& fileHash) const;
};
