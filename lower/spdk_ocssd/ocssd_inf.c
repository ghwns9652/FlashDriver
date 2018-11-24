#include "ocssd_inf.h"

static struct ctrlr_entry *g_controllers = NULL;
static struct ns_entry *g_namespaces = NULL;

queue *s_q;
pthread_t t_id;
bool stopflag;

lower_info spdk_info={
	.create=spdk_create,
	.destroy=spdk_destroy,
#if (ASYNC==1)
	.push_data=spdk_make_push,
	.pull_data=spdk_make_pull,
#elif (ASYNC==0)
	.push_data=spdk_push_data,
	.pull_data=spdk_pull_data,
#endif
	.device_badblock_checker=NULL,
	.trim_block=spdk_trim_block,
	.refresh=spdk_refresh,
	.stop=spdk_stop,
	.lower_alloc=spdk_lower_alloc,
	.lower_free=spdk_lower_free
};

struct ocssd_base {
	struct spdk_ocssd ocssd;
};

struct cb_arg {
	bool done;
	struct spdk_nvme_cpl cpl; // NVMe completion queue entry
} cb;

void* l_main(void *__input){
	void *_inf_req;
	spdk_request *inf_req;

	while(1){
		if(stopflag){
			pthread_exit(NULL);
			break;
		}
		if(!(_inf_req=q_dequeue(s_q))){
			continue;
		}
		inf_req=(spdk_request*)_inf_req;
		switch(inf_req->type){
			case FS_LOWER_W:
				spdk_push_data(inf_req->key, inf_req->size, inf_req->value, inf_req->isAsync, (algo_req*)(inf_req->upper_req));
				break;
			case FS_LOWER_R:
				spdk_pull_data(inf_req->key, inf_req->size, inf_req->value, inf_req->isAsync, (algo_req*)(inf_req->upper_req));
				break;
			case FS_LOWER_E:
				spdk_trim_block(inf_req->key, inf_req->isAsync);
				break;
		}
		free(inf_req);
	}
	return NULL;
}

static void ocssd_raw_cb(void *arg, const struct spdk_nvme_cpl *cpl) {
	struct cb_arg *cb = (struct cb_arg *)arg;

	cb->cpl  = *cpl;
	cb->done = true;
}

static void ocssd_print_geometry(struct spdk_ocssd_geometry_data *geo) {
	puts("\n[OCSSD GEOMETRY INFO]");
	printf("OCSSD v%d.%d\n", geo->mjr, geo->mnr);
	printf("Minimum / Optimal WriteSize: %dK / %dK\n", geo->ws_min, geo->ws_opt);
	printf("Num of Groups:         %d\n", geo->num_grp);
	printf("Num of Parallel Units: %d (per group)\n", geo->num_pu);
	printf("Num of Chunks:         %d (per PU)\n", geo->num_chk);
	printf("Chunk Size:            %d\n\n", geo->clba);
}

static void
ocssd_print_chunkinfo(struct spdk_ocssd_geometry_data *geo,
		      struct spdk_ocssd_chunk_information *tbl)
{
	puts("[OCSSD CHUNK INFO]");
	puts("[idx]\t[state]\t[type]\t[wli]\t[sLBA]\t[blks]\t[wr_ptr]");
	for (uint32_t i = 0; i < geo->num_grp * geo->num_pu * geo->num_chk; i += geo->num_chk) {
		printf("%d\t%x\t%x\t%d\t%lu\t%lu\t%lu\n",
			i, *((uint8_t *)&tbl[i].cs), *((uint8_t *)&tbl[i].ct), 
			tbl[i].wli, tbl[i].slba, tbl[i].cnlb, tbl[i].wp);
	}
	puts("");
}

