#ifdef _INLINEINFO_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/limits.h>
#include <sys/time.h>

#include <map>
#include <vector>
#include <set>
#include <list>
#include <string> 

#include <jvmti.h>
#include <jvmticmlr.h>

#include "bctosrc.h"
#include "agent.h"

using namespace std;

#ifdef _DEBUGINLINE_
long total_jnc_write_time = 0;
long total_voidptr_traversal_time = 0;
int num_jnc_writes = 0;
long avg_inline_info_size = 0;
long avg_jitted_code_size = 0;

int num_compiled_method_loads = 0;
int num_dynamic_code_generated=0;
long total_dynamic_generated_code_size = 0;
long total_inline_info_size = 0;
long total_pc_stack_info_size = 0;
long total_functions_with_inlined_code = 0;
long total_jitted_code_size = 0;
long total_inlining_depth = 0;
long max_inlining_depth = 0;
#endif


/**
  * Checks whether a compiled method contains inlined methods
  */
static int hasInlinedMethods(jvmtiCompiledMethodLoadInlineRecord* record)
{
	if(record != NULL && record->pcinfo != NULL)
	{
		for( int i = 0; i < record->numpcs; i++)
		{
			PCStackInfo pcinfo = record->pcinfo[i];

			if(pcinfo.numstackframes > 1)
				return 1;
		}
	}
	return 0;
}


/**
  * Writes the jvmtiCompiledMethodLoadInlineRecord to a blob in memory that will  
  * eventually be written out to a JNC file. The blob has the following format:
  *
  * Structure of inline information blob
  *
  * total_size
  * numpcs
  * pcinfo<1>
  * .
  * . 
  * .
  * pcinfo<n>
  *
  * where each pcinfo<i> has the form:
  *
  * pc
  * numstackframes
  * methodid0
  * methodid1
  * .
  * .
  * .
  * methodid(numstackframes-1)
  * bcindex0
  * bcindex1
  * .
  * .
  * .
  * bcindex(numstackframes-1)
  **/
static unsigned int createInlineDataBlob(
	jvmtiCompiledMethodLoadInlineRecord* record, 
	jint inline_info_size, 
	void*& inline_info_blob)
{
	jint total_size = inline_info_size + sizeof(jint) + sizeof(jint);
	inline_info_blob=malloc(total_size);
#ifdef _DEBUG_MALLOC_
	fprintf(stderr, "malloctest %p\n", inline_info_blob);
#endif
	int offset = 0;
	memcpy((char *)inline_info_blob+offset, &inline_info_size, sizeof(jint));
	offset += sizeof(jint);
	memcpy((char *)inline_info_blob+offset, &(record->numpcs), sizeof(jint));
	offset += sizeof(jint);
	for( int i = 0; i < record->numpcs; i++)
	{
		PCStackInfo pcinfo = record->pcinfo[i];
		jmethodID* method_id_array = pcinfo.methods;
		jint* bc_index_array = pcinfo.bcis;
		memcpy((char *)inline_info_blob+offset, &pcinfo.pc, sizeof(void *));
		offset += sizeof(void *);
		memcpy((char *)inline_info_blob+offset, &pcinfo.numstackframes, sizeof(jint));
		offset += sizeof(jint);
		for(int j = 0; j < pcinfo.numstackframes; j++)
		{
			jmethodID inlined_method_id = method_id_array[j];
			memcpy((char *)inline_info_blob+offset, &inlined_method_id, sizeof(jmethodID));
			offset += sizeof(jmethodID);
		}	
		for(int j = 0; j < pcinfo.numstackframes; j++)
		{
			jint bc_index = bc_index_array[j];
			memcpy((char *)inline_info_blob+offset, &bc_index, sizeof(jint));
			offset += sizeof(jint);
		}
	}
	return total_size;	
}


/**
  * Scans the jvmtiCompiledMethodLoadInlineRecord
  * data structure in a single pass and builds a table of
  * pc address ranges for each inlined method id. 
  * This is the first step to building the method id mapping table (bc2src). 
  */
