#include "../include/database_adapter.h"
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <iostream>

// We need to use the Transaction::TransactionType enum
using TransactionType = Transaction::TransactionType;

// Try to detect if we can include PostgreSQL headers
#if defined(__has_include)
#  if __has_include(<postgresql/libpq-fe.h>)
#    define HAVE_POSTGRESQL 1
#    include <postgresql/libpq-fe.h>
#  elif __has_include(<pgsql/libpq-fe.h>)
#    define HAVE_POSTGRESQL 1
#    include <pgsql/libpq-fe.h>
#  elif __has_include(<libpq-fe.h>)
#    define HAVE_POSTGRESQL 1
#    include <libpq-fe.h>
#  else
#    define HAVE_POSTGRESQL 0
#  endif
#else
#  define HAVE_POSTGRESQL 0
#endif

// Define PostgreSQL types
#if HAVE_POSTGRESQL
  // Use real PostgreSQL types
  typedef ExecStatusType PGExecStatusType;
#else
  // Define our own version
  typedef enum {
    PGRES_EMPTY_QUERY = 0,
    PGRES_COMMAND_OK,
    PGRES_TUPLES_OK,
    PGRES_COPY_OUT,
    PGRES_COPY_IN,
    PGRES_BAD_RESPONSE,
    PGRES_NONFATAL_ERROR,
    PGRES_FATAL_ERROR,
    PGRES_COPY_BOTH,
    PGRES_SINGLE_TUPLE
  } PGExecStatusType;
#endif

#if !HAVE_POSTGRESQL
  // Stub implementation for systems without PostgreSQL
  #define CONNECTION_OK 0
  
  namespace ahmiyat {
  
  struct pg_conn {
    bool connected;
    std::string error_message;
  };
  
  struct pg_result {
    bool success;
    std::vector<std::vector<std::string>> rows;
    std::string error_message;
  };
  
  typedef enum {
    PGRES_EMPTY_QUERY = 0,
    PGRES_COMMAND_OK,
    PGRES_TUPLES_OK,
    PGRES_COPY_OUT,
    PGRES_COPY_IN,
    PGRES_BAD_RESPONSE,
    PGRES_NONFATAL_ERROR,
    PGRES_FATAL_ERROR,
    PGRES_COPY_BOTH,
    PGRES_SINGLE_TUPLE
  } ExecStatusType;
  
  // Stub implementation namespace
  namespace pg {
    inline void PQclear(pg_result* res) {
      if (res) delete res;
    }
    
    inline void PQfinish(pg_conn* conn) {
      if (conn) delete conn;
    }
    
    inline pg_conn* PQconnectdb(const char* conninfo) {
      std::cerr << "PostgreSQL support not available. Using file-based storage instead." << std::endl;
      pg_conn* conn = new pg_conn();
      conn->connected = false;
      conn->error_message = "PostgreSQL support not available";
      return conn;
    }
    
    inline int PQstatus(const pg_conn* conn) {
      if (!conn) return -1;
      return conn->connected ? CONNECTION_OK : -1;
    }
    
    inline const char* PQerrorMessage(const pg_conn* conn) {
      if (!conn) return "Null connection";
      return conn->error_message.c_str();
    }
    
    inline pg_result* PQexec(pg_conn* conn, const char* query) {
      pg_result* res = new pg_result();
      res->success = false;
      res->error_message = "PostgreSQL support not available";
      return res;
    }
    
    inline int PQresultStatus(const pg_result* res) {
      if (!res) return PGRES_FATAL_ERROR;
      return res->success ? PGRES_TUPLES_OK : PGRES_FATAL_ERROR;
    }
    
    inline int PQntuples(const pg_result* res) {
      if (!res) return 0;
      return res->rows.size();
    }
    
    inline char* PQgetvalue(const pg_result* res, int tup_num, int field_num) {
      static std::string dummy_value = "";
      if (!res || tup_num < 0 || tup_num >= static_cast<int>(res->rows.size())) 
        return const_cast<char*>(dummy_value.c_str());
      
      if (field_num < 0 || field_num >= static_cast<int>(res->rows[tup_num].size()))
        return const_cast<char*>(dummy_value.c_str());
        
      return const_cast<char*>(res->rows[tup_num][field_num].c_str());
    }
    
    inline int PQgetisnull(const pg_result* res, int tup_num, int field_num) {
      return 0; // Always return non-null for stub implementation
    }
    
    inline int PQescapeStringConn(pg_conn* conn, char* to, const char* from, size_t length, int* error) {
      if (to && from && length > 0) {
        strncpy(to, from, length - 1);
        to[length - 1] = '\0';
      }
      if (error) *error = 0;
      return 0;
    }
  } // namespace pg
  
  // Define global functions that forward to our namespace
  #define PQclear(res) ahmiyat::pg::PQclear(res)
  #define PQfinish(conn) ahmiyat::pg::PQfinish(conn)
  #define PQconnectdb(conninfo) ahmiyat::pg::PQconnectdb(conninfo)
  #define PQstatus(conn) ahmiyat::pg::PQstatus(conn)
  #define PQerrorMessage(conn) ahmiyat::pg::PQerrorMessage(conn)
  #define PQexec(conn, query) ahmiyat::pg::PQexec(conn, query)
  #define PQresultStatus(res) ahmiyat::pg::PQresultStatus(res)
  #define PQntuples(res) ahmiyat::pg::PQntuples(res)
  #define PQgetvalue(res, tup_num, field_num) ahmiyat::pg::PQgetvalue(res, tup_num, field_num)
  #define PQgetisnull(res, tup_num, field_num) ahmiyat::pg::PQgetisnull(res, tup_num, field_num)
  #define PQescapeStringConn(conn, to, from, length, error) ahmiyat::pg::PQescapeStringConn(conn, to, from, length, error)
  
  } // namespace ahmiyat
