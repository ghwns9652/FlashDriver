#include "network.h"


lower_info net_info = {
    .create      = net_info_create,
    .destroy     = net_info_destroy,
    .push_data   = net_info_push_data,
    .pull_data   = net_info_pull_data,
    .device_badblock_checker = NULL,
    .trim_block  = net_info_trim_block,
    .refresh     = net_refresh,
    .stop        = NULL,
    .lower_alloc = NULL,
    .lower_free  = NULL,
    .lower_flying_req_wait = net_info_flying_req_wait,
    .lower_show_info = NULL
};

struct mem_seg *seg_table;

int sock_fd;
struct sockaddr_in serv_addr;
pthread_mutex_t flying_lock;

pthread_t tid;


void *poller(void *arg) {
    struct net_data data;
    int8_t type;
    KEYT ppa;
    algo_req *req;

    while (read(sock_fd, &data, sizeof(data))) {

        type = ((struct net_data *)&data)->type;
        ppa  = ((struct net_data *)&data)->ppa;
        req  = ((struct net_data *)&data)->req;

        switch (type) {
        case RQ_TYPE_CREATE:
        case RQ_TYPE_DESTROY:
            break;
        case RQ_TYPE_PUSH:
            req->end_req(req);
            break;
        case RQ_TYPE_PULL:
            req->end_req(req);
            break;
        case RQ_TYPE_TRIM:
            //req->end_req(req);
            break;
        case RQ_TYPE_FLYING:
            pthread_mutex_unlock(&flying_lock);
            break;
        }
    }

    pthread_exit(NULL);
}

static ssize_t net_make_req(int8_t type, KEYT ppa, algo_req *req) {
    struct net_data data;
    data.type = type;
    data.ppa  = ppa;
    data.req  = req;

    return write(sock_fd, &data, sizeof(data));
}

uint32_t net_info_create(lower_info *li) {
    li->NOB = _NOB;
    li->NOP = _NOP;
    li->SOB = BLOCKSIZE;
    li->SOP = PAGESIZE;
    li->SOK = sizeof(KEYT);
    li->PPB = _PPB;
    li->TS  = TOTALSIZE;

    // Mem table for metadata
    seg_table = (struct mem_seg *)malloc(sizeof(struct mem_seg) * li->NOB);
    for (int i = 0; i < li->NOB; i++) {
        seg_table[i].storage = NULL;
        seg_table[i].alloc   = false;
    }

	li->write_op=li->read_op=li->trim_op=0;

    pthread_mutex_init(&net_info.lower_lock, NULL);

    measure_init(&li->writeTime);
    measure_init(&li->readTime);

    memset(li->req_type_cnt, 0, sizeof(li->req_type_cnt));

    pthread_mutex_init(&flying_lock, NULL);

    // Socket open
    sock_fd = socket(AF_INET, SOCK_STREAM, 0); // TCP
    //sock_fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP
    if (sock_fd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(IP);
    serv_addr.sin_port        = htons(PORT);

    if (connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }

    // Polling thread create
    pthread_create(&tid, NULL, poller, NULL);

    return 1;
}

void *net_info_destroy(lower_info *li) {
    for (int i = 0; i < li->NOB; i++) {
        if (seg_table[i].alloc) {
            free(seg_table[i].storage);
        }
    }
    free(seg_table);

    pthread_mutex_destroy(&flying_lock);

    measure_init(&li->writeTime);
    measure_init(&li->readTime);

    for (int i = 0; i < LREQ_TYPE_NUM; i++) {
        printf("%d %ld\n", i, li->req_type_cnt[i]);
    }

	li->write_op=li->read_op=li->trim_op=0;

    // Socket close
    net_make_req(RQ_TYPE_DESTROY, 0, NULL);

    close(sock_fd);
    return NULL;
}

static uint8_t test_type(uint8_t type) {
    uint8_t mask = 0xff >> 1;
    return type & mask;
}

void *net_info_push_data(KEYT ppa, uint32_t size, value_set *value, bool async, algo_req *const req) {
    uint8_t t_type;

    if (value->dmatag == -1) {
        fprintf(stderr, "dmatag -1 error!\n");
        exit(1);
    }

    t_type = test_type(req->type);
    if (t_type < LREQ_TYPE_NUM) {
        net_info.req_type_cnt[t_type]++;
    }

    if (req->type <= GCMW) {
        if (!seg_table[ppa/net_info.PPB].alloc) {
            seg_table[ppa/net_info.PPB].storage = (PTR)malloc(net_info.SOB);
            seg_table[ppa/net_info.PPB].alloc   = true;
        }
        PTR loc = seg_table[ppa/net_info.PPB].storage;
        memcpy(&loc[(ppa % net_info.PPB) * net_info.SOP], value->value, size);
    }

    net_make_req(RQ_TYPE_PUSH, ppa, req);
    return NULL;
}

void *net_info_pull_data(KEYT ppa, uint32_t size, value_set *value, bool async, algo_req *const req) {
    uint8_t t_type;

    if (value->dmatag == -1) {
        fprintf(stderr, "dmatag -1 error!\n");
        exit(1);
    }

    t_type = test_type(req->type);
    if (t_type < LREQ_TYPE_NUM) {
        net_info.req_type_cnt[t_type]++;
    }

    if (req->type <= GCMW) {
        PTR loc = seg_table[ppa / net_info.PPS].storage;
        memcpy(value->value, &loc[(ppa % net_info.PPS) * net_info.SOP], size);
        req->type_lower = 1;
    }

    net_make_req(RQ_TYPE_PULL, ppa, req);
    return NULL;
}

void *net_info_trim_block(KEYT ppa, bool async) {
    net_make_req(RQ_TYPE_TRIM, ppa, NULL);

    net_info.req_type_cnt[TRIM]++;

    if (seg_table[ppa/net_info.PPB].alloc) {
        free(seg_table[ppa/net_info.PPB].storage);
        seg_table[ppa/net_info.PPB].storage = NULL;
        seg_table[ppa/net_info.PPB].alloc = 0;
    }

    //if (value == 0) {
    //    return (void *)-1;
    //} else {
    //    return NULL;
    //}
    return NULL;
}

void net_info_flying_req_wait() {
    net_make_req(RQ_TYPE_FLYING, 0, NULL);
    pthread_mutex_lock(&flying_lock);
}

void *net_refresh(struct lower_info *li) {
    measure_init(&li->writeTime);
    measure_init(&li->readTime);
    li->write_op = li->read_op = li->trim_op = 0;
    return NULL;
}