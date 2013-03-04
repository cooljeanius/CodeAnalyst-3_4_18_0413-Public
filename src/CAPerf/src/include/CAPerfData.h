#ifndef __CAPERFDATA_H__
#define __CAPERFDATA_H__

#define CA_PATH_MAX 1024

// Task command name length
// From linux-src/include/linux/sched.h
#define TASK_COMM_LEN 16

#define CA_PERF_EV_BASE	0xE000

struct CA_PERF_RECORD_MMAP
{
	u32 pid, tid;
	u64 addr;
	u64 len;
	u64 pgoff;
	char filename[CA_PATH_MAX];
};

struct CA_PERF_RECORD_COMM
{
	u32 pid, tid;
	char comm[TASK_COMM_LEN];
};

/* NOTE:
 * This structure is from perf.h
 */
struct ip_callchain {
	uint64_t nr;
	uint64_t ips[0];
};

/* NOTE:
 * This structure should be the same as 
 * struct perf_sample in event.h
 */
struct CA_PERF_RECORD_SAMPLE
{
	u64 ip;
	u32 pid, tid;
	u64 time;
	u64 addr;
	u64 id;
	u64 stream_id;
	u64 period;
	u32 cpu; 
	u32 raw_size;
	void * raw_data;
	struct ip_callchain *callchain;
};

#endif //__CAPERFDATA_H__
