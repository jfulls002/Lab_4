#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include <cstring>

#include "cache.h"


/////////////////////////////////////////////////////////////////////////////////////
// ---------------------- DO NOT MODIFY THE PRINT STATS FUNCTION --------------------
/////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache* c, char* header){
	double read_mr =0;
	double write_mr =0;

	if (c->stat_read_access) {
		read_mr = (double)(c->stat_read_miss) / (double)(c->stat_read_access);
	}

	if (c->stat_write_access) {
		write_mr = (double)(c->stat_write_miss) / (double)(c->stat_write_access);
	}

	printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
	printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
	printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
	printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
	printf("\n%s_READ_MISS_PERC  \t\t : %10.3f", header, 100*read_mr);
	printf("\n%s_WRITE_MISS_PERC \t\t : %10.3f", header, 100*write_mr);
	printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);
	printf("\n");
}


/////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the data structures 
// Initialize the required fields 
/////////////////////////////////////////////////////////////////////////////////////

Cache* cache_new(uint64_t size, uint64_t assoc, uint64_t linesize, uint64_t repl_policy){
	Cache* c=(Cache*)malloc(sizeof(Cache));
	c->tagNumBits=linesize;
	c->ways=assoc;
	c->sets=MAX_WAYS/assoc;
	int i;
	int j;
	for(i=0;i<c->sets;i++){
		CacheSet* s=(CacheSet*)malloc(sizeof(CacheSet));
		for(j=0;j<c->ways;j++){
			CacheLine* l=(CacheLine*)malloc(sizeof(CacheLine));
			l->dirty=0;
			l->freq=NULL;
			l->lastAccess=NULL;
			l->tag=NULL;
			l->valid=0;
			l->coreID=NULL;
			s->line[j]=l;
		}
		c->set[i]=s;
	}
	c->replacement=repl_policy;
	c->stat_dirty_evicts=0;
	c->stat_read_access=0;
	c->stat_read_miss=0;
	c->stat_write_access=0;
	c->stat_write_miss=0;
}

/////////////////////////////////////////////////////////////////////////////////////
// Return HIT if access hits in the cache, MISS otherwise 
// If is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
/////////////////////////////////////////////////////////////////////////////////////

bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){
	int index=lineaddr%c->sets;
	int tag=lineaddr/c->sets;
	int i;
	bool toReturn=0;
	for(i=0;i<c->ways;i++){
		if(c->set[index]->line[i]->valid){
			if(c->set[index]->line[i]->tag==tag){
				toReturn=1;
				c->set[index]->line[i]->freq++;
				c->set[index]->line[i]->lastAccess=0;
				if(is_write)c->set[index]->line[i]->dirty=1;
				c->stat_read_access++;
			}else{
				c->set[index]->line[i]->lastAccess++;
			}
		}
	}
	if(!toReturn)c->stat_read_miss++;
	return toReturn;
}

/////////////////////////////////////////////////////////////////////////////////////
// Install the line: determine victim using replacement policy 
// Copy victim into last_evicted_line for tracking writebacks
/////////////////////////////////////////////////////////////////////////////////////

void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){
	int index=lineaddr%c->sets;
	uint32_t toEvict=cache_find_victim(c,index,core_id);
	c->lastEvicted=c->set[index]->line[toEvict];
	c->set[index]->line[toEvict]->coreID=core_id;
	c->set[index]->line[toEvict]->dirty=is_write;
	c->set[index]->line[toEvict]->freq=1;
	c->set[index]->line[toEvict]->lastAccess=0;
	c->set[index]->line[toEvict]->valid=1;
	c->set[index]->line[toEvict]->tag=lineaddr/c->sets;
}

////////////////////////////////////////////////////////////////////
// You may find it useful to split victim selection from install
////////////////////////////////////////////////////////////////////
uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id){
	uint32_t toReplace;
	if(c->ways==1){
		return 0;
	}
	if(c->replacement==0){//LRU
		int least=0;
		toReplace=0;
		for(int i=0;i<c->ways;i++){
			if(!c->set[set_index]->line[i]->valid)return i;
			if(c->set[set_index]->line[i]->lastAccess>least){
				least=c->set[set_index]->line[i]->lastAccess;
				toReplace=i;
			}
			c->set[set_index]->line[i]->lastAccess++;
		}
		return toReplace;
	}
	if(c->replacement==1){//LFU+MRU
		int leastFreq=0;
		int lastAccess=0;
		toReplace=0;
		for(int i=0;i<c->ways;i++){
			if(!c->set[set_index]->line[i]->valid)return i;
			if(c->set[set_index]->line[i]->freq<leastFreq||i==1){
				leastFreq=c->set[set_index]->line[i]->freq;
				lastAccess=c->set[set_index]->line[i]->lastAccess;
				toReplace=i;
			}else if(c->set[set_index]->line[i]->freq==leastFreq&&c->set[set_index]->line[i]->lastAccess<lastAccess){
				lastAccess=c->set[set_index]->line[i]->lastAccess;
				toReplace=i;
			}
			c->set[set_index]->line[i]->lastAccess++;
		}
		return toReplace;
	}
	if(c->replacement==2){//SWP
	
	}
	return toReplace;
}