#endif

namespace ahmiyat {

// Singleton instance
DatabaseAdapter& DatabaseAdapter::getInstance() {
    static DatabaseAdapter instance;
    return instance;
}

// Constructor
DatabaseAdapter::DatabaseAdapter() : m_conn(nullptr) {
#if HAVE_POSTGRESQL
    // Check for environment variable DATABASE_URL
    const char* dbUrl = std::getenv("DATABASE_URL");
    if (dbUrl) {
        m_connString = dbUrl;
        // Try to connect automatically when the environment variable is present
        if (!connect(m_connString)) {
            std::cerr << "Failed to connect to PostgreSQL database" << std::endl;
            std::cerr << "Using file-based storage as fallback" << std::endl;
        } else {
            std::cout << "Connected to PostgreSQL database successfully!" << std::endl;
        }
    } else {
        std::cerr << "DATABASE_URL environment variable not found" << std::endl;
        std::cerr << "Using file-based storage as fallback" << std::endl;
    }
#else
    std::cerr << "PostgreSQL support not available" << std::endl;
    std::cerr << "Using file-based storage only" << std::endl;
#endif
}

// Destructor
DatabaseAdapter::~DatabaseAdapter() {
    disconnect();
}

// Connect to the database
bool DatabaseAdapter::connect(const std::string& connString) {
    std::lock_guard<std::mutex> lock(m_connMutex);
    
    // Disconnect first if already connected
    if (m_conn) {
        PQfinish(m_conn);
        m_conn = nullptr;
    }
    
    // Store connection string
    m_connString = connString;
    
#if HAVE_POSTGRESQL
    // Connect to the database
    m_conn = PQconnectdb(connString.c_str());
    
    // Check connection status
    if (PQstatus(m_conn) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(m_conn) << std::endl;
        PQfinish(m_conn);
        m_conn = nullptr;
        return false;
    }
    
    std::cout << "Connected to database successfully." << std::endl;
    return true;
#else
    // Fallback mode doesn't really connect to PostgreSQL
    m_conn = PQconnectdb("fallback_mode");
    std::cout << "PostgreSQL not available, using file-based storage." << std::endl;
    return false;
#endif
}

// Check if connected
bool DatabaseAdapter::isConnected() const {
    return (m_conn != nullptr && PQstatus(m_conn) == CONNECTION_OK);
}

// Disconnect from the database
void DatabaseAdapter::disconnect() {
    std::lock_guard<std::mutex> lock(m_connMutex);
    
    if (m_conn) {
        PQfinish(m_conn);
        m_conn = nullptr;
    }
}

// Escape a string for SQL
std::string DatabaseAdapter::escapeString(const std::string& input) {
    if (!isConnected() || input.empty()) {
        return "";
    }
    
    // Allocate enough space for the escaped string
    size_t escaped_size = input.size() * 2 + 1;
    char* escaped = new char[escaped_size];
    
    // Escape the string
    PQescapeStringConn(m_conn, escaped, input.c_str(), input.size(), nullptr);
    
    // Convert to std::string
    std::string result(escaped);
    
    // Clean up
    delete[] escaped;
    
    return result;
}

// Execute a query without result
bool DatabaseAdapter::executeQuery(const std::string& query) {
    if (!isConnected()) {
        std::cerr << "Not connected to database." << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_connMutex);
    
    // Execute the query
    PGresult* res = PQexec(m_conn, query.c_str());
    
    // Check result
    PGExecStatusType status = (PGExecStatusType)PQresultStatus(res);
    bool success = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);
    