static struct ocssd_base *
spdk_ocssd_base_init()
{
	struct ocssd_base *ocssd_base;
	int rc;

	struct ns_entry *ns_entry = g_namespaces;
	struct spdk_nvme_ctrlr *ctrlr = ns_entry->ctrlr;

	struct spdk_ocssd *ocssd;
	struct spdk_ocssd_geometry_data *geo;
	struct spdk_ocssd_chunk_information *tbl;
	size_t tbl_sz;

	/** Check if support OpenChannel */
	if (!spdk_nvme_ctrlr_is_ocssd_supported(ctrlr)) {
		SPDK_ERRLOG("[FlashSim] OpenChannel is not supported\n");
		return NULL;
	}

	/** Get geometry from OCSSD */
	geo = (struct spdk_ocssd_geometry_data *)spdk_dma_zmalloc(4096, 4096, NULL);
	if (geo == NULL) {
		SPDK_ERRLOG("[FlashSim] Cannot alloc geo\n");
		return NULL;
	}
	cb.done = false;
	rc = spdk_nvme_ocssd_ctrlr_cmd_geometry(ctrlr, 1, geo, 4096, ocssd_raw_cb, &cb);
	if (rc) {
		SPDK_ERRLOG("[FlashSim] Cannot submit geometry cmd");
		goto error_geo_free;
	}
	while (!cb.done) {
		spdk_nvme_ctrlr_process_admin_completions(ctrlr);
	}
	if (spdk_nvme_cpl_is_error(&cb.cpl)) {
		SPDK_ERRLOG("[FlashSim] Geometry failed\n");
		goto error_geo_free;
	}
	ocssd_print_geometry(geo);

	/** Get Chunk info from OCSSD */
	tbl_sz = geo->num_grp * geo->num_pu * geo->num_chk * sizeof(*tbl);
	tbl_sz = (tbl_sz + 4095) & ~4095;
	tbl = (struct spdk_ocssd_chunk_information *)spdk_dma_zmalloc(tbl_sz, 4096, NULL);
	if (!tbl) {
		SPDK_ERRLOG("[FlashSim] Cannot alloc chunk tbl\n");
		goto error_geo_free;
	}
	cb.done = false;
	rc = spdk_nvme_ctrlr_cmd_get_log_page(ctrlr, SPDK_OCSSD_LOG_CHUNK_INFO,
					      1, tbl, tbl_sz, 0, ocssd_raw_cb, &cb);
	if (rc) {
		SPDK_ERRLOG("[FlashSim] Cannot submit chunk info cmd\n");
		goto error_tbl_free;
	}
	while (!cb.done) {
		spdk_nvme_ctrlr_process_admin_completions(ctrlr);
	}
	if (spdk_nvme_cpl_is_error(&cb.cpl)) {
		SPDK_ERRLOG("[FlashSim] Chunk info failed\n");
		goto error_tbl_free;
	}
	ocssd_print_chunkinfo(geo, tbl);

	/** Register ocssd context */
	ocssd_base = (struct ocssd_base *)calloc(1, sizeof(struct ocssd_base));
	if (!ocssd_base) {
		SPDK_ERRLOG("[FlashSim] Cannot alloc ocssd_base\n");
		goto error_tbl_free;
	}
	ocssd = &ocssd_base->ocssd;
	ocssd->geo = geo;
	ocssd->tbl = tbl;
	ocssd->ctrlr = ctrlr;

	return ocssd_base;

error_tbl_free:
	spdk_dma_free(tbl);

error_geo_free:
	spdk_dma_free(geo);
	return NULL;
}

uint32_t spdk_create(lower_info *li){
	int rc;
	struct spdk_env_opts opts;
	struct ocssd_base *ocssd_base;
	struct spdk_ocssd_geometry_data *geo;

	spdk_env_opts_init(&opts);
	opts.name = "FlashSimulator";
	opts.shm_id = 0;

	if (spdk_env_init(&opts) < 0) {
		SPDK_ERRLOG("[FlashSim] Unable to initialize SPDK env\n");
		return 1;
	}

	/** Initialize NVMe controllers */
	printf("Initializing NVMe Controllers\n");
	rc = spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL);
	if (rc) {
		SPDK_ERRLOG("[FlashSim] spdk_nvme_probe() failed\n");
		spdk_destroy(li);
		return 1;
	}
	if (g_controllers == NULL) {
		fprintf(stderr, "no NVMe controllers found\n");
		spdk_destroy(li);
		return 1;
	}

	/** Get OCSSD base */
	ocssd_base = spdk_ocssd_base_init();
	if (ocssd_base == NULL) {
		SPDK_ERRLOG("[FlashSim] spdk_ocssd_base_init() failed\n");
		spdk_destroy(li);
		return 1;
	}

	/** Set system variables */
	geo = ocssd_base->ocssd.geo;

	li->NOS = geo->num_grp * geo->num_chk;
	li->PPS = geo->num_pu * geo->clba; // clba: page per block(chunk)
	li->NOP = li->NOS * li->PPS;
	li->SOP = 4096;
	li->TS  = li->NOP * li->SOP;
	//li->SOB = geo->num_pu * geo->clba * li->SOP
	li->SOK = sizeof(KEYT);
	li->write_op=li->read_op=li->trim_op=0;

	puts("[FlashSimultor spec]");
	printf("Pages:        %u\n", li->NOP);
	printf("Segments:     %u\n", li->NOS);
	printf("Page per seg: %u\n", li->PPS);
	printf("Total size:   %lu\n\n", li->TS);

	stopflag = false;

	/** Initialize OCSSD interface */
	printf("Initializing OCSSD Interface\n");
	if(!ocssd_inf_init()) {
		SPDK_ERRLOG("[FlashSim] ocssd_inf_init failed\n");
		spdk_destroy(li);
		return 1;
	}
	printf("Initialization complete.\n");

	pthread_mutex_init(&spdk_info.lower_lock,NULL);
	measure_init(&li->writeTime);
	measure_init(&li->readTime);
