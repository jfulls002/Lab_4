#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include <cstring>

#include "cache.h"
extern uint64_t SWP_CORE0_WAYS;
extern uint64_t cycle;

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
void cache_print_stats_coherence(Cache* c, char* header){
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
	printf("\n%s_INVALIDATION_MESSAGES	\t\t : %10llu",header,c->stat_num_invalidations);
	printf("\n%s_GET_X_MESSAGES		\t\t : %10llu",header,c->stat_GetX_msg);
	printf("\n%s_GET_S_MESSAGES		\t\t : %10llu",header,c->stat_GetS_msg);
	printf("\n%s_PUT_X_MESSAGES		\t\t : %10llu",header,c->stat_PutX_msg);
	printf("\n%s_CACHE_TO_CACHE_TRANSFERS	\t\t : %10llu",header,c->cache_to_cache_transfers);
	printf("\n%s_S_TO_M_TRANSITIONS \t\t : %10llu", header, c->num_s_to_m_transitions);
	printf("\n%s_I_TO_M_TRANSITIONS \t\t : %10llu", header, c->num_i_to_m_transitions);
	printf("\n%s_M_TO_S_TRANSITIONS \t\t : %10llu", header, c->num_m_to_s_transitions);
	printf("\n%s_I_TO_S_TRANSITIONS \t\t : %10llu", header, c->num_i_to_s_transitions);
	printf("\n%s_M_TO_I_TRANSITIONS \t\t : %10llu", header, c->num_m_to_i_transitions);
	printf("\n%s_S_TO_I_TRANSITIONS \t\t : %10llu", header, c->num_s_to_i_transitions);
	printf("\n");
}

/////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the data structures 
// Initialize the required fields 
/////////////////////////////////////////////////////////////////////////////////////

Cache* cache_new(uint64_t size, uint64_t assoc, uint64_t linesize, uint64_t repl_policy){
	Cache* c=(Cache*)calloc(1,sizeof(Cache));
	c->tagNumBits=linesize;
	c->ways=assoc;
	c->numSets=size/(assoc*linesize);
	int i;
	int j;
	CacheLine* evict=(CacheLine*)malloc(sizeof(CacheLine));
	evict->dirty=0;
	evict->valid=0;
	c->lastEvicted=evict;
	CacheSet* s=(CacheSet*)calloc(c->numSets,sizeof(CacheSet));
	for(i=0;i<(int)c->numSets;i++){
		for(j=0;j<(int)c->ways;j++){
			CacheLine* l=(CacheLine*)malloc(sizeof(CacheLine));
			l->dirty=0;
			l->freq=0;
			l->lastAccess=0;
			l->tag=0;
			l->valid=0;
			l->coreID=0;
			l->coherence=INVALID;
			s[i].line[j]=l;
		}
	}
	c->set=s;
	c->replacement=repl_policy;
	c->stat_dirty_evicts=0;
	c->stat_read_access=0;
	c->stat_read_miss=0;
	c->stat_write_access=0;
	c->stat_write_miss=0;
	c->stat_GetX_msg=0;
    c->stat_num_invalidations=0;
    c->stat_GetS_msg=0;
    c->stat_PutX_msg=0;
    c->cache_to_cache_transfers=0;
    c->num_s_to_m_transitions=0;
    c->num_s_to_i_transitions=0;
    c->num_i_to_m_transitions=0;
    c->num_i_to_s_transitions=0;
    c->num_m_to_s_transitions=0;
    c->num_m_to_i_transitions=0;
	return c;
}

/////////////////////////////////////////////////////////////////////////////////////
// Return HIT if access hits in the cache, MISS otherwise 
// If is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
/////////////////////////////////////////////////////////////////////////////////////

bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){
	int i;
	int index=lineaddr % c->numSets;
	CacheSet* set=&c->set[index];
	bool toReturn=0;
	int numWays=(int)c->ways;
	for(i=0;i<numWays;i++){
		if(set->line[i]->valid){
			if(set->line[i]->tag==lineaddr){
				toReturn=1;
				if(set->line[i]->freq==63){
					set->line[i]->freq=63;
				}else{
					set->line[i]->freq++;
				}	
				set->line[i]->lastAccess=cycle;
				if(is_write){
					set->line[i]->dirty=1;
					c->stat_write_access++;
				}else{
					c->stat_read_access++;
				}
			}
		}
	}
	if(!toReturn&&!is_write){
		c->stat_read_miss++;
		c->stat_read_access++;
	}
	if(!toReturn&&is_write){
		c->stat_write_miss++;
		c->stat_write_access++;
	}
	return toReturn;
}

