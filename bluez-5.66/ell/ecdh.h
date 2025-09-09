/*
 *
 *  Embedded Linux library
 *
 *  Copyright (C) 2018 Intel Corporation. All rights reserved.
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
 *
 */

#ifndef __ELL_ECDH_H
#define __ELL_ECDH_H

#ifdef __cplusplus
extern "C" {
#endif

struct l_ecc_curve;
struct l_ecc_point;
struct l_ecc_scalar;

/*
 * Generate a private/public key pair. private/public are out parameters and
 * must be freed.
 */
bool l_ecdh_generate_key_pair(const struct l_ecc_curve *curve,
					struct l_ecc_scalar **out_private,
					struct l_ecc_point **out_public);
/*
 * Generate a shared secret from a private/public key. secret is an out
 * parameters and must be freed.
 */
bool l_ecdh_generate_shared_secret(const struct l_ecc_scalar *private_key,
				const struct l_ecc_point *other_public,
				struct l_ecc_scalar **secret);

#ifdef __cplusplus
}
#endif

#endif /* __ELL_ECDH_H */
