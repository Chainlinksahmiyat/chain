#include "../include/utils.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <random>
#include <chrono>
#include <array>
#include <functional>

namespace ahmiyat {
namespace utils {

// SHA-256 Constants (first 32 bits of the fractional parts of the cube roots of the first 64 primes)
static constexpr std::array<uint32_t, 64> K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Rotate right (circular right shift) operation
inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

// SHA-256 functions
inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline uint32_t sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline uint32_t gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline uint32_t gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

// Custom SHA-256 implementation
std::string sha256(const std::string& str) {
    // Initialize hash values (first 32 bits of the fractional parts of the square roots of the first 8 primes)
    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;
    
    // Pre-processing
    size_t original_len = str.length();
    size_t total_len = ((original_len + 8) / 64 + 1) * 64; // Ensure it's a multiple of 64 bytes
    
    std::vector<uint8_t> msg(total_len, 0); // Initialize with zeros
    
    // Copy the input string
    memcpy(msg.data(), str.data(), original_len);
    
    // Append the bit '1' (0x80) after the message
    msg[original_len] = 0x80;
    
    // Append the length of the message in bits as a 64-bit big-endian integer
    uint64_t bit_len = original_len * 8;
    for (int i = 0; i < 8; ++i) {
        msg[total_len - 8 + i] = (bit_len >> (56 - i * 8)) & 0xff;
    }
    
    // Process the message in 512-bit (64-byte) chunks
    for (size_t chunk = 0; chunk < total_len; chunk += 64) {
        std::array<uint32_t, 64> w{};
        
        // Break chunk into 16 32-bit big-endian words
        for (int i = 0; i < 16; ++i) {
            w[i] = ((uint32_t)msg[chunk + i * 4] << 24) |
                   ((uint32_t)msg[chunk + i * 4 + 1] << 16) |
                   ((uint32_t)msg[chunk + i * 4 + 2] << 8) |
                   ((uint32_t)msg[chunk + i * 4 + 3]);
        }
        
        // Extend the 16 words into 64 words
        for (int i = 16; i < 64; ++i) {
            w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
        }
        
        // Initialize working variables
        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        uint32_t f = h5;
        uint32_t g = h6;
        uint32_t h = h7;
        
        // Main loop
        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = sigma0(a) + maj(a, b, c);
            
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        
        // Add the compressed chunk to the current hash value
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
        h5 += f;
        h6 += g;
        h7 += h;
    }
    
    // Produce the final hash value as a 256-bit number (32 bytes) in hex format
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << h0
       << std::setw(8) << h1
       << std::setw(8) << h2
       << std::setw(8) << h3
       << std::setw(8) << h4
       << std::setw(8) << h5
       << std::setw(8) << h6
       << std::setw(8) << h7;
    
    return ss.str();
}

std::string sha256File(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for hashing: " + filePath);
    }
    
    // Initialize hash values (first 32 bits of the fractional parts of the square roots of the first 8 primes)
    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;
    
    // Buffer to read chunks of file
    std::array<uint8_t, 64> buffer{};
    
    // Track total bytes read for padding and length calculation
    uint64_t total_bytes_read = 0;
    bool processed_padding = false;
    
