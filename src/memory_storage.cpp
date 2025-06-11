#include "../include/memory_storage.h"
#include "../include/utils.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <sstream>

MemoryStorage::MemoryStorage(const std::string& baseDir) : m_baseDir(baseDir) {
    initializeStorage();
}

void MemoryStorage::initializeStorage() {
    try {
        // Create base directory if it doesn't exist
        if (!fs::exists(m_baseDir)) {
            fs::create_directory(m_baseDir);
        }
        
        // Create subdirectories for different memory types
        std::vector<std::string> subDirs = {"images", "videos", "memes", "text"};
        for (const auto& dir : subDirs) {
            std::string fullPath = m_baseDir + "/" + dir;
            if (!fs::exists(fullPath)) {
                fs::create_directory(fullPath);
            }
        }
        
        // Load existing memory index if available
        loadIndex();
    } catch (const std::exception& e) {
        std::cerr << "Error initializing memory storage: " << e.what() << std::endl;
    }
}

MemoryProof MemoryStorage::storeMemory(const std::string& filePath, 
                                      MemoryProof::MemoryType type, 
                                      const std::string& uploader, 
                                      const std::string& description,
                                      const std::string& privateKey) {
    std::lock_guard<std::mutex> lock(m_storageMutex);
    
    if (!fs::exists(filePath)) {
        throw std::runtime_error("File does not exist: " + filePath);
    }
    
    // Check file size - limit to 50MB
    size_t fileSize = fs::file_size(filePath);
    if (fileSize > 50 * 1024 * 1024) {
        throw std::runtime_error("File is too large (> 50MB)");
    }
    
    // Create the memory proof
    MemoryProof proof(filePath, type, uploader, description);
    
    // Sign the proof with the uploader's private key
    proof.signMemory(privateKey);
    
    // Check if this file already exists in the storage
    std::string fileHash = proof.getFileHash();
    if (memoryExists(fileHash)) {
        throw std::runtime_error("Memory file already exists with hash: " + fileHash);
    }
    
    // Determine storage location based on memory type
    std::string storagePath = getStoragePath(fileHash);
    
    // Copy the file to the storage location
    if (!ahmiyat::utils::copyFile(filePath, storagePath)) {
        throw std::runtime_error("Failed to copy memory file to storage");
    }
    
    // Update indexes
    m_memoryIndex[fileHash] = proof;
    m_addressToMemories[uploader].push_back(fileHash);
    
    std::cout << "Memory stored: " << fileHash << " (" 
              << fileSize / 1024 << " KB) by " << uploader.substr(0, 10) << "..." << std::endl;
    
    // Save updated index
    saveIndex();
    
    return proof;
}

std::string MemoryStorage::retrieveMemory(const std::string& fileHash) const {
    if (!memoryExists(fileHash)) {
        throw std::runtime_error("Memory does not exist with hash: " + fileHash);
    }
    
    return getStoragePath(fileHash);
}

std::vector<MemoryProof> MemoryStorage::getMemoriesByAddress(const std::string& address) const {
    std::vector<MemoryProof> result;
    
    auto it = m_addressToMemories.find(address);
    if (it != m_addressToMemories.end()) {
        for (const auto& fileHash : it->second) {
            auto proofIt = m_memoryIndex.find(fileHash);
            if (proofIt != m_memoryIndex.end()) {
                result.push_back(proofIt->second);
            }
        }
    }
    
    return result;
}

size_t MemoryStorage::getMemoryCount(const std::string& address) const {
    auto it = m_addressToMemories.find(address);
    if (it != m_addressToMemories.end()) {
        return it->second.size();
    }
    return 0;
}

bool MemoryStorage::memoryExists(const std::string& fileHash) const {
    return m_memoryIndex.find(fileHash) != m_memoryIndex.end();
}

std::vector<std::string> MemoryStorage::getAllUploaderAddresses() const {
    std::vector<std::string> addresses;
    addresses.reserve(m_addressToMemories.size());
    
    for (const auto& entry : m_addressToMemories) {
        addresses.push_back(entry.first);
    }
    
    return addresses;
}

std::string MemoryStorage::calculateFileHash(const std::string& filePath) const {
    return ahmiyat::utils::sha256File(filePath);
}

