#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "memsys.h"

#define PAGE_SIZE 4096

//---- Cache Latencies  ------

#define DCACHE_HIT_LATENCY   1
#define ICACHE_HIT_LATENCY   1
#define L2CACHE_HIT_LATENCY  10

extern MODE   SIM_MODE;
extern uint64_t  CACHE_LINESIZE;
extern uint64_t  REPL_POLICY;

extern uint64_t  DCACHE_SIZE;
extern uint64_t  DCACHE_ASSOC;
extern uint64_t  ICACHE_SIZE;
extern uint64_t  ICACHE_ASSOC;
extern uint64_t  L2CACHE_SIZE;
extern uint64_t  L2CACHE_ASSOC;
extern uint64_t  L2CACHE_REPL;
extern uint64_t  NUM_CORES;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


Memsys* memsys_new(void){
	Memsys* sys = (Memsys*)calloc(1, sizeof (Memsys));

	switch(SIM_MODE) {
		case SIM_MODE_A:
			sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			break;  

		case SIM_MODE_B:
		case SIM_MODE_C:
			sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new();
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new();
			for (uint i=0; i<NUM_CORES; i++) {
				sys->dcache_coreid[i] = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
				sys->icache_coreid[i] = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			}
			break;
		case SIM_MODE_F:
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new();
			for (uint i = 0; i < NUM_CORES; i++) {
				sys->dcache_coreid[i] = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
				sys->icache_coreid[i] = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			}
			break;
		default:
			break;
	}
	return sys;
}


////////////////////////////////////////////////////////////////////
// Return the latency of a memory operation
////////////////////////////////////////////////////////////////////