    // Process file in 64-byte chunks
    while (true) {
        // Read a chunk from the file
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        size_t bytes_read = file.gcount();
        total_bytes_read += bytes_read;
        
        // If we've reached the end of the file
        if (bytes_read < buffer.size()) {
            // End of file - need to add padding
            size_t pad_start = bytes_read;
            
            // Append the bit '1' (0x80)
            buffer[pad_start] = 0x80;
            pad_start++;
            
            // If there's room for the length in this chunk
            if (pad_start <= 56) {
                // Fill with zeros until the last 8 bytes
                for (size_t i = pad_start; i < 56; i++) {
                    buffer[i] = 0;
                }
                
                // Last 8 bytes are the length in bits
                uint64_t bit_len = total_bytes_read * 8;
                for (int i = 0; i < 8; i++) {
                    buffer[56 + i] = (bit_len >> (56 - i * 8)) & 0xff;
                }
                
                processed_padding = true;
            } else {
                // Fill the rest of this chunk with zeros
                for (size_t i = pad_start; i < buffer.size(); i++) {
                    buffer[i] = 0;
                }
                
                // Process this chunk
                std::array<uint32_t, 64> w{};
                
                for (int i = 0; i < 16; i++) {
                    w[i] = ((uint32_t)buffer[i * 4] << 24) |
                           ((uint32_t)buffer[i * 4 + 1] << 16) |
                           ((uint32_t)buffer[i * 4 + 2] << 8) |
                           ((uint32_t)buffer[i * 4 + 3]);
                }
                
                // Extend the 16 words into 64 words
                for (int i = 16; i < 64; i++) {
                    w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
                }
                
                // Initialize working variables
                uint32_t a = h0;
                uint32_t b = h1;
                uint32_t c = h2;
                uint32_t d = h3;
                uint32_t e = h4;
                uint32_t f = h5;
                uint32_t g = h6;
                uint32_t h = h7;
                
                // Main loop
                for (int i = 0; i < 64; i++) {
                    uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
                    uint32_t t2 = sigma0(a) + maj(a, b, c);
                    
                    h = g;
                    g = f;
                    f = e;
                    e = d + t1;
                    d = c;
                    c = b;
                    b = a;
                    a = t1 + t2;
                }
                
                // Add the compressed chunk to the current hash value
                h0 += a;
                h1 += b;
                h2 += c;
                h3 += d;
                h4 += e;
                h5 += f;
                h6 += g;
                h7 += h;
                
                // Create another chunk with zeros and the length at the end
                std::fill(buffer.begin(), buffer.end(), 0);
                
                // Last 8 bytes are the length in bits
                uint64_t bit_len = total_bytes_read * 8;
                for (int i = 0; i < 8; i++) {
                    buffer[56 + i] = (bit_len >> (56 - i * 8)) & 0xff;
                }
                
                processed_padding = true;
            }
        }
        
        // If we've reached the end of file and processed padding, break
        if (bytes_read < buffer.size() && processed_padding) {
            break;
        }
        
        // Process the chunk
        std::array<uint32_t, 64> w{};
        
        for (int i = 0; i < 16; i++) {
            w[i] = ((uint32_t)buffer[i * 4] << 24) |
                   ((uint32_t)buffer[i * 4 + 1] << 16) |
                   ((uint32_t)buffer[i * 4 + 2] << 8) |
                   ((uint32_t)buffer[i * 4 + 3]);
        }
        
        // Extend the 16 words into 64 words
        for (int i = 16; i < 64; i++) {
            w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
        }
        
        // Initialize working variables
        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        uint32_t f = h5;
        uint32_t g = h6;
        uint32_t h = h7;
        
        // Main loop
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = sigma0(a) + maj(a, b, c);
            
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        
        // Add the compressed chunk to the current hash value
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
        h5 += f;
        h6 += g;
        h7 += h;
    }
    
    // Produce the final hash value as a 256-bit number in hex format
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << h0
       << std::setw(8) << h1
       << std::setw(8) << h2
       << std::setw(8) << h3
       << std::setw(8) << h4
       << std::setw(8) << h5
       << std::setw(8) << h6
       << std::setw(8) << h7;
    
    return ss.str();
}

// Generate a simple random string
std::string generateRandomString(size_t length) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::string result;
    result.reserve(length);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    
    for (size_t i = 0; i < length; ++i) {
        result += alphanum[dis(gen)];
    }
    
    return result;
}

// Generate a simple key pair
std::pair<std::string, std::string> generateKeyPair() {
    // In a real blockchain, this would use elliptic curve cryptography
    // For this simplified version, we'll create a random private key
    // and derive a public key from it using SHA-256
    
    std::string privateKey = generateRandomString(64); // 64 character private key
    std::string publicKey = derivePublicKey(privateKey);
    
    return {privateKey, publicKey};
}

// Derive a public key from a private key
std::string derivePublicKey(const std::string& privateKey) {
    // In a real blockchain, this would use elliptic curve cryptography
    // For this simplified version, the public key is the SHA-256 hash of the private key
    return sha256(privateKey);
}

// XOR two strings together
std::string xorStrings(const std::string& a, const std::string& b) {
    size_t len = std::min(a.length(), b.length());
    std::string result(len, 0);
    
    for (size_t i = 0; i < len; ++i) {
        result[i] = a[i] ^ b[i];
    }
    
    return result;
}

// Simple signing function (in real world, would use proper digital signatures)
std::string sign(const std::string& privateKey, const std::string& data) {
    // Hash the data first
    std::string dataHash = sha256(data);
    
    // Sign by combining private key and data hash
    std::string combined = privateKey + dataHash;
    std::string signature = sha256(combined);
    
    return signature;
}

// Verify a signature
bool verify(const std::string& publicKey, const std::string& signature, const std::string& data) {
    // In a real implementation, this would properly verify the signature
    // For this simplified version, we re-calculate what the signature should be
    // based on the public key (which is derived from the private key)
    // and check if it matches
    
    // Hash the data
    std::string dataHash = sha256(data);
    
    // Get the expected signature
    std::string expectedSignature = sha256(publicKey + dataHash);
    
    // In a real blockchain, this comparison would involve a proper
    // cryptographic verification algorithm
    return signature == expectedSignature;
}

