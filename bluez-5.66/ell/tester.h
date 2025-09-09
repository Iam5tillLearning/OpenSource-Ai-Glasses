/*
 *
 *  Embedded Linux library
 *
 *  Copyright (C) 2021  Intel Corporation. All rights reserved.
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

#ifndef __ELL_TESTER_H
#define __ELL_TESTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct l_tester;

enum l_tester_stage {
	L_TESTER_STAGE_INVALID,
	L_TESTER_STAGE_PRE_SETUP,
	L_TESTER_STAGE_SETUP,
	L_TESTER_STAGE_RUN,
	L_TESTER_STAGE_TEARDOWN,
	L_TESTER_STAGE_POST_TEARDOWN,
};

typedef void (*l_tester_destroy_func_t)(void *user_data);
typedef void (*l_tester_data_func_t)(const void *test_data);
typedef void (*l_tester_finish_func_t)(struct l_tester *tester);
typedef void (*l_tester_wait_func_t)(void *user_data);

struct l_tester *l_tester_new(const char *prefix, const char *substring,
							bool list_cases);
void l_tester_destroy(struct l_tester *tester);
void l_tester_start(struct l_tester *tester,
					l_tester_finish_func_t finish_func);

bool l_tester_summarize(struct l_tester *tester);

void l_tester_add_full(struct l_tester *tester, const char *name,
				const void *test_data,
				l_tester_data_func_t pre_setup_func,
				l_tester_data_func_t setup_func,
				l_tester_data_func_t test_func,
				l_tester_data_func_t teardown_func,
				l_tester_data_func_t post_teardown_func,
				unsigned int timeout,
				void *user_data,
				l_tester_destroy_func_t destroy);

void l_tester_add(struct l_tester *tester, const char *name,
					const void *test_data,
					l_tester_data_func_t setup_func,
					l_tester_data_func_t test_func,
					l_tester_data_func_t teardown_func);

void l_tester_pre_setup_complete(struct l_tester *tester);
void l_tester_pre_setup_failed(struct l_tester *tester);
void l_tester_setup_complete(struct l_tester *tester);
void l_tester_setup_failed(struct l_tester *tester);
void l_tester_test_passed(struct l_tester *tester);
void l_tester_test_failed(struct l_tester *tester);
void l_tester_test_abort(struct l_tester *tester);
void l_tester_teardown_complete(struct l_tester *tester);
void l_tester_teardown_failed(struct l_tester *tester);
void l_tester_post_teardown_complete(struct l_tester *tester);
void l_tester_post_teardown_failed(struct l_tester *tester);

enum l_tester_stage l_tester_get_stage(struct l_tester *tester);
void *l_tester_get_data(struct l_tester *tester);

void l_tester_wait(struct l_tester *tester, unsigned int seconds,
				l_tester_wait_func_t func, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* __ELL_TESTER_H */