uint64_t memsys_access(Memsys* sys, Addr addr, Access_Type type, uint32_t core_id){
	uint32_t delay = 0;

	// all cache transactions happen at line granularity, so get lineaddr
	Addr lineaddr = addr / CACHE_LINESIZE;

	switch (SIM_MODE) {
		case SIM_MODE_A:
			delay = memsys_access_modeA(sys,lineaddr,type, core_id);
			break;
		case SIM_MODE_B:
		case SIM_MODE_C:
			delay = memsys_access_modeBC(sys,lineaddr,type, core_id);
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			delay = memsys_access_modeDE(sys,lineaddr,type, core_id);
			break;
		case SIM_MODE_F:
			delay = memsys_access_modeF(sys,lineaddr,type,core_id);
			break;
		default:
			break;
	}

  	//update the stats
  
	switch (type) {
		case ACCESS_TYPE_IFETCH: 
			sys->stat_ifetch_access++;
			sys->stat_ifetch_delay += delay;
			break;
		case ACCESS_TYPE_LOAD:
			sys->stat_load_access++;
			sys->stat_load_delay += delay;
			break;
		case ACCESS_TYPE_STORE:
			sys->stat_store_access++;
			sys->stat_store_delay += delay;
			break;
		default:
			break;
	}

  return delay;
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void memsys_print_stats(Memsys* sys){
	char header[256];
	sprintf(header, "MEMSYS");

	double ifetch_delay_avg=0, load_delay_avg=0, store_delay_avg=0;

	if (sys->stat_ifetch_access) {
		ifetch_delay_avg = (double)(sys->stat_ifetch_delay) / (double)(sys->stat_ifetch_access);
	}

	if (sys->stat_load_access) {
		load_delay_avg = (double)(sys->stat_load_delay) / (double)(sys->stat_load_access);
	}

	if (sys->stat_store_access) {
		store_delay_avg = (double)(sys->stat_store_delay) / (double)(sys->stat_store_access);
	}


	printf("\n");
	printf("\n%s_IFETCH_ACCESS  \t\t : %10llu",  header, sys->stat_ifetch_access);
	printf("\n%s_LOAD_ACCESS    \t\t : %10llu",  header, sys->stat_load_access);
	printf("\n%s_STORE_ACCESS   \t\t : %10llu",  header, sys->stat_store_access);
	printf("\n%s_IFETCH_AVGDELAY\t\t : %10.3f",  header, ifetch_delay_avg);
	printf("\n%s_LOAD_AVGDELAY  \t\t : %10.3f",  header, load_delay_avg);
	printf("\n%s_STORE_AVGDELAY \t\t : %10.3f",  header, store_delay_avg);
	printf("\n");

	switch (SIM_MODE) {
		case SIM_MODE_A:
			sprintf(header, "DCACHE");
			cache_print_stats(sys->dcache, header);
			break;
		case SIM_MODE_B:
		case SIM_MODE_C:
			sprintf(header, "ICACHE");
			cache_print_stats(sys->icache, header);
			sprintf(header, "DCACHE");
			cache_print_stats(sys->dcache, header);
			sprintf(header, "L2CACHE");
			cache_print_stats(sys->l2cache, header);
			dram_print_stats(sys->dram);
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			assert(NUM_CORES==2); //Hardcoded
			sprintf(header, "ICACHE_0");
			cache_print_stats(sys->icache_coreid[0], header);
			sprintf(header, "DCACHE_0");
			cache_print_stats(sys->dcache_coreid[0], header);
			sprintf(header, "ICACHE_1");
			cache_print_stats(sys->icache_coreid[1], header);
			sprintf(header, "DCACHE_1");
			cache_print_stats(sys->dcache_coreid[1], header);
			sprintf(header, "L2CACHE");
			cache_print_stats(sys->l2cache, header);
			dram_print_stats(sys->dram);
			break;
		case SIM_MODE_F:
			assert(NUM_CORES==2); //Hardcoded
			sprintf(header, "ICACHE_0");
			cache_print_stats(sys->icache_coreid[0], header);
			sprintf(header, "DCACHE_0");
			cache_print_stats_coherence(sys->dcache_coreid[0], header);
			sprintf(header, "ICACHE_1");
			cache_print_stats(sys->icache_coreid[1], header);
			sprintf(header, "DCACHE_1");
			cache_print_stats_coherence(sys->dcache_coreid[1], header);
			sprintf(header, "L2CACHE");
			cache_print_stats(sys->l2cache, header);
			dram_print_stats(sys->dram);
			break;
		default:
			break;
	}

}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

uint64_t memsys_access_modeA(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id){
  
	// IFETCH accesses go to icache, which we don't have in part A
	bool needs_dcache_access = !(type == ACCESS_TYPE_IFETCH);

	// Stores write to the caches
	bool is_write = (type == ACCESS_TYPE_STORE);

	if (needs_dcache_access) {
		// Miss
		if (!cache_access(sys->dcache,lineaddr,is_write,core_id)) {
			// Install the new line in L1
			cache_install(sys->dcache,lineaddr,is_write,core_id);
		}
	}

	// Timing is not simulated in Part A
	return 0;
}


uint64_t memsys_access_modeBC(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id){

	uint64_t delay=0;

	// Perform the ICACHE/ DCACHE access
	bool needs_dcache_access = !(type == ACCESS_TYPE_IFETCH);
	bool is_write = (type == ACCESS_TYPE_STORE);
	if (needs_dcache_access) {
		delay+=DCACHE_HIT_LATENCY;
		if(!cache_access(sys->dcache,lineaddr,is_write,core_id)){
			delay+=memsys_L2_access(sys,lineaddr,0,core_id);
			cache_install(sys->dcache,lineaddr,is_write,core_id);
			if(sys->dcache->lastEvicted->valid&&sys->dcache->lastEvicted->dirty){
				memsys_L2_access(sys,sys->dcache->lastEvicted->tag,sys->dcache->lastEvicted->dirty,sys->dcache->lastEvicted->coreID);
				sys->dcache->lastEvicted->valid=0;
			}
		}
	}else{
		delay+=ICACHE_HIT_LATENCY;
		if(!cache_access(sys->icache,lineaddr,is_write,core_id)){
			delay+=memsys_L2_access(sys,lineaddr,0,core_id);
			cache_install(sys->icache,lineaddr,is_write,core_id);
			if(sys->icache->lastEvicted->valid&&sys->icache->lastEvicted->dirty){
				memsys_L2_access(sys,sys->icache->lastEvicted->tag,sys->icache->lastEvicted->dirty,sys->icache->lastEvicted->coreID);
				sys->icache->lastEvicted->valid=0;
			}
		}
	}
	// On DCACHE miss, access the L2 Cache + install the new line + if needed, perform writeback
	return delay;
}

uint64_t memsys_L2_access(Memsys* sys, Addr lineaddr, bool is_writeback, uint32_t core_id){

	uint64_t delay = L2CACHE_HIT_LATENCY;
	// Perform the L2 access
	if(!cache_access(sys->l2cache,lineaddr,is_writeback,core_id)){
	// On L2 miss, access DRAM + install the new line + if needed, perform writeback
		if(!is_writeback){
			if(SIM_MODE==SIM_MODE_B){
				delay+=dram_access(sys->dram,lineaddr,is_writeback);
			}else{
				delay+=dram_access_mode_CDE(sys->dram,lineaddr,is_writeback);
			}
		}
		cache_install(sys->l2cache,lineaddr,is_writeback,core_id);
		if(sys->l2cache->lastEvicted->valid&&sys->l2cache->lastEvicted->dirty){
			if(SIM_MODE==SIM_MODE_B){
				dram_access(sys->dram,sys->l2cache->lastEvicted->tag,sys->l2cache->lastEvicted->dirty);
			}else{
				dram_access_mode_CDE(sys->dram,sys->l2cache->lastEvicted->tag,sys->l2cache->lastEvicted->dirty);
			}
			sys->l2cache->lastEvicted->valid=0;
		}
	}
	return delay;
}



/////////////////////////////////////////////////////////////////////
// This function converts virtual page number (VPN) to physical frame
// number (PFN).  Note, you will need additional operations to obtain
// VPN from lineaddr and to get physical lineaddr using PFN.
/////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// DO NOT MODIFY THE CODE OF THIS FUNCTIONS
////////////////////////////////////////////////////////////////////


uint64_t memsys_convert_vpn_to_pfn(Memsys *sys, uint64_t vpn, uint32_t core_id){

	uint64_t tail = vpn & 0x000fffff;
	uint64_t head = vpn >> 20;
	uint64_t pfn  = tail + (core_id << 21) + (head << 21);
	assert(NUM_CORES==2); //Need to make this general
	return pfn;

}

////////////////////////////////////////////////////////////////////////
// For LOAD and STORE Type instructions, we use the following function.
// Note: We are artificially creating the coherence traffic to simulate
// the effect of coherence protocol in the given program mix. Please do
// not modify or change the current mapping. 
///////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
// DO NOT MODIFY THE CODE OF THIS FUNCTION
/////////////////////////////////////////////////////////////////////

uint64_t memsys_convert_vpn_to_pfn_modeF (Memsys *sys, Addr v_lineaddr, uint32_t core_id){
	Addr p_lineaddr = 0;

	uint64_t set_indexing = (PAGE_SIZE/64);
	uint64_t base = v_lineaddr % set_indexing;
	uint64_t vpn = (v_lineaddr) / (PAGE_SIZE/CACHE_LINESIZE);
	uint64_t tail = vpn & 0xFFF;
	uint64_t head = vpn >> 12;
	uint64_t pfn = tail + ((head << 6));
	p_lineaddr = pfn*(PAGE_SIZE/CACHE_LINESIZE) + base;
	assert(NUM_CORES==2);

	return p_lineaddr;
}

uint64_t memsys_access_modeDE(Memsys* sys, Addr v_lineaddr, Access_Type type, uint32_t core_id) {

	uint64_t delay=0;
	// Convert the lineaddr from virtual (v) to physical (p) using the
	// function memsys_convert_vpn_to_pfn(). Page size is defined to be 4 KB.
	// NOTE: VPN_to_PFN operates at page granularity and returns page addr.
	uint64_t offset=v_lineaddr%64;
	uint64_t vpn=v_lineaddr>>6;
	uint64_t pageaddr=memsys_convert_vpn_to_pfn(sys,vpn,core_id);
	Addr lineaddr=(pageaddr<<6)+offset;
	bool needs_dcache_access = !(type == ACCESS_TYPE_IFETCH);
	bool is_write = (type == ACCESS_TYPE_STORE);
	if (needs_dcache_access) {
		delay+=DCACHE_HIT_LATENCY;
		if(!cache_access(sys->dcache_coreid[core_id],lineaddr,is_write,core_id)){
			delay+=memsys_L2_access_multicore(sys,lineaddr,0,core_id);
			cache_install(sys->dcache_coreid[core_id],lineaddr,is_write,core_id);
			if(sys->dcache_coreid[core_id]->lastEvicted->valid&&sys->dcache_coreid[core_id]->lastEvicted->dirty){
				memsys_L2_access_multicore(sys,sys->dcache_coreid[core_id]->lastEvicted->tag,sys->dcache_coreid[core_id]->lastEvicted->dirty,sys->dcache_coreid[core_id]->lastEvicted->coreID);
				sys->dcache_coreid[core_id]->lastEvicted->valid=0;
			}
		}
	}else{
		delay+=ICACHE_HIT_LATENCY;
		if(!cache_access(sys->icache_coreid[core_id],lineaddr,is_write,core_id)){
			delay+=memsys_L2_access_multicore(sys,lineaddr,0,core_id);
			cache_install(sys->icache_coreid[core_id],lineaddr,is_write,core_id);
			if(sys->icache_coreid[core_id]->lastEvicted->valid&&sys->icache_coreid[core_id]->lastEvicted->dirty){
				memsys_L2_access_multicore(sys,sys->icache_coreid[core_id]->lastEvicted->tag,sys->icache_coreid[core_id]->lastEvicted->dirty,sys->icache_coreid[core_id]->lastEvicted->coreID);
				sys->icache_coreid[core_id]->lastEvicted->valid=0;
			}
		}
	}

	return delay;
}


uint64_t memsys_L2_access_multicore(Memsys* sys, Addr lineaddr, bool is_writeback, uint32_t core_id) {

	uint64_t delay=L2CACHE_HIT_LATENCY;
	// If there is a miss in the L1 Cache, access the L2Cache for the specific lineaddr.
	if(!cache_access(sys->l2cache,lineaddr,is_writeback,core_id)){
	// If the L2Cache does not have the cache line, access the memory + install the new line + if needed, perform the writeback.
		if(!is_writeback)delay+=dram_access_mode_CDE(sys->dram,lineaddr,is_writeback);
		cache_install(sys->l2cache,lineaddr,is_writeback,core_id);
		if(sys->l2cache->lastEvicted->valid&&sys->l2cache->lastEvicted->dirty){
			dram_access_mode_CDE(sys->dram,sys->l2cache->lastEvicted->tag,sys->l2cache->lastEvicted->dirty);
			sys->l2cache->lastEvicted->valid=0;
		}
	}
	return delay;
}

uint64_t memsys_access_modeF (Memsys *sys, Addr v_lineaddr, Access_Type type, uint32_t core_id)
{
	uint64_t delay = 0;

	Addr p_lineaddr = 0;
	bool is_write = (type == ACCESS_TYPE_STORE);

	// ICACHE does not follow coherence. For the address, we use the function memsys_convert_vpn_to_pfn()
	// Convert the lineaddr from virtual (v) to physical (p) using the
	// function memsys_convert_vpn_to_pfn(). Page size is defined to be 4 KB.
	// NOTE: VPN_to_PFN operates at page granularity and returns page addr.

	// Perform the ICACHE Access using the functions cache_access/ cache_install
	if (type == ACCESS_TYPE_IFETCH){
		uint64_t offset=v_lineaddr%64;
		uint64_t vpn=v_lineaddr>>6;
		uint64_t pageaddr=memsys_convert_vpn_to_pfn(sys,vpn,core_id);
		p_lineaddr=(pageaddr<<6)+offset;
		assert(p_lineaddr!=memsys_convert_vpn_to_pfn_modeF(sys,v_lineaddr,core_id));
		delay+=ICACHE_HIT_LATENCY;
		if(!cache_access(sys->icache_coreid[core_id],p_lineaddr,is_write,core_id)){
			delay+=memsys_L2_access_multicore(sys,p_lineaddr,0,core_id);
			cache_install(sys->icache_coreid[core_id],p_lineaddr,is_write,core_id);
			if(sys->icache_coreid[core_id]->lastEvicted->valid&&sys->icache_coreid[core_id]->lastEvicted->dirty){
				memsys_L2_access_multicore(sys,sys->icache_coreid[core_id]->lastEvicted->tag,sys->icache_coreid[core_id]->lastEvicted->dirty,sys->icache_coreid[core_id]->lastEvicted->coreID);
				sys->icache_coreid[core_id]->lastEvicted->valid=0;
			}
		}
	}
	// For other types, we will use the function memsys_convert_vpn_to_pfn_modeF().
	// This function takes in the v_lineaddr and returns the p_lineaddr for use.
	else { 
		
		// Convert the virtual lineaddr to physical lineaddr using memsys_convert_vpn_to_pfn_modeF()
		p_lineaddr=memsys_convert_vpn_to_pfn_modeF(sys,v_lineaddr,core_id);
		// Get the Coherence state of the cache line from the current core and the other core.
		Coherence_State localCoherence=get_Cacheline_state(sys->dcache_coreid[core_id],p_lineaddr);
		Coherence_State remoteCoherence=get_Cacheline_state(sys->dcache_coreid[(~core_id)%2],p_lineaddr);
		if(remoteCoherence!=INVALID){
			CacheSet* remoteSet=&(sys->dcache_coreid[(~core_id)%2]->set[p_lineaddr%(sys->dcache_coreid[(~core_id)%2]->numets)]);
			for(int i=0;i<sys->dcache_coreid[(~core_id)%2]->ways;i++){
				if(remoteSet->line[i]->tag==p_lineaddr){
					bool remoteSnooped=remoteSet->line[i]->dirty;
				}
			}
			// Set the state for the remote core based on the read/write of the current core
			set_Cacheline_state(sys->dcache_coreid[(~core_id)%2],p_lineaddr,is_write);
		}
		// Based on the remote core's coherence state, perform the read/write to the current core.
			// In case of a hit, update the delay based on the initial state of the line in the current core.
			// In case of a miss, access the L2 Cache + install the new line + if needed, perform the writeback.

		// Note that the remote core's coherence state can impact the access of the cache line.
		// In case of state upgrades and invalidation/PutX, we need to modify the delay required.

	}
	return delay;
}