static void buildInlineAddressRanges(
	jvmtiCompiledMethodLoadInlineRecord* record,
	vector<void*>& global_address_ranges,
	jint& bc2src_table_size,
	const void* jitted_code_addr,
	jint jitted_code_size)
{
	if (!record) {
		return;
	}

	vector<AddressRange>local_stack_frames;

	for( int i = 0; i < record->numpcs; i++)
	{

		PCStackInfo pcinfo = record->pcinfo[i];
		jmethodID* method_id_array = pcinfo.methods;
		for(int j = 0; j < pcinfo.numstackframes; j++)
		{

			jmethodID inlined_method_id = method_id_array[pcinfo.numstackframes - 1 - j];


			/// Add new element to list
			if(j > (int)local_stack_frames.size()-1)
			{
				AddressRange newelt;
				newelt.id = inlined_method_id;
				newelt.pc_start = pcinfo.pc;  
				newelt.pc_end = (void *)((unsigned long)pcinfo.pc + 1);            
				local_stack_frames.push_back(newelt);
			}
			else
			{ 
				AddressRange* local_stack_method = &local_stack_frames.at(j);
				if(local_stack_method->id == inlined_method_id)
				{
					local_stack_method->pc_end = pcinfo.pc;
				}
				else
				{

					// remove the method with different id and all its descendents 
					// from the local stack. Send these completed methods to the 
					// global stack. 
					// We should never reach thiz case, but just in case... 
					while((int)local_stack_frames.size() > j)
					{
						AddressRange* completed_method = new AddressRange();
						#ifdef _DEBUG_MALLOC_
						fprintf(stderr, "malloctest %p\n", completed_method);
						#endif
						completed_method->id = local_stack_frames.back().id;
						completed_method->pc_start = local_stack_frames.back().pc_start;
						if(i < record->numpcs - 1) {
							completed_method->pc_end = pcinfo.pc;
						} else {
							completed_method->pc_end = 
								(void *)((unsigned long)jitted_code_addr + (unsigned long)jitted_code_size);
						}

						global_address_ranges.push_back((void *)completed_method);
						local_stack_frames.pop_back();
					}

					/// Add method with new id to local stack
					AddressRange new_invocation;
					new_invocation.id = inlined_method_id;
					new_invocation.pc_start = pcinfo.pc;
					new_invocation.pc_end = pcinfo.pc;
					local_stack_frames.push_back(new_invocation);
				}
			} 
		}

		/// Send all done elements to global list
		int final_stack_size = (i < record->numpcs - 1)? pcinfo.numstackframes : 0;

		while((int)local_stack_frames.size() > final_stack_size)
		{
			AddressRange* completed_method = new AddressRange();

			#ifdef _DEBUG_MALLOC_
			fprintf(stderr, "malloctest %p\n", completed_method);
			#endif

			completed_method->id = local_stack_frames.back().id;
			completed_method->pc_start = local_stack_frames.back().pc_start;
			if(i < record->numpcs - 1) {
				completed_method->pc_end = pcinfo.pc;

			} else {
			 	completed_method->pc_end = 
					(void *)((unsigned long)jitted_code_addr + 
					(unsigned long)jitted_code_size);
			}

			global_address_ranges.push_back((void *)completed_method);
			local_stack_frames.pop_back();
		}
	}
	bc2src_table_size = global_address_ranges.size()*sizeof(AddressRange);
}


/**
  * Compute the total size of this jvmtiCompiledMethodLoadInlineRecord
  */
static jint computeInlineInfoSize(jvmtiCompiledMethodLoadInlineRecord* record)
{
	jint size = 0;
	if(record != NULL && record->pcinfo != NULL)
	{
		size += sizeof(jvmtiCompiledMethodLoadInlineRecord);
		for( int i = 0; i < record->numpcs; i++)
		{
			PCStackInfo pcinfo = record->pcinfo[i];
			size += sizeof(PCStackInfo) + 
				(pcinfo.numstackframes*(sizeof(jmethodID) + sizeof(jint)));
		}   
	}
	return size;
}