    if (!success) {
        std::cerr << "Query failed: " << PQerrorMessage(m_conn) << std::endl;
        std::cerr << "Query was: " << query << std::endl;
    }
    
    // Clean up
    PQclear(res);
    
    return success;
}

// Execute a query and return the result
PGresult* DatabaseAdapter::executeQueryWithResult(const std::string& query) {
    if (!isConnected()) {
        std::cerr << "Not connected to database." << std::endl;
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(m_connMutex);
    
    // Execute the query
    PGresult* res = PQexec(m_conn, query.c_str());
    
    // Check result
    PGExecStatusType status = (PGExecStatusType)PQresultStatus(res);
    if (status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(m_conn) << std::endl;
        std::cerr << "Query was: " << query << std::endl;
        PQclear(res);
        return nullptr;
    }
    
    return res;
}

// Save a block to the database
bool DatabaseAdapter::saveBlock(const Block& block) {
    // Prepare query
    std::stringstream query;
    query << "INSERT INTO blocks (hash, previous_hash, timestamp, nonce, difficulty, merkle_root, height) VALUES ('"
          << escapeString(block.getHash()) << "', '"
          << escapeString(block.getPreviousHash()) << "', "
          << block.getTimestamp() << ", "
          << block.getNonce() << ", "
          << block.getDifficulty() << ", '"
          << escapeString(block.getMerkleRoot()) << "', "
          << block.getHeight() << ") "
          << "ON CONFLICT (hash) DO UPDATE SET "
          << "previous_hash = EXCLUDED.previous_hash, "
          << "timestamp = EXCLUDED.timestamp, "
          << "nonce = EXCLUDED.nonce, "
          << "difficulty = EXCLUDED.difficulty, "
          << "merkle_root = EXCLUDED.merkle_root, "
          << "height = EXCLUDED.height";
    
    // Execute the query
    return executeQuery(query.str());
}

// Get a block by hash
bool DatabaseAdapter::getBlock(const std::string& hash, Block& block) {
    // Prepare query
    std::stringstream query;
    query << "SELECT hash, previous_hash, timestamp, nonce, difficulty, merkle_root, height FROM blocks WHERE hash = '"
          << escapeString(hash) << "' LIMIT 1";
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query.str());
    if (!res) {
        return false;
    }
    
    // Check if we got a result
    if (PQntuples(res) == 0) {
        PQclear(res);
        return false;
    }
    
    // Extract data
    std::string dbHash = PQgetvalue(res, 0, 0);
    std::string previousHash = PQgetvalue(res, 0, 1);
    uint64_t timestamp = std::stoull(PQgetvalue(res, 0, 2));
    uint32_t nonce = std::stoul(PQgetvalue(res, 0, 3));
    uint32_t difficulty = std::stoul(PQgetvalue(res, 0, 4));
    std::string merkleRoot = PQgetvalue(res, 0, 5);
    uint32_t height = std::stoul(PQgetvalue(res, 0, 6));
    
    // Create block
    block = Block(previousHash, timestamp, difficulty);
    block.setHash(dbHash);
    block.setNonce(nonce);
    block.setMerkleRoot(merkleRoot);
    block.setHeight(height);
    
    // Get transactions for this block
    std::stringstream txQuery;
    txQuery << "SELECT hash, from_address, to_address, amount, timestamp, transaction_type, signature "
            << "FROM transactions WHERE block_hash = '" << escapeString(hash) << "'";
    
    PGresult* txRes = executeQueryWithResult(txQuery.str());
    if (txRes) {
        int numTx = PQntuples(txRes);
        for (int i = 0; i < numTx; i++) {
            std::string txHash = PQgetvalue(txRes, i, 0);
            std::string fromAddress = PQgetvalue(txRes, i, 1);
            std::string toAddress = PQgetvalue(txRes, i, 2);
            double amount = std::stod(PQgetvalue(txRes, i, 3));
            uint64_t txTimestamp = std::stoull(PQgetvalue(txRes, i, 4));
            int txType = std::stoi(PQgetvalue(txRes, i, 5));
            std::string signature = PQgetvalue(txRes, i, 6);
            
            Transaction tx(fromAddress, toAddress, amount, txTimestamp, static_cast<TransactionType>(txType));
            tx.setHash(txHash);
            if (!signature.empty()) {
                tx.setSignature(signature);
            }
            
            block.addTransaction(tx);
        }
        
        PQclear(txRes);
    }
    
    // Clean up
    PQclear(res);
    
    return true;
}

