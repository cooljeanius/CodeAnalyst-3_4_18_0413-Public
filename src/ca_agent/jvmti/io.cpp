/*
 2009 Sun Microsystems, Inc.  All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 */
#include "io.h" 
#include "bctosrc.h"
using namespace std;

#ifdef _INLINEINFO_
void freeJvmtiMethodTable(jvmtiEnv *jvmti_env, vector<void *> method_table, vector<void*> line_number_tables)
{
	if (!jvmti_env)
		return;

	for(int i = 0; i < (int)method_table.size(); i++)
        {
                AddressRange* range = (AddressRange *)method_table.at(i);
		if(range != NULL)
		{
			#ifdef _DEBUG_MALLOC_
			fprintf(stderr, "freetest %p\n", range);
			#endif
			free(range);
			range = NULL;
				
		}


	}

	method_table.clear();
        for(int i = 0; i < line_number_tables.size(); i++)
        {

                jvmtiLineNumberEntry* line_number_table_ptr = (jvmtiLineNumberEntry*)line_number_tables.at(i);
		if(line_number_table_ptr != NULL)
			jvmti_env->Deallocate((unsigned char *)line_number_table_ptr);

        }
	line_number_tables.clear();
}


//This version needs to be called from the popst-processing tool
void freeMethodTable(vector<void *> method_table, vector<void*> line_number_tables)
{
        for(int i = 0; i < (int)method_table.size(); i++)
        {
                AddressRange* range = (AddressRange *)method_table.at(i);
                if(range != NULL)
                {
			#ifdef _DEBUG_MALLOC_
			fprintf(stderr, "freetest %p\n", range);
			#endif
                        free(range);
                        range = NULL;



                }


        }

        method_table.clear();
        for(int i = 0; i < line_number_tables.size(); i++)
        {

                jvmtiLineNumberEntry* line_number_table_ptr = (jvmtiLineNumberEntry*)line_number_tables.at(i);
                if(line_number_table_ptr != NULL)
                {
			#ifdef _DEBUG_MALLOC_
			fprintf(stderr, "freetest %p\n", line_number_table_ptr);
                       	#endif
			free(line_number_table_ptr);
                        line_number_table_ptr = NULL;
                }

        }
        line_number_tables.clear();
}




void freeInlineInfoRecord(jvmtiCompiledMethodLoadInlineRecord * record)
{
	if(record != NULL)
        {
                int numpcs = record->numpcs;
                for(int i = 0; i < numpcs; i++)
                {
                        PCStackInfo pcrecord = (PCStackInfo)(record->pcinfo[i]);
			if(pcrecord.methods != NULL)
			{
				free(pcrecord.methods);
				pcrecord.methods = NULL;

			}
			if(pcrecord.bcis != NULL)
			{
				free(pcrecord.bcis);
				pcrecord.bcis = NULL;

			}
                }
		free(record->pcinfo);
		free(record);
        }





}

void printStringTable(FILE* fp, vector<void*> string_table)
{
        if(fp == NULL)
                return;
        fprintf(fp, "Printing String Table with size %d\n", (int)string_table.size());

        for(int i = 0; i < (int)string_table.size(); i++)
        {
                char* str = (char *)string_table.at(i);
                if(str == NULL)
                        fprintf(fp, "NULL\n");
                else
                        fprintf(fp, "%s\n", str);

        }
        fprintf(fp, "\n");
        fprintf(fp, "End of String table\n");
}


void printMethodTable(FILE* fp, vector<void*> method_table, vector<void*> line_number_tables, vector<jint> line_number_table_entry_counts)
{
        if(fp == NULL)
                return;

        fprintf(fp, "Printing methodtable with %d entries\n", (int)method_table.size());

        for(int i = 0; i < (int)method_table.size(); i++)
        {
                AddressRange* range = (AddressRange *)method_table.at(i);
                fprintf(fp, "methodtable[%d]\n", i);
                if(range == NULL)
                        fprintf(fp, "NULL address range\n");
                else
                {
                        fprintf(fp, "id = %d\n", (range->id));
                        fprintf(fp, "pcstart = 0x%lx\n", (unsigned long)(range->pc_start));
                        fprintf(fp, "pcend = 0x%lx\n", (unsigned long)(range->pc_end));
                        fprintf(fp, "methodnameoffset = %d\n", (range->method_name_offset));
                        fprintf(fp, "methodsignatureoffset = %d\n", (range->method_signature_offset));
                        fprintf(fp, "sourcenameoffset = %d\n", (range->source_name_offset));
                        fprintf(fp, "linenumbertableoffset = %d\n\n", (range->line_number_table_offset));
                }
        }
        fprintf(fp, "End of methodTable\n\n");

	fprintf(fp, "Printing LineNumberTables\n\n");

        for(int i = 0; i < line_number_tables.size(); i++)
        {

                jvmtiLineNumberEntry* line_number_table_ptr = (jvmtiLineNumberEntry*)line_number_tables.at(i);
                jint entry_count = line_number_table_entry_counts.at(i);
		fprintf(fp, "LineNumberTable %d\n\n", i);
                for(int j = 0; j < entry_count; j++)
                {
                        jvmtiLineNumberEntry line_number_table_entry = line_number_table_ptr[j];
                        jlocation location = line_number_table_entry.start_location;
                        jint line_number = line_number_table_entry.line_number;
			fprintf(fp, "Entry %d\n", j);
			fprintf(fp, "location: %lld line_number: %d\n\n", location, line_number);
                }

        }
	fprintf(fp, "End of LineNumberTable\n");


}

