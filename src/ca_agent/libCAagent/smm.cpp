/**
 * @file smm.h
 *
 * @remark Read the file COPYING
 *
 * @author Lei Yu 
 * @author Jason Yeh
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <iostream>
#include <limits.h>
#include <string.h>

#include "smm.h"
#include "slock.h"
#include "config.h"

using namespace std;

namespace {
	// This is old logic from Lei:
	// key is defined as group id and page number
	// each page contains 512 entries. That means we have 
	// 51200 entries total.
	//void generate_key(pid_t tgid, unsigned int page, key_t * key)
	//{
	//	*key = tgid * 100 + page;
	//}


	// NOTE [Suravee]:
	// key is defined as group id and page number and value 0xCA.
	// - We have 256 pages (0x00 - 0xFF) 
	// - Each page contains 512 entries. 
	// - Total 131072 entries total per thread group id.
	// - TGID max is 0x8000 before wrapping over
	// - We need to add the key to make sure that the shmem belongs
	//   to CA agent.  This will be used for verification before
	//   cleaning up any left unused pages.
	void generate_agent_key(pid_t tgid, unsigned int page, key_t * key)
	{
		*key = (tgid << 16) | (page  << 8) | 0xCA;
	}
	
	bool is_valid_key(pid_t tgid, key_t key)
	{
		key_t mask = ~((tgid << 16) | 0xFFCA); 
		return ((mask & key) == 0);
	}

};  /// anonymous namespacesize

smm::smm()
{
}


void * smm::create_shared_memory(pid_t tgid, 
				unsigned int page_id,
				unsigned int size) 
{
	void *buffer = NULL;
	int shmid;
	key_t key;
	
	if (NULL != buffer)
		goto out;

	generate_agent_key(tgid, page_id, &key);
	if (key < 0) {
		goto out;
	}

	shmid = shmget(key, size, IPC_CREAT|0666);    
	if (shmid < 0) {
		goto out;
	}
    
    /// attach the segment to the data space.
    if ((buffer = shmat(shmid, NULL, 0)) == (void *) -1)  {
		buffer = NULL;
	}
    
out:
	return buffer;
}


void* smm::open_shared_memory(pid_t tgid, 
				unsigned int page_id,
				unsigned int size)
{
	void *buffer = NULL;
	int shmid;
	key_t key;

	generate_agent_key(tgid, page_id, &key);
	if(key < 0)
		goto out;

	if ((shmid = shmget(key, size, 0666)) < 0)
		goto out;

	buffer = (char*) shmat(shmid, NULL, SHM_RDONLY );
	if ( buffer == (char *) -1)
		buffer = NULL;

out:
	return buffer;
}


void smm::remove_shared_memory(pid_t tgid, 
				unsigned int page_id, unsigned int size)
{
	key_t key;
	int shmid;

	generate_agent_key(tgid, page_id, &key);
    if ((shmid = shmget(key, size, 0666)) > 0)
		shmctl(shmid, IPC_RMID, NULL);
}


void smm::cleanup_unused_shared_memory(bool bVerb)
{
	// Look for all shmem in the system
	struct shmid_ds ds;
	int max_shm_index = shmctl(0, SHM_INFO, &ds);

	if (bVerb)
		fprintf(stdout,"Number of existing shmem = %d\n", max_shm_index + 1);
	
	// For each shmem
	for (int i = 0 ; i <= max_shm_index; i++) 
	{
		if (bVerb) {
			fprintf(stdout,"\n");
			fprintf(stdout,"Index = %u: ", i);
		}

		// Get the shmem descriptor
		int shm_id = shmctl(i, SHM_STAT,&ds);
		if (shm_id < 0) {
			if (bVerb) fprintf(stdout,"Failed to get SHM_STAT.");
			continue;
		}

		if (bVerb) fprintf(stdout,"shmid = %u: ", shm_id);

		// Check number of attached process
		if (0 != ds.shm_nattch) {
			if (bVerb) fprintf(stdout,"shmem in used by %u processes.", 
				ds.shm_nattch);
			continue;
		}

		// Verify if this is from the CAagent shem from key
		if (!is_valid_key(
				ds.shm_cpid /*pid*/, 
				ds.shm_perm.__key /*key*/))
		{
			if (bVerb) fprintf(stdout,"Not belong to ca_agent.");
			continue;
		}

		unsigned int key = ds.shm_perm.__key;
		// Remove this shmem
		if (0 == shmctl(shm_id, IPC_RMID, NULL)) {
			if (bVerb) fprintf(stdout,"REMOVED shmem with key = %u (0x%x)", key, key);
		} else {
			if (bVerb) fprintf(stdout,"Failed to removed shmem.");

		}
		
	}
	if (bVerb) fprintf(stdout,"\n");
}