#if (ASYNC==1)
	q_init(&s_q, QSIZE);
	pthread_create(&t_id,NULL,&l_main,NULL);
#endif

	return 0;
}

static bool ocssd_inf_init() {
	struct ns_entry *ns_entry = g_namespaces;
	struct spdk_nvme_buffer *ptr;
	char *buffer;

	ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(ns_entry->ctrlr, NULL, 0);
	if (ns_entry->qpair == NULL){
		printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
		return false;
	}
	ns_entry->usable = QSIZE;
	
	/* device buffer memory version */
#if 0
	ns_entry.using_cmb_io = 1;
	ns_entry.buf = spdk_nvme_ctrlr_alloc_cmb_io_buffer(ns_entry->ctrlr, PAGESIZE);
	if (ns_entry.buf == NULL) {
		ns_entry.using_cmb_io = 0;
		ns_entry.buf = spdk_zmalloc(PAGESIZE, PAGESIZE, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
	}
#endif
	/* system buffer memory version */
	ns_entry->bufarr = (struct spdk_nvme_buffer*)malloc(sizeof(struct spdk_nvme_buffer) * QSIZE);
	ns_entry->using_cmb_io = 0;
	/* align designate continuous spaces in memory ??? */
	buffer = (char*)spdk_zmalloc(QSIZE * PAGESIZE, PAGESIZE, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
	ptr = ns_entry->bufarr;
	for(int i = 0; i < QSIZE; i++){
		*ptr = (struct spdk_nvme_buffer){buffer, 0}; //{buf, busy}
		buffer += PAGESIZE;
		ptr++;
	}

	if (ns_entry->bufarr == NULL) {
		printf("ERROR: write buffer allocation failed\n");
		return false;
	}
	if (ns_entry->using_cmb_io) {
		printf("INFO: using controller memory buffer for IO\n");
	} else {
		printf("INFO: using host memory buffer for IO\n");
	}
	printf("INFO: %u sized buffer allocated\n", ns_entry->usable);
	return true;
}

static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	 struct spdk_nvme_ctrlr_opts *opts)
{
	printf("Attaching to %s\n", trid->traddr);

	return true;
}

static void
attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	  struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
	int nsid, num_ns;
	struct ctrlr_entry *entry;
	struct spdk_nvme_ns *ns;
	const struct spdk_nvme_ctrlr_data *cdata = spdk_nvme_ctrlr_get_data(ctrlr);

	entry = (struct ctrlr_entry*)malloc(sizeof(struct ctrlr_entry));
	if (entry == NULL) {
		perror("ctrlr_entry malloc");
		exit(1);
	}

	printf("Attached to %s\n", trid->traddr);

	snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);

	entry->ctrlr = ctrlr;
	entry->next = g_controllers;
	g_controllers = entry;

	/*
	 * Each controller has one or more namespaces.  An NVMe namespace is basically
	 *  equivalent to a SCSI LUN.  The controller's IDENTIFY data tells us how
	 *  many namespaces exist on the controller.  For Intel(R) P3X00 controllers,
	 *  it will just be one namespace.
	 *
	 * Note that in NVMe, namespace IDs start at 1, not 0.
	 */
	num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
	printf("Using controller %s with %d namespaces.\n", entry->name, num_ns);
	for (nsid = 1; nsid <= num_ns; nsid++) {
		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (ns == NULL) {
			continue;
		}
		register_ns(ctrlr, ns);
	}
}

