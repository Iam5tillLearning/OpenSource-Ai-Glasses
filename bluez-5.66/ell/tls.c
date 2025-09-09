/*
 *  Embedded Linux library
 *
 *  Copyright (C) 2015  Intel Corporation. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>

#include "useful.h"
#include "private.h"
#include "tls.h"
#include "checksum.h"
#include "cipher.h"
#include "random.h"
#include "queue.h"
#include "pem.h"
#include "pem-private.h"
#include "asn1-private.h"
#include "cert.h"
#include "cert-private.h"
#include "tls-private.h"
#include "key.h"
#include "strv.h"
#include "missing.h"
#include "string.h"

bool tls10_prf(const void *secret, size_t secret_len,
		const char *label,
		const void *seed, size_t seed_len,
		uint8_t *out, size_t out_len)
{
	uint8_t p_hash2[out_len];
	uint8_t l_s1 = (secret_len + 1) / 2;
	unsigned int i;

	/*
	 * RFC2246 section 5:
	 * S1 and S2 are the two halves of the secret, and each is the same
	 * length.  S1 is taken from the first half of the secret, S2 from the
	 * second half.  Their length is created by rounding up the length of
	 * the overall secret, divided by two; thus, if the original secret is
	 * an odd number of bytes long, the last byte of S1 will be the same as
	 * the first byte of S2.
	 */

	if (!tls12_prf(L_CHECKSUM_MD5, secret, l_s1,
			label, seed, seed_len,
			out, out_len))
		return false;

	if (secret_len > 0)
		secret += secret_len - l_s1;

	if (!tls12_prf(L_CHECKSUM_SHA1, secret, l_s1,
			label, seed, seed_len,
			p_hash2, out_len))
		return false;

	for (i = 0; i < out_len; i++)
		out[i] ^= p_hash2[i];

	return true;
}

bool tls12_prf(enum l_checksum_type type,
		const void *secret, size_t secret_len,
		const char *label,
		const void *seed, size_t seed_len,
		uint8_t *out, size_t out_len)
{
	struct l_checksum *hmac = l_checksum_new_hmac(type, secret, secret_len);
	size_t a_len, chunk_len, prfseed_len = strlen(label) + seed_len;
	uint8_t a[64 + prfseed_len], prfseed[prfseed_len];

	if (!hmac)
		return false;

	/* Generate the hash seed or A(0) as label + seed */
	memcpy(prfseed, label, strlen(label));
	memcpy(prfseed + strlen(label), seed, seed_len);

	memcpy(a, prfseed, prfseed_len);
	a_len = prfseed_len;

	while (out_len) {
		/* Generate A(i) */
		l_checksum_reset(hmac);
		l_checksum_update(hmac, a, a_len);
		a_len = l_checksum_get_digest(hmac, a, sizeof(a));

		/* Append seed & generate output */
		memcpy(a + a_len, prfseed, prfseed_len);
		l_checksum_reset(hmac);
		l_checksum_update(hmac, a, a_len + prfseed_len);

		chunk_len = l_checksum_get_digest(hmac, out, out_len);
		out += chunk_len;
		out_len -= chunk_len;
	}

	l_checksum_free(hmac);
	return true;
}

static bool tls_prf_get_bytes(struct l_tls *tls,
				const void *secret, size_t secret_len,
				const char *label,
				const void *seed, size_t seed_len,
				uint8_t *buf, size_t len)
{
	if (tls->negotiated_version >= L_TLS_V12)
		return tls12_prf(tls->prf_hmac->l_id,
					secret, secret_len, label,
					seed, seed_len, buf, len);
	return tls10_prf(secret, secret_len, label, seed, seed_len,
					buf, len);
}

LIB_EXPORT bool l_tls_prf_get_bytes(struct l_tls *tls, bool use_master_secret,
				const char *label, uint8_t *buf, size_t len)
{
	uint8_t seed[64];
	bool r;

	if (unlikely(!tls || !tls->prf_hmac))
		return false;

	memcpy(seed +  0, tls->pending.client_random, 32);
	memcpy(seed + 32, tls->pending.server_random, 32);

	if (use_master_secret)
		r = tls_prf_get_bytes(tls, tls->pending.master_secret, 48,
					label, seed, 64, buf, len);
	else
		r = tls_prf_get_bytes(tls, "", 0, label, seed, 64, buf, len);

	explicit_bzero(seed, 64);

	return r;
}

static void tls_write_random(uint8_t *buf)
{
	l_put_be32(time(NULL), buf);

	l_getrandom(buf + 4, 28);
}

static void tls_drop_handshake_hash(struct l_tls *tls,
					enum handshake_hash_type hash)
{
	if (tls->handshake_hash[hash]) {
		l_checksum_free(tls->handshake_hash[hash]);

		tls->handshake_hash[hash] = NULL;
	}
}

static void tls_reset_handshake(struct l_tls *tls)
{
	enum handshake_hash_type hash;

	explicit_bzero(tls->pending.key_block, sizeof(tls->pending.key_block));

	if (tls->pending.cipher_suite &&
			tls->pending.cipher_suite->key_xchg->free_params)
		tls->pending.cipher_suite->key_xchg->free_params(tls);

	l_cert_free(tls->peer_cert);
	l_key_free(tls->peer_pubkey);

	tls->peer_cert = NULL;
	tls->peer_pubkey = NULL;
	tls->peer_pubkey_size = 0;
	tls->negotiated_curve = NULL;
	tls->negotiated_ff_group = NULL;

	for (hash = 0; hash < __HANDSHAKE_HASH_COUNT; hash++)
		tls_drop_handshake_hash(tls, hash);

	TLS_SET_STATE(TLS_HANDSHAKE_WAIT_START);
	tls->cert_requested = 0;
	tls->cert_sent = 0;
}

static void tls_cleanup_handshake(struct l_tls *tls)
{
	explicit_bzero(tls->pending.client_random, 32);
	explicit_bzero(tls->pending.server_random, 32);
	explicit_bzero(tls->pending.master_secret, 48);
}

static bool tls_change_cipher_spec(struct l_tls *tls, bool txrx,
					const char **error)
{
	struct tls_bulk_encryption_algorithm *enc;
	struct tls_mac_algorithm *mac;
	int key_offset;
	static char error_buf[200];

	if (tls->cipher_type[txrx] == TLS_CIPHER_AEAD) {
		if (tls->aead_cipher[txrx]) {
			l_aead_cipher_free(tls->aead_cipher[txrx]);
			tls->aead_cipher[txrx] = NULL;
		}
	} else {
		if (tls->cipher[txrx]) {
			l_cipher_free(tls->cipher[txrx]);
			tls->cipher[txrx] = NULL;
		}
	}

	tls->cipher_type[txrx] = TLS_CIPHER_STREAM;

	if (tls->mac[txrx]) {
		l_checksum_free(tls->mac[txrx]);
		tls->mac[txrx] = NULL;
	}

	tls->mac_length[txrx] = 0;
	tls->block_length[txrx] = 0;
	tls->record_iv_length[txrx] = 0;

	if (tls->fixed_iv_length[txrx]) {
		explicit_bzero(tls->fixed_iv[txrx], tls->fixed_iv_length[txrx]);
		tls->fixed_iv_length[txrx] = 0;
	}

	tls->auth_tag_length[txrx] = 0;
	tls->seq_num[txrx] = 0;

	tls->cipher_suite[txrx] = tls->pending.cipher_suite;
	if (!tls->cipher_suite[txrx])
		return true;

	key_offset = 0;

	if (tls->cipher_suite[txrx]->mac) {
		mac = tls->cipher_suite[txrx]->mac;

		/* Server write / client read is 2nd in the key block */
		if ((tls->server && txrx) || (!tls->server && !txrx))
			key_offset += mac->mac_length;

		tls->mac[txrx] = l_checksum_new_hmac(mac->hmac_type,
						tls->pending.key_block +
						key_offset, mac->mac_length);

		/* Wipe out the now unneeded part of the key block */
		explicit_bzero(tls->pending.key_block + key_offset,
				mac->mac_length);

		if (!tls->mac[txrx]) {
			if (error) {
				*error = error_buf;
				snprintf(error_buf, sizeof(error_buf),
						"Can't create %s's %s HMAC",
						tls->cipher_suite[txrx]->name,
						txrx ? "Tx" : "Rx");
			}

			return false;
		}

		tls->mac_length[txrx] = mac->mac_length;

		key_offset = 2 * mac->mac_length;
	}

	if (tls->cipher_suite[txrx]->encryption) {
		void *cipher;

		enc = tls->cipher_suite[txrx]->encryption;

		/* Server write / client read is 4th in the key block */
		if ((tls->server && txrx) || (!tls->server && !txrx))
			key_offset += enc->key_length;

		if (enc->cipher_type == TLS_CIPHER_AEAD) {
			cipher = l_aead_cipher_new(enc->l_aead_id,
						tls->pending.key_block +
						key_offset, enc->key_length,
						enc->auth_tag_length);
			tls->aead_cipher[txrx] = cipher;
		} else {
			cipher = l_cipher_new(enc->l_id,
						tls->pending.key_block +
						key_offset, enc->key_length);
			tls->cipher[txrx] = cipher;
		}

		/* Wipe out the now unneeded part of the key block */
		explicit_bzero(tls->pending.key_block + key_offset,
				enc->key_length);

		if (!cipher) {
			if (error) {
				*error = error_buf;
				snprintf(error_buf, sizeof(error_buf),
						"Can't create %s's %s cipher",
						tls->cipher_suite[txrx]->name,
						txrx ? "Tx" : "Rx");
			}

			return false;
		}

		tls->cipher_type[txrx] = enc->cipher_type;
		tls->record_iv_length[txrx] = enc->iv_length -
			enc->fixed_iv_length;
		tls->block_length[txrx] = enc->block_length;
		tls->auth_tag_length[txrx] = enc->auth_tag_length;

		if ((tls->server && txrx) || (!tls->server && !txrx))
			key_offset += enc->key_length;
		else
			key_offset += 2 * enc->key_length;
	}

	if (tls->negotiated_version <= L_TLS_V10 &&
			tls->cipher_suite[txrx]->encryption &&
			tls->cipher_suite[txrx]->encryption->cipher_type ==
			TLS_CIPHER_BLOCK) {
		enc = tls->cipher_suite[txrx]->encryption;

		/* Server write / client read is 6th in the key block */
		if ((tls->server && txrx) || (!tls->server && !txrx))
			key_offset += enc->iv_length;

		l_cipher_set_iv(tls->cipher[txrx], tls->pending.key_block +
				key_offset, enc->iv_length);

		/* Wipe out the now unneeded part of the key block */
		explicit_bzero(tls->pending.key_block + key_offset,
				enc->iv_length);
	} else if (tls->cipher_suite[txrx]->encryption &&
			tls->cipher_suite[txrx]->encryption->fixed_iv_length) {
		enc = tls->cipher_suite[txrx]->encryption;

		/* Server write / client read is 6th in the key block */
		if ((tls->server && txrx) || (!tls->server && !txrx))
			key_offset += enc->fixed_iv_length;

		tls->fixed_iv_length[txrx] = enc->fixed_iv_length;
		memcpy(tls->fixed_iv[txrx], tls->pending.key_block + key_offset,
			enc->fixed_iv_length);

		/* Wipe out the now unneeded part of the key block */
		explicit_bzero(tls->pending.key_block + key_offset,
				enc->fixed_iv_length);
	}

	return true;
}

static void tls_reset_cipher_spec(struct l_tls *tls, bool txrx)
{
	/* Reset everything to the TLS_NULL_WITH_NULL_NULL state */

	tls->pending.cipher_suite = NULL;

	tls_change_cipher_spec(tls, txrx, NULL);
}

