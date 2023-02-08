#ifndef CRYPT_LEGACY_H
#define CRYPT_LEGACY_H

#ifdef __cplusplus
extern "C" {
#endif

void crypt_legacy(const char *key, const char *salt, char *outbuf);

#ifdef __cplusplus
}
#endif

#endif // CRYPT_LEGACY_H
