#ifndef CRYPT_DES_H
#define CRYPT_DES_H

#ifdef __cplusplus
extern "C" {
#endif

struct expanded_key {
	uint32_t l[16], r[16];
};

char *_crypt_des_r(const char *key, const char *salt, char *outbuf);

void __des_setkey(const unsigned char *, struct expanded_key *);
void __do_des(uint32_t, uint32_t, uint32_t *, uint32_t *,
	uint32_t, uint32_t, const struct expanded_key *);

#ifdef __cplusplus
}
#endif

#endif // CRYPT_DES_H
