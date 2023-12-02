#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include "types.h"
#define MAX_WAYS 16

///////////////////////////////////////////////////////////////////////////////////
/// Define the necessary data structures here (Look at Appendix A for more details)
///////////////////////////////////////////////////////////////////////////////////
typedef enum Coherence_State_Enum {
	INVALID,
	SHARED,
	MODIFIED
} Coherence_State;
typedef struct CacheLine{
    bool valid;
    bool dirty;
    Addr tag;
    uint32_t lastAccess;
    uint32_t freq;
    uint32_t coreID;
    Coherence_State coherence;
}CacheLine;
typedef struct CacheSet{
    CacheLine* line[MAX_WAYS];
}CacheSet;
typedef struct Cache{
    uint64_t numSets;
    CacheSet* set;
    uint64_t ways;
    uint64_t replacement;
    CacheLine* lastEvicted;
    unsigned long long stat_read_access;
    unsigned long long stat_write_access;
    unsigned long long stat_read_miss;
    unsigned long long stat_write_miss;
    unsigned long long stat_dirty_evicts;
    unsigned long long stat_GetX_msg;
    unsigned long long stat_num_invalidations;
    unsigned long long stat_GetS_msg;
    unsigned long long stat_PutX_msg;
    unsigned long long cache_to_cache_transfers;
    unsigned long long num_s_to_m_transitions;
    unsigned long long num_s_to_i_transitions;
    unsigned long long num_i_to_m_transitions;
    unsigned long long num_i_to_s_transitions;
    unsigned long long num_m_to_s_transitions;
    unsigned long long num_m_to_i_transitions;
    int tagNumBits;
    int indexNumBits;
}Cache;



//////////////////////////////////////////////////////////////////////////////////////
// Mandatory variables required for generating the desired final reports as necessary
// Used by cache_print_stats()
//////////////////////////////////////////////////////////////////////////////////////

// stat_read_access : Number of read (lookup accesses do not count as READ accesses) accesses made to the cache
// stat_write_access : Number of write accesses made to the cache
// stat_read_miss : Number of READ requests that lead to a MISS at the respective cache.
// stat_write_miss : Number of WRITE requests that lead to a MISS at the respective cache
// stat_dirty_evicts : Count of requests to evict DIRTY lines.

/////////////////////////////////////////////////////////////////////////////////////////
// Functions to be implemented
//////////////////////////////////////////////////////////////////////////////////////////////

Cache* cache_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);
bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id);
bool cache_access_coherence (Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_install_coherence (Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
Coherence_State get_Cacheline_state(Cache *c, Addr lineaddr);
void set_Cacheline_state(Cache *c, Addr lineaddr, bool snooped_action);
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache* c, char* header);
void cache_print_stats_coherence(Cache* c, char* header); // Part F


#endif // CACHE_H