/////////////////////////////////////////////////////////////////////////////////////
// Install the line: determine victim using replacement policy 
// Copy victim into last_evicted_line for tracking writebacks
/////////////////////////////////////////////////////////////////////////////////////

void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){
	int index=lineaddr%c->numSets;
	uint32_t toEvict=cache_find_victim(c,index,core_id);
	c->lastEvicted->coreID=c->set[index].line[toEvict]->coreID;
	c->lastEvicted->dirty=c->set[index].line[toEvict]->dirty;
	c->lastEvicted->freq=c->set[index].line[toEvict]->freq;
	c->lastEvicted->lastAccess=c->set[index].line[toEvict]->lastAccess;
	c->lastEvicted->tag=c->set[index].line[toEvict]->tag;
	c->lastEvicted->valid=c->set[index].line[toEvict]->valid;
	if(c->lastEvicted->dirty&&c->lastEvicted->valid)c->stat_dirty_evicts++;
	c->set[index].line[toEvict]->coreID=core_id;
	c->set[index].line[toEvict]->dirty=is_write;
	c->set[index].line[toEvict]->freq=1;
	c->set[index].line[toEvict]->lastAccess=cycle;
	c->set[index].line[toEvict]->valid=1;
	c->set[index].line[toEvict]->tag=lineaddr;
}

////////////////////////////////////////////////////////////////////
// You may find it useful to split victim selection from install
////////////////////////////////////////////////////////////////////
uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id){
	uint32_t toReplace=0;
	uint32_t core0usage=0;
	uint64_t accessCycle=cycle;
	uint64_t leastFreq=64;
	int numWays=(int)c->ways;
	if(numWays==1)return toReplace;
	if(c->replacement==0){//LRU
		for(int i=0;i<numWays;i++){
			if(!c->set[set_index].line[i]->valid)return i;
			if(c->set[set_index].line[i]->lastAccess<accessCycle){
				accessCycle=c->set[set_index].line[i]->lastAccess;
				toReplace=i;
			}
		}
		return toReplace;
	}
	if(c->replacement==1){//LFU+MRU
		for(int i=0;i<numWays;i++){
			if(!c->set[set_index].line[i]->valid)return i;
			if(c->set[set_index].line[i]->freq<leastFreq||i==0){
				leastFreq=c->set[set_index].line[i]->freq;
				accessCycle=c->set[set_index].line[i]->lastAccess;
				toReplace=i;
			}else if(c->set[set_index].line[i]->freq==leastFreq&&c->set[set_index].line[i]->lastAccess>accessCycle){
				accessCycle=c->set[set_index].line[i]->lastAccess;
				toReplace=i;
			}
		}
		return toReplace;
	}
	if(c->replacement==2){//SWP
		uint32_t bestCandidate=0;
		uint32_t coreBestCandidate=0;
		uint64_t coreLeastFreq=64;
		uint64_t coreAccessCycle=cycle;
		for(int i=0;i<numWays;i++){
			if(!c->set[set_index].line[i]->valid)return i;
			if(c->set[set_index].line[i]->coreID==0)core0usage++;
			if(c->set[set_index].line[i]->freq<leastFreq){
				leastFreq=c->set[set_index].line[i]->freq;
				accessCycle=c->set[set_index].line[i]->lastAccess;
				bestCandidate=i;
			}else if(c->set[set_index].line[i]->freq==leastFreq&&c->set[set_index].line[i]->lastAccess>accessCycle){
				accessCycle=c->set[set_index].line[i]->lastAccess;
				bestCandidate=i;
			}
			if(c->set[set_index].line[i]->coreID==core_id){
				if(c->set[set_index].line[i]->freq<coreLeastFreq){
					coreLeastFreq=c->set[set_index].line[i]->freq;
					coreAccessCycle=c->set[set_index].line[i]->lastAccess;
					coreBestCandidate=i;
				}else if(c->set[set_index].line[i]->freq==coreLeastFreq&&c->set[set_index].line[i]->lastAccess>coreAccessCycle){
					coreAccessCycle=c->set[set_index].line[i]->lastAccess;
					coreBestCandidate=i;
				}
			}
			if(core0usage>SWP_CORE0_WAYS){
				if(core_id==0)toReplace=coreBestCandidate;
				else toReplace=bestCandidate;
			}else if(core0usage<SWP_CORE0_WAYS){
				if(core_id==0)toReplace=bestCandidate;
				else toReplace=coreBestCandidate;
			}else{
				toReplace=coreBestCandidate;
			}
		}	
	}
	return toReplace;
}
// Cache access based on coherence, will involve the coherence state transitions.
/////////////////////////////////////////////////////////////////////////////////////
// Return HIT if access hits in the cache, MISS otherwise 
// If is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats and type of messages.
// Update the coherence state based on the type of the transcation
/////////////////////////////////////////////////////////////////////////////////////