bool tls_cipher_suite_is_compatible(struct l_tls *tls,
					const struct tls_cipher_suite *suite,
					const char **error)
{
	static char error_buf[200];
	struct l_cert *leaf;
	enum l_tls_version min_version =
		tls->negotiated_version ?: tls->min_version;
	enum l_tls_version max_version =
		tls->negotiated_version ?: tls->max_version;

	if (suite->encryption &&
			suite->encryption->cipher_type == TLS_CIPHER_AEAD) {
		if (max_version < L_TLS_V12) {
			if (error) {
				*error = error_buf;
				snprintf(error_buf, sizeof(error_buf),
						"Cipher suite %s uses an AEAD "
						"cipher (TLS 1.2+) but "
						TLS_VER_FMT
						" was negotiated or is the max "
						"version allowed", suite->name,
						TLS_VER_ARGS(tls->max_version));
			}

			return false;
		}

		if (!l_aead_cipher_is_supported(suite->encryption->l_aead_id)) {
			if (error) {
				*error = error_buf;
				snprintf(error_buf, sizeof(error_buf),
						"Cipher suite %s's AEAD cipher "
						"algorithm not supported by "
						"the kernel", suite->name);
			}

			return false;
		}
	} else if (suite->encryption) { /* Block or stream cipher */
		if (!l_cipher_is_supported(suite->encryption->l_id)) {
			if (error) {
				*error = error_buf;
				snprintf(error_buf, sizeof(error_buf),
						"Cipher suite %s's block/stream"
						" cipher algorithm not "
						"supported by the kernel",
						suite->name);
			}

			return false;
		}
	}

	if (suite->mac &&
			!l_checksum_is_supported(suite->mac->hmac_type, true)) {
		if (error) {
			*error = error_buf;
			snprintf(error_buf, sizeof(error_buf),
					"Cipher suite %s's HMAC algorithm not "
					"supported by the kernel", suite->name);
		}

		return false;
	}

	if (
			(max_version < L_TLS_V12 &&
			 (!l_checksum_is_supported(L_CHECKSUM_MD5, true) ||
			  !l_checksum_is_supported(L_CHECKSUM_SHA1, true))) ||
			(min_version >= L_TLS_V12 &&
			 !l_checksum_is_supported(
					suite->prf_hmac != L_CHECKSUM_NONE ?
					suite->prf_hmac : L_CHECKSUM_SHA256,
					true))) {
		if (error) {
			*error = error_buf;
			snprintf(error_buf, sizeof(error_buf),
					"Cipher suite %s's PRF algorithm not "
					"supported by the kernel", suite->name);
		}

		return false;
	}

	if (suite->key_xchg->need_ffdh &&
			!l_key_is_supported(L_KEY_FEATURE_DH)) {
		if (error) {
			*error = error_buf;
			snprintf(error_buf, sizeof(error_buf),
					"Cipher suite %s's key exchange "
					"mechanism needs kernel DH support",
					suite->name);
		}

		return false;
	}

	/*
	 * If the certificate is compatible with the signature algorithm it
	 * also must be compatible with the key exchange mechanism because
	 * the cipher suites are defined so that the same certificates can
	 * be used by both.
	 */
	leaf = l_certchain_get_leaf(tls->cert);
	if (leaf && suite->signature &&
			!suite->signature->validate_cert_key_type(leaf)) {
		if (error) {
			*error = error_buf;
			snprintf(error_buf, sizeof(error_buf),
					"Local certificate has key type "
					"incompatible with cipher suite %s's "
					"signature algorithm", suite->name);
		}

		return false;
	}

	/*
	 * On the server we know what elliptic curve we'll be using as soon
	 * as we've processed the ClientHello so for EC-based key exchange
	 * methods require that a curve has been selected.
	 */
	if (suite->key_xchg->need_ecc && tls->server &&
			!tls->negotiated_curve) {
		if (error) {
			*error = error_buf;
			snprintf(error_buf, sizeof(error_buf),
					"No common supported elliptic curves "
					"with the client, can't use %s",
					suite->name);
		}

		return false;
	}

	/* Similarly for FF DH groups */
	if (suite->key_xchg->need_ffdh && tls->server &&
			!tls->negotiated_ff_group) {
		if (error) {
			*error = error_buf;
			snprintf(error_buf, sizeof(error_buf),
					"No common supported finite-field "
					"groups with the client, can't use %s",
					suite->name);
		}

		return false;
	}

	return true;
}

static struct tls_cipher_suite *tls_find_cipher_suite(const uint8_t *id)
{
	struct tls_cipher_suite **suite;

	for (suite = tls_cipher_suite_pref; *suite; suite++)
		if ((*suite)->id[0] == id[0] && (*suite)->id[1] == id[1])
			return *suite;

	return NULL;
}

static struct tls_compression_method tls_compression_pref[] = {
	{
		0,
		"CompressionMethod.null",
	},
};

static struct tls_compression_method *tls_find_compression_method(
							const uint8_t id)
{
	int i;

	for (i = 0; i < (int) L_ARRAY_SIZE(tls_compression_pref); i++)
		if (tls_compression_pref[i].id == id)
			return &tls_compression_pref[i];

	return NULL;
}

const struct tls_hash_algorithm tls_handshake_hash_data[] = {
	[HANDSHAKE_HASH_SHA384]	= { 5, HANDSHAKE_HASH_SHA384,
						L_CHECKSUM_SHA384, "SHA384" },
	[HANDSHAKE_HASH_SHA256]	= { 4, HANDSHAKE_HASH_SHA256,
						L_CHECKSUM_SHA256, "SHA256" },
	[HANDSHAKE_HASH_MD5]	= { 1, HANDSHAKE_HASH_MD5,
						L_CHECKSUM_MD5, "MD5" },
	[HANDSHAKE_HASH_SHA1]	= { 2, HANDSHAKE_HASH_SHA1,
						L_CHECKSUM_SHA1, "SHA1" },
};

static bool tls_init_handshake_hash(struct l_tls *tls)
{
	enum handshake_hash_type hash;
	bool tls10 = tls->max_version < L_TLS_V12;

	for (hash = 0; hash < __HANDSHAKE_HASH_COUNT; hash++) {
		/* Skip hash types we already know we won't need */
		if (tls10 && hash != HANDSHAKE_HASH_SHA1 &&
				hash != HANDSHAKE_HASH_MD5)
			continue;

		if (tls->handshake_hash[hash]) {
			TLS_DEBUG("Handshake hash %s already exists",
					tls_handshake_hash_data[hash].name);
			goto err;
		}

		tls->handshake_hash[hash] = l_checksum_new(
					tls_handshake_hash_data[hash].l_id);

		if (!tls->handshake_hash[hash]) {
			TLS_DEBUG("Can't create %s hash",
					tls_handshake_hash_data[hash].name);
			goto err;
		}
	}

	return true;
err:
	for (hash = 0; hash < __HANDSHAKE_HASH_COUNT; hash++)
		tls_drop_handshake_hash(tls, hash);

	return false;
}

static const struct tls_hash_algorithm *tls_set_prf_hmac(struct l_tls *tls)
{
	enum handshake_hash_type hash;

	if (tls->pending.cipher_suite->prf_hmac == L_CHECKSUM_NONE) {
		tls->prf_hmac = &tls_handshake_hash_data[HANDSHAKE_HASH_SHA256];
		return tls->prf_hmac;
	}

	for (hash = 0; hash < __HANDSHAKE_HASH_COUNT; hash++)
		if (tls_handshake_hash_data[hash].l_id ==
				tls->pending.cipher_suite->prf_hmac) {
			tls->prf_hmac = &tls_handshake_hash_data[hash];
			return tls->prf_hmac;
		}

	return NULL;
}

static bool tls_domain_match_mask(const char *name, size_t name_len,
					const char *mask, size_t mask_len)
{
	bool at_start = true;

	while (1) {
		const char *name_seg_end = memchr(name, '.', name_len);
		const char *mask_seg_end = memchr(mask, '.', mask_len);
		size_t name_seg_len = name_seg_end ?
			(size_t) (name_seg_end - name) : name_len;
		size_t mask_seg_len = mask_seg_end ?
			(size_t) (mask_seg_end - mask) : mask_len;

		if (mask_seg_len == 1 && mask[0] == '*') {
			/*
			 * A * at the beginning of the mask matches any
			 * number of labels.
			 */
			if (at_start && name_seg_end &&
					tls_domain_match_mask(name_seg_end + 1,
						name_len - name_seg_len - 1,
						mask, mask_len))
				return true;

			goto ok_next;
		}

		if (name_seg_len != mask_seg_len ||
				memcmp(name, mask, name_seg_len))
			return false;

ok_next:
		/* If either string ends here both must end here */
		if (!name_seg_end || !mask_seg_end)
			return !name_seg_end && !mask_seg_end;

		at_start = false;
		name = name_seg_end + 1;
		name_len -= name_seg_len + 1;
		mask = mask_seg_end + 1;
		mask_len -= mask_seg_len + 1;
	}
}

static const struct asn1_oid subject_alt_name_oid =
	{ 3, { 0x55, 0x1d, 0x11 } };
static const struct asn1_oid dn_common_name_oid =
	{ 3, { 0x55, 0x04, 0x03 } };

#define SAN_DNS_NAME_ID ASN1_CONTEXT_IMPLICIT(2)

static bool tls_cert_domains_match_mask(struct l_cert *cert, char **mask,
					char **error_msg)
{
	const uint8_t *san, *dn, *end;
	size_t san_len, dn_len;
	uint8_t san_tag;
	const char *cn = NULL;
	size_t cn_len;
	char **i;
	struct l_string *dns_names = NULL;
	int dns_name_count = 0;

	/*
	 * Locate SubjectAltName (RFC5280 Section 4.2.1.6) and descend into
	 * the sole SEQUENCE element, check if any DNSName matches.
	 */
	san = cert_get_extension(cert, &subject_alt_name_oid, NULL, &san_len);
	if (san) {
		san = asn1_der_find_elem(san, san_len, 0, &san_tag, &san_len);
		if (unlikely(!san || san_tag != ASN1_ID_SEQUENCE))
			goto parse_error;

		end = san + san_len;
		while (san < end) {
			const uint8_t *value;
			uint8_t tag;
			size_t len;

			value = asn1_der_find_elem(san, end - san,
							SAN_DNS_NAME_ID,
							&tag, &len);
			if (!value)
				break;

			/* Type is implicitly IA5STRING */

			for (i = mask; *i; i++)
				if (tls_domain_match_mask((const char *) value,
							len, *i, strlen(*i))) {
					l_string_free(dns_names);
					return true;
				}

			if (!dns_names) {
				dns_names = l_string_new(128);
				l_string_append(dns_names, "tried DNSName(s) ");
				l_string_append_fixed(dns_names,
							(char *) value, len);
			} else if (dns_name_count < 20) {
				l_string_append(dns_names, ", ");
				l_string_append_fixed(dns_names,
							(char *) value, len);
			}

			san = value + len;
			dns_name_count++;
		}
	}

	/*
	 * Retrieve the Common Name from the Subject DN and check if it
	 * matches.
	 *
	 * We look at the Common Name only if no DNSNames were present in
	 * the certificate, following Wi-Fi Alliance's Hotspot 2.0
	 * Specification v3.1 section 7.3.3.2 step 2:
	 * "Verify in the AAA server certificate that the domain name from
	 * the FQDN [...] is a suffix match of the domain name in at least
	 * one of the DNSName SubjectAltName extensions. If a SubjectAltName
	 * of type DNSName is not present, then the domain name from the
	 * FQDN shall be a suffix match to the CommonName portion of the
	 * SubjectName. If neither of these conditions holds, then
	 * verification fails."
	 */
	if (dns_name_count) {
		if (dns_name_count > 20)
			l_string_append_printf(dns_names, " and %i other",
						dns_name_count - 20);

		*error_msg = l_string_unwrap(dns_names);
		return false;
	}

	dn = l_cert_get_dn(cert, &dn_len);
	if (unlikely(!dn))
		goto parse_error;

	end = dn + dn_len;
	while (dn < end) {
		const uint8_t *set, *seq, *oid, *name;
		uint8_t tag;
		size_t len, oid_len, name_len;

		set = asn1_der_find_elem(dn, end - dn, 0, &tag, &len);
		if (unlikely(!set || tag != ASN1_ID_SET))
			goto parse_error;

		dn = set + len;

		seq = asn1_der_find_elem(set, len, 0, &tag, &len);
		if (unlikely(!seq || tag != ASN1_ID_SEQUENCE))
			goto parse_error;

		oid = asn1_der_find_elem(seq, len, 0, &tag, &oid_len);
		if (unlikely(!oid || tag != ASN1_ID_OID))
			goto parse_error;

		name = asn1_der_find_elem(seq, len, 1, &tag, &name_len);
		if (unlikely(!name || (tag != ASN1_ID_PRINTABLESTRING &&
					tag != ASN1_ID_UTF8STRING &&
					tag != ASN1_ID_IA5STRING)))
			continue;

		if (asn1_oid_eq(&dn_common_name_oid, oid_len, oid)) {
			cn = (const char *) name;
			cn_len = name_len;
			break;
		}
	}

	if (unlikely(!cn))
		goto parse_error;

	for (i = mask; *i; i++)
		if (tls_domain_match_mask(cn, cn_len, *i, strlen(*i)))
			return true;

	*error_msg = l_strdup_printf("tried CommonName %.*s", (int) cn_len, cn);
	return false;

parse_error:
	*error_msg = l_strdup("couldn't locate DNSName or CommonName");

	return false;
}

