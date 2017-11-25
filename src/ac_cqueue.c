/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * ac_cqueue.c - Circular queue
 *
 * Copyright (c) 2017		Andrew Clayton <andrew@digital-domain.net>
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "include/libac.h"

static void null_free_item(void *data __always_unused)
{
}

#define AC_CQUEUE_ALLOC_CHUNK_SZ	16
/**
 * ac_cqueue_new - create a new circular queue
 *
 * @size: The number of elements the queue can handle
 * @free_item: A function to free an element of the queue
 *
 * Returns:
 *
 * A pointer to the newly created circular queue or NULL if an unknown flag
 * was specified and sets errno
 */
ac_cqueue_t *ac_cqueue_new(size_t size, void (*free_item)(void *item),
			   int flags)
{
	ac_cqueue_t *cqueue;

	/* Check for unknown flags */
	if (flags & ~(AC_CQUEUE_OVERWRITE)) {
		errno = EINVAL;
		return NULL;
	}

	cqueue = malloc(sizeof(struct ac_cqueue_t));

	if (!size) {
		cqueue->size = AC_CQUEUE_ALLOC_CHUNK_SZ;
		cqueue->dyn_size = true;
	} else {
		cqueue->size = size;
		cqueue->dyn_size = false;
	}

	cqueue->queue = malloc(cqueue->size * sizeof(void *));
	cqueue->front = 0;
	cqueue->rear = 0;
	cqueue->count = 0;
	cqueue->overwrite = (flags & AC_CQUEUE_OVERWRITE) ? true : false;
	if (cqueue->overwrite)
		memset(cqueue->queue, 0, cqueue->size * sizeof(void *));

	if (!free_item)
		cqueue->free_item = null_free_item;
	else
		cqueue->free_item = free_item;

	return cqueue;
}

/**
 * ac_cqueue_push - add an item to the queue
 *
 * @cqueue: The queue to add the item to
 * @item: The item to be added
 *
 * Returns:
 *
 * -1 on failure (queue was full) or 0 on success
 */
int ac_cqueue_push(ac_cqueue_t *cqueue, void *item)
{
	if (cqueue->count == cqueue->size &&
	    !cqueue->dyn_size &&
	    !cqueue->overwrite)
		return -1;

	if (cqueue->count == cqueue->size && cqueue->dyn_size) {
		void *p = cqueue->queue;

		cqueue->queue = realloc(cqueue->queue, (cqueue->size +
					AC_CQUEUE_ALLOC_CHUNK_SZ) *
					sizeof(void *));
		if (!cqueue->queue) {
			cqueue->queue = p;
			return -1;
		}

		cqueue->size += AC_CQUEUE_ALLOC_CHUNK_SZ;
	}

	if (cqueue->rear == cqueue->size)
		cqueue->rear = 0;

	if (cqueue->overwrite && cqueue->queue[cqueue->rear] != NULL)
		cqueue->free_item(cqueue->queue[cqueue->rear]);

	cqueue->queue[cqueue->rear] = item;
	cqueue->rear++;
	if (cqueue->count < cqueue->size)
		cqueue->count++;

	/*
	 * Handle wrap around in overwrite mode, making sure front gets
	 * moved to the oldest entry.
	 */
	if (cqueue->overwrite && cqueue->front < cqueue->rear) {
		cqueue->front = cqueue->rear;
		if (cqueue->front == cqueue->size)
			cqueue->front = 0;
	}

	return 0;
}

/**
 * ac_cqueue_pop - get the head element from the queue
 *
 * @cqueue: The queue to get the element from
 *
 * Returns:
 *
 * A pointer to element
 */
void *ac_cqueue_pop(ac_cqueue_t *cqueue)
{
	void *ptr;

	if (cqueue->count == 0)
		return NULL;
	else if (cqueue->front == cqueue->size)
		cqueue->front = 0;
	cqueue->count--;

	ptr = cqueue->queue[cqueue->front];
	if (cqueue->overwrite)
		/* We don't want this being free'd behind our back */
		cqueue->queue[cqueue->front] = NULL;
	cqueue->front++;

	return ptr;
}

/**
 * ac_cqueue_foreach - iterate over elements in a queue
 *
 * @cqueue: The queue to iterate over
 * @action: The function to call on each element
 * @user_data: Optional user data to pass to action. Can be NULL
 */
void ac_cqueue_foreach(const ac_cqueue_t *cqueue,
		       void (*action)(void *item, void *data), void *user_data)
{
	size_t i;

	if (cqueue->count == 0)
		return;

	if (cqueue->front < cqueue->rear) {
		for (i = cqueue->front; i < cqueue->rear; i++)
			action(cqueue->queue[i], user_data);
	} else {
		for (i = cqueue->front; i < cqueue->size; i++)
			action(cqueue->queue[i], user_data);
		for (i = 0; i < cqueue->rear; i++)
			action(cqueue->queue[i], user_data);
	}
}

/**
 * ac_cqueue_is_empty - check if a queue is empty
 *
 * @cqueue: The queue to check
 *
 * Returns:
 *
 * true if the queue is empty, false otherwise
 */
bool ac_cqueue_is_empty(const ac_cqueue_t *cqueue)
{
	return (cqueue->count == 0);
}

/**
 * ac_cqueue_nr_items - get the number of elements in the queue
 *
 * @cqueue: The queue to operate on
 *
 * Returns:
 *
 * The number of elements in the queue
 */
size_t ac_cqueue_nr_items(const ac_cqueue_t *cqueue)
{
	return cqueue->count;
}

/**
 * ac_cqueue_destroy - destroy a circular queue freeing all its memory
 *
 * @cqueue: The queue to destroy
 */
void ac_cqueue_destroy(ac_cqueue_t *cqueue)
{
	size_t i;

	if (cqueue->front < cqueue->rear) {
		for (i = cqueue->front; i < cqueue->rear; i++)
			cqueue->free_item(cqueue->queue[i]);
	} else {
		for (i = cqueue->front; i < cqueue->size; i++)
			cqueue->free_item(cqueue->queue[i]);
		for (i = 0; i < cqueue->rear; i++)
			cqueue->free_item(cqueue->queue[i]);
	}

	free(cqueue->queue);
	free(cqueue);
}