static void
register_ns(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns)
{
	struct ns_entry *entry;
	const struct spdk_nvme_ctrlr_data *cdata;

	/*
	 * spdk_nvme_ctrlr is the logical abstraction in SPDK for an NVMe
	 *  controller.  During initialization, the IDENTIFY data for the
	 *  controller is read using an NVMe admin command, and that data
	 *  can be retrieved using spdk_nvme_ctrlr_get_data() to get
	 *  detailed information on the controller.  Refer to the NVMe
	 *  specification for more details on IDENTIFY for NVMe controllers.
	 */
	cdata = spdk_nvme_ctrlr_get_data(ctrlr);

	if (!spdk_nvme_ns_is_active(ns)) {
		printf("Controller %-20.20s (%-20.20s): Skipping inactive NS %u\n",
		       cdata->mn, cdata->sn,
		       spdk_nvme_ns_get_id(ns));
		return;
	}

	entry = (struct ns_entry*)malloc(sizeof(struct ns_entry));
	if (entry == NULL) {
		perror("ns_entry malloc");
		exit(1);
	}

	entry->ctrlr = ctrlr;
	entry->ns = ns;
	entry->next = g_namespaces;
	g_namespaces = entry;

	printf("  Namespace ID: %d size: %juGB\n", spdk_nvme_ns_get_id(ns),
	       spdk_nvme_ns_get_size(ns) / 1000000000);
}

void* spdk_destroy(lower_info *li){
	struct ns_entry *ns_entry = g_namespaces;
	struct ctrlr_entry *ctrlr_entry = g_controllers;

	free_qpair(ns_entry);
	free_buffer(ns_entry);
	while (ns_entry) {
		struct ns_entry *next = ns_entry->next;
		free(ns_entry);
		ns_entry = next;
	}

	while (ctrlr_entry) {
		struct ctrlr_entry *next = ctrlr_entry->next;
		spdk_nvme_detach(ctrlr_entry->ctrlr);
		free(ctrlr_entry);
		ctrlr_entry = next;
	}

	stopflag = true;

	return NULL;
}

static void
free_qpair(struct ns_entry *ns_entry)
{
	//while(!ns_entry->is_completed){
	spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
	//}

	spdk_nvme_ctrlr_free_io_qpair(ns_entry->qpair);

	return;
}

static void
free_buffer(struct ns_entry *ns_entry)
{
	if(!ns_entry->bufarr) {
		return;
	}

	if(ns_entry->using_cmb_io){
		spdk_nvme_ctrlr_free_cmb_io_buffer(ns_entry->ctrlr, ns_entry->bufarr->buf, PAGESIZE);
	} else {
		spdk_free(ns_entry->bufarr->buf);
	}
	free(ns_entry->bufarr);
	return;
}

int spdk_lower_alloc(int type, char** v_ptr){
	struct ns_entry *ns_entry = g_namespaces;
	struct spdk_nvme_buffer *ptr = ns_entry->bufarr;

	while(1){
		if(ns_entry->usable) 
			break;
	}

	while(1){
		for(int i=0; i<QSIZE; i++){
			if(!ptr->busy) {
				*v_ptr=ptr->buf;
				ptr->busy=1;
				//printf("%d allocated\n", i);
				return i;
			}
			ptr++;
		}
		ptr = ns_entry->bufarr;
	}
	return -1;
}

void spdk_lower_free(int type, int tag){
	struct ns_entry *ns_entry = g_namespaces;
	struct spdk_nvme_buffer *ptr = ns_entry->bufarr;

	(ptr+tag)->busy = 0;
	//printf("tag %d freed\n", tag);
}

void* spdk_make_pull(KEYT PPA, uint32_t size, value_set *value, bool async, algo_req *const req)
{
	bool flag = false;
	spdk_request *s_req = (spdk_request*)malloc(sizeof(spdk_request));
	s_req->type=FS_LOWER_R;
	s_req->key=PPA;
	s_req->size=size;
	s_req->value=value;
	s_req->isAsync=async;
	s_req->upper_req=(void*)req;

	while(!flag){
		if(q_enqueue((void*)s_req,s_q)){
			flag=true;
			break;
		}
		else{
			flag=false;
			continue;
		}
	}
	return NULL;
}

void* spdk_make_push(KEYT PPA, uint32_t size, value_set *value, bool async, algo_req *const req)
{
	bool flag=false;
	spdk_request *s_req=(spdk_request*)malloc(sizeof(spdk_request));
	s_req->type=FS_LOWER_W;
	s_req->key=PPA;
	s_req->size=size;
	s_req->value=value;
	s_req->isAsync=async;
	s_req->upper_req=(void*)req;

	while(!flag){
		if(q_enqueue((void*)s_req,s_q)){
			flag=true;
			break;
		}
		else{
			flag=false;
			continue;
		}
	}
	return NULL;
}