// Get a block by height
bool DatabaseAdapter::getBlockByHeight(int height, Block& block) {
    // Prepare query
    std::stringstream query;
    query << "SELECT hash, previous_hash, timestamp, nonce, difficulty, merkle_root, height FROM blocks WHERE height = "
          << height << " LIMIT 1";
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query.str());
    if (!res) {
        return false;
    }
    
    // Check if we got a result
    if (PQntuples(res) == 0) {
        PQclear(res);
        return false;
    }
    
    // Extract data
    std::string hash = PQgetvalue(res, 0, 0);
    
    // Clean up
    PQclear(res);
    
    // Use getBlock to fetch the full block with transactions
    return getBlock(hash, block);
}

// Get blocks with pagination
std::vector<Block> DatabaseAdapter::getBlocks(int limit, int offset) {
    std::vector<Block> blocks;
    
    // Prepare query
    std::stringstream query;
    query << "SELECT hash FROM blocks ORDER BY height DESC LIMIT " << limit << " OFFSET " << offset;
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query.str());
    if (!res) {
        return blocks;
    }
    
    // Extract blocks
    int numBlocks = PQntuples(res);
    for (int i = 0; i < numBlocks; i++) {
        std::string hash = PQgetvalue(res, i, 0);
        
        Block block;
        if (getBlock(hash, block)) {
            blocks.push_back(block);
        }
    }
    
    // Clean up
    PQclear(res);
    
    return blocks;
}

// Get blockchain height
int DatabaseAdapter::getBlockchainHeight() {
    // Prepare query
    std::string query = "SELECT MAX(height) FROM blocks";
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query);
    if (!res) {
        return 0;
    }
    
    // Check if we got a result
    if (PQntuples(res) == 0 || PQgetisnull(res, 0, 0)) {
        PQclear(res);
        return 0;
    }
    
    // Extract height
    int height = std::stoi(PQgetvalue(res, 0, 0));
    
    // Clean up
    PQclear(res);
    
    return height;
}