bool cache_access_coherence (Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id){
	int i;
	int index=lineaddr % c->numSets;
	CacheSet* set=&c->set[index];
	bool toReturn=0;
	int numWays=(int)c->ways;
	for(i=0;i<numWays;i++){
		if(set->line[i]->valid){
			if(set->line[i]->tag==lineaddr){
				toReturn=1;
				if(set->line[i]->freq==63){
					set->line[i]->freq=63;
				}else{
					set->line[i]->freq++;
				}	
				set->line[i]->lastAccess=cycle;
				if(is_write){
					set->line[i]->dirty=1;
					c->stat_write_access++;
				}else{
					c->stat_read_access++;
				}
			}
		}
	}
	if(!toReturn&&!is_write){
		c->stat_read_miss++;
		c->stat_read_access++;
	}
	if(!toReturn&&is_write){
		c->stat_write_miss++;
		c->stat_write_access++;
	}
	return toReturn;
}


/////////////////////////////////////////////////////////////////////////////////////
// Install the line: determine victim using replacement policy 
// Copy victim into last_evicted_line for tracking writebacks
// Update the coherence flag as needed.
/////////////////////////////////////////////////////////////////////////////////////

void cache_install_coherence (Cache *c, Addr lineaddr, uint32_t is_write, uint32_t core_id) { //Install a new line into the cache with a coherence state.
	int index=lineaddr%c->numSets;
	uint32_t toEvict=cache_find_victim(c,index,core_id);
	c->lastEvicted->coreID=c->set[index].line[toEvict]->coreID;
	c->lastEvicted->dirty=c->set[index].line[toEvict]->dirty;
	c->lastEvicted->freq=c->set[index].line[toEvict]->freq;
	c->lastEvicted->lastAccess=c->set[index].line[toEvict]->lastAccess;
	c->lastEvicted->tag=c->set[index].line[toEvict]->tag;
	c->lastEvicted->valid=c->set[index].line[toEvict]->valid;
	c->lastEvicted->coherence=c->set[index].line[toEvict]->coherence;
	if(c->lastEvicted->dirty&&c->lastEvicted->valid)c->stat_dirty_evicts++;
	c->set[index].line[toEvict]->coreID=core_id;
	c->set[index].line[toEvict]->dirty=is_write;
	c->set[index].line[toEvict]->freq=1;
	c->set[index].line[toEvict]->lastAccess=cycle;
	c->set[index].line[toEvict]->valid=1;
	c->set[index].line[toEvict]->tag=lineaddr;
	if(is_write){
		c->stat_PutX_msg++;
		c->set[index].line[toEvict]->coherence=MODIFIED;
	}else{
		c->set[index].line[toEvict]->coherence=SHARED;
	}
}

//////////////////////////////////////////////////////////////////////////////////////
// Obtain the Coherence state for the given cacheline.
/////////////////////////////////////////////////////////////////////////////////////
Coherence_State get_Cacheline_state (Cache* c, Addr lineaddr){
	// If a line is present, the saved coherence state is returned
	CacheSet* set=&(c->set[lineaddr%c->numSets]);
	for(int i=0;i<(int)c->ways;i++){
		if(set->line[i]->tag==lineaddr){
			if(set->line[i]->coherence==SHARED){
				c->stat_GetS_msg++;
			}else if(set->line[i]->coherence==MODIFIED){
				c->stat_GetX_msg++;
			}
			return set->line[i]->coherence;
		}
	}
	// Else, return INVALID.
	return INVALID;
}

////////////////////////////////////////////////////////////////////////////////////////////
// We set the cacheline flag based on the Coherence state of the next core. 
// Here, we are trying to either invalidate or make it shared based on the transaction of the core.
// The remote core's bus actions are monitored. In an ideal coherence protocol, 
// we manage the coherence messages through GetX, GetS, Inv, PutX. Instead, we monitor only the
// read or write from the remote core and change states accordingly.
/////////////////////////////////////////////////////////////////////////////////////////////
void set_Cacheline_state (Cache* c, Addr lineaddr, bool snooped_action) {
	CacheSet* set=&(c->set[lineaddr%c->numSets]);
	CacheLine* line;
	for(int i=0;i<(int)c->ways;i++){
		if(set->line[i]->tag==lineaddr){
			line=set->line[i];
		}	
	}
	Coherence_State currCoherence=line->coherence;
	if(snooped_action){
		line->coherence=
	}
	if(currCoherence==SHARED){

	}else if(currCoherence==MODIFIED){

	}else{

	}
}
