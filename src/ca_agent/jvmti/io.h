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
#ifndef _IO_H_
#define _IO_H_
#include <jvmti.h>
#include "jvmticmlr.h"
#include <stdlib.h>
#include <vector>
using namespace std;

#ifdef _INLINEINFO_
void printMethodTable(FILE* fp, vector<void*> method_table, vector<void*> line_number_tables, vector<jint> line_number_table_entry_counts);


/**
  * Prints the string table
  */
void printStringTable(jvmtiEnv *jvmti_env, FILE* fp, jmethodID id, vector<void*> string_table);


void printInlineInfoRecord(jvmtiEnv *jvmti_env,  FILE* fp, jvmtiCompiledMethodLoadInlineRecord* record);


void printMethodTable(FILE* fp, vector<void*> method_table);
void printStringTable(FILE* fp, vector<void*> string_table);
void printInlineInfoRecord(FILE* fp, jvmtiCompiledMethodLoadInlineRecord* record);
void freeInlineInfoRecord(jvmtiCompiledMethodLoadInlineRecord * record);
void freeMethodTable(vector<void *> method_table);
void freeMethodTable(vector<void *> method_table, vector<void*> line_number_tables);
void freeJvmtiMethodTable(jvmtiEnv *jvmti_env, vector<void *> method_table, vector<void*> line_number_tables);
#endif

#endif
