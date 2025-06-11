#include "../include/wallet.h"
#include "../include/utils.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

Wallet::Wallet() {
    // Generate a new key pair
    generateKeyPair();
}

Wallet::Wallet(const std::string& privateKey, const std::string& publicKey)
    : m_privateKey(privateKey), m_publicKey(publicKey) {
    m_address = deriveAddress(publicKey);
}

Wallet::Wallet(const std::string& privateKey) : m_privateKey(privateKey) {
    // Derive public key from private key
    m_publicKey = ahmiyat::utils::derivePublicKey(privateKey);
    m_address = deriveAddress(m_publicKey);
}

void Wallet::generateKeyPair() {
    auto keyPair = ahmiyat::utils::generateKeyPair();
    m_privateKey = keyPair.first;
    m_publicKey = keyPair.second;
    m_address = deriveAddress(m_publicKey);
}

std::string Wallet::deriveAddress(const std::string& publicKey) {
    // Create wallet address by hashing the public key
    return ahmiyat::utils::sha256(publicKey).substr(0, 40);
}

Transaction Wallet::createTransaction(const std::string& recipientAddress, double amount) const {
    if (recipientAddress.empty()) {
        throw std::invalid_argument("Recipient address cannot be empty");
    }
    
    if (amount <= 0) {
        throw std::invalid_argument("Transaction amount must be positive");
    }
    
    // Create transaction
    Transaction tx(m_address, recipientAddress, amount);
    
    // Sign it with the private key
    tx.signTransaction(m_privateKey);
    
    return tx;
}

std::string Wallet::sign(const std::string& data) const {
    return ahmiyat::utils::sign(m_privateKey, data);
}

bool Wallet::verifySignature(const std::string& publicKey, const std::string& signature, const std::string& data) {
    return ahmiyat::utils::verify(publicKey, signature, data);
}

std::string Wallet::getPublicKey() const {
    return m_publicKey;
}

std::string Wallet::getPrivateKey() const {
    return m_privateKey;
}

std::string Wallet::getAddress() const {
    return m_address;
}

bool Wallet::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open wallet file for saving" << std::endl;
        return false;
    }
    
    file << "Private Key: " << m_privateKey << std::endl;
    file << "Public Key: " << m_publicKey << std::endl;
    file << "Address: " << m_address << std::endl;
    
    file.close();
    return true;
}

Wallet Wallet::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open wallet file for loading");
    }
    
    std::string privateKey, publicKey;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.find("Private Key: ") == 0) {
            privateKey = line.substr(13);
        } else if (line.find("Public Key: ") == 0) {
            publicKey = line.substr(12);
        }
    }
    
    if (privateKey.empty() || publicKey.empty()) {
        throw std::runtime_error("Invalid wallet file format");
    }
    
    return Wallet(privateKey, publicKey);
}
