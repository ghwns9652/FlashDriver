#include "FS.h"
#include "container.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef bdbm_drv
extern lower_info memio_info;
#endif
#ifdef spdk
extern lower_info spdk_info;
#endif

int F_malloc(void **ptr, int size,int rw){
	int dmatag=0;
#ifdef bdbm_drv
	dmatag=memio_info.lower_alloc(rw,(char**)ptr);
#elif defined(spdk)
	dmatag=spdk_info.lower_alloc(1, (char**)ptr);
#else
	(*ptr)=malloc(size);
#endif	
	if(rw==FS_MALLOC_R){
	//	printf("alloc tag:%d\n",dmatag);
	}
	return dmatag;
}

void F_free(void *ptr,int tag,int rw){
	if(rw==FS_MALLOC_R){
	//	printf("free tag:%d\n",tag);
	}
#ifdef bdbm_drv
	memio_info.lower_free(rw,tag);
#elif defined(spdk)
	spdk_info.lower_free(1, tag);
#else 
	free(ptr);
#endif
	return;
}
