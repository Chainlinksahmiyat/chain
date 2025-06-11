#pragma once

#include <string>
#include <vector>
#include <ctime>
#include <cstdint>

/**
 * @class MemoryProof
 * @brief Implements the "Proof of Memories" consensus mechanism
 * 
 * Represents uploaded media content with metadata and verification
 */
class MemoryProof {
public:
    // Types of memories that can be uploaded
    enum class MemoryType {
        IMAGE,
        VIDEO,
        MEME,
        TEXT
    };
    
    // Constructor for creating memory proof from actual file
    MemoryProof(const std::string& filePath, 
                MemoryType type, 
                const std::string& uploader, 
                const std::string& description);
    
    // Default constructor for STL containers
    MemoryProof() : m_type(MemoryType::TEXT), m_timestamp(0) {}
    
    // Constructor for reconstructing memory proof from data
    MemoryProof(const std::string& fileHash,
                MemoryType type,
                const std::string& uploader,
                const std::string& description,
                time_t timestamp,
                const std::string& signature);
                
    // Constructor for database reconstruction
    MemoryProof(const std::string& ownerAddress, 
                const std::string& filePath, 
                const std::string& fileHash, 
                uint64_t fileSize, 
                const std::string& fileType, 
                uint64_t timestamp);
    
    // Sign the memory proof with uploader's private key
    void signMemory(const std::string& privateKey);
    
    // Verify memory proof signature
    bool isValid() const;
    
    // Calculate proof difficulty based on content
    uint32_t calculateProofDifficulty() const;
    
    // Getters
    std::string getFileHash() const;
    MemoryType getType() const;
    std::string getUploader() const;
    std::string getDescription() const;
    time_t getTimestamp() const;
    std::string getSignature() const;
    std::string getProofHash() const;
    
    // For JSON serialization
    std::string toJson() const;
    static MemoryProof fromJson(const std::string& json);
    
    // For database operations
    void setHash(const std::string& hash) { m_fileHash = hash; }
    
    // Additional getters/setters for database operations
    std::string getOwnerAddress() const { return m_uploader; }
    std::string getFilePath() const { return m_description; } // Using description as file path
    uint64_t getFileSize() const { return 0; } // Not tracked in current implementation
    std::string getFileType() const { return memoryTypeToString(m_type); }
    std::string getHash() const { return m_fileHash; } // Alias for getFileHash
    
private:
    std::string m_fileHash;      // Hash of the uploaded file content
    MemoryType m_type;           // Type of memory (image, video, etc.)
    std::string m_uploader;      // Address of the uploader
    std::string m_description;   // User description of the memory
    time_t m_timestamp;          // When the memory was uploaded
    std::string m_signature;     // Cryptographic signature by uploader
    
    // Calculate hash of memory data for signing
    std::string calculateHash() const;
    
    // Helper to convert MemoryType to string for internal use
    static MemoryType stringToMemoryType(const std::string& typeStr);
    
public:
    // Make the memory type conversion function public for easier use
    static std::string memoryTypeToString(MemoryType type);
};
