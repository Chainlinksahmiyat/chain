#pragma once

#include <string>
#include <vector>
#include "transaction.h"

/**
 * @class Wallet
 * @brief Manages cryptographic keys and transaction signing for users
 */
class Wallet {
public:
    // Create a new wallet with fresh key pair
    Wallet();
    
    // Load a wallet from existing keys
    Wallet(const std::string& privateKey, const std::string& publicKey);
    
    // Load a wallet from just a private key (public key will be derived)
    Wallet(const std::string& privateKey);
    
    // Create a transaction
    Transaction createTransaction(const std::string& recipientAddress, double amount) const;
    
    // Sign data with private key
    std::string sign(const std::string& data) const;
    
    // Verify signature with public key
    static bool verifySignature(const std::string& publicKey, const std::string& signature, const std::string& data);
    
    // Getters
    std::string getPublicKey() const;
    std::string getPrivateKey() const;
    std::string getAddress() const; // Derived from public key
    
    // Save/load wallet
    bool saveToFile(const std::string& filename) const;
    static Wallet loadFromFile(const std::string& filename);
    
private:
    std::string m_privateKey;
    std::string m_publicKey;
    std::string m_address;
    
    // Generate key pair
    void generateKeyPair();
    
    // Derive address from public key
    static std::string deriveAddress(const std::string& publicKey);
};
