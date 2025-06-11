cmake_minimum_required(VERSION 3.10)
project(AhmiyatBlockchain)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PostgreSQL support - dynamic approach
include_directories(include)

# Try to find PostgreSQL
find_package(PostgreSQL)
if(PostgreSQL_FOUND)
    include_directories(${PostgreSQL_INCLUDE_DIRS})
    set(HAVE_POSTGRESQL 1)
    message(STATUS "PostgreSQL found: ${PostgreSQL_LIBRARIES}")
else()
    # Fallback when PostgreSQL is not found
    set(PostgreSQL_LIBRARIES "")
    set(HAVE_POSTGRESQL 0)
    message(STATUS "PostgreSQL NOT found, using fallback implementation")
    
    # Still try to include typical PostgreSQL directories
    include_directories("/usr/include")
    include_directories("/usr/include/postgresql")
    include_directories("/usr/local/include")
    include_directories("/usr/local/include/postgresql")
endif()

# Add HAVE_POSTGRESQL definition to compiler
add_definitions(-DHAVE_POSTGRESQL=${HAVE_POSTGRESQL})

# Source files for the blockchain core (excluding main.cpp)
file(GLOB CORE_SOURCES "src/block.cpp" "src/blockchain.cpp" "src/memory_proof.cpp" 
                      "src/memory_storage.cpp" "src/transaction.cpp" "src/utils.cpp"
                      "src/wallet.cpp" "src/database_adapter.cpp")

# Include blockchain core main.cpp separately
set(CORE_MAIN "src/main.cpp")

# Create the Ahmiyat blockchain core executable
add_executable(ahmiyat ${CORE_SOURCES} ${CORE_MAIN})

# Link with required libraries
target_link_libraries(ahmiyat pthread ${PostgreSQL_LIBRARIES})

# If using Linux and need to link with dl
if(UNIX AND NOT APPLE)
    target_link_libraries(ahmiyat dl)
endif()

# Web application
# Include header-only libraries for web application
include_directories(web/include include)

# Source files for the web application - use explicit list instead of glob
set(WEB_SOURCES 
    "web/src/ahmiyat_web.cpp"
    "web/src/simple_http_server.cpp"
    "web/src/main.cpp"
)

# Create the Ahmiyat web application executable (using core sources but not core main.cpp)
add_executable(ahmiyat_web ${WEB_SOURCES} ${CORE_SOURCES})

# Link with required libraries for web application
target_link_libraries(ahmiyat_web pthread ${PostgreSQL_LIBRARIES})
if(UNIX AND NOT APPLE)
    target_link_libraries(ahmiyat_web dl)
endif()

# Copy web assets to build directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/public)
file(COPY web/public DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Create necessary directories for blockchain data
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/memories)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/memories/images)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/memories/videos)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/memories/memes)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/memories/text)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/wallets)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blockchain_data)