#define SWITCH_ENUM_TO_STR(val) \
	case (val):		\
		return L_STRINGIFY(val);

static const char *tls_handshake_type_to_str(enum tls_handshake_type type)
{
	static char buf[100];

	switch (type) {
	SWITCH_ENUM_TO_STR(TLS_HELLO_REQUEST)
	SWITCH_ENUM_TO_STR(TLS_CLIENT_HELLO)
	SWITCH_ENUM_TO_STR(TLS_SERVER_HELLO)
	SWITCH_ENUM_TO_STR(TLS_CERTIFICATE)
	SWITCH_ENUM_TO_STR(TLS_SERVER_KEY_EXCHANGE)
	SWITCH_ENUM_TO_STR(TLS_CERTIFICATE_REQUEST)
	SWITCH_ENUM_TO_STR(TLS_SERVER_HELLO_DONE)
	SWITCH_ENUM_TO_STR(TLS_CERTIFICATE_VERIFY)
	SWITCH_ENUM_TO_STR(TLS_CLIENT_KEY_EXCHANGE)
	SWITCH_ENUM_TO_STR(TLS_FINISHED)
	}

	snprintf(buf, sizeof(buf), "tls_handshake_type(%i)", type);
	return buf;
}

static void tls_send_alert(struct l_tls *tls, bool fatal,
				enum l_tls_alert_desc alert_desc)
{
	uint8_t buf[2];

	TLS_DEBUG("Sending a %s Alert: %s", fatal ? "Fatal" : "Warning",
			l_tls_alert_to_str(alert_desc));

	buf[0] = fatal ? 2 : 1;
	buf[1] = alert_desc;

	tls_tx_record(tls, TLS_CT_ALERT, buf, 2);
}

/*
 * Callers make sure this is about the last function before returning
 * from the stack frames up to the exported library call so that the
 * user-supplied disconnected callback here is free to use l_tls_free
 * for example.
 */
void tls_disconnect(struct l_tls *tls, enum l_tls_alert_desc desc,
			enum l_tls_alert_desc local_desc)
{
	tls_send_alert(tls, true, desc);

	tls_reset_handshake(tls);
	tls_cleanup_handshake(tls);

	tls_reset_cipher_spec(tls, 0);
	tls_reset_cipher_spec(tls, 1);

	tls->negotiated_version = 0;
	tls->ready = false;

	tls->disconnected(local_desc ?: desc, local_desc && !desc,
				tls->user_data);
}

void tls_tx_handshake(struct l_tls *tls, int type, uint8_t *buf, size_t length)
{
	int i;

	TLS_DEBUG("Sending a %s of %zi bytes",
			tls_handshake_type_to_str(type),
			length - TLS_HANDSHAKE_HEADER_SIZE);

	/* Fill in the handshake header */

	buf[0] = type;
	buf[1] = (length - TLS_HANDSHAKE_HEADER_SIZE) >> 16;
	buf[2] = (length - TLS_HANDSHAKE_HEADER_SIZE) >>  8;
	buf[3] = (length - TLS_HANDSHAKE_HEADER_SIZE) >>  0;

	for (i = 0; i < __HANDSHAKE_HASH_COUNT; i++)
		if (tls->handshake_hash[i])
			l_checksum_update(tls->handshake_hash[i], buf, length);

	tls_tx_record(tls, TLS_CT_HANDSHAKE, buf, length);
}

static ssize_t tls_append_hello_extensions(struct l_tls *tls,
						struct l_queue *extensions,
						uint8_t *buf, size_t len)
{
	uint8_t *ptr = buf;
	uint8_t *extensions_len_ptr = ptr;
	bool client_hello = !tls->server;
	unsigned int i = 0;
	const struct l_queue_entry *entry = l_queue_get_entries(extensions);

	if (len < 2)
		return -ENOSPC;

	ptr += 2;
	len -= 2;

	while (1) {
		const struct tls_hello_extension *extension;
		ssize_t ext_len;
		ssize_t (*ext_write)(struct l_tls *tls,
					uint8_t *buf, size_t len);

		if (client_hello) {
			extension = &tls_extensions[i++];
			if (!extension->name)
				break;

			ext_write = extension->client_write;
		} else  {
			uint16_t ext_id;

			if (!entry)
				break;

			ext_id = L_PTR_TO_UINT(entry->data);
			entry = entry->next;

			for (i = 0; tls_extensions[i].name; i++)
				if (tls_extensions[i].id == ext_id)
					break;

			extension = &tls_extensions[i];
			if (!extension->name)
				continue;

			ext_write = extension->server_write;
		}

		/*
		 * Note: could handle NULL client_write with non-NULL
		 * server_handle or server_handle_absent as "server-oriented"
		 * extension (7.4.1.4) and write empty extension_data and
		 * simliarly require empty extension_data in
		 * tls_handle_client_hello if client_handle NULL.
		 */
		if (!ext_write)
			continue;

		if (len < 4)
			return -ENOSPC;

		ext_len = ext_write(tls, ptr + 4, len - 4);
		if (ext_len == -ENOMSG)
			continue;

		if (ext_len < 0) {
			TLS_DEBUG("%s extension's %s_write: %s",
					extension->name,
					client_hello ? "client" : "server",
					strerror(-ext_len));
			return ext_len;
		}

		l_put_be16(extension->id, ptr + 0);
		l_put_be16(ext_len, ptr + 2);
		ptr += 4 + ext_len;
		len -= 4 + ext_len;
	}

	if (ptr > extensions_len_ptr + 2)
		l_put_be16(ptr - (extensions_len_ptr + 2), extensions_len_ptr);
	else /* Skip the length if no extensions */
		ptr = extensions_len_ptr;

	return ptr - buf;
}

static bool tls_send_client_hello(struct l_tls *tls)
{
	uint8_t buf[1024 + L_ARRAY_SIZE(tls_compression_pref)];
	uint8_t *ptr = buf + TLS_HANDSHAKE_HEADER_SIZE;
	uint8_t *len_ptr;
	unsigned int i;
	ssize_t r;
	struct tls_cipher_suite **suite;

	/* Fill in the Client Hello body */

	*ptr++ = (uint8_t) (tls->max_version >> 8);
	*ptr++ = (uint8_t) (tls->max_version >> 0);

	tls_write_random(tls->pending.client_random);
	memcpy(ptr, tls->pending.client_random, 32);
	ptr += 32;

	*ptr++ = 0; /* No SessionID */

	len_ptr = ptr;
	ptr += 2;

	for (suite = tls->cipher_suite_pref_list; *suite; suite++) {
		const char *error;

		if (!tls_cipher_suite_is_compatible(tls, *suite, &error)) {
			TLS_DEBUG("non-fatal: %s", error);
			continue;
		}

		*ptr++ = (*suite)->id[0];
		*ptr++ = (*suite)->id[1];
	}

	if (ptr == len_ptr + 2) {
		TLS_DEBUG("No compatible cipher suites, check kernel config, "
				"certificate's key type and TLS version range");
		return false;
	}

	l_put_be16((ptr - len_ptr - 2), len_ptr);
	*ptr++ = L_ARRAY_SIZE(tls_compression_pref);

	for (i = 0; i < L_ARRAY_SIZE(tls_compression_pref); i++)
		*ptr++ = tls_compression_pref[i].id;

	r = tls_append_hello_extensions(tls, NULL,
					ptr, buf + sizeof(buf) - ptr);
	if (r < 0)
		return false;

	ptr += r;

	tls_tx_handshake(tls, TLS_CLIENT_HELLO, buf, ptr - buf);
	return true;
}

static bool tls_send_server_hello(struct l_tls *tls, struct l_queue *extensions)
{
	uint8_t buf[1024];
	uint8_t *ptr = buf + TLS_HANDSHAKE_HEADER_SIZE;
	ssize_t r;

	/* Fill in the Server Hello body */

	*ptr++ = tls->negotiated_version >> 8;
	*ptr++ = tls->negotiated_version >> 0;

	tls_write_random(tls->pending.server_random);
	memcpy(ptr, tls->pending.server_random, 32);
	ptr += 32;

	*ptr++ = 0; /* Sessions are not cached */

	*ptr++ = tls->pending.cipher_suite->id[0];
	*ptr++ = tls->pending.cipher_suite->id[1];

	*ptr++ = tls->pending.compression_method->id;

	r = tls_append_hello_extensions(tls, extensions,
					ptr, buf + sizeof(buf) - ptr);
	if (r < 0) {
		TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
				"Error appending extensions: %s",
				strerror(-r));
		return false;
	}

	ptr += r;

	tls_tx_handshake(tls, TLS_SERVER_HELLO, buf, ptr - buf);
	return true;
}

static bool tls_cert_list_add_size(struct l_cert *cert, void *user_data)
{
	size_t *total = user_data;
	size_t der_len;

	l_cert_get_der_data(cert, &der_len);
	*total += 3 + der_len;

	return false;
}

static bool tls_cert_list_append(struct l_cert *cert, void *user_data)
{
	uint8_t **ptr = user_data;
	const uint8_t *der;
	size_t der_len;

	der = l_cert_get_der_data(cert, &der_len);
	*(*ptr)++ = der_len >> 16;
	*(*ptr)++ = der_len >>  8;
	*(*ptr)++ = der_len >>  0;
	memcpy(*ptr, der, der_len);
	*ptr += der_len;

	return false;
}

static bool tls_send_certificate(struct l_tls *tls)
{
	uint8_t *buf, *ptr;
	size_t total;

	if (tls->server && !tls->cert) {
		TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, TLS_ALERT_BAD_CERT,
				"Certificate needed in server mode");
		return false;
	}

	/*
	 * TODO: check that the certificate is compatible with hash and
	 * signature algorithms lists supplied to us in the Client Hello
	 * extensions (if we're a 1.2+ server) or in the Certificate Request
	 * (if we act as a 1.2+ client).
	 *
	 *  - for the hash and signature_algorithms list, check all
	 *    certs in the cert chain.
	 *
	 *  - also if !cipher_suite->key_xchg->key_exchange_msg, check that the
	 *    end entity certificate's key type matches and is usable with some
	 *    hash/signature pair.
	 *
	 *  - on client check if any of the supplied DNs (if any) match
	 *    anything in our cert chain.
	 */

	total = 0;
	l_certchain_walk_from_leaf(tls->cert, tls_cert_list_add_size, &total);

	buf = l_malloc(128 + total);
	ptr = buf + TLS_HANDSHAKE_HEADER_SIZE;

	/* Fill in the Certificate body */

	*ptr++ = total >> 16;
	*ptr++ = total >>  8;
	*ptr++ = total >>  0;
	l_certchain_walk_from_leaf(tls->cert, tls_cert_list_append, &ptr);

	tls_tx_handshake(tls, TLS_CERTIFICATE, buf, ptr - buf);

	l_free(buf);

	if (tls->cert)
		tls->cert_sent = true;

	return true;
}

/*
 * Note: ClientCertificateType.rsa_sign value coincides with the
 * SignatureAlgorithm.rsa value but other values in those enum are
 * different so we don't mix them, can't extract them from
 * tls->pending.cipher_suite->signature.
 */