// Save a transaction to the database
bool DatabaseAdapter::saveTransaction(const Transaction& tx, const std::string& blockHash) {
    // Prepare query
    std::stringstream query;
    query << "INSERT INTO transactions (hash, from_address, to_address, amount, timestamp, transaction_type, block_hash, signature) VALUES ('"
          << escapeString(tx.getHash()) << "', ";
    
    if (tx.getFromAddress().empty()) {
        query << "NULL, ";
    } else {
        query << "'" << escapeString(tx.getFromAddress()) << "', ";
    }
    
    query << "'" << escapeString(tx.getToAddress()) << "', "
          << tx.getAmount() << ", "
          << tx.getTimestamp() << ", "
          << static_cast<int>(tx.getType()) << ", ";
    
    if (blockHash.empty()) {
        query << "NULL, ";
    } else {
        query << "'" << escapeString(blockHash) << "', ";
    }
    
    if (tx.getSignature().empty()) {
        query << "NULL";
    } else {
        query << "'" << escapeString(tx.getSignature()) << "'";
    }
    
    query << ") ON CONFLICT (hash) DO UPDATE SET "
          << "block_hash = EXCLUDED.block_hash";
    
    // Execute the query
    return executeQuery(query.str());
}

// Get a transaction by hash
bool DatabaseAdapter::getTransaction(const std::string& hash, Transaction& tx) {
    // Prepare query
    std::stringstream query;
    query << "SELECT hash, from_address, to_address, amount, timestamp, transaction_type, signature FROM transactions WHERE hash = '"
          << escapeString(hash) << "' LIMIT 1";
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query.str());
    if (!res) {
        return false;
    }
    
    // Check if we got a result
    if (PQntuples(res) == 0) {
        PQclear(res);
        return false;
    }
    
    // Extract data
    std::string txHash = PQgetvalue(res, 0, 0);
    std::string fromAddress = PQgetisnull(res, 0, 1) ? "" : PQgetvalue(res, 0, 1);
    std::string toAddress = PQgetvalue(res, 0, 2);
    double amount = std::stod(PQgetvalue(res, 0, 3));
    uint64_t timestamp = std::stoull(PQgetvalue(res, 0, 4));
    int type = std::stoi(PQgetvalue(res, 0, 5));
    std::string signature = PQgetisnull(res, 0, 6) ? "" : PQgetvalue(res, 0, 6);
    
    // Create transaction
    tx = Transaction(fromAddress, toAddress, amount, timestamp, static_cast<TransactionType>(type));
    tx.setHash(txHash);
    if (!signature.empty()) {
        tx.setSignature(signature);
    }
    
    // Clean up
    PQclear(res);
    
    return true;
}

// Get transactions for an address
std::vector<Transaction> DatabaseAdapter::getTransactionsForAddress(const std::string& address, int limit, int offset) {
    std::vector<Transaction> transactions;
    
    // Prepare query
    std::stringstream query;
    query << "SELECT hash, from_address, to_address, amount, timestamp, transaction_type, signature FROM transactions "
          << "WHERE from_address = '" << escapeString(address) << "' OR to_address = '" << escapeString(address) << "' "
          << "ORDER BY timestamp DESC LIMIT " << limit << " OFFSET " << offset;
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query.str());
    if (!res) {
        return transactions;
    }
    
    // Extract transactions
    int numTx = PQntuples(res);
    for (int i = 0; i < numTx; i++) {
        std::string txHash = PQgetvalue(res, i, 0);
        std::string fromAddress = PQgetisnull(res, i, 1) ? "" : PQgetvalue(res, i, 1);
        std::string toAddress = PQgetvalue(res, i, 2);
        double amount = std::stod(PQgetvalue(res, i, 3));
        uint64_t timestamp = std::stoull(PQgetvalue(res, i, 4));
        int type = std::stoi(PQgetvalue(res, i, 5));
        std::string signature = PQgetisnull(res, i, 6) ? "" : PQgetvalue(res, i, 6);
        
        Transaction tx(fromAddress, toAddress, amount, timestamp, static_cast<TransactionType>(type));
        tx.setHash(txHash);
        if (!signature.empty()) {
            tx.setSignature(signature);
        }
        
        transactions.push_back(tx);
    }
    
    // Clean up
    PQclear(res);
    
    return transactions;
}

