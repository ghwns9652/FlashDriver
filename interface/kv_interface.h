#ifndef __KV_INTERFACE_H__
#define __KV_INTERFACE_H__

#include "settings.h"

int kvs_init();

int kvs_retrieve(const KEYT, char **, bool);
int kvs_store(const KEYT, char **, bool);
int kvs_delete(const KEYT, bool);

int kvs_create_iterator();
int kvs_delete_iterator();
int kvs_iterate_next();

#endif
