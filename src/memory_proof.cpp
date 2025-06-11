#include "../include/memory_proof.h"
#include "../include/utils.h"
#include <sstream>
#include <stdexcept>
#include <iostream>

MemoryProof::MemoryProof(const std::string& filePath, 
                         MemoryType type, 
                         const std::string& uploader, 
                         const std::string& description)
    : m_type(type),
      m_uploader(uploader),
      m_description(description),
      m_timestamp(std::time(nullptr)) {
    
    // Calculate hash of the file
    m_fileHash = ahmiyat::utils::sha256File(filePath);
}

MemoryProof::MemoryProof(const std::string& fileHash,
                         MemoryType type,
                         const std::string& uploader,
                         const std::string& description,
                         time_t timestamp,
                         const std::string& signature)
    : m_fileHash(fileHash),
      m_type(type),
      m_uploader(uploader),
      m_description(description),
      m_timestamp(timestamp),
      m_signature(signature) {
}

// Constructor for database reconstruction
MemoryProof::MemoryProof(const std::string& ownerAddress, 
                         const std::string& filePath, 
                         const std::string& fileHash, 
                         uint64_t fileSize, 
                         const std::string& fileType, 
                         uint64_t timestamp)
    : m_fileHash(fileHash),
      m_type(MemoryType::TEXT), // Default type, will be set based on fileType below
      m_uploader(ownerAddress),
      m_description(filePath),  // Use filePath as description for now
      m_timestamp(timestamp) {
    
    // Convert fileType string to MemoryType enum
    if (fileType == "IMAGE") {
        m_type = MemoryType::IMAGE;
    } else if (fileType == "VIDEO") {
        m_type = MemoryType::VIDEO;
    } else if (fileType == "MEME") {
        m_type = MemoryType::MEME;
    } else if (fileType == "TEXT") {
        m_type = MemoryType::TEXT;
    }
}

void MemoryProof::signMemory(const std::string& privateKey) {
    if (m_uploader.empty()) {
        throw std::invalid_argument("Uploader address cannot be empty");
    }
    
    // Calculate the hash and sign it with the private key
    std::string proofHash = calculateHash();
    m_signature = ahmiyat::utils::sign(privateKey, proofHash);
}

bool MemoryProof::isValid() const {
    if (m_uploader.empty()) {
        return false;
    }
    
    if (m_signature.empty()) {
        return false;
    }
    
    return ahmiyat::utils::verify(m_uploader, m_signature, calculateHash());
}

std::string MemoryProof::calculateHash() const {
    std::stringstream ss;
    ss << m_fileHash << memoryTypeToString(m_type) << m_uploader << m_description << m_timestamp;
    return ahmiyat::utils::sha256(ss.str());
}

uint32_t MemoryProof::calculateProofDifficulty() const {
    // The difficulty of proof can be calculated based on the memory type, size, etc.
    // This is a simplified implementation
    
    switch (m_type) {
        case MemoryType::IMAGE:
            return 3;
        case MemoryType::VIDEO:
            return 4; // Videos are worth more in this example
        case MemoryType::MEME:
            return 2;
        case MemoryType::TEXT:
            return 1;
        default:
            return 2;
    }
}

std::string MemoryProof::getFileHash() const {
    return m_fileHash;
}

MemoryProof::MemoryType MemoryProof::getType() const {
    return m_type;
}

std::string MemoryProof::getUploader() const {
    return m_uploader;
}

std::string MemoryProof::getDescription() const {
    return m_description;
}

time_t MemoryProof::getTimestamp() const {
    return m_timestamp;
}

std::string MemoryProof::getSignature() const {
    return m_signature;
}

std::string MemoryProof::getProofHash() const {
    return calculateHash();
}

std::string MemoryProof::memoryTypeToString(MemoryType type) {
    switch (type) {
        case MemoryType::IMAGE:
            return "IMAGE";
        case MemoryType::VIDEO:
            return "VIDEO";
        case MemoryType::MEME:
            return "MEME";
        case MemoryType::TEXT:
            return "TEXT";
        default:
            return "UNKNOWN";
    }
}

MemoryProof::MemoryType MemoryProof::stringToMemoryType(const std::string& typeStr) {
    if (typeStr == "IMAGE") return MemoryType::IMAGE;
    if (typeStr == "VIDEO") return MemoryType::VIDEO;
    if (typeStr == "MEME") return MemoryType::MEME;
    if (typeStr == "TEXT") return MemoryType::TEXT;
    
    throw std::invalid_argument("Unknown memory type: " + typeStr);
}

std::string MemoryProof::toJson() const {
    std::stringstream ss;
    ss << "{";
    ss << "\"fileHash\":\"" << ahmiyat::utils::jsonEscape(m_fileHash) << "\",";
    ss << "\"type\":\"" << memoryTypeToString(m_type) << "\",";
    ss << "\"uploader\":\"" << ahmiyat::utils::jsonEscape(m_uploader) << "\",";
    ss << "\"description\":\"" << ahmiyat::utils::jsonEscape(m_description) << "\",";
    ss << "\"timestamp\":" << m_timestamp << ",";
    ss << "\"signature\":\"" << ahmiyat::utils::jsonEscape(m_signature) << "\"";
    ss << "}";
    
    return ss.str();
}

MemoryProof MemoryProof::fromJson(const std::string& json) {
    // In a real implementation, this would parse the JSON and reconstruct the memory proof
    // For this example, we'll create a dummy memory proof
    std::cerr << "MemoryProof::fromJson not fully implemented" << std::endl;
    return MemoryProof("dummy_hash", MemoryType::MEME, "dummy_address", "dummy_description", std::time(nullptr), "");
}