// Get pending transactions
std::vector<Transaction> DatabaseAdapter::getPendingTransactions() {
    std::vector<Transaction> transactions;
    
    // Prepare query
    std::string query = "SELECT hash, from_address, to_address, amount, timestamp, transaction_type, signature FROM transactions "
                      "WHERE block_hash IS NULL ORDER BY timestamp ASC";
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query);
    if (!res) {
        return transactions;
    }
    
    // Extract transactions
    int numTx = PQntuples(res);
    for (int i = 0; i < numTx; i++) {
        std::string txHash = PQgetvalue(res, i, 0);
        std::string fromAddress = PQgetisnull(res, i, 1) ? "" : PQgetvalue(res, i, 1);
        std::string toAddress = PQgetvalue(res, i, 2);
        double amount = std::stod(PQgetvalue(res, i, 3));
        uint64_t timestamp = std::stoull(PQgetvalue(res, i, 4));
        int type = std::stoi(PQgetvalue(res, i, 5));
        std::string signature = PQgetisnull(res, i, 6) ? "" : PQgetvalue(res, i, 6);
        
        Transaction tx(fromAddress, toAddress, amount, timestamp, static_cast<TransactionType>(type));
        tx.setHash(txHash);
        if (!signature.empty()) {
            tx.setSignature(signature);
        }
        
        transactions.push_back(tx);
    }
    
    // Clean up
    PQclear(res);
    
    return transactions;
}

// Calculate balance for an address
double DatabaseAdapter::getBalance(const std::string& address) {
    double balance = 0.0;
    
    // Outgoing transactions (sent)
    std::stringstream outQuery;
    outQuery << "SELECT COALESCE(SUM(amount), 0) FROM transactions "
             << "WHERE from_address = '" << escapeString(address) << "'";
    
    PGresult* outRes = executeQueryWithResult(outQuery.str());
    if (outRes && PQntuples(outRes) > 0 && !PQgetisnull(outRes, 0, 0)) {
        double outgoing = std::stod(PQgetvalue(outRes, 0, 0));
        balance -= outgoing;
        PQclear(outRes);
    }
    
    // Incoming transactions (received)
    std::stringstream inQuery;
    inQuery << "SELECT COALESCE(SUM(amount), 0) FROM transactions "
            << "WHERE to_address = '" << escapeString(address) << "'";
    
    PGresult* inRes = executeQueryWithResult(inQuery.str());
    if (inRes && PQntuples(inRes) > 0 && !PQgetisnull(inRes, 0, 0)) {
        double incoming = std::stod(PQgetvalue(inRes, 0, 0));
        balance += incoming;
        PQclear(inRes);
    }
    
    return balance;
}

// Save a wallet to the database
bool DatabaseAdapter::saveWallet(const Wallet& wallet) {
    // Prepare query
    std::stringstream query;
    query << "INSERT INTO wallets (address, public_key, private_key_encrypted) VALUES ('"
          << escapeString(wallet.getAddress()) << "', '"
          << escapeString(wallet.getPublicKey()) << "', '"
          << escapeString(wallet.getPrivateKey()) << "') "
          << "ON CONFLICT (address) DO UPDATE SET "
          << "public_key = EXCLUDED.public_key, "
          << "private_key_encrypted = EXCLUDED.private_key_encrypted";
    
    // Execute the query
    return executeQuery(query.str());
}

// Get a wallet by address
bool DatabaseAdapter::getWallet(const std::string& address, Wallet& wallet) {
    // Prepare query
    std::stringstream query;
    query << "SELECT address, public_key, private_key_encrypted FROM wallets WHERE address = '"
          << escapeString(address) << "' LIMIT 1";
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query.str());
    if (!res) {
        return false;
    }
    
    // Check if we got a result
    if (PQntuples(res) == 0) {
        PQclear(res);
        return false;
    }
    
    // Extract data
    std::string walletAddress = PQgetvalue(res, 0, 0);
    std::string publicKey = PQgetvalue(res, 0, 1);
    std::string privateKey = PQgetvalue(res, 0, 2);
    
    // Create wallet from private key
    wallet = Wallet(privateKey);
    
    // Clean up
    PQclear(res);
    
    return true;
}

// Get all wallet addresses
std::vector<std::string> DatabaseAdapter::getAllWalletAddresses() {
    std::vector<std::string> addresses;
    
    // Prepare query
    std::string query = "SELECT address FROM wallets";
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query);
    if (!res) {
        return addresses;
    }
    
    // Extract addresses
    int numWallets = PQntuples(res);
    for (int i = 0; i < numWallets; i++) {
        addresses.push_back(PQgetvalue(res, i, 0));
    }
    
    // Clean up
    PQclear(res);
    
    return addresses;
}

