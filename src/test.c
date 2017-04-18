/*
 * test.c - Test harness for libac
 *
 * Copyright (c) 2017		Andrew Clayton <andrew@digital-domain.net>
 *
 * Licensed under the GNU Lesser General Public License (LGPL) version 2.1
 * See COPYING
 */

#define _GNU_SOURCE			/* strdup(3), struct addrinfo */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <search.h>
#include <netdb.h>
#include <time.h>

#include "include/libac.h"

struct tnode {
	int key;
	void *data;
};

static void free_tnode(void *data)
{
	struct tnode *tn = data;

	if (!tn)
		return;

	free(tn->data);
	free(tn);
}

static void print_node(const void *data, const VISIT which, const int depth)
{
	struct tnode *tn = *(struct tnode **)data;

	printf("%d : %s\n", tn->key, (char *)tn->data);
}

static int compare(const void *pa, const void *pb)
{
	const struct tnode *tn1 = pa;
	const struct tnode *tn2 = pb;

	if (tn1->key < tn2->key)
		return -1;
	else if (tn1->key > tn2->key)
		return 1;
	else
		return 0;
}

static void btree_test(void)
{
	ac_btree_t *tree;
	struct tnode *tn;
	struct tnode stn;

	printf("*** %s\n", __func__);
	tree = ac_btree_new(compare, free_tnode);

	tn = malloc(sizeof(struct tnode));
	tn->key = 1;
	tn->data = strdup("Hello");
	ac_btree_add(tree, tn);

	tn = malloc(sizeof(struct tnode));
	tn->key = 2;
	tn->data = strdup("World");
	ac_btree_add(tree, tn);

	stn.key = 1;
	tn = ac_btree_lookup(tree, &stn);
	printf("Found tnode: %d - %s\n", tn->key, (char *)tn->data);

	stn.key = 2;
	tn = ac_btree_lookup(tree, &stn);
	printf("Found tnode: %d - %s\n", tn->key, (char *)tn->data);

	ac_btree_foreach(tree, print_node);

	ac_btree_remove(tree, &stn);
	ac_btree_destroy(tree);

	printf("*** %s\n\n", __func__);
}

static void fs_test(void)
{
	const char name1[] = "-bad name";
	const char name2[] = "a_good_name";

	printf("*** %s\n", __func__);

	if (ac_fs_is_posix_name(name1))
		printf("Good : ");
	else
		printf("Bad  : ");
	printf("'%s'\n", name1);

	if (ac_fs_is_posix_name(name2))
		printf("Good : ");
	else
		printf("Bad  : ");
	printf("'%s'\n", name2);

	printf("Creating direstory : /tmp/libac/mkdir_p/test\n");
	ac_fs_mkdir_p("/tmp/libac/mkdir_p/test");

	printf("*** %s\n\n", __func__);
}

static void misc_test(void)
{
	ac_misc_ppb_t ppb;
	int bytes = 14568264;
	u64 bytes2 = 7375982736;

	printf("*** %s\n", __func__);

	ac_misc_ppb(bytes, AC_SI_UNITS_YES, &ppb);
	if (ppb.factor == AC_MISC_PPB_BYTES)
		printf("%d bytes : %hu bytes\n", bytes, ppb.value.v_u16);
	else
		printf("%d bytes : %.2f %s\n", bytes, ppb.value.v_float,
				ppb.prefix);

	ac_misc_ppb(bytes2, AC_SI_UNITS_NO, &ppb);
	if (ppb.factor == AC_MISC_PPB_BYTES)
		printf("%" PRIu64 " bytes : %hu bytes\n", bytes2,
				ppb.value.v_u16);
	else
		printf("%" PRIu64 " bytes : %.2f %s\n", bytes2,
				ppb.value.v_float, ppb.prefix);

	printf("*** %s\n\n", __func__);
}

static bool ns_lookup_cb(const struct addrinfo *ai, const char *res)
{
	printf("\t%s\n", res);

	return true;
}

static void net_test(void)
{
	int err;
	struct addrinfo hints;

	printf("*** %s\n", __func__);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	printf("localhost ->\n");
	err = ac_net_ns_lookup_by_host(&hints, "localhost", ns_lookup_cb);
	if (err)
		perror("ac_net_ns_lookup_by_host");

	printf("::1 ->\n");
	err = ac_net_ns_lookup_by_ip(&hints, "::1", ns_lookup_cb);
	if (err)
		perror("ac_net_ns_lookup_by_ip");

	printf("*** %s\n\n", __func__);
}

static void str_test(void)
{
	char str1[] = "Hello World\r\n";
	char str2[] = "Hello World\r\n";
	char dst[32];

	printf("*** %s\n", __func__);
	printf("ac_str_chomp  : %s\n", ac_str_chomp(str1));
	printf("ac_str_substr : %s\n", ac_str_substr(str2, dst, 32, 3, 4));
	printf("*** %s\n\n", __func__);
}

static void time_test(void)
{
	int i;
	int h;
	int m;
	int s;
	struct timespec delta;
	struct _times {
		struct timespec start;
		struct timespec end;
	};
	static const struct _times times[] = {
		{ { 100, 899972154 }, { 101, 15534107 } },
		{ { 100, 250000000 }, { 100, 700000000 } },
		{ { 100, 500000000 }, { 102, 350000000 } },
		{ { 150, 250000000 }, { 151, 250000000 } },
		{ { 150, 500000000 }, { 140, 0 } },
		{ { 150, 0 }, { 140, 500000000 } },
		{ { 0, 0 }, { 0, 0} }
	};

	printf("*** %s\n", __func__);

	for (i = 0; times[i].start.tv_nsec != 0; i++) {
		double et;

		et = ac_time_tspec_diff(&delta, &times[i].end, &times[i].start);
		printf("Time difference is %f seconds\n", et);
	}

	ac_time_secs_to_hms(3675, &h, &m, &s);
	printf("%lds = %dh %dm %ds\n", 3675L, h, m, s);

	printf("*** %s\n\n", __func__);
}

int main(void)
{
	btree_test();
	fs_test();
	misc_test();
	net_test();
	str_test();
	time_test();

	exit(EXIT_SUCCESS);
}