void printStackFrames(PCStackInfo* record, FILE* fp) {
  fprintf(fp, "Printing %d stack frames\n", record->numstackframes);
  if(record == NULL)
	fprintf(fp, "Null PCStackInfo record\n");
  else if(record->methods == NULL)
	fprintf(fp, "Null methodidarray\n");
  else if(record->bcis == NULL)
	fprintf(fp, "Null bcindexarray\n");
	
  if(record != NULL && record->methods != NULL && record->bcis != NULL) {
    int i;
    for(i = 0; i < record->numstackframes; i++) {

        fprintf(fp, "%d :: methodid %d @ bcis %d\n", i, record->methods[i], record->bcis[i]);
    }
  }
}

void printInlineInfoRecord(FILE* fp,  jvmtiCompiledMethodLoadInlineRecord* record)
{
        fprintf(fp, "Printing inline information record\n");
        if(record != NULL && record->pcinfo != NULL)
        {
                int numpcs = record->numpcs;
                for(int i = 0; i < numpcs; i++)
                {
                        PCStackInfo pcrecord = (PCStackInfo)(record->pcinfo[i]);
                        fprintf(fp, "PcDescriptor(pc=0x%lx):\n", (pcrecord.pc));
                        printStackFrames(&pcrecord, fp);
                }
        }
        fprintf(fp, "End of inline information record\n");
}

void printMethodTable(jvmtiEnv *jvmti_env, FILE* fp, jmethodID id, vector<void*> method_table, vector<void*>& line_number_tables, vector<jint>& line_number_table_entry_counts)
{
        if(fp == NULL)
                return;

        char * enclosing_method_name = NULL;
        char * enclosing_method_signature = NULL;

        jvmti_env->GetMethodName(id, &enclosing_method_name, &enclosing_method_signature, NULL);

        if(enclosing_method_name == NULL)
                enclosing_method_name = "Unknown Function";

        if(enclosing_method_signature == NULL)
                 enclosing_method_signature = "Unknown Signature";

        fprintf(fp, "PrintMethodTable of method %s with signature %s\n",enclosing_method_name,enclosing_method_signature);


        fprintf(fp, "Printing methodtable with %d entries\n", (int)method_table.size());

        for(int i = 0; i < (int)method_table.size(); i++)
        {
                AddressRange* range = (AddressRange *)method_table.at(i);
                fprintf(fp, "methodtable[%d]\n", i);
                if(range == NULL)
                        fprintf(fp, "NULL address range\n");
                else
                {
                        char * method_name_ptr = NULL;
                        char * method_signature_ptr = NULL;

                        jvmti_env->GetMethodName(range->id, &method_name_ptr, &method_signature_ptr, NULL);

                        if(method_name_ptr == NULL)
                                method_name_ptr = "NULL";
                        if(method_signature_ptr == NULL)
                                method_signature_ptr = "NULL";

                        fprintf(fp, "%s %s\n", method_name_ptr, method_signature_ptr);
                        fprintf(fp, "id = %d\n", (range->id));
                        fprintf(fp, "pcstart = 0x%lx\n", (unsigned long)(range->pc_start));
                        fprintf(fp, "pcend = 0x%lx\n", (unsigned long)(range->pc_end));
                        fprintf(fp, "methodnameoffset = %d\n", (range->method_name_offset));
                        fprintf(fp, "methodsignatureoffset = %d\n", (range->method_signature_offset));
                        fprintf(fp, "sourcenameoffset = %d\n", (range->source_name_offset));
                        fprintf(fp, "linenumbertableoffset = %d\n\n", (range->line_number_table_offset));

                }
        }
        fprintf(fp, "End of methodTable of %s %s\n", enclosing_method_name,enclosing_method_signature);
 
	fprintf(fp, "Printing LineNumberTable of method %s with signature %s\n",enclosing_method_name,enclosing_method_signature);

        for(int i = 0; i < line_number_tables.size(); i++)
        {

                jvmtiLineNumberEntry* line_number_table_ptr = (jvmtiLineNumberEntry*)line_number_tables.at(i);
                jint entry_count = line_number_table_entry_counts.at(i);

                for(int j = 0; j < entry_count; j++)
                {
                        jvmtiLineNumberEntry line_number_table_entry = line_number_table_ptr[j];
			jlocation location = line_number_table_entry.start_location;
			jint line_number = line_number_table_entry.line_number;
                }

        }


 	fprintf(fp, "End of LineNumberTable of %s %s\n", enclosing_method_name,enclosing_method_signature);
	

}

