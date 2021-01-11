//
// Created by jianyu on 6/1/19.
//

#include <sodium.h>
#include <iostream>

int main() {
    unsigned char buf[1500];
    unsigned char cipher[1500];
    unsigned long long l;
    unsigned long long len;
    unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
    unsigned char key[crypto_aead_aes256gcm_KEYBYTES];
    for (int i = 0;i < 10000;i++) {
        randombytes_buf(nonce, crypto_aead_aes256gcm_NPUBBYTES);
        crypto_aead_aes256gcm_encrypt(cipher, &l, buf, 150, nullptr, 0, nullptr, nonce, key);
        if (crypto_aead_aes256gcm_decrypt(buf, &len, nullptr, cipher, l, nullptr, 0, nonce, key) < 0) {
            std::cout << "error during decryption\n";
        }
    }
    return 0;
}