// Save a memory proof to the database
bool DatabaseAdapter::saveMemoryProof(const MemoryProof& proof, const std::string& txHash) {
    // Prepare query
    std::stringstream query;
    query << "INSERT INTO memory_proofs (hash, owner_address, file_hash, file_path, file_size, file_type, timestamp, transaction_hash) VALUES ('"
          << escapeString(proof.getHash()) << "', '"
          << escapeString(proof.getOwnerAddress()) << "', '"
          << escapeString(proof.getFileHash()) << "', '"
          << escapeString(proof.getFilePath()) << "', "
          << proof.getFileSize() << ", '"
          << escapeString(proof.getFileType()) << "', "
          << proof.getTimestamp() << ", ";
    
    if (txHash.empty()) {
        query << "NULL";
    } else {
        query << "'" << escapeString(txHash) << "'";
    }
    
    query << ") ON CONFLICT (hash) DO UPDATE SET "
          << "transaction_hash = EXCLUDED.transaction_hash";
    
    // Execute the query
    return executeQuery(query.str());
}

// Get a memory proof by hash
bool DatabaseAdapter::getMemoryProof(const std::string& hash, MemoryProof& proof) {
    // Prepare query
    std::stringstream query;
    query << "SELECT hash, owner_address, file_hash, file_path, file_size, file_type, timestamp FROM memory_proofs WHERE hash = '"
          << escapeString(hash) << "' LIMIT 1";
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query.str());
    if (!res) {
        return false;
    }
    
    // Check if we got a result
    if (PQntuples(res) == 0) {
        PQclear(res);
        return false;
    }
    
    // Extract data
    std::string proofHash = PQgetvalue(res, 0, 0);
    std::string ownerAddress = PQgetvalue(res, 0, 1);
    std::string fileHash = PQgetvalue(res, 0, 2);
    std::string filePath = PQgetvalue(res, 0, 3);
    uint64_t fileSize = std::stoull(PQgetvalue(res, 0, 4));
    std::string fileType = PQgetvalue(res, 0, 5);
    uint64_t timestamp = std::stoull(PQgetvalue(res, 0, 6));
    
    // Create memory proof
    proof = MemoryProof(ownerAddress, filePath, fileHash, fileSize, fileType, timestamp);
    proof.setHash(proofHash);
    
    // Clean up
    PQclear(res);
    
    return true;
}

// Get memory proofs for an address
std::vector<MemoryProof> DatabaseAdapter::getMemoryProofsForAddress(const std::string& address, int limit, int offset) {
    std::vector<MemoryProof> proofs;
    
    // Prepare query
    std::stringstream query;
    query << "SELECT hash, owner_address, file_hash, file_path, file_size, file_type, timestamp FROM memory_proofs "
          << "WHERE owner_address = '" << escapeString(address) << "' "
          << "ORDER BY timestamp DESC LIMIT " << limit << " OFFSET " << offset;
    
    // Execute the query
    PGresult* res = executeQueryWithResult(query.str());
    if (!res) {
        return proofs;
    }
    
    // Extract proofs
    int numProofs = PQntuples(res);
    for (int i = 0; i < numProofs; i++) {
        std::string proofHash = PQgetvalue(res, i, 0);
        std::string ownerAddress = PQgetvalue(res, i, 1);
        std::string fileHash = PQgetvalue(res, i, 2);
        std::string filePath = PQgetvalue(res, i, 3);
        uint64_t fileSize = std::stoull(PQgetvalue(res, i, 4));
        std::string fileType = PQgetvalue(res, i, 5);
        uint64_t timestamp = std::stoull(PQgetvalue(res, i, 6));
        
        MemoryProof proof(ownerAddress, filePath, fileHash, fileSize, fileType, timestamp);
        proof.setHash(proofHash);
        
        proofs.push_back(proof);
    }
    
    // Clean up
    PQclear(res);
    
    return proofs;
}

} // namespace ahmiyat