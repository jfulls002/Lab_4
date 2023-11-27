#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "dram.h"
extern bool DRAM_PAGE_POLICY;
extern uint64_t CACHE_LINESIZE;
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

void dram_print_stats(DRAM* dram){
	double rddelay_avg=0, wrdelay_avg=0;
	char header[256];
	sprintf(header, "DRAM");

	if (dram->stat_read_access) {
		rddelay_avg = (double)(dram->stat_read_delay) / (double)(dram->stat_read_access);
	}

	if (dram->stat_write_access) {
		wrdelay_avg = (double)(dram->stat_write_delay) / (double)(dram->stat_write_access);
	}

	printf("\n%s_READ_ACCESS\t\t : %10llu", header, dram->stat_read_access);
	printf("\n%s_WRITE_ACCESS\t\t : %10llu", header, dram->stat_write_access);
	printf("\n%s_READ_DELAY_AVG\t\t : %10.3f", header, rddelay_avg);
	printf("\n%s_WRITE_DELAY_AVG\t\t : %10.3f", header, wrdelay_avg);

}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

DRAM* dram_new(){
	DRAM* d=(DRAM*)malloc(sizeof(DRAM));
	d->banks=16;
	d->maxRows=256;
	d->numCols=1024;
	RowBuffer* r=(RowBuffer*)calloc(d->maxRows,sizeof(RowBuffer));
	for(int i=1;i<d->maxRows;i++){
		r->rowID=0;
		r->valid=0;
	}
	d->rows=r;
	d->stat_read_access=0;
	d->stat_read_delay=0;
	d->stat_write_access=0;
	d->stat_write_delay=0;
	d->pagePolicy=DRAM_PAGE_POLICY;
	d->RAS=45;
	d->CAS=45;
	d->PRE=45;
	d->BUS=10;
	return d;
}

uint64_t dram_access(DRAM* dram, Addr lineaddr, bool is_dram_write) {
	uint64_t delay=100;
	if(is_dram_write){
		dram->stat_write_access++;
		dram->stat_write_delay+=delay;
	}else{
		dram->stat_read_access++;
		dram->stat_read_delay+=delay;
	}
	return delay;
}

///////////////////////////////////////////////////////////////////
// Modify the function below only for Parts C,D,E
///////////////////////////////////////////////////////////////////

uint64_t dram_access_mode_CDE(DRAM* dram, Addr lineaddr, bool is_dram_write) {
	uint64_t delay=dram->BUS;
	if(is_dram_write){
		dram->stat_write_access++;
	}else{
		dram->stat_read_access++;
	}
	int rowBuffHMC;
	int bankNum=lineaddr%dram->banks;
	int rowNum=lineaddr/((dram->numCols/CACHE_LINESIZE)*dram->banks);
	if(!(dram->rows[bankNum].valid)){
		rowBuffHMC=1;
		dram->rows[bankNum].valid=1;
		dram->rows[bankNum].rowID=rowNum;
	}else if((int)dram->rows[bankNum].rowID==rowNum){
		rowBuffHMC=0;
	}else{
		rowBuffHMC=2;
		dram->rows[bankNum].rowID=rowNum;
	}
	if(dram->pagePolicy==0){
		if(rowBuffHMC==0){
			delay+=dram->CAS;
		}else if(rowBuffHMC==1){
			delay+=(dram->CAS+dram->RAS);
		}else if(rowBuffHMC==2){
			delay+=(dram->CAS+dram->RAS+dram->PRE);
		}
	}else{
		delay+=(dram->CAS+dram->RAS);
	}
	if(is_dram_write){
		dram->stat_write_delay+=delay;
	}else{
		dram->stat_read_delay+=delay;
	}
	return delay;
}

