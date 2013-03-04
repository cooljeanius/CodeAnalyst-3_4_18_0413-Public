#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <map>

#include "bctosrc.h"
#include "agent.h"

/**
  * Builds method id mapping table (bc2src). Each entry has the form:
  * <method id, pcstart, pcend, nameoffset, signatureoffset, 
	sourcefilenameoffset, linenumbertableoffset>
  * The name, signature and source file name offsets are offsets into a
  * string table. This function also populates the string table and a list of line nubmer tables.
  **/
void buildBc2SrcTable(jvmtiEnv *jvmti_env,
	vector<void*>& global_address_ranges,
	vector<void*>& string_table,
	vector<void*>& line_number_tables,
	vector<jint>& line_number_table_entry_counts,
	jint& string_table_size,
	jint& line_number_table_size)
{
	map<jmethodID,TableOffsets> id2table_offsets;
	char * name_ptr = NULL;
	string_table_size = 0;
	line_number_table_size = 0;

	jint method_section_size = 
		(int)global_address_ranges.size()*sizeof(AddressRange) + 2*sizeof(jint);

	//fprintf(stderr,"DEBUG: method_section_size = %d\n", method_section_size);
	//fprintf(stderr,"DEBUG: global_address_ranges size = %d\n", global_address_ranges.size());
	for(int i = 0; i < (int)global_address_ranges.size(); i++)
	{

		AddressRange* range = (AddressRange *)global_address_ranges.at(i);

		if(range == NULL)
			continue;
		
		jmethodID id = range->id;

		/// We haven't seen this method id before. Create a new entry for it.
		if(id2table_offsets.find(id) == id2table_offsets.end())
		{
			char * method_name_ptr = NULL;
			char * method_signature_ptr = NULL;
			char * source_name_ptr = NULL;
			jclass method_declaring_class;
			jvmtiLineNumberEntry* line_number_table_ptr = NULL;
			jint entry_count_ptr = 0;
			jlong line_number_table_offset = 0;
			jlong method_name_offset = 0;
			jlong method_signature_offset = 0;
			jlong source_file_name_offset = 0;

			/// Get all of the information associated with this method id
			jvmti_env->GetMethodName(id, &method_name_ptr, &method_signature_ptr, NULL);
			jvmti_env->GetMethodDeclaringClass(id, &method_declaring_class);
			jvmti_env->GetSourceFileName(method_declaring_class, &source_name_ptr);
			if(method_name_ptr == NULL)
				method_name_ptr = "unknown method";
			if(method_signature_ptr == NULL)
				method_signature_ptr = "unknown signature";
			if(source_name_ptr == NULL)
				source_name_ptr = "unknown source file";
			jvmti_env->GetLineNumberTable(id,&entry_count_ptr,&line_number_table_ptr);
			
			/// Add the name, signature and source file name to the string table
			string_table.push_back((void *)method_name_ptr);
			string_table.push_back((void *)method_signature_ptr);
			string_table.push_back((void *)source_name_ptr);

			/// Add the line number table for this method to list 
			/// of line number tables and compute the offset 
			/// of this method's line number table in that list.
			line_number_tables.push_back((void *)line_number_table_ptr);
			line_number_table_entry_counts.push_back(entry_count_ptr);
			line_number_table_offset = line_number_table_size;
			line_number_table_size += sizeof(jint) + 
				(entry_count_ptr)*(sizeof(jvmtiLineNumberEntry));

			/// Compute the offsets of this method's name, 
			/// signature and source file name in the string table
			method_name_offset = string_table_size;
			jint method_name_size =  strlen(method_name_ptr) + sizeof(char);
			jint method_signature_size = strlen(method_signature_ptr) + sizeof(char);
			jint source_file_name_size = strlen(source_name_ptr) + sizeof(char);
			method_signature_offset = method_name_offset + method_name_size;
			source_file_name_offset = method_signature_offset + method_signature_size;
			string_table_size += method_name_size + 
				method_signature_size + source_file_name_size;
			/// Write the string table and line number table offsets for this entry
			range->method_name_offset = method_name_offset;
			range->method_signature_offset = method_signature_offset;
			range->source_name_offset = source_file_name_offset;
			range->line_number_table_offset = method_section_size + line_number_table_offset;
			TableOffsets t;
			t.method_name_offset = method_name_offset;
			t.source_name_offset = source_file_name_offset;
			t.method_signature_offset = method_signature_offset;
			t.line_number_table_offset = method_section_size + line_number_table_offset;

			id2table_offsets[id] = t; 
		}

		/// We have seen this method id before. 
		/// Just update the string table and line number table offsets
		else
		{
			TableOffsets t = id2table_offsets.find(id)->second;
			range->method_name_offset = t.method_name_offset;
			range->method_signature_offset = t.method_signature_offset;
			range->line_number_table_offset = t.line_number_table_offset;
			range->source_name_offset = t.source_name_offset;
		}   
	}
}