static uint8_t tls_cert_type_pref[] = {
	1, /* RSA_sign */
};

static bool tls_send_certificate_request(struct l_tls *tls)
{
	uint8_t *buf, *ptr, *dn_ptr;
	size_t len;
	const struct l_queue_entry *entry;
	unsigned int i;
	size_t dn_total = 0;

	for (entry = l_queue_get_entries(tls->ca_certs); entry;
			entry = entry->next) {
		struct l_cert *ca_cert = entry->data;
		size_t dn_size;

		if (l_cert_get_dn(ca_cert, &dn_size))
			dn_total += 10 + dn_size;
	}

	len = 256 + L_ARRAY_SIZE(tls_cert_type_pref) + dn_total;
	buf = l_malloc(len);
	ptr = buf + TLS_HANDSHAKE_HEADER_SIZE;

	/* Fill in the Certificate Request body */

	*ptr++ = L_ARRAY_SIZE(tls_cert_type_pref);
	for (i = 0; i < L_ARRAY_SIZE(tls_cert_type_pref); i++)
		*ptr++ = tls_cert_type_pref[i];

	if (tls->negotiated_version >= L_TLS_V12) {
		ssize_t ret = tls_write_signature_algorithms(tls, ptr,
							buf + len - ptr);

		if (ret < 0) {
			TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
					"tls_write_signature_algorithms: %s",
					strerror(-ret));
			l_free(buf);
			return false;
		}

		ptr += ret;
	}

	dn_ptr = ptr;
	ptr += 2;			/* Leave space for the total DN size */

	for (entry = l_queue_get_entries(tls->ca_certs); entry;
			entry = entry->next) {
		struct l_cert *ca_cert = entry->data;
		size_t dn_size;
		const uint8_t *dn = l_cert_get_dn(ca_cert, &dn_size);
		uint8_t *cur_dn_ptr = ptr;

		if (!dn)
			continue;

		ptr += 2;		/* Leave space for current DN size */
		*ptr++ = ASN1_ID_SEQUENCE;	/* DER outer SEQUENCE tag */
		asn1_write_definite_length(&ptr, dn_size); /* length */
		memcpy(ptr, dn, dn_size);	/* value */
		ptr += dn_size;
		l_put_be16(ptr - cur_dn_ptr - 2, cur_dn_ptr);
	}

	l_put_be16(ptr - dn_ptr - 2, dn_ptr);	/* DistinguishedNames size */

	tls_tx_handshake(tls, TLS_CERTIFICATE_REQUEST, buf, ptr - buf);

	l_free(buf);

	return true;
}

static void tls_send_server_hello_done(struct l_tls *tls)
{
	uint8_t buf[32];

	/* No body */

	tls_tx_handshake(tls, TLS_SERVER_HELLO_DONE, buf,
				TLS_HANDSHAKE_HEADER_SIZE);
}

void tls_generate_master_secret(struct l_tls *tls,
				const uint8_t *pre_master_secret,
				int pre_master_secret_len)
{
	uint8_t seed[64];
	int key_block_size;

	memcpy(seed +  0, tls->pending.client_random, 32);
	memcpy(seed + 32, tls->pending.server_random, 32);

	tls_prf_get_bytes(tls, pre_master_secret, pre_master_secret_len,
				"master secret", seed, 64,
				tls->pending.master_secret, 48);

	/* Directly generate the key block while we're at it */
	key_block_size = 0;

	if (tls->pending.cipher_suite->encryption)
		key_block_size += 2 *
			tls->pending.cipher_suite->encryption->key_length;

	if (tls->pending.cipher_suite->mac)
		key_block_size += 2 *
			tls->pending.cipher_suite->mac->mac_length;

	if (tls->pending.cipher_suite->encryption &&
			tls->negotiated_version <= L_TLS_V10 &&
			tls->pending.cipher_suite->encryption->cipher_type ==
			TLS_CIPHER_BLOCK)
		key_block_size += 2 *
			tls->pending.cipher_suite->encryption->iv_length;

	if (tls->pending.cipher_suite->encryption)
		key_block_size += 2 * tls->pending.cipher_suite->encryption->
			fixed_iv_length;

	/* Reverse order from the master secret seed */
	memcpy(seed +  0, tls->pending.server_random, 32);
	memcpy(seed + 32, tls->pending.client_random, 32);

	tls_prf_get_bytes(tls, tls->pending.master_secret, 48,
				"key expansion", seed, 64,
				tls->pending.key_block, key_block_size);

	explicit_bzero(seed, 64);
}

static void tls_get_handshake_hash(struct l_tls *tls,
					enum handshake_hash_type type,
					uint8_t *out)
{
	struct l_checksum *hash = l_checksum_clone(tls->handshake_hash[type]);

	if (!hash)
		return;

	l_checksum_get_digest(hash, out, l_checksum_digest_length(
					tls_handshake_hash_data[type].l_id));
	l_checksum_free(hash);
}

static bool tls_get_handshake_hash_by_type(struct l_tls *tls,
					enum handshake_hash_type type,
					const uint8_t *data, size_t data_len,
					uint8_t *out, size_t *out_len)
{
	if (!tls->handshake_hash[type])
		return false;

	if (out_len)
		*out_len = l_checksum_digest_length(
					tls_handshake_hash_data[type].l_id);

	tls_get_handshake_hash(tls, type, out);
	return true;
}

static bool tls_send_certificate_verify(struct l_tls *tls)
{
	uint8_t buf[2048];
	int i;
	ssize_t sign_len;

	/* Fill in the Certificate Verify body */

	sign_len = tls->pending.cipher_suite->signature->sign(tls,
					buf + TLS_HANDSHAKE_HEADER_SIZE,
					2048 - TLS_HANDSHAKE_HEADER_SIZE,
					tls_get_handshake_hash_by_type,
					NULL, 0);

	if (sign_len < 0)
		return false;

	/* Stop maintaining handshake message hashes other than the PRF hash */
	if (tls->negotiated_version >= L_TLS_V12)
		for (i = 0; i < __HANDSHAKE_HASH_COUNT; i++)
			if (&tls_handshake_hash_data[i] != tls->prf_hmac)
				tls_drop_handshake_hash(tls, i);

	tls_tx_handshake(tls, TLS_CERTIFICATE_VERIFY, buf,
				sign_len + TLS_HANDSHAKE_HEADER_SIZE);

	return true;
}

static void tls_send_change_cipher_spec(struct l_tls *tls)
{
	uint8_t buf = 1;

	tls_tx_record(tls, TLS_CT_CHANGE_CIPHER_SPEC, &buf, 1);
}

static size_t tls_verify_data_length(struct l_tls *tls, unsigned int index)
{
	/*
	 * RFC 5246, Section 7.4.9:
	 *
	 * In previous versions of TLS, the verify_data was always 12 octets
	 * long.  In the current version of TLS, it depends on the cipher
	 * suite.  Any cipher suite which does not explicitly specify
	 * verify_data_length has a verify_data_length equal to 12.
	 */
	return maxsize(tls->cipher_suite[index]->verify_data_length, 12);
}

static void tls_send_finished(struct l_tls *tls)
{
	uint8_t buf[512];
	uint8_t *ptr = buf + TLS_HANDSHAKE_HEADER_SIZE;
	uint8_t seed[HANDSHAKE_HASH_MAX_SIZE * 2];
	size_t vdl = tls_verify_data_length(tls, 1);
	size_t seed_len;

	if (tls->negotiated_version >= L_TLS_V12) {
		/* Same hash type as that used for the PRF (usually SHA256) */
		tls_get_handshake_hash(tls, tls->prf_hmac->type, seed);
		seed_len = l_checksum_digest_length(tls->prf_hmac->l_id);
	} else {
		tls_get_handshake_hash(tls, HANDSHAKE_HASH_MD5, seed + 0);
		tls_get_handshake_hash(tls, HANDSHAKE_HASH_SHA1, seed + 16);
		seed_len = 36;
	}

	tls_prf_get_bytes(tls, tls->pending.master_secret, 48,
				tls->server ? "server finished" :
				"client finished",
				seed, seed_len,
				ptr, vdl);
	ptr += vdl;

	tls_tx_handshake(tls, TLS_FINISHED, buf, ptr - buf);
}

static bool tls_verify_finished(struct l_tls *tls, const uint8_t *received,
				size_t len)
{
	size_t vdl = tls_verify_data_length(tls, 0);
	uint8_t expected[vdl];
	uint8_t *seed;
	size_t seed_len;

	if (len != vdl) {
		TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
				"TLS_FINISHED length not %zu", vdl);

		return false;
	}

	if (tls->negotiated_version >= L_TLS_V12) {
		enum handshake_hash_type hash = tls->prf_hmac->type;

		seed = tls->prev_digest[hash];
		seed_len = l_checksum_digest_length(tls->prf_hmac->l_id);
	} else {
		seed = alloca(36);
		memcpy(seed + 0, tls->prev_digest[HANDSHAKE_HASH_MD5], 16);
		memcpy(seed + 16, tls->prev_digest[HANDSHAKE_HASH_SHA1], 20);
		seed_len = 36;
	}

	tls_prf_get_bytes(tls, tls->pending.master_secret, 48,
				tls->server ? "client finished" :
				"server finished",
				seed, seed_len,
				expected, vdl);

	if (memcmp(received, expected, len)) {
		TLS_DISCONNECT(TLS_ALERT_DECRYPT_ERROR, 0,
				"TLS_FINISHED contents don't match");

		return false;
	}

	return true;
}

static bool tls_ptr_match(const void *a, const void *b)
{
	return a == b;
}

static bool tls_handle_hello_extensions(struct l_tls *tls,
					const uint8_t *buf, size_t len,
					struct l_queue *seen)
{
	unsigned int i;
	const struct tls_hello_extension *extension;
	bool client_hello = tls->server;
	uint16_t extensions_size;

	if (!len)
		return true;

	if (len < 2 || len > 2 + 65535)
		goto decode_error;

	extensions_size = l_get_be16(buf);
	len -= 2;
	buf += 2;

	if (len != extensions_size)
		goto decode_error;

	while (len) {
		uint16_t ext_id;
		size_t ext_len;
		bool (*handler)(struct l_tls *tls,
				const uint8_t *buf, size_t len);

		if (len < 4)
			goto decode_error;

		ext_id = l_get_be16(buf + 0);
		ext_len = l_get_be16(buf + 2);
		buf += 4;
		len -= 4;

		if (ext_len > len)
			goto decode_error;

		/*
		 * RFC 5246, Section 7.4.1.4: "There MUST NOT be more than
		 * one extension of the same type."
		 */
		if (l_queue_find(seen, tls_ptr_match, L_UINT_TO_PTR(ext_id))) {
			TLS_DEBUG("Duplicate extension %u", ext_id);
			goto decode_error;
		}

		l_queue_push_tail(seen, L_UINT_TO_PTR(ext_id));

		extension = NULL;

		for (i = 0; tls_extensions[i].name; i++)
			if (tls_extensions[i].id == ext_id) {
				extension = &tls_extensions[i];
				break;
			}

		if (!extension)
			goto next;

		handler = client_hello ?
			extension->client_handle : extension->server_handle;

		/*
		 * RFC 5246, Section 7.4.1.4: "If a client receives an
		 * extension type in ServerHello that it did not request in
		 * the associated ClientHello, it MUST abort the handshake
		 * with an unsupported_extension fatal alert."
		 * There are however servers that include an unsolicited
		 * Supported Point Format extension where the handshake
		 * still completes fine if the extension is ignored so we
		 * do this instead.
		 */
		if (!client_hello && !handler) {
			TLS_DEBUG("non-fatal: %s extension not expected in "
					"a ServerHello", extension->name);
			goto next;
		}

		if (!handler(tls, buf, ext_len)) {
			TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
					"Hello %s extension parse error",
					extension->name);
			return false;
		}

next:
		buf += ext_len;
		len -= ext_len;
	}

	/*
	 * Trigger any actions needed when an extension is missing and its
	 * handler has not been called yet.
	 */
	for (i = 0; tls_extensions[i].name; i++) {
		bool (*handler)(struct l_tls *tls);

		extension = &tls_extensions[i];
		handler = client_hello ?
			extension->client_handle_absent :
			extension->server_handle_absent;

		if (!handler)
			continue;

		if (l_queue_find(seen, tls_ptr_match,
					L_UINT_TO_PTR(extension->id)))
			continue;

		if (!handler(tls)) {
			TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
					"Hello %s extension missing",
					extension->name);
			return false;
		}
	}

	return true;

