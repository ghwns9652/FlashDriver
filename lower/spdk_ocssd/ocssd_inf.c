#include "ocssd_inf.h"

static struct ctrlr_entry *g_controllers = NULL;
static struct ns_entry *g_namespaces = NULL;

bool stopflag;
uint32_t nr_submit;

uint8_t ch_bits, pu_bits, ck_bits, lb_bits;

pthread_mutex_t allocator_lock;
pthread_mutex_t count_lock;

lower_info spdk_info={
	.create=spdk_create,
	.destroy=spdk_destroy,
	.push_data=spdk_push_data,
	.pull_data=spdk_pull_data,
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

struct io_arg {
	uint64_t *lba_list;
	algo_req *req;
};

struct cb_arg {
	bool done;
	struct spdk_nvme_cpl cpl; // NVMe completion queue entry
} cb;

static int poller_main(void *arg);

static void 
ocssd_raw_cb(void *arg, const struct spdk_nvme_cpl *cpl)
{
	struct cb_arg *cb = (struct cb_arg *)arg;

	cb->cpl  = *cpl;
	cb->done = true;
}

static void
ocssd_print_geometry(struct spdk_ocssd_geometry_data *geo)
{
	puts("\n[OCSSD GEOMETRY INFO]");
	printf("OCSSD v%d.%d\n", geo->mjr, geo->mnr);
	printf("Minimum / Optimal WriteSize: %d / %d\n", geo->ws_min, geo->ws_opt);
	printf("Num of Groups:         %d\n", geo->num_grp);
	printf("Num of Parallel Units: %d (per group)\n", geo->num_pu);
	printf("Num of Chunks:         %d (per PU)\n", geo->num_chk);
	printf("Chunk Size:            %d pages\n\n", geo->clba);
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
	int rc;

	struct ns_entry *ns_entry = g_namespaces;
	struct spdk_nvme_ctrlr *ctrlr = ns_entry->ctrlr;

	struct ocssd_base *ocssd_base;
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

static void
display_controller(struct ctrlr_entry *dev){
	const struct spdk_nvme_ctrlr_data *cdata;

	cdata = spdk_nvme_ctrlr_get_data(dev->ctrlr);

	printf("%04x:%02x:%02x.%02x ",
			dev->pci_addr.domain, dev->pci_addr.bus, dev->pci_addr.dev, dev->pci_addr.func);
	printf("%-40.40s %-20.20s ",
			cdata->mn, cdata->sn);
	printf("%5d ", cdata->cntlid);
	printf("\n");
	return ;
}

static void
select_nvme_dev(){
	struct ctrlr_entry *iter, *temp_ctrl;
	struct ns_entry *iter2, *temp_ns;
	int i = 1;
	int cmd;

	for (iter = g_controllers; iter != NULL; iter = iter->next){
		printf("[%d]\n", i++);
		display_controller(iter);
	}

	printf("Please Input list number:\n");
	while (1) {
		if (!scanf("%d", &cmd)) {
			printf("Invalid Command: input number\n");
			while (getchar() != '\n');
			continue;
		}

		if(cmd < 1){
			printf("Invalid Command: larger than 0\n");
			while (getchar() != '\n');
			continue;
		}

		iter2 = g_namespaces;
		for(i = 1; i < cmd; i++){
			iter2 = iter2->next;
			if(iter2 == NULL){
				printf("Invalid Command: too much\n");
				while (getchar() != '\n');
				continue;
			}	
		}
		break;
	}
	temp_ns = iter2;
	for (iter = g_controllers; iter != NULL; iter = iter->next){
		if(iter->ctrlr == temp_ns->ctrlr){
			temp_ctrl = iter;
			break;
		}
	}

	iter = g_controllers;
	iter2 = g_namespaces;

	while (iter) {
		struct ctrlr_entry *next = iter->next;
		if(iter != temp_ctrl){
			spdk_nvme_detach(iter->ctrlr);
			free(iter);
		}
		iter = next;
	}	

	while (iter2) {
		struct ns_entry *next = iter2->next;
		if(iter2 != temp_ns){
			free(iter2);
		}
		iter2 = next;
	}

	g_controllers = temp_ctrl;
	g_namespaces = temp_ns;
	g_controllers->next = NULL;
	g_namespaces->next = NULL;

	return ;
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

static int
ocssd_reset(struct ocssd_base *ocssd_base) {
	struct ns_entry *ns_entry = g_namespaces;
	struct spdk_ocssd_geometry_data *geo = ocssd_base->ocssd.geo;
	struct spdk_ocssd_chunk_information *tbl = ocssd_base->ocssd.tbl;
	
	int rc;
        uint64_t *lba;

	for (uint32_t i = 0; i < geo->num_grp * geo->num_pu * geo->num_chk; i++) {
		if (!tbl[i].wp) continue;

		lba = (uint64_t *)spdk_dma_zmalloc(sizeof(uint64_t), 4096, NULL);
		if (!lba) {
			SPDK_ERRLOG("[FlashSim] Cannot alloc lba\n");
			return 1;
		}
		*lba = tbl[i].slba;

		cb.done = false;
		rc = spdk_nvme_ocssd_ns_cmd_vector_reset(ns_entry->ns, ns_entry->qpair,
							 lba, 1, &tbl[i],
							 ocssd_raw_cb, &cb);
		if (rc) {
			SPDK_ERRLOG("[FlashSim] Cannot submit reset command\n");
			spdk_dma_free(lba);
			return 1;
		}
		while (!cb.done) {
			spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}
		if (spdk_nvme_cpl_is_error(&cb.cpl)) {
			SPDK_ERRLOG("[FlashSim] Reset failed\n");
			spdk_dma_free(lba);
			return 1;
		}
		spdk_dma_free(lba);
	}

	puts("-- AFTER RESET --");
	ocssd_print_chunkinfo(geo, tbl);

	return 0;
}

static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	 struct spdk_nvme_ctrlr_opts *opts)
{
	printf("Attaching to %s\n", trid->traddr);

	return true;
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

uint32_t spdk_create(lower_info *li){
	int rc;
	int amt, cnt;
	struct spdk_env_opts opts;
	struct ocssd_base *ocssd_base;
	struct spdk_ocssd_geometry_data *geo;
	uint32_t master_core;

	spdk_env_opts_init(&opts);
	opts.name = "FlashSimulator";
	opts.shm_id = 0;
	opts.core_mask = "0x03";

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

	select_nvme_dev();

	/** Get OCSSD base */
	li->ocssd_base = (void *)spdk_ocssd_base_init();
	if (li->ocssd_base == NULL) {
		SPDK_ERRLOG("[FlashSim] spdk_ocssd_base_init() failed\n");
		spdk_destroy(li);
		return 1;
	}

	/** Set system variables */
	ocssd_base = (struct ocssd_base *)li->ocssd_base;
	geo = ocssd_base->ocssd.geo;

	li->NOS = geo->num_grp * geo->num_chk;
	li->PPS = geo->num_pu * geo->clba; // clba: page per block(chunk)
	li->NOP = li->NOS * li->PPS;
	li->SOP = 4096;
	li->TS  = (uint64_t)li->NOP * li->SOP;
	li->SOK = sizeof(KEYT);
	li->write_op=li->read_op=li->trim_op=0;

	puts("[FlashSimultor spec]");
	printf("Pages:        %u\n", li->NOP);
	printf("Segments:     %u\n", li->NOS);
	printf("Page per seg: %u\n", li->PPS);
	printf("Pagesize:     %u\n", li->SOP);
	printf("Total size:   %lu\n\n", li->TS);

	stopflag = false;

	/** Set encode variables */
	cnt = 0;
	amt = geo->clba;
	while((amt >>= 1) != 0){
		cnt++;
	}
	lb_bits = cnt;

	cnt = 0;
	amt = geo->num_chk;
	while((amt >>= 1) != 0){
		cnt++;
	}
	ck_bits = cnt;
	
	cnt = 0;
	amt = geo->num_pu;
	while((amt >>= 1) != 0){
		cnt++;
	}
	pu_bits = cnt;
	
	cnt = 0;
	amt = geo->num_grp;
	while((amt >>= 1) != 0){
		cnt++;
	}
	ch_bits = cnt;

	/** Initialize OCSSD interface */
	printf("Initializing OCSSD Interface\n");
	if(!ocssd_inf_init()) {
		SPDK_ERRLOG("[FlashSim] ocssd_inf_init failed\n");
		spdk_destroy(li);
		return 1;
	}
	printf("Initialization complete.\n");

	pthread_mutex_init(&count_lock, NULL);
	pthread_mutex_init(&allocator_lock, NULL);
	pthread_mutex_init(&spdk_info.lower_lock,NULL);
	measure_init(&li->writeTime);
	measure_init(&li->readTime);
#if (ASYNC==1)
	master_core = spdk_env_get_current_core();
	spdk_env_thread_launch_pinned(master_core+1, poller_main, NULL);
#endif

	printf("\nResetting all chunks\n");
	rc = ocssd_reset((struct ocssd_base *)li->ocssd_base);	
	if (rc) {
		SPDK_ERRLOG("[FlashSim] OCSSD initial reset failed\n");
		return 1;
	}

	return 0;
}

static void
free_qpair(struct ns_entry *ns_entry)
{
	spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
	int nsid, num_ns;
	struct ctrlr_entry *entry;
	struct spdk_nvme_ns *ns;
	const struct spdk_nvme_ctrlr_data *cdata = spdk_nvme_ctrlr_get_data(ctrlr);

	entry = (struct ctrlr_entry*)malloc(sizeof(struct ctrlr_entry));
	if (entry == NULL) {
perror("ctrlr_entry malloc");
		exit(1);
	}

	spdk_pci_addr_parse(&entry->pci_addr, trid->traddr);

	printf("Attached to %s\n", trid->traddr);

	snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", cdata->mn, cdata->sn);

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
				pthread_mutex_lock(&allocator_lock);
				*v_ptr=ptr->buf;
				ptr->busy=1;
				pthread_mutex_unlock(&allocator_lock);
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

	pthread_mutex_lock(&allocator_lock);
	(ptr+tag)->busy = 0;
	pthread_mutex_unlock(&allocator_lock);
}

void raw_trans_layer(KEYT PPA, uint64_t *LBA){
	uint32_t ch, pu, ck, lb;
	struct spdk_ocssd_geometry_data *geo;

	geo = ((struct ocssd_base*)spdk_info.ocssd_base)->ocssd.geo;

	ch = PPA % geo->num_grp;
	pu = (PPA / geo->num_grp) % geo->num_pu;
	ck = PPA / (geo->num_grp * geo->num_pu * geo->clba);
	lb = (PPA / (geo->num_grp * geo->num_pu)) % geo->clba;

	//printf("ch: %u, pu: %u, ck: %u, lb: %u\n", ch, pu, ck, lb);

	*LBA = lb;
	*LBA |= (ck << lb_bits); // chuck
	*LBA |= (pu << (ck_bits + lb_bits)); // parallel unit
	*LBA |= (ch << (pu_bits + ck_bits + lb_bits)); // group

	return ;
}

void raw_trans_layer_trim(KEYT PPA, uint64_t *LBA){
	int i;
	uint32_t ch, pu, ck, lb;
	uint64_t temp;
	struct spdk_ocssd_geometry_data *geo;

	geo = ((struct ocssd_base*)spdk_info.ocssd_base)->ocssd.geo;

	ch = PPA % geo->num_grp;
	pu = (PPA / geo->num_grp) % geo->num_pu;
	ck = PPA / (geo->num_grp * geo->num_pu * geo->clba);
	lb = (PPA / (geo->num_grp * geo->num_pu)) % geo->clba;

	//printf("ch: %u, pu: %u, ck: %u, lb: %u\n", ch, pu, ck, lb);

	temp = lb;
	temp |= (ck << lb_bits); // chuck
	temp |= (pu << (ck_bits + lb_bits)); // parallel unit
	temp |= (ch << (pu_bits + ck_bits + lb_bits)); // group

	for(i = 0; i < geo->num_grp * geo->num_pu; i++){
		LBA[i] = temp;
		temp += geo->num_chk * geo->clba;
	}

	return ;
}

static void
ocssd_io_done(void *arg, const struct spdk_nvme_cpl *cpl) {
	struct io_arg *io = (struct io_arg *)arg;
	uint64_t *lba_list = io->lba_list;
	algo_req *req = io->req;

	printf("[%d completed]\n", lba_list[0]);

	cb.done = true;
	pthread_mutex_lock(&count_lock);
	nr_submit--;
	pthread_mutex_unlock(&count_lock);

	if (spdk_nvme_cpl_is_error(cpl)) {
		SPDK_ERRLOG("[FlashSim] IO completion failed\n");
		spdk_dma_free(io->lba_list);
		exit(1);
	}

	req->end_req(req);

	spdk_dma_free(lba_list);
	spdk_dma_free(io);
}

void* spdk_pull_data(KEYT PPA, uint32_t size, value_set *value, bool async, algo_req *const req)
{
	int rc;
	struct ns_entry *ns_entry = g_namespaces;
	struct ocssd_base *ocssd_base = (struct ocssd_base *)spdk_info.ocssd_base;
	
	struct io_arg *io;

	uint64_t *lba_list = (uint64_t *)spdk_dma_zmalloc(sizeof(uint64_t), 4096, NULL);
	if (!lba_list) {
		SPDK_ERRLOG("[FlashSim] Cannot alloc lba_list\n");
		return NULL;
	}

	raw_trans_layer(PPA, lba_list);

	io = (struct io_arg *)spdk_dma_zmalloc(sizeof(struct io_arg), 4096, NULL);
	if (!io) {
		SPDK_ERRLOG("[FlashSim] Cannot alloc io\n");
		spdk_dma_free(lba_list);
		return NULL;
	}
	io->lba_list = lba_list;
	io->req = req;

	cb.done = false;
	rc = spdk_nvme_ocssd_ns_cmd_vector_read(ns_entry->ns,
						ns_entry->qpair,
						value->value,
						lba_list, 1,
						ocssd_io_done, io, 0);
	if (rc) {
		SPDK_ERRLOG("[FlashSim] Cannot submit read command\n");
		spdk_dma_free(lba_list);
		return NULL;
	}

	pthread_mutex_lock(&count_lock);
	nr_submit++;
	pthread_mutex_unlock(&count_lock);

	if (!async) {
		while (!cb.done) {
			spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}
	}
	//spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);

	return NULL;
}

void* spdk_push_data(KEYT PPA, uint32_t size, value_set *value, bool async, algo_req *const req)
{
	int rc;
	struct ns_entry *ns_entry = g_namespaces;
	struct ocssd_base *ocssd_base = (struct ocssd_base *)spdk_info.ocssd_base;

	struct io_arg *io;

	uint64_t *lba_list = (uint64_t *)spdk_dma_zmalloc(sizeof(uint64_t), 4096, NULL);
	if (!lba_list) {
		SPDK_ERRLOG("[FlashSim] Cannot alloc lba_list\n");
		return NULL;
	}
	raw_trans_layer(PPA, lba_list);
	printf("%d %d pushed\n", PPA, lba_list[0]);

	io = (struct io_arg *)spdk_dma_zmalloc(sizeof(struct io_arg), 4096, NULL);
	if (!io) {
		SPDK_ERRLOG("[FlashSim] Cannot alloc io\n");
		spdk_dma_free(lba_list);
		return NULL;
	}
	io->lba_list = lba_list;
	io->req = req;

	cb.done = false;
	rc = spdk_nvme_ocssd_ns_cmd_vector_write(ns_entry->ns,
						 ns_entry->qpair,
						 value->value,
						 lba_list, 1,
						 ocssd_io_done, io, 0);
	if (rc) {
		SPDK_ERRLOG("[FlashSim] Cannot submit write command\n");
		spdk_dma_free(lba_list);
		return NULL;
	}

	pthread_mutex_lock(&count_lock);
	nr_submit++;
	pthread_mutex_unlock(&count_lock);

	if (!async) {
		while (!cb.done) {
			spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}
	}
	//spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);

	return NULL;
}

void* spdk_trim_block(KEYT PPA, bool async){
	int rc;
	uint32_t ck_idx;

	struct ns_entry *ns_entry = g_namespaces;
	struct ocssd_base *ocssd_base = (struct ocssd_base *)spdk_info.ocssd_base;
	struct spdk_ocssd_geometry_data *geo = ocssd_base->ocssd.geo;
	struct spdk_ocssd_chunk_information *tbl = ocssd_base->ocssd.tbl;

	uint64_t *lba_list = (uint64_t *)spdk_dma_zmalloc(sizeof(uint64_t) * geo->num_grp * geo->num_pu, 4096, NULL);
	raw_trans_layer_trim(PPA, lba_list);

	ck_idx = PPA / (geo->num_grp * geo->num_pu * geo->clba);

	for (int i = 0; i < geo->num_grp * geo->num_pu; i++) {
		/* TODO: Implement Async for trim */

		cb.done = false;
		rc = spdk_nvme_ocssd_ns_cmd_vector_reset(ns_entry->ns, ns_entry->qpair,
							 &lba_list[i], 1, &tbl[ck_idx],
							 ocssd_raw_cb, &cb);
		if (rc) {
			SPDK_ERRLOG("[FlashSim] Cannot submit reset command\n");
			spdk_dma_free(lba_list);
			return NULL;
		}

		while (!cb.done) {
			spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		}
		if (spdk_nvme_cpl_is_error((struct spdk_ocssd_vector_cpl *)&cb.cpl)) {
			SPDK_ERRLOG("[FlashSim] Reset failed\n");
			spdk_dma_free(lba_list);
			return NULL;
		}
		ck_idx += geo->num_chk;
	}

	spdk_dma_free(lba_list);

	return NULL;
}

void* spdk_refresh(lower_info* li){
	return NULL;
}

void spdk_stop(void){
	return;
}

static int
poller_main(void *arg)
{
	struct ns_entry *ns_entry = g_namespaces;
	puts("poller started");
	while (!stopflag) {
		if (nr_submit) {
			spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
		} else {
			usleep(1000);
		}
	}
	
	return 0;
}