/** 
  * Compute the total size of all pcstackinfo structures 
  * in this jvmtiCompiledMethodLoadInlineRecord
  */
static long computePCStackInfoSize(jvmtiCompiledMethodLoadInlineRecord* record)
{
	long size = 0;
	if(record != NULL && record->pcinfo != NULL)
	{
		for( int i = 0; i < record->numpcs; i++)
		{
			PCStackInfo pcinfo = record->pcinfo[i];
			size += sizeof(PCStackInfo) 
				+ (pcinfo.numstackframes*(sizeof(jmethodID) 
				+ sizeof(jint)));
		}
	}
	return size;
}


/**
  * Compute the maximum depth of inlining for this method
  */
static int computeMaxInliningDepth(jvmtiCompiledMethodLoadInlineRecord* record)
{
	int maxdepth = 0;

	if(record != NULL && record->pcinfo != NULL)
	{
		for( int i = 0; i < record->numpcs; i++)
		{
			PCStackInfo pcinfo = record->pcinfo[i];

			if(pcinfo.numstackframes > maxdepth)
				maxdepth = pcinfo.numstackframes; 
		}
	}
	return maxdepth;

}


long total_line_number_table_size = 0;
std::map<jmethodID, long> global_line_table_sizes;

/**
  * Compute the total size of all line number tables for this method
  */
static long computeLineNumberTableSize(
	jvmtiEnv *jvmti_env,
	jmethodID enclosing_method,
	jvmtiCompiledMethodLoadInlineRecord* record)
{

	long total_size = 0;
	long entry_size = 0;
	std::map<jmethodID, long>line_number_table_map;
	jvmtiLineNumberEntry* table_ptr = NULL;
	jint entry_count_ptr = 0;

	//fprintf(stderr, "computeLineNumberTableSize methodid %d\n", enclosing_method);
	if(record == NULL || record->pcinfo == NULL)
		return total_size;
	
	for( int i = 0; i < record->numpcs; i++)
	{
		PCStackInfo pcinfo = record->pcinfo[i];
		jmethodID* method_id_array = pcinfo.methods;
		//fprintf(stderr, "pcinfo[%d]\n", i);            
		for(int j = 0; j < pcinfo.numstackframes; j++)
		{
			jmethodID id = method_id_array[j];
			//char * name_ptr = NULL;
			//jvmti_env->GetMethodName(id, &name_ptr, NULL, NULL);

			if(line_number_table_map.find(id) != line_number_table_map.end())
				continue;

			char * name_ptr = NULL;
			jvmti_env->GetMethodName(id, &name_ptr, NULL, NULL);
			jvmti_env->GetLineNumberTable(id,&entry_count_ptr,&table_ptr);
			entry_size = (entry_count_ptr)*(sizeof(jvmtiLineNumberEntry));

			if(id != enclosing_method)
				total_size+=entry_size;

			line_number_table_map[id] = entry_size;
			if(global_line_table_sizes.find(id) == global_line_table_sizes.end())
			{  
				global_line_table_sizes[id] = entry_size;
				total_line_number_table_size += entry_size;
			}
		}
	}

	//fprintf(stderr, "total line number table sizes: %ld\n", total_size);
	return total_size;
}


