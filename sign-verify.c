#include "common.h"
#include "ec.h"
#include "hash.h"
#include <openssl/err.h>
#include <openssl/crypto.h>

void signMessageDigest(ECDSA_SIG **signature, uint8_t *digest, EC_KEY *key, const uint8_t *message, size_t msg_len) {
    bbp_sha256(digest, message, msg_len);
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    bbp_print_hex("digest: ", digest, 32);
#endif

    *signature = ECDSA_do_sign(digest, 32, key);
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    const BIGNUM *sig_r, *sig_s;
    ECDSA_SIG_get0(*signature, &sig_r, &sig_s);
    printf("r: %s\n", BN_bn2hex(sig_r));
    printf("s: %s\n", BN_bn2hex(sig_s));
#endif
}

int LLVMFuzzerTestOneInput(uint8_t *Data, size_t Size) {

    // We need 32 bytes for private key and at least
    // 1 byte for message to be signed
    if (Size < 33) {
        return 0;
    }

    EC_KEY *key;
    ECDSA_SIG *signature;
    uint8_t digest[32];
    int verified;
    uint8_t *priv_bytes = malloc(32);
    memcpy(priv_bytes, Data, 32);
    Data += 32;
    Size -= 32;

    const uint8_t *message = Data;

    key = bbp_ec_new_keypair(priv_bytes);
    if (!key) {
#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
        printf("Unable to create keypair\n");
#endif
        goto done;
    }

    signMessageDigest(&signature, (uint8_t *)&digest[0], key, message, Size);
    assert(signature);
    verified = ECDSA_do_verify(digest, sizeof(digest), signature, key);
    assert(verified == 1);

done:
    ECDSA_SIG_free(signature);
    EC_KEY_free(key);
    free(priv_bytes);
    return 0;
}

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    ERR_get_state();
    return 1;
}