decode_error:
	TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
			"Hello extensions decode error");
	return false;
}

static void tls_handle_client_hello(struct l_tls *tls,
					const uint8_t *buf, size_t len)
{
	uint16_t cipher_suites_size;
	uint8_t session_id_size, compression_methods_size;
	const uint8_t *cipher_suites;
	const uint8_t *compression_methods;
	int i;
	struct l_queue *extensions_offered = NULL;
	enum l_tls_alert_desc alert_desc = TLS_ALERT_HANDSHAKE_FAIL;

	/* Do we have enough for ProtocolVersion + Random + SessionID size? */
	if (len < 2 + 32 + 1)
		goto decode_error;

	memcpy(tls->pending.client_random, buf + 2, 32);
	session_id_size = buf[34];
	len -= 35;

	/*
	 * Do we have enough to hold the actual session ID + 2 byte field for
	 * cipher_suite len + minimum of a single cipher suite identifier
	 */
	if (len < (size_t) session_id_size + 4)
		goto decode_error;

	len -= session_id_size + 2;

	cipher_suites_size = l_get_be16(buf + 35 + session_id_size);
	cipher_suites = buf + 37 + session_id_size;

	/*
	 * Check that size is not odd, more than 0 and we have enough
	 * data in the packet for cipher_suites_size + 2 bytes for
	 * compression_methods_size + a single compression method
	 */
	if (len < (size_t) cipher_suites_size + 2 ||
			(cipher_suites_size & 1) || cipher_suites_size == 0)
		goto decode_error;

	len -= cipher_suites_size + 1;

	compression_methods_size = cipher_suites[cipher_suites_size];
	compression_methods = cipher_suites + cipher_suites_size + 1;

	if (len < (size_t) compression_methods_size ||
			compression_methods_size == 0)
		goto decode_error;

	len -= compression_methods_size;

	extensions_offered = l_queue_new();

	if (!tls_handle_hello_extensions(tls, compression_methods +
					compression_methods_size,
					len, extensions_offered))
		goto cleanup;

	/*
	 * Note: if the client is supplying a SessionID we know it is false
	 * because our server implementation never generates any SessionIDs
	 * yet so either the client is attempting something strange or was
	 * trying to connect somewhere else.  We might want to throw an error.
	 */

	/* Save client_version for Premaster Secret verification */
	tls->client_version = l_get_be16(buf);

	if (tls->client_version < tls->min_version) {
		TLS_DISCONNECT(TLS_ALERT_PROTOCOL_VERSION, 0,
				"Client version too low: %02x",
				tls->client_version);
		goto cleanup;
	}

	tls->negotiated_version = tls->client_version > tls->max_version ?
		tls->max_version : tls->client_version;

	/* Stop maintaining handshake message hashes other than MD1 and SHA. */
	if (tls->negotiated_version < L_TLS_V12)
		for (i = 0; i < __HANDSHAKE_HASH_COUNT; i++)
			if (i != HANDSHAKE_HASH_SHA1 && i != HANDSHAKE_HASH_MD5)
				tls_drop_handshake_hash(tls, i);

	TLS_DEBUG("Negotiated TLS " TLS_VER_FMT,
			TLS_VER_ARGS(tls->negotiated_version));

	if (!tls->cipher_suite_pref_list) {
		TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
				"No usable cipher suites");
		goto cleanup;
	}

	/* Select a cipher suite according to client's preference list */
	while (cipher_suites_size) {
		struct tls_cipher_suite *suite =
			tls_find_cipher_suite(cipher_suites);
		struct tls_cipher_suite **iter;
		const char *error;

		for (iter = tls->cipher_suite_pref_list; *iter; iter++)
			if (*iter == suite)
				break;

		if (!suite)
			TLS_DEBUG("non-fatal: Cipher suite %04x unknown",
					l_get_be16(cipher_suites));
		else if (!tls_cipher_suite_is_compatible(tls, suite, &error))
			TLS_DEBUG("non-fatal: %s", error);
		else if (!*iter) {
			/*
			 * We have at least one matching compatible suite but
			 * it is not allowed in this security profile.  If the
			 * handshake ends up failing then we blame the security
			 * profile.
			 */
			alert_desc = TLS_ALERT_INSUFFICIENT_SECURITY;
			TLS_DEBUG("non-fatal: Cipher suite %s disallowed "
					"by config", suite->name);
		} else {
			tls->pending.cipher_suite = suite;
			break;
		}

		cipher_suites += 2;
		cipher_suites_size -= 2;
	}

	if (!cipher_suites_size) {
		TLS_DISCONNECT(alert_desc, 0,
				"No common cipher suites matching negotiated "
				"TLS version and our certificate's key type");
		goto cleanup;
	}

	if (!tls_set_prf_hmac(tls)) {
		TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
				"Error selecting the PRF HMAC");
		goto cleanup;
	}

	TLS_DEBUG("Negotiated %s", tls->pending.cipher_suite->name);

	/* Select a compression method */

	/* CompressionMethod.null must be present in the vector */
	while (compression_methods_size) {
		tls->pending.compression_method =
			tls_find_compression_method(*compression_methods);

		if (tls->pending.compression_method)
			break;

		compression_methods++;
		compression_methods_size--;
	}

	if (!compression_methods_size) {
		TLS_DISCONNECT(TLS_ALERT_HANDSHAKE_FAIL, 0,
				"No common compression methods");
		goto cleanup;
	}

	TLS_DEBUG("Negotiated %s", tls->pending.compression_method->name);

	if (!tls_send_server_hello(tls, extensions_offered))
		goto cleanup;

	l_queue_destroy(extensions_offered, NULL);

	if (tls->pending.cipher_suite->signature && tls->cert)
		if (!tls_send_certificate(tls))
			return;

	if (tls->pending.cipher_suite->key_xchg->send_server_key_exchange)
		if (!tls->pending.cipher_suite->key_xchg->
				send_server_key_exchange(tls))
			return;

	/* TODO: don't bother if configured to not authenticate client */
	if (tls->pending.cipher_suite->signature && tls->ca_certs)
		if (!tls_send_certificate_request(tls))
			return;

	tls_send_server_hello_done(tls);

	if (tls->pending.cipher_suite->signature && tls->ca_certs)
		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_CERTIFICATE);
	else
		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_KEY_EXCHANGE);

	return;

decode_error:
	TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
			"ClientHello decode error");

cleanup:
	l_queue_destroy(extensions_offered, NULL);
}

static void tls_handle_server_hello(struct l_tls *tls,
					const uint8_t *buf, size_t len)
{
	uint8_t session_id_size, cipher_suite_id[2], compression_method_id;
	const char *error;
	struct tls_cipher_suite **iter;
	int i;
	struct l_queue *extensions_seen;
	bool result;

	/* Do we have enough for ProtocolVersion + Random + SessionID len ? */
	if (len < 2 + 32 + 1)
		goto decode_error;

	memcpy(tls->pending.server_random, buf + 2, 32);
	session_id_size = buf[34];
	len -= 35;

	/* Do we have enough for SessionID + CipherSuite ID + Compression ID */
	if (len < (size_t) session_id_size + 2 + 1)
		goto decode_error;

	cipher_suite_id[0] = buf[35 + session_id_size + 0];
	cipher_suite_id[1] = buf[35 + session_id_size + 1];
	compression_method_id = buf[35 + session_id_size + 2];
	len -= session_id_size + 2 + 1;

	extensions_seen = l_queue_new();
	result = tls_handle_hello_extensions(tls, buf + 38 + session_id_size,
						len, extensions_seen);
	l_queue_destroy(extensions_seen, NULL);

	if (!result)
		return;

	tls->negotiated_version = l_get_be16(buf);

	if (tls->negotiated_version < tls->min_version ||
			tls->negotiated_version > tls->max_version) {
		TLS_DISCONNECT(tls->negotiated_version < tls->min_version ?
				TLS_ALERT_PROTOCOL_VERSION :
				TLS_ALERT_ILLEGAL_PARAM, 0,
				"Unsupported version %02x",
				tls->negotiated_version);
		return;
	}

	/* Stop maintaining handshake message hashes other than MD1 and SHA. */
	if (tls->negotiated_version < L_TLS_V12)
		for (i = 0; i < __HANDSHAKE_HASH_COUNT; i++)
			if (i != HANDSHAKE_HASH_SHA1 && i != HANDSHAKE_HASH_MD5)
				tls_drop_handshake_hash(tls, i);

	TLS_DEBUG("Negotiated TLS " TLS_VER_FMT,
			TLS_VER_ARGS(tls->negotiated_version));

	/* Set the new cipher suite and compression method structs */
	tls->pending.cipher_suite = tls_find_cipher_suite(cipher_suite_id);
	if (!tls->pending.cipher_suite) {
		TLS_DISCONNECT(TLS_ALERT_HANDSHAKE_FAIL, 0,
				"Unknown cipher suite %04x",
				l_get_be16(cipher_suite_id));
		return;
	}

	for (iter = tls->cipher_suite_pref_list; *iter; iter++)
		if (*iter == tls->pending.cipher_suite)
			break;
	if (!*iter) {
		TLS_DISCONNECT(TLS_ALERT_INSUFFICIENT_SECURITY, 0,
				"Selected cipher suite %s disallowed by config",
				tls->pending.cipher_suite->name);
		return;
	}

	if (!tls_cipher_suite_is_compatible(tls, tls->pending.cipher_suite,
						&error)) {
		TLS_DISCONNECT(TLS_ALERT_HANDSHAKE_FAIL, 0,
				"Selected cipher suite not compatible: %s",
				error);
		return;
	}

	if (!tls_set_prf_hmac(tls)) {
		TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
				"Error selecting the PRF HMAC");
		return;
	}

	TLS_DEBUG("Negotiated %s", tls->pending.cipher_suite->name);

	tls->pending.compression_method =
		tls_find_compression_method(compression_method_id);
	if (!tls->pending.compression_method) {
		TLS_DISCONNECT(TLS_ALERT_HANDSHAKE_FAIL, 0,
				"Unknown compression method %i",
				compression_method_id);
		return;
	}

	TLS_DEBUG("Negotiated %s", tls->pending.compression_method->name);

	if (tls->pending.cipher_suite->signature)
		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_CERTIFICATE);
	else
		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_KEY_EXCHANGE);

	return;

decode_error:
	TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
			"ServerHello decode error");
}

