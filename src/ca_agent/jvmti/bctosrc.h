/*
© 2009 Sun Microsystems, Inc.  All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 */

#ifndef _BCTOSRC_H_
#define _BCTOSRC_H_

#include <vector>
#include <jvmti.h>

using namespace std;

typedef struct _AddressRange {
	jmethodID id;
	void* pc_start;
	void* pc_end;
	jlong method_name_offset;
	jlong method_signature_offset;
	jlong source_name_offset;
	jlong line_number_table_offset;
}AddressRange;

typedef struct _TableOffsets {
	jlong method_name_offset;
	jlong method_signature_offset;
	jlong source_name_offset;
	jlong line_number_table_offset;
}TableOffsets;

extern void buildBc2SrcTable(jvmtiEnv *jvmti_env,
	vector<void*>& global_address_ranges,
	vector<void*>& string_table,
	vector<void*>& line_number_tables,
	vector<jint>& line_number_table_entry_counts,
	jint& string_table_size,
	jint& line_number_table_size);

extern void createMethodTableBlob(
	jvmtiEnv* jvmti_env, 
	vector<void *>& method_table, 
	vector<void*>& line_number_tables, 
	vector<jint>& line_number_table_entry_counts, 
	void*& method_table_blob, 
	jint method_table_size, 
	jint line_number_table_size, 
	jint& method_table_blob_size);

extern void createStringTableBlob(
	vector<void*>& string_table, 
	void*& string_table_blob, 
	jint& string_table_size);

extern void buildAddressRanges(
	jmethodID method,
	vector<void*>& global_address_ranges,
	jint& bc2src_table_size,
	const void* jitted_code_addr,
	jint jitted_code_size);

extern int createJNCMethodLoadLineInfoBlob(
	jvmtiEnv *jvmti_env, 
	jmethodID *pMethodId, 
	const jvmtiAddrLocationMap* pLocMap,
	jint locMapSize,
	void *& lineInfoBlob,
	jint & lineinfoBlobSize);

extern void freeJvmtiMethodTable(
	jvmtiEnv *jvmti_env, 
	vector<void *> method_table, 
	vector<void*> line_number_tables);

#endif // _BCTOSRC_H_
