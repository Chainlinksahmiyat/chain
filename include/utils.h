#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>
#include <utility>  // For std::pair

namespace ahmiyat {
namespace utils {

/**
 * @brief Calculate SHA-256 hash of a string
 * @param str String to hash
 * @return Hex representation of the hash
 */
std::string sha256(const std::string& str);

/**
 * @brief Calculate SHA-256 hash of a file
 * @param filePath Path to the file
 * @return Hex representation of the hash
 */
std::string sha256File(const std::string& filePath);

/**
 * @brief Generate a simple key pair (public key is derived from private key)
 * @return Pair of (privateKey, publicKey)
 */
std::pair<std::string, std::string> generateKeyPair();

/**
 * @brief Derive a public key from a private key
 * @param privateKey Private key string
 * @return Public key string
 */
std::string derivePublicKey(const std::string& privateKey);

/**
 * @brief Sign data with a private key using a simple digital signature algorithm
 * @param privateKey Private key string
 * @param data Data to sign
 * @return Base64-encoded signature
 */
std::string sign(const std::string& privateKey, const std::string& data);

/**
 * @brief Verify a signature with a public key
 * @param publicKey Public key string
 * @param signature Base64-encoded signature
 * @param data Data that was signed
 * @return True if the signature is valid, false otherwise
 */
bool verify(const std::string& publicKey, const std::string& signature, const std::string& data);

/**
 * @brief Encode binary data to Base64
 * @param data Binary data to encode
 * @return Base64-encoded string
 */
std::string base64Encode(const std::vector<uint8_t>& data);

/**
 * @brief Decode Base64 string to binary data
 * @param encoded Base64-encoded string
 * @return Decoded binary data
 */
std::vector<uint8_t> base64Decode(const std::string& encoded);

/**
 * @brief Perform an XOR operation between two strings
 * @param a First string
 * @param b Second string
 * @return XOR result as string
 */
std::string xorStrings(const std::string& a, const std::string& b);

/**
 * @brief Generate a simple random string with given length
 * @param length Length of string to generate
 * @return Random string
 */
std::string generateRandomString(size_t length);

/**
 * @brief Escape special characters in a string for JSON
 * @param input String to escape
 * @return JSON-safe string
 */
std::string jsonEscape(const std::string& input);

/**
 * @brief Encode key-value pairs to JSON
 * @param data Vector of key-value pairs
 * @return JSON string
 */
std::string encodeToJson(const std::vector<std::pair<std::string, std::string>>& data);

/**
 * @brief Convert a vector of strings to a JSON array
 * @param vec Vector of strings
 * @return JSON array string
 */
std::string vectorToJsonArray(const std::vector<std::string>& vec);

/**
 * @brief Write a string to a file
 * @param filePath Path to the file
 * @param content Content to write
 * @return True if successful, false otherwise
 */
bool writeToFile(const std::string& filePath, const std::string& content);

/**
 * @brief Read a file into a string
 * @param filePath Path to the file
 * @return Content of the file
 */
std::string readFromFile(const std::string& filePath);

/**
 * @brief Copy a file
 * @param source Source file path
 * @param destination Destination file path
 * @return True if successful, false otherwise
 */
bool copyFile(const std::string& source, const std::string& destination);

/**
 * @brief Read a binary file
 * @param filePath Path to the file
 * @return Binary content of the file
 */
std::vector<uint8_t> readBinaryFile(const std::string& filePath);

/**
 * @brief Write binary data to a file
 * @param filePath Path to the file
 * @param data Binary data to write
 * @return True if successful, false otherwise
 */
bool writeBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data);

/**
 * @brief Split a string by a delimiter
 * @param s String to split
 * @param delimiter Character to split by
 * @return Vector of substrings
 */
std::vector<std::string> split(const std::string& s, char delimiter);

/**
 * @brief Trim whitespace from a string
 * @param s String to trim
 * @return Trimmed string
 */
std::string trim(const std::string& s);

/**
 * @brief Check if a string starts with a prefix
 * @param str String to check
 * @param prefix Prefix to look for
 * @return True if the string starts with the prefix, false otherwise
 */
bool startsWith(const std::string& str, const std::string& prefix);

/**
 * @brief Get the current timestamp as a formatted string
 * @return Formatted timestamp
 */
std::string getCurrentTimestamp();

/**
 * @brief Convert a double to a string with specified precision
 * @param value Double to convert
 * @param precision Number of decimal places
 * @return Formatted string
 */
std::string doubleToString(double value, int precision = 8);

/**
 * @brief Convert a string to a double
 * @param str String to convert
 * @return Double value
 */
double stringToDouble(const std::string& str);

/**
 * @brief Convert a time_t to a string
 * @param time Time to convert
 * @return String representation
 */
std::string timeToString(time_t time);

/**
 * @brief Convert a string to a time_t
 * @param timeStr String to convert
 * @return time_t value
 */
time_t stringToTime(const std::string& timeStr);

} // namespace utils
} // namespace ahmiyat
