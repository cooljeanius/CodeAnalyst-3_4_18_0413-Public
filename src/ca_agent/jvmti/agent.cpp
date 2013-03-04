/**
 * @file libCAagent.cpp
 * Implementation of CAagent library
 *
 * @remark Read the file COPYING
 *
 * @author Jason Yeh
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "CAagent_buffer.h"
#include "config.h"
#include "../jnc/jncwriter.h"


static CAagent_buffer * shared_buffer = NULL;


int ca_open_agent(pid_t tgid)
{
	struct stat buf;
	int status = stat(CA_JIT_DIR, &buf);   

	if (status < 0 && ENOENT == errno)
		mkdir(CA_JIT_DIR, 0777);

	if (status < 0 && ENOENT == errno)
		mkdir(CA_JAVA_DIR, 0777);

	if (NULL == shared_buffer)
		shared_buffer = new CAagent_buffer();

	if (NULL == shared_buffer)
		return -1;

	return shared_buffer->open_agent(tgid);
}


void ca_close_agent(pid_t tgid)
{
	if (NULL != shared_buffer) {
		shared_buffer->close_agent(tgid);
		delete shared_buffer;
		shared_buffer = NULL;
	}
}


int ca_add_mapping(struct ca_jit_shm_entry const * new_entry)
{
	if (NULL != shared_buffer) 
		return shared_buffer->add_mapping(new_entry);
	else 
		return -1;
}


int ca_write_native_code(char const * symbol_name, 
                         const void* jitted_code_addr,
                         unsigned int jitted_code_size)
{
	//fprintf(stderr,"DEBUG: symbol_name (no pc2bc) = %s\n", symbol_name);
	CJNCWriter jncWriter;
	if (0 != jncWriter.init(symbol_name, jitted_code_addr, jitted_code_size))
		return -1;

	return jncWriter.write();
}

int ca_write_native_code(
                         char const * symbol_name,
                         const void* jitted_code_addr,
                         unsigned int jitted_code_size,
                         const void* pc2bc_blob,
                         unsigned int pc2bc_blob_size,
                         void* methodtableblob,
                         unsigned int methodtableblobsize,
                         void* stringtableblob,
                         unsigned int stringtableblobsize)
{
	//fprintf(stderr,"DEBUG: symbol_name = %s\n", symbol_name);
	int ret = -1; 
	CJNCWriter jncWriter;
	if (0 != jncWriter.init(symbol_name, jitted_code_addr, jitted_code_size))
		return ret;

	if (pc2bc_blob != NULL && pc2bc_blob_size > 0) {
		if (0 != jncWriter.addSection(".pc2bc", pc2bc_blob, pc2bc_blob_size, 
			NULL, 0, SEC_READONLY | SEC_HAS_CONTENTS))
			return ret;
	}

	if (stringtableblob != NULL && stringtableblobsize > 0) {
		if (0 != jncWriter.addSection(".stringtable", stringtableblob, stringtableblobsize,
			NULL, 0, SEC_READONLY | SEC_HAS_CONTENTS))
			return ret;
	}

	if (methodtableblob != NULL && methodtableblobsize > 0) {	
		if (0 != jncWriter.addSection(".bc2src", methodtableblob, methodtableblobsize,
			NULL, 0, SEC_READONLY | SEC_HAS_CONTENTS))
			return ret;
	}

	return jncWriter.write();
}
