//
// caperf-dumper.cpp - test program to test the functionality of CAPerfDataReader.cpp
//

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "CAPerfDataReader.h"

typedef unsigned int u32;
typedef unsigned long long u64;
typedef __signed__ long long s64;

#include "CAPerfData.h"
#include "CAError.h"

// parse_ca_options
//
static int
parse_ca_options(int argc, char*argv)
{
  return 0;
}

int dumpData();

CAPerfDataReader reader;

int
main (int argc, char*argv[])
{
  int ret;
  char *file;

  if (argc > 1) {
    file = strdup(argv[1]);
  } else {
    file = strdup("/tmp/caperf.data");
  }

  ret = reader.init(file);

  reader.dumpCAPerfHeader();
  reader.dumpCAPerfEvents();

  // reader.dumpCAPerfData();
  dumpData();

  reader.clear();
}

// Below code to dump the PERF records are borrowd from Suravee's implementation
// in translatior

int parse_perf_event_sample(
	void * ptr, 
	void * pSamp)
{
	uint64_t sampleType = reader.getAttrSampleType();

	if (!ptr || !pSamp)
		return E_FAIL;

	char * pCur = (char *) ptr;
	
	if (sampleType & PERF_SAMPLE_TID) {
		((CA_PERF_RECORD_SAMPLE *)pSamp)->pid = *((u32*) pCur);
		pCur += 4;
		((CA_PERF_RECORD_SAMPLE *)pSamp)->tid = *((u32*) pCur);
		pCur += 4;
	}

	if (sampleType & PERF_SAMPLE_TIME) {
		((CA_PERF_RECORD_SAMPLE *)pSamp)->time = *((unsigned long long *) pCur);
	}

	// TODO: fill other fields..

	return S_OK;	
}


int dump_PERF_RECORD_MMAP (
	struct perf_event_header *phdr,
	void *ptr,
	uint32_t recCnt
)
{
	if (!ptr) {
		return E_FAIL;
 	}

	struct CA_PERF_RECORD_MMAP * pRec= (struct CA_PERF_RECORD_MMAP *) ptr;
	char * pCur = (char *) ptr;

	// Skip pid + tid + addr + len + pgoff
	pCur += 4 + 4 + 8 + 8 + 8;

	// Skip filename. 
	int len = strlen(pRec->filename) + 1;  
	int tmp = len % 8;
	if (tmp != 0)
		len += (8 - tmp);
	pCur += len;

	CA_PERF_RECORD_SAMPLE samp;
	parse_perf_event_sample(pCur, &samp); 

	// CA_DBG_PRINTF(5, "MMAP: time:0x%016llx, addr:0x%016llx, pid:%d, tid:%d, "
	CA_DBG_PRINTF(5, "[%5d] MMAP: time:0x%016llx, addr:0x%016llx, pid:%d, tid:%d, "
		"len:0x%016llx, pgoff:0x%016llx, filename:%s\n",
		recCnt, samp.time, pRec->addr, pRec->pid, pRec->tid, 
		pRec->len, pRec->pgoff, pRec->filename ); 

	// if perf_event_attr::sample_id_all is set to 1, then even these
	// mmap records will have fields marked by perf_event_attr::sample_type:
	// TODO: check for - perf_event_attr::sample_id_all -

	return S_OK;
}

int dump_PERF_RECORD_COMM (
	struct perf_event_header *phdr,
	void *ptr,
	uint32_t recCnt
)
{
	if (!ptr) {
		return E_FAIL;
 	}

	struct CA_PERF_RECORD_COMM * pRec= (struct CA_PERF_RECORD_COMM *) ptr;
	char * pCur = (char *) ptr;

	// Skip pid and tid
	pCur += (4 + 4);

	// Skip comm. comm is 64-bit align and 
	// max length at TASK_COMM_LEN byte
	int len = strlen(pRec->comm) + 1; 
	if (len <= 8)
		pCur += 8;
	else
		pCur += TASK_COMM_LEN;

	CA_PERF_RECORD_SAMPLE samp;
	parse_perf_event_sample(pCur, &samp); 

	CA_DBG_PRINTF(5, "[%5d] COMM: time:0x%016llx, pid:%d, tid:%d, comm:%s\n", 
			recCnt, samp.time, pRec->pid, pRec->tid, pRec->comm);

	return S_OK;
}