static void tls_handle_certificate(struct l_tls *tls,
					const uint8_t *buf, size_t len)
{
	size_t total;
	_auto_(l_certchain_free) struct l_certchain *certchain = NULL;
	struct l_cert *leaf;
	size_t der_len;
	const uint8_t *der;
	bool dummy;
	const char *error_str;
	char *subject_str;

	if (len < 3)
		goto decode_error;

	/* Length checks */
	total = *buf++ << 16;
	total |= *buf++ << 8;
	total |= *buf++ << 0;
	if (total + 3 != len)
		goto decode_error;

	if (tls_parse_certificate_list(buf, total, &certchain) < 0) {
		TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
				"Error decoding peer certificate chain");

		return;
	}

	/*
	 * "Note that a client MAY send no certificates if it does not have any
	 * appropriate certificate to send in response to the server's
	 * authentication request." -- for now we unconditionally accept
	 * an empty certificate chain from the client.  Later on we need to
	 * make this configurable, if we don't want to authenticate the
	 * client then also don't bother sending a Certificate Request.
	 */
	if (!certchain) {
		if (!tls->server) {
			TLS_DISCONNECT(TLS_ALERT_HANDSHAKE_FAIL, 0,
					"Server sent no certificate chain");

			return;
		}

		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_KEY_EXCHANGE);

		return;
	}

	if (tls->cert_dump_path) {
		int r = pem_write_certificate_chain(certchain,
							tls->cert_dump_path);

		if (r < 0)
			TLS_DEBUG("Error %i (%s) writing the peer certchain "
					"to %s",
					-r, strerror(-r), tls->cert_dump_path);
		else
			TLS_DEBUG("Peer certchain written to %s",
					tls->cert_dump_path);
	}

	/*
	 * Validate the certificate chain's consistency and validate it
	 * against our CAs if we have any.
	 */
	if (!l_certchain_verify(certchain, tls->ca_certs, &error_str)) {
		if (tls->ca_certs) {
			TLS_DISCONNECT(TLS_ALERT_BAD_CERT, 0,
					"Peer certchain verification failed "
					"consistency check%s: %s",
					tls->ca_certs ?
					" or against local CA certs" : "",
					error_str);

			return;
		}

		/*
		 * Until the mainstream kernel can handle the occasionally
		 * used certificates without the AKID extension (both root,
		 * which is legal, and non-root, which is iffy but still
		 * happens) don't fail on peer certificate chain verification
		 * failure when CA certificates were not provided.  Knowing
		 * that the chain is self-consistent alone doesn't
		 * authenticate the peer in any way.  Only warn when it looks
		 * like the chain is bad but parses and we can get the peer
		 * public key from it below.
		 */
		TLS_DEBUG("Peer certchain verification failed (%s.)  No local "
				"CA certs provided so proceeding anyway.  This "
				"failure can signal a security issue or a "
				"known kernel problem with some certificates.",
				error_str);
	}

	/*
	 * RFC5246 7.4.2:
	 * "The end entity certificate's public key (and associated
	 * restrictions) MUST be compatible with the selected key exchange
	 * algorithm."
	 */
	leaf = l_certchain_get_leaf(certchain);
	if (!tls->pending.cipher_suite->signature->
			validate_cert_key_type(leaf)) {
		TLS_DISCONNECT(TLS_ALERT_UNSUPPORTED_CERT, 0,
				"Peer certificate key type incompatible with "
				"pending cipher suite %s",
				tls->pending.cipher_suite->name);

		return;
	}

	if (tls->subject_mask && !tls_cert_domains_match_mask(leaf,
							tls->subject_mask,
							&subject_str)) {
		char *mask = l_strjoinv(tls->subject_mask, '|');

		TLS_DISCONNECT(TLS_ALERT_BAD_CERT, 0,
				"Peer certificate's subject domain "
				"doesn't match mask %s: %s", mask, subject_str);
		l_free(mask);
		l_free(subject_str);

		return;
	}

	/* Save the end-entity certificate and free the chain */
	der = l_cert_get_der_data(leaf, &der_len);
	tls->peer_cert = l_cert_new_from_der(der, der_len);

	tls->peer_pubkey = l_cert_get_pubkey(tls->peer_cert);
	if (!tls->peer_pubkey) {
		TLS_DISCONNECT(TLS_ALERT_UNSUPPORTED_CERT, 0,
				"Error loading peer public key to kernel");

		return;
	}

	switch (l_cert_get_pubkey_type(tls->peer_cert)) {
	case L_CERT_KEY_RSA:
		if (!l_key_get_info(tls->peer_pubkey, L_KEY_RSA_PKCS1_V1_5,
				L_CHECKSUM_NONE,
				&tls->peer_pubkey_size, &dummy))
			goto pubkey_unsupported;
		break;
	case L_CERT_KEY_ECC:
		if (!l_key_get_info(tls->peer_pubkey, L_KEY_ECDSA_X962,
				L_CHECKSUM_SHA1,
				&tls->peer_pubkey_size, &dummy))
			goto pubkey_unsupported;
		break;
	case L_CERT_KEY_UNKNOWN:
		TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
				"Unknown public key type");
		return;
	}

	tls->peer_pubkey_size /= 8;

	if (tls->server || tls->pending.cipher_suite->key_xchg->
			handle_server_key_exchange)
		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_KEY_EXCHANGE);
	else
		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_HELLO_DONE);

	return;

pubkey_unsupported:
	TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
				"Can't l_key_get_info for peer public key");
	return;
decode_error:
	TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
			"TLS_CERTIFICATE decode error");
}

static void tls_handle_certificate_request(struct l_tls *tls,
						const uint8_t *buf, size_t len)
{
	unsigned int cert_type_len, dn_len, i;

	tls->cert_requested = 1;

	cert_type_len = *buf++;
	if (len < 1 + cert_type_len + 2)
		goto decode_error;

	for (i = 0; i < sizeof(tls_cert_type_pref); i++)
		if (memchr(buf, tls_cert_type_pref[i], cert_type_len))
			break;

	if (i == sizeof(tls_cert_type_pref)) {
		TLS_DISCONNECT(TLS_ALERT_UNSUPPORTED_CERT, 0,
				"Requested certificate types not supported");
		return;
	}

	buf += cert_type_len;
	len -= 1 + cert_type_len;

	/*
	 * TODO: parse and save certificate_types,
	 * supported_signature_algorithms and certificate_authorities
	 * lists for use in tls_send_certificate.
	 */

	if (tls->negotiated_version >= L_TLS_V12) {
		enum handshake_hash_type hash;
		ssize_t ret = tls_parse_signature_algorithms(tls, buf, len);

		if (ret == -ENOTSUP) {
			TLS_DISCONNECT(TLS_ALERT_UNSUPPORTED_CERT, 0,
					"No supported signature hash type");
			return;
		}

		if (ret < 0)
			goto decode_error;

		len -= ret;
		buf += ret;

		/*
		 * We can now safely stop maintaining handshake message
		 * hashes other than the PRF hash and the one selected for
		 * signing.
		 */
		for (hash = 0; hash < __HANDSHAKE_HASH_COUNT; hash++)
			if (&tls_handshake_hash_data[hash] != tls->prf_hmac &&
					hash != tls->signature_hash)
				tls_drop_handshake_hash(tls, hash);
	}

	dn_len = l_get_be16(buf);
	if (2 + dn_len != len)
		goto decode_error;

	return;

decode_error:
	TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
			"CertificateRequest decode error");
}

static void tls_handle_server_hello_done(struct l_tls *tls,
						const uint8_t *buf, size_t len)
{
	const char *error;

	if (len) {
		TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
				"ServerHello not empty");
		return;
	}

	if (tls->cert_requested)
		if (!tls_send_certificate(tls))
			return;

	if (!tls->pending.cipher_suite->key_xchg->send_client_key_exchange(tls))
		return;

	if (tls->cert_sent)
		if (!tls_send_certificate_verify(tls))
			return;

	tls_send_change_cipher_spec(tls);

	if (!tls_change_cipher_spec(tls, 1, &error)) {
		TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
				"change_cipher_spec: %s", error);
		return;
	}

	tls_send_finished(tls);

	TLS_SET_STATE(TLS_HANDSHAKE_WAIT_CHANGE_CIPHER_SPEC);
}

static bool tls_get_prev_digest_by_type(struct l_tls *tls,
					enum handshake_hash_type type,
					const uint8_t *data, size_t data_len,
					uint8_t *out, size_t *out_len)
{
	size_t len;

	if (!tls->handshake_hash[type])
		return false;

	len = l_checksum_digest_length(tls_handshake_hash_data[type].l_id);
	memcpy(out, tls->prev_digest[type], len);

	if (out_len)
		*out_len = len;

	return 0;
}

static void tls_handle_certificate_verify(struct l_tls *tls,
						const uint8_t *buf, size_t len)
{
	int i;

	if (!tls->pending.cipher_suite->signature->verify(tls, buf, len,
						tls_get_prev_digest_by_type,
						NULL, 0))
		return;

	/* Stop maintaining handshake message hashes other than the PRF hash */
	if (tls->negotiated_version >= L_TLS_V12)
		for (i = 0; i < __HANDSHAKE_HASH_COUNT; i++)
			if (&tls_handshake_hash_data[i] != tls->prf_hmac)
				tls_drop_handshake_hash(tls, i);

	/*
	 * The client's certificate is now verified based on the following
	 * logic:
	 *   - If we received an (expected) Certificate Verify, we must have
	 *     sent a Certificate Request.
	 *   - If we sent a Certificate Request that's because
	 *     tls->ca_certs is non-NULL.
	 *   - If tls->ca_certs is non-NULL then tls_handle_certificate
	 *     will have checked the whole certificate chain to be valid and
	 *     additionally trusted by our CAs if known.
	 *   - Additionally cipher_suite->signature->verify has just confirmed
	 *     that the peer owns the end-entity certificate because it was
	 *     able to sign the contents of the handshake messages and that
	 *     signature could be verified with the public key from that
	 *     certificate.
	 */
	tls->peer_authenticated = true;

	TLS_SET_STATE(TLS_HANDSHAKE_WAIT_CHANGE_CIPHER_SPEC);
}

struct dn_element_info {
	const char *str;
	const struct asn1_oid oid;
};

static const struct dn_element_info dn_elements[] = {
	{ "CN", { 3, { 0x55, 0x04, 0x03 } } },
	{ "SN", { 3, { 0x55, 0x04, 0x04 } } },
	{ "serialNumber", { 3, { 0x55, 0x04, 0x05 } } },
	{ "C", { 3, { 0x55, 0x04, 0x06 } } },
	{ "ST", { 3, { 0x55, 0x04, 0x07 } } },
	{ "L", { 3, { 0x55, 0x04, 0x08 } } },
	{ "street", { 3, { 0x55, 0x04, 0x09 } } },
	{ "O", { 3, { 0x55, 0x04, 0x0a } } },
	{ "OU", { 3, { 0x55, 0x04, 0x0b } } },
	{ "title", { 3, { 0x55, 0x04, 0x0c } } },
	{ "telephoneNumber", { 3, { 0x55, 0x04, 0x14 } } },
	{ "givenName", { 3, { 0x55, 0x04, 0x2a } } },
	{ "initials", { 3, { 0x55, 0x04, 0x2b } } },
	{ "emailAddress", {
		9,
		{ 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01 }
	} },
	{ "domainComponent", {
		10,
		{ 0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19 }
	} },
	{}
};

static void tls_str_escape_append(struct l_string *out, char *str, size_t len)
{
	while (len--) {
		switch (*str) {
		case '\\':
		case '/':
		case '=':
			l_string_append_c(out, '\\');
			l_string_append_c(out, *str);
			break;
		default:
			l_string_append_c(out, *str);
			break;
		}

		str++;
	}
}

static char *tls_get_peer_identity_str(struct l_cert *cert)
{
	const uint8_t *dn, *end;
	size_t dn_size;
	struct l_string *id_str;

	if (!cert)
		return NULL;

	dn = l_cert_get_dn(cert, &dn_size);
	if (!dn)
		return NULL;

	id_str = l_string_new(200);

	end = dn + dn_size;
	while (dn < end) {
		const uint8_t *set, *seq, *oid, *name;
		uint8_t tag;
		size_t len, oid_len, name_len;
		const struct dn_element_info *info;

		set = asn1_der_find_elem(dn, end - dn, 0, &tag, &len);
		if (!set || tag != ASN1_ID_SET)
			goto error;

		dn = set + len;

		seq = asn1_der_find_elem(set, len, 0, &tag, &len);
		if (!seq || tag != ASN1_ID_SEQUENCE)
			goto error;

		oid = asn1_der_find_elem(seq, len, 0, &tag, &oid_len);
		if (!oid || tag != ASN1_ID_OID)
			goto error;

		name = asn1_der_find_elem(seq, len, 1, &tag, &name_len);
		if (!name || (tag != ASN1_ID_PRINTABLESTRING &&
					tag != ASN1_ID_UTF8STRING &&
					tag != ASN1_ID_IA5STRING))
			continue;

		for (info = dn_elements; info->str; info++)
			if (asn1_oid_eq(&info->oid, oid_len, oid))
				break;
		if (!info->str)
			continue;

		l_string_append_c(id_str, '/');
		l_string_append(id_str, info->str);
		l_string_append_c(id_str, '=');
		tls_str_escape_append(id_str, (char *) name, name_len);
	}

	return l_string_unwrap(id_str);

error:
	l_string_free(id_str);
	return NULL;
}

