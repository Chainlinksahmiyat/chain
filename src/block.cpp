#include "../include/block.h"
#include "../include/utils.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>

// Constructor implementation
Block::Block(uint32_t indexIn, const std::vector<Transaction>& dataIn, const std::string& previousHashIn)
    : m_index(indexIn), 
      m_timestamp(std::time(nullptr)), 
      m_transactions(dataIn), 
      m_previousHash(previousHashIn),
      m_nonce(0),
      m_minerAddress("") {
    // Calculate the hash immediately upon creation
    m_hash = calculateHash();
}

// Default constructor implementation
Block::Block() 
    : m_index(0), 
      m_timestamp(std::time(nullptr)), 
      m_nonce(0),
      m_minerAddress("") {
    m_hash = calculateHash();
}

std::string Block::calculateHash() const {
    std::stringstream ss;
    ss << m_index << m_timestamp;
    
    // Include all transaction hashes
    for (const auto& tx : m_transactions) {
        ss << tx.calculateHash();
    }
    
    ss << m_previousHash << m_nonce;
    
    return ahmiyat::utils::sha256(ss.str());
}

bool Block::mineBlock(int difficulty, const std::string& minerAddress) {
    m_minerAddress = minerAddress;
    
    // Create a string with 'difficulty' number of 0s
    std::string target(difficulty, '0');
    
    std::cout << "Mining block with difficulty " << difficulty << "..." << std::endl;
    
    do {
        m_nonce++;
        m_hash = calculateHash();
        
        // Check if we've hit our target (hash starts with the required number of zeros)
        if (m_hash.substr(0, difficulty) == target) {
            std::cout << "Block mined: " << m_hash << std::endl;
            return true;
        }
        
        // Don't run forever if we can't find a valid hash
        if (m_nonce > 1000000) {
            std::cerr << "Mining aborted after too many attempts" << std::endl;
            return false;
        }
        
        // Show progress periodically
        if (m_nonce % 100000 == 0) {
            std::cout << "Mining in progress... " << m_nonce << " hashes calculated" << std::endl;
        }
    } while (true);
}

uint32_t Block::getIndex() const {
    return m_index;
}

time_t Block::getTimestamp() const {
    return m_timestamp;
}

std::string Block::getPreviousHash() const {
    return m_previousHash;
}

std::string Block::getHash() const {
    return m_hash;
}

std::vector<Transaction> Block::getTransactions() const {
    return m_transactions;
}

uint32_t Block::getNonce() const {
    return m_nonce;
}

std::string Block::getMinerAddress() const {
    return m_minerAddress;
}

std::string Block::toJson() const {
    std::stringstream ss;
    ss << "    {\n";
    ss << "      \"index\": " << m_index << ",\n";
    ss << "      \"timestamp\": " << m_timestamp << ",\n";
    ss << "      \"previousHash\": \"" << ahmiyat::utils::jsonEscape(m_previousHash) << "\",\n";
    ss << "      \"hash\": \"" << ahmiyat::utils::jsonEscape(m_hash) << "\",\n";
    ss << "      \"nonce\": " << m_nonce << ",\n";
    ss << "      \"minerAddress\": \"" << ahmiyat::utils::jsonEscape(m_minerAddress) << "\",\n";
    ss << "      \"transactions\": [\n";
    
    for (size_t i = 0; i < m_transactions.size(); ++i) {
        ss << "        " << m_transactions[i].toJson();
        if (i < m_transactions.size() - 1) {
            ss << ",";
        }
        ss << "\n";
    }
    
    ss << "      ]\n";
    ss << "    }";
    
    return ss.str();
}

Block Block::fromJson(const std::string& json) {
    // This is a simplified implementation for demonstration
    // In a real implementation, we would use a proper JSON parser
    
    try {
        // Create an empty block
        Block block;
        
        // Find key-value pairs
        auto extractValue = [&json](const std::string& key) -> std::string {
            size_t keyPos = json.find("\"" + key + "\"");
            if (keyPos == std::string::npos) return "";
            
            size_t colonPos = json.find(":", keyPos);
            if (colonPos == std::string::npos) return "";
            
            size_t valueStart = json.find_first_not_of(" \t\n\r", colonPos + 1);
            if (valueStart == std::string::npos) return "";
            
            if (json[valueStart] == '\"') {
                // String value
                valueStart++; // Skip the opening quote
                size_t valueEnd = json.find("\"", valueStart);
                if (valueEnd == std::string::npos) return "";
                return json.substr(valueStart, valueEnd - valueStart);
            } else {
                // Numeric value
                size_t valueEnd = json.find_first_of(",}\n", valueStart);
                if (valueEnd == std::string::npos) return "";
                return json.substr(valueStart, valueEnd - valueStart);
            }
        };
        
        // Extract values
        std::string indexStr = extractValue("index");
        std::string timestampStr = extractValue("timestamp");
        block.m_previousHash = extractValue("previousHash");
        block.m_hash = extractValue("hash");
        std::string nonceStr = extractValue("nonce");
        block.m_minerAddress = extractValue("minerAddress");
        
        // Convert to appropriate types
        if (!indexStr.empty()) {
            block.m_index = std::stoi(indexStr);
        }
        
        if (!timestampStr.empty()) {
            block.m_timestamp = std::stoull(timestampStr);
        }
        
        if (!nonceStr.empty()) {
            block.m_nonce = std::stoi(nonceStr);
        }
        
        // Extract transactions
        size_t txArrayStart = json.find("\"transactions\"");
        if (txArrayStart != std::string::npos) {
            txArrayStart = json.find("[", txArrayStart);
            if (txArrayStart != std::string::npos) {
                size_t txArrayEnd = json.find("]", txArrayStart);
                if (txArrayEnd != std::string::npos) {
                    std::string txArrayJson = json.substr(txArrayStart + 1, txArrayEnd - txArrayStart - 1);
                    
                    // Split into individual transaction JSONs
                    size_t pos = 0;
                    while (pos < txArrayJson.length()) {
                        size_t txStart = txArrayJson.find("{", pos);
                        if (txStart == std::string::npos) break;
                        
                        size_t txEnd = txArrayJson.find("}", txStart);
                        if (txEnd == std::string::npos) break;
                        
                        std::string txJson = txArrayJson.substr(txStart, txEnd - txStart + 1);
                        block.m_transactions.push_back(Transaction::fromJson(txJson));
                        
                        pos = txEnd + 1;
                    }
                }
            }
        }
        
        return block;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing block JSON: " << e.what() << std::endl;
        // Create an empty block and return it
        Block emptyBlock;
        emptyBlock.m_previousHash = "0";
        return emptyBlock;
    }
}
