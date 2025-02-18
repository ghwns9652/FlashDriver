#include"bloomfilter.h"
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#ifdef __GNUC__
#define FORCE_INLINE __attribute__((always_inline)) inline
#else
#define FORCE_INLINE inline
#endif

extern int save_fd;

void BITSET(char *input, char offset){
	char test=1;
	test<<=offset;
	(*input)|=test;
}
bool BITGET(char input, char offset){
	char test=1;
	test<<=offset;
	return input&test;
}
static FORCE_INLINE uint32_t rotl32 ( uint32_t x, int8_t r )
{
	return (x << r) | (x >> (32 - r));
}

static FORCE_INLINE uint64_t rotl64 ( uint64_t x, int8_t r )
{
	return (x << r) | (x >> (64 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)
#define ROTL64(x,y)	rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

//-----------------------------------------------------------------------------
//// Block read - if your platform needs to do endian-swapping or can only
//// handle aligned reads, do the conversion here
//
#define getblock(p, i) (p[i])
//

static FORCE_INLINE uint32_t fmix32 ( uint32_t h )
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}
static FORCE_INLINE uint64_t fmix64 ( uint64_t k )
{
	k ^= k >> 33;
	k *= BIG_CONSTANT(0xff51afd7ed558ccd);
	k ^= k >> 33;
	k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
	k ^= k >> 33;

	return k;
}

void MurmurHash3_x86_32( const void * key, int len,uint32_t seed, void * out )
{
	const uint8_t * data = (const uint8_t*)key;
	const int nblocks = len / 4;
	int i;

	uint32_t h1 = seed;

	uint32_t c1 = 0xcc9e2d51;
	uint32_t c2 = 0x1b873593;
	const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

	for(i = -nblocks; i; i++)
	{
		uint32_t k1 = getblock(blocks,i);

		k1 *= c1;
		k1 = ROTL32(k1,15);
		k1 *= c2;

		h1 ^= k1;
		h1 = ROTL32(h1,13); 
		h1 = h1*5+0xe6546b64;
	}
	const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

	uint32_t k1 = 0;

	switch(len & 3)
	{
		case 3: k1 ^= tail[2] << 16;
		case 2: k1 ^= tail[1] << 8;
		case 1: k1 ^= tail[0];
				k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
	}; h1 ^= len;

	h1 = fmix32(h1);

	*(uint32_t*)out = h1;
} 

KEYT hashfunction(KEYT key){
	key = ~key + (key << 15); // key = (key << 15) - key - 1;
	key = key ^ (key >> 12);
	key = key + (key << 2);
	key = key ^ (key >> 4);
	key = key * 2057; // key = (key + (key << 3)) + (key << 11);
	key = key ^ (key >> 16);
	return key;
}
BF* bf_init(int entry, float fpr){
	BF *res=(BF*)malloc(sizeof(BF));
	res->n=entry;
	res->m=ceil((res->n * log(fpr)) / log(1.0 / (pow(2.0, log(2.0)))));
	res->k=round(log(2.0) * (float)res->m / res->n);
	int targetsize=res->m/8;
	if(res->m%8)
		targetsize++;
	res->body=(char*)malloc(targetsize);
	memset(res->body,0,targetsize);
	res->p=fpr;
	res->targetsize=targetsize;
	return res;
}

BF* bf_cpy(BF *src){
	BF* res=(BF*)malloc(sizeof(BF));
	memcpy(res,src,sizeof(BF));

	res->body=(char *)malloc(res->targetsize);
	memcpy(res->body,src->body,res->targetsize);
	return res;
}

uint64_t bf_bits(int entry, float fpr){
	uint64_t n=entry;
	uint64_t m=ceil((n * log(fpr)) / log(1.0 / (pow(2.0, log(2.0)))));
	int targetsize=m/8;
	if(m%8)
		targetsize++;
	return targetsize;
}

void bf_set(BF *input, KEYT key){
	KEYT h;
	int block;
	int offset;
	for(int i=0; i<input->k; i++){
		//MurmurHash3_x86_32(&key,sizeof(key),i,&h);
		h=hashfunction(key+i);
		h%=input->m;
		block=h/8;
		offset=h%8;
		BITSET(&input->body[block],offset);
	}
}

bool bf_check(BF* input, KEYT key){
	KEYT h;
	int block,offset;
	for(int i=0; i<input->k; i++){
		//MurmurHash3_x86_32(&key,sizeof(key),i,&h);
		h=hashfunction(key+i);
		h%=input->m;
		block=h/8;
		offset=h%8;
		if(!BITGET(input->body[block],offset))
			return false;
	}
	return true;
}
void bf_free(BF *input){
	free(input->body);
	free(input);
}
void bf_save(BF* input){
	write(save_fd,input,sizeof(BF));
	write(save_fd,input->body,input->targetsize);
}

BF* bf_load(){
	BF *res=(BF*)malloc(sizeof(BF));
	read(save_fd,res,sizeof(BF));
	res->body=(char*)malloc(res->targetsize);
	read(save_fd,res->body,res->targetsize);
	return res;
}
/*
   int main(){
   int check=0;
   printf("test\n");
   BF *test=bf_init(KEYNUM,0.01);
   for(int i=0; i<1024; i++){
   bf_set(test,i);
   }

   for(int i=0; i<100000; i++){
   if(bf_check(test,i)){
   printf("%d\n",i);
   check++;
   }
   }

   printf("%d\n",check);
   }*/