static void printInlinedStats()
{
#ifdef _DEBUGINLINE_
	fprintf(stderr, "****Overall Performance statistics****\n\n");
	fprintf(stderr, "Total number of Jitted methods: %d\n", 
		num_compiled_method_loads);
	fprintf(stderr, "Total number of Top Level Jitted methods with inlined code: %ld \n", 
		total_functions_with_inlined_code);
	fprintf(stderr, "Total number of dynamicCodeGenerated events: %d\n", 
		num_dynamic_code_generated);
	fprintf(stderr, "Total jitted code size: %ld bytes\n", 
		total_jitted_code_size);
	fprintf(stderr, "Average jitted code size per compiled method: %ld bytes\n", 
		(total_jitted_code_size/num_compiled_method_loads));
	fprintf(stderr, "Total dynamically generated code size: %ld bytes\n", 
		total_dynamic_generated_code_size);
	fprintf(stderr, "Average code size per dynamically generated method %ld bytes\n", 
		(total_dynamic_generated_code_size/num_dynamic_code_generated));

	fprintf(stderr, "Total inlinerecord size %ld bytes\n", 
		total_inline_info_size);
	fprintf(stderr, "Average inlinerecord size per compiled method: %ld bytes\n", 
		(total_inline_info_size/num_compiled_method_loads));
	fprintf(stderr, "Total pcstackinfo size %ld bytes\n", 
		total_pc_stack_info_size);
	fprintf(stderr, "Average pcstackinfo size %ld bytes\n", 
		(total_pc_stack_info_size/num_compiled_method_loads));

	fprintf(stderr, "Total inlining depth: %ld\n", 
		(total_inlining_depth));
	fprintf(stderr, "Average inlining depth per compiled method: %ld\n", 
		((total_inlining_depth/num_compiled_method_loads)));
	fprintf(stderr, "Max inlining depth overall %ld\n", 
		max_inlining_depth);
	fprintf(stderr, "Total size of JvmtiLineNumberEntry[]: %ld bytes\n", 
		total_line_number_table_size);

	fprintf(stderr, "Average size of JvmtiLineNumberEntry[] per compiled method: %ld bytes\n", 
		total_line_number_table_size/num_compiled_method_loads);

	fprintf(stderr, "Total void pointer data traversal time %ld microseconds\n", 
		total_voidptr_traversal_time);
	fprintf(stderr, "Average void pointer data traversal time %ld microseconds\n", 
		(total_voidptr_traversal_time)/num_jnc_writes);
	fprintf(stderr, "Total time spent in JNC file writes: %ld microseconds\n", 
		total_jnc_write_time);

	fprintf(stderr, "Average time for JNC file writes: %ld microseconds\n", 
		(total_jnc_write_time/num_jnc_writes)) ;

	fprintf(stderr, "Total Jitted code size: %ld bytes\n", 
		avg_jitted_code_size);
	fprintf(stderr, "Avg Jitted code size: %ld bytes\n", 
		(avg_jitted_code_size/num_jnc_writes)); 

	fprintf(stderr, "Total inline info size %ld bytes\n", 
		avg_inline_info_size);
	fprintf(stderr, "Average inline info size: %ld bytes\n", 
		(avg_inline_info_size/num_jnc_writes));
#endif	
}