static void tls_finished(struct l_tls *tls)
{
	char *peer_identity = NULL;

	if (tls->peer_authenticated) {
		peer_identity = tls_get_peer_identity_str(tls->peer_cert);
		if (!peer_identity) {
			TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
					"tls_get_peer_identity_str failed");
			return;
		}
	}

	/* Free up the resources used in the handshake */
	tls_reset_handshake(tls);

	TLS_SET_STATE(TLS_HANDSHAKE_DONE);
	tls->ready = true;

	tls->in_callback = true;
	tls->ready_handle(peer_identity, tls->user_data);
	tls->in_callback = false;
	l_free(peer_identity);

	tls_cleanup_handshake(tls);
}

static void tls_handle_handshake(struct l_tls *tls, int type,
					const uint8_t *buf, size_t len)
{
	TLS_DEBUG("Handling a %s of %zi bytes",
			tls_handshake_type_to_str(type), len);

	switch (type) {
	case TLS_HELLO_REQUEST:
		if (tls->server) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in server mode");
			break;
		}

		if (len != 0) {
			TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
					"HelloRequest not empty");
			break;
		}

		/*
		 * May be sent by the server at any time but "SHOULD be ignored
		 * by the client if it arrives in the middle of a handshake"
		 * and "MAY be ignored by the client if it does not wish to
		 * renegotiate a session".
		 */

		break;

	case TLS_CLIENT_HELLO:
		if (!tls->server) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in client mode");
			break;
		}

		if (tls->state != TLS_HANDSHAKE_WAIT_HELLO &&
				tls->state != TLS_HANDSHAKE_DONE) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in state %s",
					tls_handshake_state_to_str(tls->state));
			break;
		}

		tls_handle_client_hello(tls, buf, len);

		break;

	case TLS_SERVER_HELLO:
		if (tls->server) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in server mode");
			break;
		}

		if (tls->state != TLS_HANDSHAKE_WAIT_HELLO) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in state %s",
					tls_handshake_state_to_str(tls->state));
			break;
		}

		tls_handle_server_hello(tls, buf, len);

		break;

	case TLS_CERTIFICATE:
		if (tls->state != TLS_HANDSHAKE_WAIT_CERTIFICATE) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in state %s",
					tls_handshake_state_to_str(tls->state));
			break;
		}

		tls_handle_certificate(tls, buf, len);

		break;

	case TLS_SERVER_KEY_EXCHANGE:
		if (tls->server) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in server mode");
			break;
		}

		if (tls->state != TLS_HANDSHAKE_WAIT_KEY_EXCHANGE) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in state %s",
					tls_handshake_state_to_str(tls->state));
			break;
		}

		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_HELLO_DONE);

		tls->pending.cipher_suite->key_xchg->handle_server_key_exchange(
								tls, buf, len);

		break;

	case TLS_CERTIFICATE_REQUEST:
		if (tls->server) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in server mode");
			break;
		}

		/*
		 * Server sends this optionally so in the WAIT_HELLO_DONE
		 * state we accept either this or a Server Hello Done (below).
		 */
		if (tls->state != TLS_HANDSHAKE_WAIT_HELLO_DONE ||
				tls->cert_requested ||
				!tls->pending.cipher_suite->signature) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in current state "
					"or certificate check not supported "
					"in pending cipher suite");
			break;
		}

		tls_handle_certificate_request(tls, buf, len);

		break;

	case TLS_SERVER_HELLO_DONE:
		if (tls->state != TLS_HANDSHAKE_WAIT_HELLO_DONE) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in state %s",
					tls_handshake_state_to_str(tls->state));
			break;
		}

		tls_handle_server_hello_done(tls, buf, len);

		break;

	case TLS_CERTIFICATE_VERIFY:
		if (tls->state != TLS_HANDSHAKE_WAIT_CERTIFICATE_VERIFY) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in state %s",
					tls_handshake_state_to_str(tls->state));
			break;
		}

		tls_handle_certificate_verify(tls, buf, len);

		break;

	case TLS_CLIENT_KEY_EXCHANGE:
		if (!tls->server) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in client mode");
			break;
		}

		if (tls->state != TLS_HANDSHAKE_WAIT_KEY_EXCHANGE) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in state %s",
					tls_handshake_state_to_str(tls->state));
			break;
		}

		/*
		 * If we accepted a client Certificate message with a
		 * certificate that has signing capability (TODO: check
		 * usage bitmask), Certificate Verify is received next.  It
		 * sounds as if this is mandatory for the client although
		 * this isn't 100% clear.
		 */
		if (tls->peer_pubkey)
			TLS_SET_STATE(TLS_HANDSHAKE_WAIT_CERTIFICATE_VERIFY);
		else
			TLS_SET_STATE(TLS_HANDSHAKE_WAIT_CHANGE_CIPHER_SPEC);

		tls->pending.cipher_suite->key_xchg->handle_client_key_exchange(
								tls, buf, len);

		break;

	case TLS_FINISHED:
		if (tls->state != TLS_HANDSHAKE_WAIT_FINISHED) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Message invalid in state %s",
					tls_handshake_state_to_str(tls->state));
			break;
		}

		if (!tls_verify_finished(tls, buf, len))
			break;

		if (tls->server) {
			const char *error;

			tls_send_change_cipher_spec(tls);
			if (!tls_change_cipher_spec(tls, 1, &error)) {
				TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
						"change_cipher_spec: %s",
						error);
				break;
			}
			tls_send_finished(tls);
		}

		/*
		 * On the client, the server's certificate is now verified
		 * regardless of the key exchange method, based on the
		 * following logic:
		 *
		 *  - tls->ca_certs is non-NULL so tls_handle_certificate
		 *    (always called on the client) must have veritifed the
		 *    server's certificate chain to be valid and additionally
		 *    trusted by our CA.
		 *
		 *  - the peer owns the end-entity certificate because:
		 *    either:
		 *
		 *    * (RSA key exchange algorithm case) the correct
		 *      receival of this Finished message confirms the
		 *      posession of the master secret, it is verified by
		 *      both the successful decryption and the MAC of this
		 *      message (either should be enough) because we entered
		 *      the TLS_HANDSHAKE_WAIT_FINISHED state only after
		 *      encryption and MAC were enabled in ChangeCipherSpec.
		 *      To obtain the master secret the server must have been
		 *      able to decrypt the pre_master_secret which we had
		 *      encrypted with the public key from that certificate.
		 *
		 *    * (ECDHE and DHE key exchange algorithms) server was
		 *      able to sign the client random together with the
		 *      ServerKeyExchange parameters using its certified key
		 *      pair.
		 */
		if (!tls->server && tls->cipher_suite[0]->signature &&
				tls->ca_certs)
			tls->peer_authenticated = true;

		tls_finished(tls);

		break;

	default:
		TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
				"Invalid message");
	}
}

LIB_EXPORT struct l_tls *l_tls_new(bool server,
				l_tls_write_cb_t app_data_handler,
				l_tls_write_cb_t tx_handler,
				l_tls_ready_cb_t ready_handler,
				l_tls_disconnect_cb_t disconnect_handler,
				void *user_data)
{
	struct l_tls *tls;

	if (!l_key_is_supported(L_KEY_FEATURE_CRYPTO))
		return NULL;

	tls = l_new(struct l_tls, 1);
	tls->server = server;
	tls->rx = app_data_handler;
	tls->tx = tx_handler;
	tls->ready_handle = ready_handler;
	tls->disconnected = disconnect_handler;
	tls->user_data = user_data;
	tls->cipher_suite_pref_list = tls_cipher_suite_pref;
	tls->min_version = TLS_MIN_VERSION;
	tls->max_version = TLS_MAX_VERSION;

	/* If we're the server wait for the Client Hello already */
	if (tls->server)
		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_HELLO);
	else
		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_START);

	return tls;
}

LIB_EXPORT void l_tls_free(struct l_tls *tls)
{
	enum handshake_hash_type hash;

	if (unlikely(!tls))
		return;

	if (tls->in_callback) {
		tls->pending_destroy = true;
		return;
	}

	l_tls_set_cacert(tls, NULL);
	l_tls_set_auth_data(tls, NULL, NULL);
	l_tls_set_domain_mask(tls, NULL);
	l_tls_set_cert_dump_path(tls, NULL);

	tls_reset_handshake(tls);
	tls_cleanup_handshake(tls);

	tls_reset_cipher_spec(tls, 0);
	tls_reset_cipher_spec(tls, 1);

	if (tls->record_buf)
		l_free(tls->record_buf);

	if (tls->message_buf)
		l_free(tls->message_buf);

	for (hash = 0; hash < __HANDSHAKE_HASH_COUNT; hash++)
		tls_drop_handshake_hash(tls, hash);

	if (tls->debug_destroy)
		tls->debug_destroy(tls->debug_data);

	if (tls->cipher_suite_pref_list != tls_cipher_suite_pref)
		l_free(tls->cipher_suite_pref_list);

	l_free(tls);
}

LIB_EXPORT void l_tls_write(struct l_tls *tls, const uint8_t *data, size_t len)
{
	if (unlikely(!tls->ready)) {
		return;
	}

	tls_tx_record(tls, TLS_CT_APPLICATION_DATA, data, len);
}

bool tls_handle_message(struct l_tls *tls, const uint8_t *message,
			int len, enum tls_content_type type, uint16_t version)
{
	enum handshake_hash_type hash;
	const char *error;

	switch (type) {
	case TLS_CT_CHANGE_CIPHER_SPEC:
		if (len != 1 || message[0] != 0x01) {
			TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
					"ChangeCipherSpec msg decode error");

			return false;
		}

		if (tls->state != TLS_HANDSHAKE_WAIT_CHANGE_CIPHER_SPEC) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"ChangeCipherSpec invalid in state %s",
					tls_handshake_state_to_str(tls->state));

			return false;
		}

		if (!tls_change_cipher_spec(tls, 0, &error)) {
			TLS_DISCONNECT(TLS_ALERT_INTERNAL_ERROR, 0,
					"change_cipher_spec: %s", error);

			return false;
		}

		TLS_SET_STATE(TLS_HANDSHAKE_WAIT_FINISHED);

		return true;

	case TLS_CT_ALERT:
		/* Verify AlertLevel */
		if (message[0] != 0x01 && message[0] != 0x02) {
			TLS_DISCONNECT(TLS_ALERT_DECODE_ERROR, 0,
					"Received bad AlertLevel %i",
					message[0]);

			return false;
		}

		/*
		 * On a fatal alert we are obligated to respond with a
		 * fatal alert and disconnect but also not complain if
		 * the connection has been torn down by the peer before
		 * we were able to send our alert.  However on a non-fatal
		 * alert (warning) we're also allowed to panic and send
		 * a fatal alert, then disconnect, so we do that
		 * regardless of the alert level.
		 */
		TLS_DISCONNECT(TLS_ALERT_CLOSE_NOTIFY, message[1],
				"Peer sent a %s Alert: %s",
				message[0] == 0x02 ? "Fatal" : "Warning",
				l_tls_alert_to_str(message[1]));

		return false;

	case TLS_CT_HANDSHAKE:
		/* Start hashing the handshake contents on first message */
		if (tls->server && message[0] == TLS_CLIENT_HELLO &&
				(tls->state == TLS_HANDSHAKE_WAIT_HELLO ||
				 tls->state == TLS_HANDSHAKE_DONE))
			if (!tls_init_handshake_hash(tls))
				return false;

		/*
		 * Corner case: When handling a Certificate Verify or a
		 * Finished message we need access to the messages hash from
		 * before this message was transmitted on the Tx side so we
		 * can verify it matches the hash the sender included in the
		 * message.  We save it here for that purpose.  Everywhere
		 * else we need to update the hash before handling the new
		 * message because 1. we may need the new hash to build our
		 * own Certificate Verify or Finished messages, and 2. we
		 * update the message hash with newly transmitted messages
		 * inside tls_tx_handshake which may be called as part of
		 * handling incoming message, and if we didn't call
		 * l_checksum_update before, the calls would end up being
		 * out of order.
		 */
		if (message[0] == TLS_CERTIFICATE_VERIFY ||
				message[0] == TLS_FINISHED)
			for (hash = 0; hash < __HANDSHAKE_HASH_COUNT; hash++) {
				if (!tls->handshake_hash[hash])
					continue;

				tls_get_handshake_hash(tls, hash,
							tls->prev_digest[hash]);
			}

		/*
		 * RFC 5246, Section 7.4.1.1:
		 * This message MUST NOT be included in the message hashes
		 * that are maintained throughout the handshake and used in
		 * the Finished messages and the certificate verify message.
		 */
		if (message[0] != TLS_HELLO_REQUEST)
			for (hash = 0; hash < __HANDSHAKE_HASH_COUNT; hash++) {
				if (!tls->handshake_hash[hash])
					continue;

				l_checksum_update(tls->handshake_hash[hash],
							message, len);
			}

		tls_handle_handshake(tls, message[0],
					message + TLS_HANDSHAKE_HEADER_SIZE,
					len - TLS_HANDSHAKE_HEADER_SIZE);

		if (tls->pending_destroy) {
			l_tls_free(tls);
			return false;
		}

		return true;

	case TLS_CT_APPLICATION_DATA:
		if (!tls->ready) {
			TLS_DISCONNECT(TLS_ALERT_UNEXPECTED_MESSAGE, 0,
					"Application data message before "
					"handshake finished");

			return false;
		}

		if (!len)
			return true;

		tls->in_callback = true;
		tls->rx(message, len, tls->user_data);
		tls->in_callback = false;

		if (tls->pending_destroy) {
			l_tls_free(tls);
			return false;
		}

		return true;
	}

	return false;
}