/**
  * Writes method id mapping table (bc2src) to memory in the format in which 
  * it will later be written to the JNC file. 
  *
  * Format of the method table blob
  * <Header>
  * sizeof(methodtable)
  * sizeof(line_number_tables)
  * <MethodTable>
  *  methodtableentry1
  *  methodtableentry2
  *  .
  *  .
  *  .
  *  methodtableentryk
  * <LineNumberTable1>
  *  numentries = k1
  *  linenumberentry1
  *  linenumberentry2
  *  .
  *  .
  *  .
  *  linenumberentryk1
  * .
  * .
  * .
  * <LineNumberTablen>
  *  numentries = kn
  *  linenumberentry1
  *  linenumberentry2
  *  .
  *  .
  *  .
  *  linenumberentrykn
  *
  **/
void createMethodTableBlob(
	jvmtiEnv* jvmti_env, 
	vector<void *>& method_table, 
	vector<void*>& line_number_tables, 
	vector<jint>& line_number_table_entry_counts, 
	void*& method_table_blob, 
	jint method_table_size, 
	jint line_number_table_size, 
	jint& method_table_blob_size)
{

	method_table_blob_size = 2*sizeof(jint) + method_table_size + line_number_table_size;

	method_table_blob = malloc(method_table_blob_size);
	
	int offset = 0;

	/// Copy the header information
	memcpy((char *)method_table_blob + offset, &method_table_size, sizeof(jint));
	offset += sizeof(jint);
	memcpy((char *)method_table_blob + offset, &line_number_table_size, sizeof(jint));
	offset += sizeof(jint);	

	/// Copy each method table entry
	for(int i = 0; i < method_table.size(); i++)
	{
		AddressRange* entry = (AddressRange *)method_table.at(i);
		memcpy((char *)method_table_blob + offset, entry, sizeof(AddressRange));
		offset += sizeof(AddressRange);
	}

	/// Copy each line number table. Each table is prefixed 
	/// with a header indicating the number of entries.
	for(int i = 0; i < line_number_tables.size(); i++)
	{

		jvmtiLineNumberEntry* line_number_table_ptr = 
			(jvmtiLineNumberEntry*)line_number_tables.at(i);
		jint entry_count = line_number_table_entry_counts.at(i);
		memcpy((char *)method_table_blob + offset, &entry_count, sizeof(jint));
		offset += sizeof(jint);

		for(int j = 0; j < entry_count; j++)
		{
			jvmtiLineNumberEntry line_number_table_entry = line_number_table_ptr[j];
			memcpy((char *)method_table_blob+offset, 
				&line_number_table_ptr[j], sizeof(jvmtiLineNumberEntry));
			offset += sizeof(jvmtiLineNumberEntry);

		}

	}
}


/**
  * Writes the string table and its header information to a blob in memory
  * which will eventually be written to the JNC file. The string table is
  * preceded by its size in bytes. This information is needed for postprocessing
  * tools.
  */