// Base64 encoding/decoding tables
static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 encoding
std::string base64Encode(const std::vector<uint8_t>& data) {
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t array3[3];
    uint8_t array4[4];
    
    for (size_t i = 0; i < data.size(); i++) {
        array3[j++] = data[i];
        
        if (j == 3) {
            array4[0] = (array3[0] & 0xfc) >> 2;
            array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
            array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
            array4[3] = array3[2] & 0x3f;
            
            for (j = 0; j < 4; j++) {
                ret += base64_chars[array4[j]];
            }
            
            j = 0;
        }
    }
    
    // Handle the remaining bytes
    if (j) {
        for (int i = j; i < 3; i++) {
            array3[i] = 0;
        }
        
        array4[0] = (array3[0] & 0xfc) >> 2;
        array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
        array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
        array4[3] = array3[2] & 0x3f;
        
        for (int i = 0; i < j + 1; i++) {
            ret += base64_chars[array4[i]];
        }
        
        while (j++ < 3) {
            ret += '=';
        }
    }
    
    return ret;
}

// Base64 decoding
std::vector<uint8_t> base64Decode(const std::string& encoded) {
    auto is_base64 = [](unsigned char c) -> bool {
        return (isalnum(c) || (c == '+') || (c == '/'));
    };
    
    size_t in_len = encoded.size();
    size_t i = 0;
    size_t j = 0;
    size_t in_ = 0;
    unsigned char array4[4], array3[3];
    std::vector<uint8_t> ret;
    
    while (in_len-- && (encoded[in_] != '=') && is_base64(encoded[in_])) {
        array4[i++] = encoded[in_]; in_++;
        
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                array4[i] = base64_chars.find(array4[i]);
            }
            
            array3[0] = (array4[0] << 2) + ((array4[1] & 0x30) >> 4);
            array3[1] = ((array4[1] & 0xf) << 4) + ((array4[2] & 0x3c) >> 2);
            array3[2] = ((array4[2] & 0x3) << 6) + array4[3];
            
            for (i = 0; i < 3; i++) {
                ret.push_back(array3[i]);
            }
            
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j < 4; j++) {
            array4[j] = 0;
        }
        
        for (j = 0; j < 4; j++) {
            array4[j] = base64_chars.find(array4[j]);
        }
        
        array3[0] = (array4[0] << 2) + ((array4[1] & 0x30) >> 4);
        array3[1] = ((array4[1] & 0xf) << 4) + ((array4[2] & 0x3c) >> 2);
        array3[2] = ((array4[2] & 0x3) << 6) + array4[3];
        
        for (j = 0; j < i - 1; j++) {
            ret.push_back(array3[j]);
        }
    }
    
    return ret;
}

std::string jsonEscape(const std::string& input) {
    std::ostringstream escaped;
    for (char c : input) {
        switch (c) {
            case '\"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 32) {
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    escaped << c;
                }
        }
    }
    return escaped.str();
}

std::string encodeToJson(const std::vector<std::pair<std::string, std::string>>& data) {
    std::ostringstream json;
    json << "{";
    
    for (size_t i = 0; i < data.size(); ++i) {
        json << "\"" << jsonEscape(data[i].first) << "\":\"" << jsonEscape(data[i].second) << "\"";
        if (i < data.size() - 1) {
            json << ",";
        }
    }
    
    json << "}";
    return json.str();
}

std::string vectorToJsonArray(const std::vector<std::string>& vec) {
    std::ostringstream json;
    json << "[";
    
    for (size_t i = 0; i < vec.size(); ++i) {
        json << "\"" << jsonEscape(vec[i]) << "\"";
        if (i < vec.size() - 1) {
            json << ",";
        }
    }
    
    json << "]";
    return json.str();
}

bool writeToFile(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    return !file.bad();
}

std::string readFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool copyFile(const std::string& source, const std::string& destination) {
    try {
        std::ifstream src(source, std::ios::binary);
        if (!src.is_open()) {
            return false;
        }
        
        std::ofstream dst(destination, std::ios::binary);
        if (!dst.is_open()) {
            return false;
        }
        
        dst << src.rdbuf();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error copying file: " << e.what() << std::endl;
        return false;
    }
}

std::vector<uint8_t> readBinaryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + filePath);
    }
    
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    
    return buffer;
}

bool writeBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return !file.bad();
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    
    return std::string(start, end + 1);
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string doubleToString(double value, int precision) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

double stringToDouble(const std::string& str) {
    try {
        return std::stod(str);
    } catch (const std::exception& e) {
        throw std::invalid_argument("Invalid number format: " + str);
    }
}

std::string timeToString(time_t time) {
    std::stringstream ss;
    ss << time;
    return ss.str();
}

time_t stringToTime(const std::string& timeStr) {
    try {
        return std::stoll(timeStr);
    } catch (const std::exception& e) {
        throw std::invalid_argument("Invalid time format: " + timeStr);
    }
}

} // namespace utils
} // namespace ahmiyat