static void debug_dump(
	jvmtiEnv *jvmti_env,
	jmethodID method,
	jint code_size,
	const void* code_addr,
	const void* compile_info)
{
#ifdef _DEBUGINLINE_
	fprintf(stderr, "Method id: %d\n", method);
	fprintf(stderr, "Start address: %ld\n", (unsigned long)code_addr);
	fprintf(stderr, "End address: %ld\n", 
		(unsigned long)((unsigned long)code_addr + (unsigned long)code_size));

	num_compiled_method_loads++;

	long inline_info_size = 0;
	long pc_stack_info_size = 0;
	long local_inlining_depth = 0;
	long local_line_number_table_size = 0;

	jvmtiCompiledMethodLoadRecordHeader* record = 
		(jvmtiCompiledMethodLoadRecordHeader*) compile_info;

	if(record != NULL && record->kind == JVMTI_CMLR_INLINE_INFO)
	{
		inline_info_size = computeInlineInfoSize((jvmtiCompiledMethodLoadInlineRecord *)record);
		pc_stack_info_size = computePCStackInfoSize(
			(jvmtiCompiledMethodLoadInlineRecord *)record);
		local_line_number_table_size = 
			computeLineNumberTableSize(jvmti_env,
				method,(jvmtiCompiledMethodLoadInlineRecord *)record );
		total_inline_info_size += inline_info_size;
		total_pc_stack_info_size += pc_stack_info_size;
		if(hasInlinedMethods((jvmtiCompiledMethodLoadInlineRecord *)record) == 1)
			total_functions_with_inlined_code ++;
		local_inlining_depth = computeMaxInliningDepth(
			(jvmtiCompiledMethodLoadInlineRecord *)record);
		total_inlining_depth += local_inlining_depth;
		if(local_inlining_depth > max_inlining_depth)
			max_inlining_depth = local_inlining_depth;

		jvmtiLineNumberEntry* table_ptr = NULL;
		jint entry_count_ptr = 0;
		jvmti_env->GetLineNumberTable(method, &entry_count_ptr, &table_ptr);
		fprintf(stderr, "Printing inlining statistics for this compiled method load\n");
		fprintf(stderr, "Size of InlineRecord: %ld bytes\n", inline_info_size); 
		fprintf(stderr, "Size of pc stack info array %ld bytes\n", pc_stack_info_size);
		fprintf(stderr, "Max inlining depth for this method %ld\n", local_inlining_depth);
		fprintf(stderr, "Size of jitted code: %ld bytes\n", code_size);
		fprintf(stderr, "Line number table size of this top level method: %ld bytes \n", 
			(entry_count_ptr*(sizeof(jvmtiLineNumberEntry))));
		fprintf(stderr, "Sum of all line number table sizes of inlined methods: %ld bytes\n\n", 
			local_line_number_table_size);
	}
	total_jitted_code_size += code_size;
#endif
}

