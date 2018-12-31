#include <stdlib.h>
#include <string.h>
#include "vsheader.h"

void vsheader_insert(vsheader *header, snode *input) {
	int key_size = strlen(input->lpa)+1;
	char *start = ((char*header) + header->size;
	uint16_t idx = header->key_cnt;

	header->map[idx]->addr = start;
	header->map[idx]->ppa = input->isvalid?input->ppa:UINT_MAX;
	header->map[idx]->size = (uint8_t)key_size;

	header->key_cnt++;
	header->size += key_size;

	memcpy(start, input->lpa, key_size);
}
