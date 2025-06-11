#include <openssl/sha.h>
#include <openssl/evp.h>

int main() {
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    return 0;
}