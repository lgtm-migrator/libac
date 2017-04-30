/*
 * ac_slist.c - Singly linked list
 *
 * Copyright (c) 2017		Andrew Clayton <andrew@digital-domain.net>
 *
 * Licensed under the GNU Lesser General Public License (LGPL) version 2.1
 * See COPYING
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdbool.h>

#include "include/libac.h"

static ac_slist_t *slist_last(ac_slist_t *list)
{
	while (list->next)
		list = list->next;

	return list;
}

/**
 * ac_slist_add - add an item to the end of the list
 *
 * @list: The list to add the item to
 * @data: The data to add
 */
void ac_slist_add(ac_slist_t **list, void *data)
{
	ac_slist_t *new = malloc(sizeof(ac_slist_t));

	new->data = data;
	new->next = NULL;

	if (*list) {
		ac_slist_t *last = slist_last(*list);

		last->next = new;
	} else {
		*list = new;
	}
}

/**
 * ac_slist_preadd - add an item to the front of the list
 *
 * @list: The list to add the item to
 * @data: The data to add
 *
 * This would be quicker than ac_slist_add when adding multiple items,
 * so it may be better to use this function and then ac_slist_reverse()
 */
void ac_slist_preadd(ac_slist_t **list, void *data)
{
	ac_slist_t *new = malloc(sizeof(ac_slist_t));

	new->data = data;
	if (*list)
		new->next = *list;
	else
		new->next = NULL;

	*list = new;
}

/**
 * ac_slist_remove - remove an item from the list
 *
 * @list: The list to remove the item from
 * @data: The data to be removed
 *
 * Returns:
 *
 * true if the item was found and removed, false otherwise
 */
bool ac_slist_remove(ac_slist_t **list, void *data, void (*free_data)
							 (void *data))
{
	ac_slist_t **pp = list;
	ac_slist_t *p;
	bool ret = false;

	while ((p = *pp) != NULL) {
		if (p->data == data) {
			*pp = p->next;

			if (free_data)
				free_data(p->data);
			free(p);
			ret = true;
			break;
		}
		pp = &p->next;
	}

	return ret;
}

/**
 * ac_slist_reverse - reverse a list
 *
 * @list: The list to reverse
 */
void ac_slist_reverse(ac_slist_t **list)
{
	ac_slist_t *prev = NULL;

	while (*list) {
		ac_slist_t *next = (*list)->next;

		(*list)->next = prev;
		prev = *list;
		*list = next;
	}

	*list = prev;
}

/**
 * ac_slist_find - find an item in the list
 *
 * @list: The list to look for the item in
 * @data: The data to find
 *
 * Returns:
 *
 * The list entry if found, NULL otherwise
 */
ac_slist_t *ac_slist_find(ac_slist_t *list, void *data)
{
	while (list) {
		if (list->data == data)
			return list;
		list = list->next;
	}

	return NULL;
}

/**
 * ac_slist_foreach - execute a function for each item in the list
 *
 * @list: The list to operate on
 * @action: The function to execute on each item
 * @user_data: Optional data to pass to @action, can be NULL
 */
void ac_slist_foreach(ac_slist_t *list, void (*action)(void *item, void *data),
		      void *user_data)
{
	while (list) {
		action(list->data, user_data);
		list = list->next;
	}
}

/**
 * ac_slist_destroy - destroy a list, optionally freeing all its items memory
 *
 * @list@ The list to destroy
 * @free_data: Function to free an items memory, can be NULL
 */
void ac_slist_destroy(ac_slist_t **list, void (*free_data)(void *data))
{
	while (*list) {
		ac_slist_t *p = (*list)->next;

		free_data((*list)->data);
		free(*list);
		*list = p;
	}
}