LIB_EXPORT bool l_tls_start(struct l_tls *tls)
{
	if (tls->max_version < tls->min_version)
		return false;

	if (!tls->cipher_suite_pref_list)
		return false;

	/* This is a nop in server mode */
	if (tls->server)
		return true;

	if (tls->state != TLS_HANDSHAKE_WAIT_START) {
		TLS_DEBUG("Call invalid in state %s",
				tls_handshake_state_to_str(tls->state));
		return false;
	}

	if (!tls_init_handshake_hash(tls))
		return false;

	if (!tls_send_client_hello(tls))
		return false;

	TLS_SET_STATE(TLS_HANDSHAKE_WAIT_HELLO);
	return true;
}

LIB_EXPORT void l_tls_close(struct l_tls *tls)
{
	tls->record_buf_len = 0;
	tls->message_buf_len = 0;

	TLS_DISCONNECT(TLS_ALERT_CLOSE_NOTIFY, 0, "Closing session");
}

LIB_EXPORT void l_tls_reset(struct l_tls *tls)
{
	/*
	 * Similar to l_tls_close but without sending the alert or a
	 * disconnect callback.
	 */

	tls_reset_handshake(tls);
	tls_cleanup_handshake(tls);

	tls_reset_cipher_spec(tls, 0);
	tls_reset_cipher_spec(tls, 1);

	tls->negotiated_version = 0;
	tls->ready = false;
	tls->record_flush = true;
	tls->record_buf_len = 0;
	tls->message_buf_len = 0;
}

LIB_EXPORT bool l_tls_set_cacert(struct l_tls *tls, struct l_queue *ca_certs)
{
	if (tls->ca_certs) {
		l_queue_destroy(tls->ca_certs,
				(l_queue_destroy_func_t) l_cert_free);
		tls->ca_certs = NULL;
	}

	if (ca_certs) {
		if (!l_key_is_supported(L_KEY_FEATURE_RESTRICT)) {
			TLS_DEBUG("keyctl restrict support missing, "
					"check kernel configuration");
			return false;
		}

		tls->ca_certs = ca_certs;
	}

	return true;
}

LIB_EXPORT bool l_tls_set_auth_data(struct l_tls *tls,
					struct l_certchain *certchain,
					struct l_key *priv_key)
{
	if (tls->cert) {
		l_certchain_free(tls->cert);
		tls->cert = NULL;
	}

	if (tls->priv_key) {
		l_key_free(tls->priv_key);
		tls->priv_key = NULL;
		tls->priv_key_size = 0;
	}

	if (certchain)
		tls->cert = certchain;

	if (priv_key) {
		bool is_public = true;

		tls->priv_key = priv_key;

		if (!l_key_get_info(tls->priv_key, L_KEY_RSA_PKCS1_V1_5,
					L_CHECKSUM_NONE, &tls->priv_key_size,
					&is_public) || is_public) {
			TLS_DEBUG("Not a private key or l_key_get_info failed");
			tls->cert = NULL;
			tls->priv_key = NULL;
			tls->priv_key_size = 0;
			return false;
		}

		tls->priv_key_size /= 8;
	}

	return true;
}

bool tls_set_cipher_suites(struct l_tls *tls, const char **suite_list)
{
	struct tls_cipher_suite **suite;

	if (tls->cipher_suite_pref_list != tls_cipher_suite_pref)
		l_free(tls->cipher_suite_pref_list);

	if (!suite_list) {
		/* Use our default cipher suite preference list */
		tls->cipher_suite_pref_list = tls_cipher_suite_pref;
		return true;
	}

	tls->cipher_suite_pref_list = l_new(struct tls_cipher_suite *,
				l_strv_length((char **) suite_list) + 1);
	suite = tls->cipher_suite_pref_list;

	for (; *suite_list; suite_list++) {
		unsigned int i;

		for (i = 0; tls_cipher_suite_pref[i]; i++)
			if (!strcmp(tls_cipher_suite_pref[i]->name,
						*suite_list))
				break;

		if (tls_cipher_suite_pref[i])
			*suite++ = tls_cipher_suite_pref[i];
		else
			TLS_DEBUG("Cipher suite %s is not supported",
					*suite_list);
	}

	if (suite > tls->cipher_suite_pref_list)
		return true;

	TLS_DEBUG("None of the supplied suite names is supported");
	l_free(suite);
	tls->cipher_suite_pref_list = NULL;
	return false;
}

LIB_EXPORT void l_tls_set_version_range(struct l_tls *tls,
					enum l_tls_version min_version,
					enum l_tls_version max_version)
{
	tls->min_version =
		(min_version && min_version > TLS_MIN_VERSION) ?
		min_version : TLS_MIN_VERSION;
	tls->max_version =
		(max_version && max_version < TLS_MAX_VERSION) ?
		max_version : TLS_MAX_VERSION;
}

/**
 * l_tls_set_domain_mask:
 * @tls: TLS object being configured
 * @mask: NULL-terminated array of domain masks
 *
 * Sets a mask for domain names contained in the peer certificate
 * (eg. the subject Common Name) to be matched against.  If none of the
 * domains match the any mask, authentication will fail.  At least one
 * domain has to match at least one mask from the list.
 *
 * The masks are each split into segments at the dot characters and each
 * segment must match the corresponding label of the domain name --
 * a domain name is a sequence of labels joined by dots.  An asterisk
 * segment in the mask matches any label.  An asterisk segment at the
 * beginning of the mask matches one or more consecutive labels from
 * the beginning of the domain string.
 */
LIB_EXPORT void l_tls_set_domain_mask(struct l_tls *tls, char **mask)
{
	l_strv_free(tls->subject_mask);

	tls->subject_mask = l_strv_copy(mask);
}

LIB_EXPORT const char *l_tls_alert_to_str(enum l_tls_alert_desc desc)
{
	switch (desc) {
	case TLS_ALERT_CLOSE_NOTIFY:
		return "close_notify";
	case TLS_ALERT_UNEXPECTED_MESSAGE:
		return "unexpected_message";
	case TLS_ALERT_BAD_RECORD_MAC:
		return "bad_record_mac";
	case TLS_ALERT_DECRYPT_FAIL_RESERVED:
		return "decryption_failure_RESERVED";
	case TLS_ALERT_RECORD_OVERFLOW:
		return "record_overflow";
	case TLS_ALERT_DECOMPRESS_FAIL:
		return "decompression_failure";
	case TLS_ALERT_HANDSHAKE_FAIL:
		return "handshake_failure";
	case TLS_ALERT_NO_CERT_RESERVED:
		return "no_certificate_RESERVED";
	case TLS_ALERT_BAD_CERT:
		return "bad_certificate";
	case TLS_ALERT_UNSUPPORTED_CERT:
		return "unsupported_certificate";
	case TLS_ALERT_CERT_REVOKED:
		return "certificate_revoked";
	case TLS_ALERT_CERT_EXPIRED:
		return "certificate_expired";
	case TLS_ALERT_CERT_UNKNOWN:
		return "certificate_unknown";
	case TLS_ALERT_ILLEGAL_PARAM:
		return "illegal_parameter";
	case TLS_ALERT_UNKNOWN_CA:
		return "unknown_ca";
	case TLS_ALERT_ACCESS_DENIED:
		return "access_denied";
	case TLS_ALERT_DECODE_ERROR:
		return "decode_error";
	case TLS_ALERT_DECRYPT_ERROR:
		return "decrypt_error";
	case TLS_ALERT_EXPORT_RES_RESERVED:
		return "export_restriction_RESERVED";
	case TLS_ALERT_PROTOCOL_VERSION:
		return "protocol_version";
	case TLS_ALERT_INSUFFICIENT_SECURITY:
		return "insufficient_security";
	case TLS_ALERT_INTERNAL_ERROR:
		return "internal_error";
	case TLS_ALERT_USER_CANCELED:
		return "user_canceled";
	case TLS_ALERT_NO_RENEGOTIATION:
		return "no_renegotiation";
	case TLS_ALERT_UNSUPPORTED_EXTENSION:
		return "unsupported_extension";
	}

	return NULL;
}

const char *tls_handshake_state_to_str(enum tls_handshake_state state)
{
	static char buf[100];

	switch (state) {
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_WAIT_START)
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_WAIT_HELLO)
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_WAIT_CERTIFICATE)
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_WAIT_KEY_EXCHANGE)
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_WAIT_HELLO_DONE)
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_WAIT_CERTIFICATE_VERIFY)
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_WAIT_CHANGE_CIPHER_SPEC)
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_WAIT_FINISHED)
	SWITCH_ENUM_TO_STR(TLS_HANDSHAKE_DONE)
	}

	snprintf(buf, sizeof(buf), "tls_handshake_state(%i)", state);
	return buf;
}

int tls_parse_certificate_list(const void *data, size_t len,
				struct l_certchain **out_certchain)
{
	const uint8_t *buf = data;
	struct l_certchain *chain = NULL;

	while (len) {
		struct l_cert *cert;
		size_t cert_len;

		if (len < 3)
			goto decode_error;

		cert_len = *buf++ << 16;
		cert_len |= *buf++ << 8;
		cert_len |= *buf++ << 0;

		if (cert_len + 3 > len)
			goto decode_error;

		cert = l_cert_new_from_der(buf, cert_len);
		if (!cert)
			goto decode_error;

		if (!chain) {
			chain = certchain_new_from_leaf(cert);
			if (!chain)
				goto decode_error;
		} else
			certchain_link_issuer(chain, cert);

		buf += cert_len;
		len -= cert_len + 3;
	}

	if (out_certchain)
		*out_certchain = chain;
	else
		l_certchain_free(chain);

	return 0;

decode_error:
	l_certchain_free(chain);
	return -EBADMSG;
}

LIB_EXPORT bool l_tls_set_debug(struct l_tls *tls, l_tls_debug_cb_t function,
				void *user_data, l_tls_destroy_cb_t destroy)
{
	if (unlikely(!tls))
		return false;

	if (tls->debug_destroy)
		tls->debug_destroy(tls->debug_data);

	tls->debug_handler = function;
	tls->debug_destroy = destroy;
	tls->debug_data = user_data;

	return true;
}

LIB_EXPORT bool l_tls_set_cert_dump_path(struct l_tls *tls, const char *path)
{
	l_free(tls->cert_dump_path);
	tls->cert_dump_path = path ? l_strdup(path) : NULL;
	return true;
}