void JNICALL
writeJNCWithInlineInfo(jvmtiEnv *jvmti_env,
	jmethodID method,
	jint code_size,
	const void* code_addr,
	jint map_length,
	const jvmtiAddrLocationMap* map,
	const void* compile_info)
{
	string str_name = "UnknownClass";

	/* Get the declaring class of the method */
	jclass declaringClass;
	jvmti_env->GetMethodDeclaringClass(method, &declaringClass);
	
	char * classSig = NULL;
	jvmti_env->GetClassSignature(declaringClass, &classSig, NULL);
	if (classSig) {
		str_name = classSig;
		str_name += "::";
	}

	char * methodName = NULL;
	jvmti_env->GetMethodName(method, &methodName, NULL, NULL);
	if (methodName) {
		str_name += methodName;
	} else {
		str_name += "UnknownFunction";
	}

	
#ifdef _DEBUGINLINE_
	char inline_info_file_name[PATH_MAX];
#endif
	/// Access compile_info void pointer which contains inlining information	
	jint inline_info_size = 0;
	jvmtiCompiledMethodLoadRecordHeader* record = 
		(jvmtiCompiledMethodLoadRecordHeader*) compile_info;
	if (!record)
		return;

	if (record != NULL 
	&&  record->kind == JVMTI_CMLR_INLINE_INFO)
	{
		inline_info_size = computeInlineInfoSize(
			(jvmtiCompiledMethodLoadInlineRecord *)record);
	}
//        jvmti_env->RawMonitorEnter(lock);
#ifdef _DEBUGINLINE_
	struct timeval startjncwrite, endjncwrite, starttraversal,endtraversal;
	long elapsed_sec;
	long elapsed_usec;
	long elapsed_utime;
	gettimeofday(&startjncwrite, NULL);
#endif
	vector<void*> global_address_ranges;
	vector<void*> string_table;
	vector<void*> line_number_tables;
	vector<jint> line_number_table_entry_counts;
	jint bc2src_table_size;
	jint string_table_size = 0;
	jint line_number_table_size = 0;
	void* string_table_blob = NULL;
	void* method_table_blob = NULL;
	jint method_table_blob_size = 0;
	void* inline_data_blob = NULL;

	/// Build table mapping inlined method id's to pc address ranges
	buildInlineAddressRanges((jvmtiCompiledMethodLoadInlineRecord *)record, 
			global_address_ranges, bc2src_table_size, code_addr, code_size);

	/// Populate method id mapping table with offsets into the string table and line number tables 
	buildBc2SrcTable(jvmti_env, global_address_ranges, string_table, 
			line_number_tables, line_number_table_entry_counts, 
			string_table_size, line_number_table_size);

	/// Write string table to memory
	createStringTableBlob(string_table, string_table_blob, string_table_size);

	/// Write method id mapping table (.bc2src) to memory
	createMethodTableBlob(jvmti_env, global_address_ranges, line_number_tables, 
			line_number_table_entry_counts, method_table_blob, 
			bc2src_table_size, line_number_table_size, method_table_blob_size);

	inline_info_size = createInlineDataBlob(
			(jvmtiCompiledMethodLoadInlineRecord *)record, 
			inline_info_size, inline_data_blob);

	// Call routine that writes the JNC file, passing it 
	// pointers to the inlining data, method id mapping table, 
	// and string table.
	ca_write_native_code(
		str_name.c_str(), 
		code_addr, code_size, 
		inline_data_blob, inline_info_size, 
		method_table_blob, (jint)method_table_blob_size, 
		string_table_blob, (jint)string_table_size);

#ifdef _DEBUG_MALLOC_
fprintf(stderr, "freetest %p\n", inline_data_blob);
#endif
	if (inline_data_blob) {
		free(inline_data_blob);
		inline_data_blob = NULL;
	}
#ifdef _DEBUG_MALLOC_
fprintf(stderr, "freetest %p\n", method_table_blob);
#endif
	if (method_table_blob) {
		free(method_table_blob);
		method_table_blob = NULL;
	}
#ifdef _DEBUG_MALLOC_
fprintf(stderr, "freetest %p\n", string_table_blob);
#endif
	if (string_table_blob) {
		free(string_table_blob);
		string_table_blob = NULL;
	}
	

#ifdef _DEBUGINLINE_
	fprintf(stderr, "Printing statistics for this method load event\n");
	fprintf(stderr, "bfd_name from agent %s\n", inline_info_file_name);
	fprintf(stderr, "string_table_size %ld\n", string_table_size);
	fprintf(stderr, "method table blob size %ld\n", method_table_blob_size);
	fprintf(stderr, "inline_info_blob_size %ld\n", inline_info_size);
#endif

	freeJvmtiMethodTable(jvmti_env, global_address_ranges, line_number_tables);

#ifdef _DEBUGINLINE_	
	gettimeofday(&endjncwrite, NULL);
	gettimeofday(&endjncwrite, NULL);
	elapsed_sec = endjncwrite.tv_sec - startjncwrite.tv_sec;
	elapsed_usec = endjncwrite.tv_usec - startjncwrite.tv_usec;

	elapsed_utime = elapsed_sec * 1000000 + elapsed_usec;
	total_jnc_write_time += elapsed_utime;
	num_jnc_writes ++;  
	avg_inline_info_size += inline_info_size;
	avg_jitted_code_size += code_size;

	fprintf(stderr, "Elapsed time for JNC file write %ld microseconds\n", elapsed_utime);
	fprintf(stderr, "Size of void pointer data: %d bytes\n", inline_info_size);  
	fprintf(stderr, "Size of jitted code: %d bytes\n", code_size);
#endif

	/* A mock test to measure the worst-case time for traversal 
	 * the entire data structure passed through the void pointer 
	 */


	//fprintf(stdout, 
	//	"Elapsed time for void ptr data traversal %ld microseconds\n", 
	//	elapsed_utime);

//        jvmti_env->RawMonitorExit(lock);

	// need to de-allocate the methodName and classSig;
	if (NULL != classSig)
	    jvmti_env->Deallocate ((unsigned char *) classSig);

	if (NULL != methodName)
	    jvmti_env->Deallocate ((unsigned char *) methodName);
#ifdef _DEBUGINLINE_
	debug_dump(jvmti_env, method, code_size, code_addr, compile_info);
#endif
}


#endif 