void* spdk_make_trim(KEYT PPA, bool async)
{
	bool flag=false;
	spdk_request *s_req=(spdk_request*)malloc(sizeof(spdk_request));
	s_req->key=PPA;
	s_req->isAsync=async;

	while(!flag){
		if(q_enqueue((void*)s_req,s_q)){
			flag=true;
			break;
		}
		else{
			flag=false;
			continue;
		}
	}
	return NULL;
}

void* spdk_pull_data(KEYT PPA, uint32_t size, value_set *value, bool async, algo_req *const req)
{
	int rc;
	int complete;
	struct ns_entry *ns_entry = g_namespaces;

	//submit I/O request
	rc = spdk_nvme_ns_cmd_read(ns_entry->ns, ns_entry->qpair, value->value, PPA, 1, io_complete, req, 0);
	if (rc != 0) {
		fprintf(stderr, "read I/O failed\n");
		exit(1);
	}

	//polling
	do{
		complete = spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
	}while(complete<1);

	//while(!spdk_nvme_qpair_process_completions(ns_entry->qpair, 0)) //0 for unlimited max completion
	//	; 

	return NULL;
}

void* spdk_push_data(KEYT PPA, uint32_t size, value_set *value, bool async, algo_req *const req)
{
	int rc;
	int complete;
	struct ns_entry *ns_entry = g_namespaces;

	//submit I/O request
	rc = spdk_nvme_ns_cmd_write(ns_entry->ns, ns_entry->qpair, value->value, PPA, 1, io_complete, req, 0);
	if (rc != 0) {
		fprintf(stderr, "write I/O failed\n");
		exit(1);
	}

	//polling
	do{
		complete = spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
	}while(complete<1);
	//while(spdk_nvme_qpair_process_completions(ns_entry->qpair, 0)) //0 for unlimited max completion
	//	; 

	return NULL;
}

static void io_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct algo_req *req = (struct algo_req*)arg;
	req->end_req(req);
}

void* spdk_trim_block(KEYT PPA, bool async){
	uint64_t *lba_list = (uint64_t *)malloc(sizeof(uint64_t));
	*lba_list = PPA;

	//spdk_nvme_ocssd_ns_cmd_vector_reset(ns_entry->ns, ns_entry->qpair, lba_list, 1, chunk_info, cb_fn, cb_arg);
	return NULL;
}

void* spdk_refresh(lower_info* li){
	return NULL;
}

void spdk_stop(void){
	return;
}

/*
struct hello_world_sequence {
	struct ns_entry	*ns_entry;
	char		*buf;
	unsigned        using_cmb_io;
	int		is_completed;
};
*/

/*(
static void
read_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence *sequence = arg;
	printf("%s", sequence->buf);
	spdk_free(sequence->buf);
	sequence->is_completed = 1;
}
*/

/*
static void
write_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence	*sequence = arg;
	struct ns_entry			*ns_entry = sequence->ns_entry;
	int				rc;

	if (sequence->using_cmb_io) {
		spdk_nvme_ctrlr_free_cmb_io_buffer(ns_entry->ctrlr, sequence->buf, PAGESIZE);
	} else {
		spdk_free(sequence->buf);
	}
	sequence->buf = spdk_zmalloc(PAGESIZE, PAGESIZE, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	rc = spdk_nvme_ns_cmd_read(ns_entry->ns, ns_entry->qpair, sequence->buf,
				   0, 
				   1,
				   read_complete, (void *)sequence, 0);
	if (rc != 0) {
		fprintf(stderr, "starting read I/O failed\n");
		exit(1);
	}
}
*/

/*
static void
cleanup(void)
{
	struct ns_entry *ns_entry = g_namespaces;
	struct ctrlr_entry *ctrlr_entry = g_controllers;

	while (ns_entry) {
		struct ns_entry *next = ns_entry->next;
		free(ns_entry);
		ns_entry = next;
	}

	while (ctrlr_entry) {
		struct ctrlr_entry *next = ctrlr_entry->next;
		spdk_nvme_detach(ctrlr_entry->ctrlr);
		free(ctrlr_entry);
		ctrlr_entry = next;
	}
}
*/