void createStringTableBlob(
	vector<void*>& string_table, 
	void*& string_table_blob, 
	jint& string_table_size)
{
	string_table_blob = malloc(sizeof(char)*string_table_size + sizeof(jint));
	int offset = 0;

	memcpy((char *)string_table_blob+offset, &string_table_size, sizeof(jint));

	offset += sizeof(jint);
	for(int i = 0; i < string_table.size(); i++)
	{
		char* str = (char *)string_table.at(i);
		strcpy((char *)string_table_blob+offset, str);
		offset += strlen(str) + 1;
	}
	string_table_size += sizeof(jint);
	
}


void buildAddressRanges(
	jmethodID method,
	vector<void*>& global_address_ranges,
	jint& bc2src_table_size,
	const void* jitted_code_addr,
	jint jitted_code_size)
{
	AddressRange* range = new AddressRange();
	range->id = method;
	range->pc_start = (void*) jitted_code_addr;
	range->pc_end = (void*) ((char*)jitted_code_addr + jitted_code_size);
	global_address_ranges.push_back((void *)range);

	bc2src_table_size = global_address_ranges.size()*sizeof(AddressRange);
}


/**
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
int createJNCMethodLoadLineInfoBlob(
		jvmtiEnv *jvmti_env, 
		jmethodID *pMethodId, 
		const jvmtiAddrLocationMap* pLocMap,
		jint locMapSize,
		void *& lineInfoBlob,
		jint & lineInfoBlobSize)
{
	if (!pLocMap) {
		return -1;
	}

	unsigned int sizeOfEachPcStackInfo = 
		sizeof(void*) + 
		sizeof(jint) + 
		sizeof(jmethodID) + 
		sizeof(jint);

	lineInfoBlobSize = 
		(sizeOfEachPcStackInfo * locMapSize) + 
		sizeof(jint) +  // numpcs
		sizeof(jint);   // totalSize	

	lineInfoBlob = malloc(lineInfoBlobSize);
	if (lineInfoBlob== NULL)
		return -1;

	int offset = 0;

	// Set totalSize
	memcpy ((char *)lineInfoBlob + offset, &lineInfoBlobSize, sizeof(jint));
	offset += sizeof(jint);

	// Set number of PCs 
	memcpy ((char *)lineInfoBlob + offset, &locMapSize, sizeof(jint));
	offset += sizeof(jint);

	// Set each JNCPCStackInfo
	for (unsigned int i = 0 ; i < locMapSize; i++) {
		// pc
		memcpy ((char *)lineInfoBlob + offset, &(pLocMap[i].start_address), sizeof(void*));
		offset += sizeof(void*);	
		// numstackframes = 1;
		jint numstackframe = 1;
		memcpy ((char *)lineInfoBlob + offset, &numstackframe, sizeof(jint));
		offset += sizeof(jint);	
		// methods		
		memcpy ((char *)lineInfoBlob + offset, pMethodId, sizeof(jmethodID));
		offset += sizeof(jmethodID);
		// bcis
		memcpy ((char *)lineInfoBlob + offset, &(pLocMap[i].location), sizeof(jint));
		offset += sizeof(jint);
	}
	return 0;
}


void freeJvmtiMethodTable(
	jvmtiEnv *jvmti_env, 
	vector<void *> method_table, 
	vector<void*> line_number_tables)
{
	for(int i = 0; i < (int)method_table.size(); i++)
        {
                AddressRange* range = (AddressRange *)method_table.at(i);
		if(range != NULL)
		{
			free(range);
			range = NULL;
		}
	}

	method_table.clear();
        for(int i = 0; i < line_number_tables.size(); i++)
        {
                jvmtiLineNumberEntry* line_number_table_ptr = 
			(jvmtiLineNumberEntry*)line_number_tables.at(i);
		if(line_number_table_ptr != NULL)
			jvmti_env->Deallocate((unsigned char *)line_number_table_ptr);
        }
	line_number_tables.clear();
}


