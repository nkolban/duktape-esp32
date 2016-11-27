#include <stdlib.h>
#include <assert.h>
#include "c_list.h"

/**
 * A list is an ordered set of entries.
 */

/**
 * Create a new list.
 */
list_t *list_createList() {
	list_t *pList = malloc(sizeof(list_t));
	pList->next = NULL;
	pList->prev = NULL;
	pList->value = NULL;
	return pList;
} // list_createList

/**
 * Delete a list.
 */
void list_deleteList(list_t *pList, int withFree) {
	list_t *pNext;
	while(pList != NULL) {
		pNext = pList->next;
		if (withFree) {
			free(pList->value);
		}
		free(pList);
		pList = pNext;
	}
} // list_deleteList


/**
 * Insert a new item at the end of the list.
 * The value is set to the value of the new entry.
 * The new entry is malloced().
 *
 *[A] -> [endOLD]    ------>   [A] -> [endOLD] -> [X]
 *
 */
void list_insert(list_t *pList, void *value) {
	while(pList->next != NULL) {
		pList = pList->next;
	}
	list_insert_after(pList, value);
} // list_insert

/**
 * Insert a new list entry after the entry supplied as a
 * parameter.  The value is set to the value of the new entry.
 * The new entry is malloced().
 *
 * [pEntry] -> [B]    ------>   [pEntry] -> [X] -> [B]
 *
 */
void list_insert_after(list_t *pEntry, void *value) {
	list_t *pNew = malloc(sizeof(list_t));
	pNew->next = pEntry->next;
	pNew->prev = pEntry;
	pNew->value = value;

	// Order IS important here.
	if (pEntry->next != NULL) {
		pEntry->next->prev = pNew;
	}
	pEntry->next = pNew;
} // list_insert_after

/**
 * [A] -> [pEntry]   ------>   [A] -> [X] -> [pEntry]
 *
 */
void list_insert_before(list_t *pEntry, void *value) {
	// Can't insert before the list itself.
	if (pEntry->prev == NULL) {
		return;
	}
	list_t *pNew = malloc(sizeof(list_t));
	pNew->next = pEntry;
	pNew->prev = pEntry->prev;
	pNew->value = value;

	// Order IS important here.
	pEntry->prev->next = pNew;
	pEntry->prev = pNew;
} // list_insert_before

/**
 * Delete an item from the list.
 */
void list_delete(list_t *pList, list_t *pEntry, int withFree) {
	while(pList != NULL && pList->next != pEntry) {
		pList = pList->next;
	}
	if (pList == NULL) {
		return;
	}
	pList->next = pEntry->next;
	if (pEntry->next != NULL) {
		pEntry->next->prev = pList;
	}
	if (withFree) {
		free(pEntry->value);
	}
	free(pEntry);
} // list_delete

/**
 * Delete a list entry by value.
 */
void list_deleteByValue(list_t *pList, void *value, int withFree) {
	list_t *pNext = pList->next;
	while(pNext != NULL) {
		if (pNext->value == value) {
			list_delete(pList, pNext, withFree);
			return;
		}
	} // End while
} // list_deleteByValue

/**
 * Get the next item in a list.
 */
list_t *list_next(list_t *pList) {
	if (pList == NULL) {
		return NULL;
	}
	return (pList->next);
} // list_next

/**
 * Retrieve the first item in the list, may be NULL.  Subsequent items can be
 * retrieved with list_next().
 */
list_t *list_first(list_t *pList) {
	assert(pList->prev == NULL); // Sanity check to make sure this is the first.
	return pList->next;
} // list_first

