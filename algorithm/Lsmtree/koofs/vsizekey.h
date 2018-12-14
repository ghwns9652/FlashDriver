#ifndef __VSIZEKEY_H__
#ifndef __VSIZEKEY_H__

#include "skiplist.h"
#include "level.h"

/* 
   vsizekey_compare(key1, key2)
   return value
   > 0  : key1 is big
   == 0 : key1 == key2
   < 0  : key2 is big
*/
int vsizekey_compare (char *key1, char *key2);
void sk_to_header(skiplist *input);
void sk_to_header();
void sk_to_header();

#endif
