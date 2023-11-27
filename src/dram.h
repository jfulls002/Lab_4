#ifndef DRAM_H
#define DRAM_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "types.h"

//////////////////////////////////////////////////////////////////
// Define the Data structures here with the correct field (Refer to Appendix B for more details)
//////////////////////////////////////////////////////////////////
typedef struct RowBuffer{
    bool valid;
    uint64_t rowID;
}RowBuffer;
typedef struct DRAM{
    int banks;
    int maxRows;
    int numCols;
    RowBuffer* rows;
    unsigned long long stat_read_access;
    unsigned long long stat_write_access;
    unsigned long long stat_read_delay;
    unsigned long long stat_write_delay;
    bool pagePolicy;
    int RAS;
    int CAS;
    int PRE;
    int BUS;
}DRAM;


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

DRAM* dram_new();
void dram_print_stats(DRAM *dram);
uint64_t dram_access(DRAM* dram, Addr lineaddr, bool is_dram_write);
uint64_t dram_access_mode_CDE(DRAM* dram, Addr lineaddr, bool is_dram_write);


#endif // DRAM_H
