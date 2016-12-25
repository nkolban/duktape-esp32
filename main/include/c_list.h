/*
 * c_utils.h
 *
 *  Created on: Nov 15, 2016
 *      Author: kolban
 */

#ifndef COMPONENTS_C_LIST_H_
#define COMPONENTS_C_LIST_H_

typedef struct _list_t {
	void *value;
	struct _list_t *next;
	struct _list_t *prev;
} list_t;

list_t *list_createList();
void    list_delete(list_t *pList, list_t *pEntry, int withFree);
void    list_deleteByValue(list_t *pList, void *value, int withFree);
void    list_deleteList(list_t *pList, int withFree);
list_t *list_first(list_t *pList);
void    list_insert(list_t *pList, void *value);
void    list_insert_after(list_t *pEntry, void *value);
list_t *list_next(list_t *pList);

#endif /* COMPONENTS_C_LIST_H_ */