std::string MemoryStorage::getStoragePath(const std::string& fileHash) const {
    // Determine the folder based on memory type
    std::string subFolder = "other";
    
    if (m_memoryIndex.find(fileHash) != m_memoryIndex.end()) {
        const auto& proof = m_memoryIndex.at(fileHash);
        
        switch (proof.getType()) {
            case MemoryProof::MemoryType::IMAGE:
                subFolder = "images";
                break;
            case MemoryProof::MemoryType::VIDEO:
                subFolder = "videos";
                break;
            case MemoryProof::MemoryType::MEME:
                subFolder = "memes";
                break;
            case MemoryProof::MemoryType::TEXT:
                subFolder = "text";
                break;
            default:
                subFolder = "other";
        }
    } else {
        // If we don't know the type yet, use a hash-based distribution
        // This helps avoid too many files in one directory
        subFolder = "other";
    }
    
    return m_baseDir + "/" + subFolder + "/" + fileHash;
}

bool MemoryStorage::saveIndex() const {
    try {
        std::string indexPath = m_baseDir + "/memory_index.json";
        
        std::ofstream file(indexPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open memory index file for saving: " << indexPath << std::endl;
            return false;
        }
        
        file << "{\n";
        file << "  \"memories\": [\n";
        
        size_t count = 0;
        for (const auto& entry : m_memoryIndex) {
            file << "    " << entry.second.toJson();
            if (++count < m_memoryIndex.size()) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ],\n";
        file << "  \"addressToMemories\": {\n";
        
        count = 0;
        for (const auto& entry : m_addressToMemories) {
            file << "    \"" << entry.first << "\": " 
                 << ahmiyat::utils::vectorToJsonArray(entry.second);
            
            if (++count < m_addressToMemories.size()) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  }\n";
        file << "}\n";
        
        file.close();
        std::cout << "Memory index saved to: " << indexPath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving memory index: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryStorage::loadIndex() {
    try {
        std::string indexPath = m_baseDir + "/memory_index.json";
        
        if (!fs::exists(indexPath)) {
            std::cout << "No existing memory index found. Creating new index." << std::endl;
            return false;
        }
        
        std::cout << "Loading memory index from: " << indexPath << std::endl;
        
        // Read the entire file
        std::ifstream file(indexPath);
        if (!file.is_open()) {
            std::cerr << "Failed to open memory index file for loading" << std::endl;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = buffer.str();
        file.close();
        
        // Parse the JSON (simplified implementation)
        // In a real implementation, we would use a JSON parser library
        
        // Clear existing data
        m_memoryIndex.clear();
        m_addressToMemories.clear();
        
        // Find and extract memories array
        size_t memoriesStart = json.find("\"memories\"");
        if (memoriesStart != std::string::npos) {
            memoriesStart = json.find("[", memoriesStart);
            if (memoriesStart != std::string::npos) {
                size_t memoriesEnd = json.find("]", memoriesStart);
                if (memoriesEnd != std::string::npos) {
                    std::string memoriesJson = json.substr(memoriesStart + 1, memoriesEnd - memoriesStart - 1);
                    
                    // Split into individual memory JSONs (very simplified)
                    size_t pos = 0;
                    while (pos < memoriesJson.length()) {
                        size_t memStart = memoriesJson.find("{", pos);
                        if (memStart == std::string::npos) break;
                        
                        size_t memEnd = memoriesJson.find("}", memStart);
                        if (memEnd == std::string::npos) break;
                        
                        // Find the next comma or end of array
                        size_t nextComma = memoriesJson.find(",", memEnd);
                        
                        // Extract the memory JSON
                        std::string memJson = memoriesJson.substr(memStart, memEnd - memStart + 1);
                        
                        // Parse the memory
                        MemoryProof proof = MemoryProof::fromJson(memJson);
                        
                        // Add to index
                        std::string fileHash = proof.getFileHash();
                        m_memoryIndex[fileHash] = proof;
                        m_addressToMemories[proof.getUploader()].push_back(fileHash);
                        
                        // Move to next memory
                        if (nextComma != std::string::npos) {
                            pos = nextComma + 1;
                        } else {
                            break;
                        }
                    }
                }
            }
        }
        
        std::cout << "Loaded " << m_memoryIndex.size() << " memories from " 
                  << m_addressToMemories.size() << " addresses" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading memory index: " << e.what() << std::endl;
        return false;
    }
}

bool MemoryStorage::storeMemory(const std::string& uploader, const MemoryProof& proof) {
    std::lock_guard<std::mutex> lock(m_storageMutex);
    
    // Check if this file already exists in the storage
    std::string fileHash = proof.getFileHash();
    if (memoryExists(fileHash)) {
        std::cerr << "Memory file already exists with hash: " << fileHash << std::endl;
        return false;
    }
    
    // Add to indexes
    m_memoryIndex[fileHash] = proof;
    m_addressToMemories[uploader].push_back(fileHash);
    
    std::cout << "Memory proof stored in index: " << fileHash << " by " 
              << uploader.substr(0, 10) << "..." << std::endl;
    
    // Save updated index
    saveIndex();
    
    return true;
}