void printStringTable(jvmtiEnv *jvmti_env, FILE* fp, jmethodID id, vector<void*> string_table)
{
        if(fp == NULL)
                return;

        char * enclosing_method_name = NULL;
        char * enclosing_method_signature = NULL;

        jvmti_env->GetMethodName(id, &enclosing_method_name, &enclosing_method_signature, NULL);

        if(enclosing_method_name == NULL)
                enclosing_method_name = "Unknown Function";
        if(enclosing_method_signature == NULL)
                enclosing_method_signature = "Unknown Signature";

        fprintf(fp, "PrintStringTable of method %s with signature %s\n", enclosing_method_name, enclosing_method_signature);

        fprintf(fp, "Printing String Table with size %d\n", (int)string_table.size());

        for(int i = 0; i < (int)string_table.size(); i++)
        {
                char* str = (char *)string_table.at(i);
                if(str == NULL)
                        fprintf(fp, "NULL ");
                else
                        fprintf(fp, "%s ", str);

        }
        fprintf(fp, "\n");
        fprintf(fp, "End of String table of %s %s\n",enclosing_method_name, enclosing_method_signature);
}

void printStackFrames(PCStackInfo* record, jvmtiEnv *jvmti_env, FILE* fp) {
  if(record != NULL && record->methods != NULL) {
    int i;
    for(i = 0; i < record->numstackframes; i++) {
      char* method_name = NULL;
      char* class_name = NULL;
      char* method_signature = NULL;
      char* class_signature = NULL;
      char* generic_ptr_method = NULL;
      char* generic_ptr_class = NULL;
      jmethodID id;
      jclass declaringclassptr;
      id = record->methods[i];
     
      jvmti_env->GetMethodDeclaringClass(id, &declaringclassptr);
      jvmti_env->GetClassSignature(declaringclassptr, &class_signature, &generic_ptr_class);
      if(jvmti_env->GetMethodName(id, &method_name, &method_signature, &generic_ptr_method) == JVMTI_ERROR_NONE) {
        fprintf(fp, "%s::%s %s %s @%d\n", class_signature, method_name, method_signature, generic_ptr_method, record->bcis[i]);
      }
      if(method_name != NULL) {
       
        jvmti_env->Deallocate((unsigned char*)method_name);
      
      }
      if(method_signature != NULL) {
      
        jvmti_env->Deallocate((unsigned char*)method_signature);
        
      }
      if(generic_ptr_method != NULL) {
        
        jvmti_env->Deallocate((unsigned char*)generic_ptr_method);
       
      }
      if(class_name != NULL) {
       
        jvmti_env->Deallocate((unsigned char*)class_name);
    
      }
      if(class_signature != NULL) {
       
        jvmti_env->Deallocate((unsigned char*)class_signature);
      
      }
      if(generic_ptr_class != NULL) {
	  
        jvmti_env->Deallocate((unsigned char*)generic_ptr_class);
       
      }
    }
  } 
}


void printInlineInfoRecord(jvmtiEnv *jvmti_env, FILE* fp,  jvmtiCompiledMethodLoadInlineRecord* record)
{
	fprintf(fp, "Printing inline information record\n");
	if(record != NULL && record->pcinfo != NULL)
	{
		int numpcs = record->numpcs;
		for(int i = 0; i < numpcs; i++)
		{
			PCStackInfo pcrecord = (PCStackInfo)(record->pcinfo[i]);
			fprintf(fp, "PcDescriptor(pc=0x%lx):\n", (pcrecord.pc));
			//fprintf(fp, "Numstackframes = %d\n", (jint)(pcrecord.numstackframes));
			printStackFrames(&pcrecord, jvmti_env, fp);
		}

	
	}
	fprintf(fp, "End of inline information record\n");
}

#endif