int dump_PERF_RECORD_SAMPLE(
	struct perf_event_header *pHdr, 
	void * ptr,
	uint32_t recCnt)
{
	struct CA_PERF_RECORD_SAMPLE rec;
	char * pCur = (char* )ptr;
	uint64_t sampleType = reader.getAttrSampleType();

	memset(&rec, 0, sizeof(struct CA_PERF_RECORD_SAMPLE));
	
	if (!ptr) {
		return E_FAIL;
	}

	CA_DBG_PRINTF(5, "[%5d] SAMP:", recCnt);

	if (sampleType & PERF_SAMPLE_TIME) {
		CA_DBG_PRINTF(5, " time:0x%016llx",
			*((u64 *)(pCur + 8 + 4 + 4)));
	}

	if (sampleType & PERF_SAMPLE_IP) {
		rec.ip = *((u64 *)pCur);
		pCur += 8;
		CA_DBG_PRINTF(5, ", ip:0x%016llx", rec.ip);
	}
	
	if (sampleType & PERF_SAMPLE_TID) {
		rec.pid = *((u32 *)pCur);
		pCur += 4;
		CA_DBG_PRINTF(5, ", pid:%d", rec.pid);
		rec.tid = *((u32 *)pCur);
		pCur += 4;
		CA_DBG_PRINTF(5, ", tid:%d", rec.tid);
	}

	if (sampleType & PERF_SAMPLE_TIME) {
		rec.time = *((u64 *)pCur);
		pCur += 8;
		// CA_DBG_PRINTF(5, ", time:0x%016llx", rec.time);
	}

	if (sampleType & PERF_SAMPLE_ADDR) {
		rec.addr = *((u64 *)pCur);
		pCur += 8;
		CA_DBG_PRINTF(5, ", addr:0x%016llx", rec.ip);
	}
	
	if (sampleType & PERF_SAMPLE_ID) {
		rec.id = *((u64 *)pCur);
		pCur += 8;
		CA_DBG_PRINTF(5, ", id:0x%016llx", rec.id);
	}
		
	if (sampleType & PERF_SAMPLE_CPU) {
		rec.cpu = *((u32 *)pCur);
		pCur += 4;
		CA_DBG_PRINTF(5, ", cpu:0x08%x", rec.cpu);
	}
		
	if (sampleType & PERF_SAMPLE_PERIOD) {
		rec.period = *((u64 *)pCur);
		pCur += 8;
		CA_DBG_PRINTF(5, ", period:0x%016llx", rec.period);
	}
		
	if (sampleType & PERF_SAMPLE_STREAM_ID) {
		rec.stream_id = *((u64 *)pCur);
		pCur += 8;
		CA_DBG_PRINTF(5, ", stream_id:0x%016llx", rec.stream_id);
	}

        if (sampleType & PERF_SAMPLE_CALLCHAIN) {
                uint64_t nbr = *((uint64_t *)pCur);
                pCur += 8;
                CA_DBG_PRINTF(5, ", callchain depth:0x%llx", nbr);
                for (int i=0; i < nbr; i++) {
                        CA_DBG_PRINTF(5, ", 0x%016llx", *((uint64_t *)pCur));
                        pCur += 8;
                }
        }

	if (sampleType & PERF_SAMPLE_RAW) {
		rec.raw_size = *((u32*)pCur);
		pCur += 4;
		
		rec.raw_data = (void *)pCur;
		CA_DBG_PRINTF(5, ", raw_data[%u]: ", rec.raw_size);
		for (int i = 0 ; i < rec.raw_size; i++) {
			CA_DBG_PRINTF(5, "%x", (u32*) pCur);
			pCur += 1;
		}
	}
	
	CA_DBG_PRINTF(5, "\n");

	return S_OK;
}



int dumpData()
{
	CA_DBG_PRINTF(5, "\n-------- PERF DATA SECTION -------- \n");

	int retVal = S_OK;
	struct perf_event_header hdr;
	void * pBuf = NULL;
	int curBufSz = 0;
	ssize_t rdSz = 0;
	uint32_t recCnt = 0;

	if (reader.getFirstRecord(&hdr, (const void **)(&pBuf)) != S_OK) {
		return E_FAIL;
	}

	do {
		if (hdr.size == 0 || !pBuf) {
			retVal = E_FAIL;
			break;
		}

		switch (hdr.type) {
		case PERF_RECORD_MMAP:
			dump_PERF_RECORD_MMAP(&hdr, pBuf, recCnt);
			break;
		case PERF_RECORD_COMM:
			dump_PERF_RECORD_COMM(&hdr, pBuf, recCnt);
			break;
		case PERF_RECORD_SAMPLE:
			dump_PERF_RECORD_SAMPLE(&hdr, pBuf, recCnt);
			break;
		default:
			reader.dump_PERF_RECORD(&hdr, pBuf, recCnt);
			break;
		}

		recCnt++;
	} while	(reader.getNextRecord(&hdr, (const void **)(&pBuf)) == S_OK);

	return S_OK;
}
