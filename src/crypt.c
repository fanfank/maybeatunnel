/******************************************
 *
 * 2015 reetsee.com
 *
 ******************************************/

/**
 * @file   crypt.c
 * @author xuruiqi
 * @date   2015-11-22 14:48:00
 * @brief  use to encrypt/decrypt data
 **/

#ifndef CRYPT_C
#define CRYPT_C

typedef rts_str_t* (*real_encrypt_func)(
        unsigned char* plaintext, 
        int32_t plaintext_len, 
        rts_str_t* key,
        rts_str_t* iv);

typedef rts_str_t* (*real_decrypt_func)(
        unsigned char* ciphertext,
        int32_t ciphertext_len,
        rts_str_t* key,
        rts_str_t* iv);

typedef struct cryption_s cryption_t;
struct cryption_s {
    rts_str_t* secret_key;
    rts_str_t* cryption_method;
    real_encrypt_func real_encrypt;
    real_decrypt_func real_decrypt;
};

rts_str_t* aes256cbc_encrypt(
        unsigned char* plaintext,
        int32_t plaintext_len,
        rts_str_t* key, 
        rts_str_t* iv) {

    // init buf
    rts_str_t* rts_ciphertext = rts_str_init(plaintext_len + 16 + 16);
    CHECK_RES(
        (rts_ciphertext == NULL),
        ERROR, true, NULL,
        "call rts_str_init failed when encrypting text"
    );

    EVP_CIPHER_CTX* ctx;
    
    int32_t len;

    // Create and initialise the context
    if ((ctx = EVP_CIPHER_CTX_new() == NULL)) {
        LOG_ERROR("call EVP_CIPHER_CTX_new failed\n");
        rts_str_free(rts_ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    // Initialise the encryption operation. 
    // IMPORTANT - ensure you use a key
    // and IV size appropriate for your cipher
    // In this example we are using 256 bit AES (i.e. a 256 bit key). The
    // IV size for *most* modes is the same as the block size. For AES this
    // is 128 bits
    if (1 != EVP_EncryptInit_ex(
            ctx, EVP_aes_256_cbc(), NULL, key->buf, iv->buf)) {
        LOG_ERROR("call EVP_EncryptInit_ex failed\n");
        rts_str_free(rts_ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    // Provide the message to be encrypted, 
    // and obtain the encrypted output.
    // EVP_EncryptUpdate can be called multiple times if necessary
    if (1 != EVP_EncryptUpdate(
            ctx, rts_ciphertext->buf, &len, plaintext, plaintext_len)) {
        LOG_ERROR("call EVP_EncryptUpdate failed\n");
        rts_str_free(rts_ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    rts_ciphertext->size = len;

    // Finalise the encryption. Further rts_ciphertext bytes may be written at
    // this stage.
    if (1 != EVP_EncryptFinal_ex(ctx, rts_ciphertext->buf + len, &len)) {
        LOG_ERROR("call EVP_EncryptFinal_ex failed\n");
        rts_str_free(rts_ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    rts_ciphertext->size += len;

    // Append IV to the end of the message
    int32_t rts_str_append_res = rts_str_append(
            rts_ciphertext, iv->buf, iv->size);
    if (rts_str_append_res != 0) {
        LOG_ERROR("append iv to rts_ciphertext failed\n");
        rts_str_free(rts_ciphertext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    EVP_CIPHER_CTX_free(ctx);

    return rts_ciphertext;
} //function aes256cbc_encrypt

rts_str_t* aes256cbc_decrypt(
        unsigned char* ciphertext,
        int32_t ciphertext_len,
        rts_str_t* key) {

    // plaintext_len <= ciphertext_len
    rts_str_t* rts_plaintext = rts_str_init(ciphertext_len); 
    rts_str_t* iv = rts_str_init(16 + 1);
    if (rts_plaintext == NULL || iv == NULL) {
        LOG_ERROR("init rts_plaintext or iv failed\n");
        return NULL;
    }

    // extract iv
    rts_str_append(iv, &ciphertext[ciphertext_len - 16], 16);

    EVP_CIPHER_CTX* ctx;

    int32_t len;

    // Create and initialise the context
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        LOG_ERROR("call EVP_CIPHER_CTX_new failed");
        rts_str_free(rts_plaintext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    // Initialise the decryption operation. 
    // IMPORTANT - ensure you use a key
    // and IV size appropriate for your cipher
    // In this example we are using 256 bit AES (i.e. a 256 bit key). The
    // IV size for *most* modes is the same as the block size. For AES this
    // is 128 bits */
    if (1 != EVP_DecryptInit_ex(
            ctx, EVP_aes_256_cbc(), NULL, key->buf, iv->buf)) {
        LOG_ERROR("call EVP_DecryptInit_ex failed");
        rts_str_free(rts_plaintext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }

    // Provide the message to be decrypted, 
    // and obtain the plaintext output.
    // EVP_DecryptUpdate can be called multiple times if necessary
    if (1 != EVP_DecryptUpdate(
            ctx, plaintext->buf, &len, ciphertext, ciphertext_len)) {
        LOG_ERROR("call EVP_DecryptUpdate failed");
        rts_str_free(rts_plaintext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    rts_plaintext->size = len;
  
    // Finalise the decryption. Further plaintext bytes may be written at
    // this stage.
    if (1 != EVP_DecryptFinal_ex(ctx, rts_plaintext->buf + len, &len)) {
        LOG_ERROR("call EVP_DecryptFinal_ex failed");
        rts_str_free(rts_plaintext);
        EVP_CIPHER_CTX_free(ctx);
        return NULL;
    }
    rts_plaintext->size += len;
  
    // Cleanup
    EVP_CIPHER_CTX_free(ctx);

    return rts_plaintext;
} //function aes256cbc_decrypt

int32_t rc4_encrypt(
        unsigned char* plaintext,
        int32_t plaintext_len,
        rts_str_t* key) {
    //TODO(xuruiqi) finish function

} //function rc4_encrypt

int32_t rc4_decrypt(
        unsigned char* ciphertext,
        int32_t ciphertext_len,
        rts_str_t* key) {
    //TODO(xuruiqi) finish function

} //function rc4_decrypt

rts_str_t* encrypt(
        cryption_t* crpt,
        unsigned char* plaintext,
        int32_t plaintext_len) {

    if (crpt == NULL || plaintext == NULL) {
        return NULL;
    }

    return crpt->real_encrypt(
        plaintext, 
        plaintext_len, 
        crpt->secret_key
    );
} //function encrypt

rts_str_t* decrypt(
        cryption_t* crpt,
        unsigned char* ciphertext,
        int32_t ciphertext_len) {

    if (crpt == NULL || ciphertext == NULL) {
        return NULL;
    }

    return crpt->real_decrypt(
        ciphertext,
        ciphertext_len,
        crpt->secret_key
    );
} //function decrypt

#endif //CRYPT_C
