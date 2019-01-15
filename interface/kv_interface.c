#include "kv_interface.h"

#include "queue.h"
#include "settings.h"
#include "interface.h"
#include "dl_sync.h"

#include <pthread.h>

struct kvs_kvp {
	FSTYPE type;
    KEYT key;
    char *value;
	dl_sync lock;
};

queue sub_q;

int kvs_init() {
	inf_init();

	q_init(&sub_q, QSIZE);

	pthread_create(&tid, NULL, &submitter, NULL);
}

int kvs_free() {
	q_free(sub_q);
	inf_free();
}

static struct kvs_kvp *get_new_kvp(FSTYPE type, const KEYT key, char *value, bool async) {
	struct kvs_kvp *new = (struct kvs_kvp *)malloc(sizeof(struct kvs_kvp));

	new->type  = type;
	new->key   = key;
	new->value = value;
	if (!async) {
		dl_sync_init(&new->lock, 1);
	}

	return new;
}

static int kvs_make_req(FSTYPE type, const KEYT key, char *value, bool async) {
	struct kvs_kvp *kvp = get_new_kvp(type, key, value, async);

	q_enqueue((void *)kvp, sub_q);

	if (!async) {
		dl_sync_wait(&kvp->lock);
	}

	return 0;
}

int kvs_retrieve(const KEYT key, char *value, bool async) {
	return kvs_make_req(FS_GET_T, key, value, async);
}

int kvs_store(const KEYT key, char *value, bool async) {
	return kvs_make_req(FS_SET_T, key, value, async);
}

int kvs_delete(const KEYT key, bool async) {
	return kvs_make_req(FS_DELETE_T, key, NULL, async);
}

int kvs_create_iterator(uint8_t *bitmask, uint8_t *bit_patttern) {
	int iter_id;

	return iter_id;
}

int kvs_delete_iterator(const int iter_id) {
	// free
}

int kvs_iterate_next(const int iter_id, bool async) {

}

void *kvs_end_req(void *arg) {

}

static int kvs_submit_req(struct kvs_kvp *kvp) {
	int rc;

	rc = inf_make_req_with_arg(	kvp->type,
								kvp->key,
								kvp->value,
								1,
								kvs_end_req,
								(void *)kvp);
}

void *submitter(void *arg) {
	struct kvs_kvp *kvp;

	while (1) {
		kvp = (struct kvs_kvp *)q_dequeue(sub_q);

		if (kvp) {
			kvs_submit_req(kvp);
		}
	}
}
