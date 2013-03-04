#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <popt.h>


#include "calog.h"

using namespace std;

//static int fd;
//static struct stat sb;
//static calog_header * header;
static calog_t calog;

/* Options */
static int tgid = -1;
static int tid  = -1;
static unsigned long long mod = 0;
static unsigned long long app = 0;
static int listMod = 0;
static int printHdr = -1;
static char * input = NULL;

enum {
	OPT_FILENAME = 1,
	OPT_MOD,
	OPT_APP,
	OPT_TGID,
	OPT_TID,
	OPT_LIST,
	OPT_HDR,
	OPT_VERSION,
	OPT_HELP
};

static struct poptOption options[] = {
{ "file"       , 'f', POPT_ARG_STRING, &input, OPT_FILENAME, "", NULL },
{ "mod"        , 'm', POPT_ARG_STRING  , NULL , OPT_MOD     , "", NULL },
{ "app"        , 'a', POPT_ARG_STRING  , NULL , OPT_APP     , "", NULL },
{ "tgid"       , 'g', POPT_ARG_INT   , &tgid, OPT_TGID    , "", NULL },
{ "tid"        , 't', POPT_ARG_INT   , &tid , OPT_TID     , "", NULL },
{ "list"       , 'l', POPT_ARG_NONE  , &listMod, OPT_LIST , "", NULL },
{ "header"     , 'h', POPT_ARG_NONE  , &printHdr, OPT_HDR  , "", NULL },
{ "version"    , 'v', POPT_ARG_NONE  , NULL , OPT_VERSION , "", NULL },
{ "help"       , 0  , POPT_ARG_NONE  , NULL , OPT_HELP    , "", NULL },
{ NULL         , 0  , 0              , NULL , 0           , NULL , NULL },
};

void show_version() 
{
        fprintf(stdout,"calog_retport version 1.0\n");
}


void show_options() 
{
	printf( " Options:\n"
		"    -f|--file=<filename>   Input file.\n"
		"    -m|--mod=<cookie>      Filter by module name.\n"
		"    -a|--app=<cookie>      Filter by application name.\n"
		"    -t|--tid=<tid>         Filter by tid.\n"
		"    -g|--tgid=<tgid>       Filter by tgid.\n"
		"    -l|--list              List module name.\n"
		"    -h|--header            Print file header.\n"
		"    -v|--version           Show version number.\n"
		"    --help                 Show this message.\n"
		"\n"
		" Description :\n"
		"     calog report utility.\n"
		"\n");
}


int do_options(int argc, char const * argv[])
{	
	const char * tmp;
	poptContext optcon;
	int c = 0, ret = 0;

	optcon = poptGetContext(NULL, argc, argv, options, 0);

	if (argc < 2) {
		poptPrintUsage(optcon, stdout, 0);
		ret = -1;
		goto out;
	}

	while ( (c = poptGetNextOpt(optcon)) != -1  ) {
		switch (c) {
		case OPT_FILENAME:
		case OPT_TID:
		case OPT_TGID:
		case OPT_LIST:
		case OPT_HDR:
			break;	
		case OPT_MOD:
			tmp = poptGetOptArg(optcon);
			mod = strtoull(tmp,NULL,0);
			break;
		case OPT_APP:
			tmp = poptGetOptArg(optcon);
			app = strtoull(tmp,NULL,0);
			break;
			break;
		case OPT_HELP:
			show_options();
			exit(0);
			break;	
		case OPT_VERSION:
			show_version();
			exit(0);
			break;	
		default:
			fprintf(stderr,"Error: Invalid arguments.\n");	
			show_options();
			exit(1);
			break;
		}
	}

	poptFreeContext(optcon);
out:
	return ret;
}


int print_header()
{
	calog_header * header = calog.header; 
	fprintf(stdout, "calog Header\n");
	fprintf(stdout, "============\n");
	
	fprintf(stdout, "    signature:         %s\n", header->signature);
	fprintf(stdout, "    ver_major:         %u\n", header->ver_major);
	fprintf(stdout, "    ver_minor:         %u\n", header->ver_minor);
	fprintf(stdout, "    calogdata_begin:   0x%x\n", header->calogdata_begin);
	fprintf(stdout, "    calogdata_end:     0x%x\n", header->calogdata_end);
	fprintf(stdout, "    moddata_begin:     0x%x\n", header->moddata_begin);
	fprintf(stdout, "    moddata_end:       0x%x\n", header->moddata_end);
	fprintf(stdout, "    data_type:         %u\n", header->data_type);
	fprintf(stdout, "    data_size:         %u\n", header->data_size);
	return 0;
}


bool applyFilter(calog_data * entry)
{
	if (tgid >= 0 && tgid != entry->tgid)
		return false;
	if (tid >= 0 && tid != entry->tid)
		return false;
	if (mod && mod != entry->cookie)
		return false;
	if (app && app != entry->app_cookie)
		return false;
	return true;
}


int dump_bta()
{
	char * ptr = NULL;

	fprintf(stdout, "\nBranch Target Address Dump\n");
	fprintf(stdout, "===========================\n");
	fprintf(stdout, "\nmod_cookie | app_cookie | cpu | tgid | tid | offset | bta\n");
	fprintf(stdout, "----------------------------------------------------------\n");
	while ((ptr = (char*)calog_get_next_entry(&calog, (void**) &ptr)) != NULL) {
		calog_data  * entry = (calog_data *) (ptr);
		bta_data    * data  = (bta_data *) (ptr + sizeof(calog_data));
		
		if (!applyFilter(entry)) 
			continue;

		fprintf(stdout, "0x%016llx 0x%016llx %02u %08u %08u 0x%08x 0x%016llx\n",
			entry->cookie, entry->app_cookie, entry->cpu, entry->tgid, 
			entry->tid, entry->offset,
			data->bta);

	} 
	return 0;
}


int dump_dcmiss()
{
	char * ptr = NULL;

	fprintf(stdout, "\nDC Miss Dump\n");
	fprintf(stdout, "=============\n");
	fprintf(stdout, "\nmod_cookie | app_cookie | cpu | tgid | tid | offset | phy | lin | ld/st | lat\n");
	fprintf(stdout, "------------------------------------------------------------------------------\n");
	while ((ptr = (char*)calog_get_next_entry(&calog, (void**) &ptr)) != NULL) {
		calog_data  * entry = (calog_data *) (ptr);
		dcmiss_data * data  = (dcmiss_data *) (ptr + sizeof(calog_data));

		if (!applyFilter(entry))
			continue; 

		fprintf(stdout, "0x%016llx 0x%016llx %02u %08u %08u 0x%08x 0x%016llx 0x%016llx %s %08u\n",
			entry->cookie, entry->app_cookie, entry->cpu, entry->tgid, 
			entry->tid, entry->offset,
			data->phy, data->lin, (data->ld)? "LD": "ST", data->lat);
	}
	return 0;
}


int dump_cacss()
{
	char * ptr = NULL;

	fprintf(stdout, "\nCA-CSS Dump\n");
	fprintf(stdout, "===========================\n");
	fprintf(stdout, "\nmod_cookie | tgid | offset | self\n");
	fprintf(stdout, "----------------------------------\n");
	while ((ptr = (char*)calog_get_next_entry(&calog, (void**) &ptr)) != NULL) {
		calog_data  * entry = (calog_data *) (ptr);
		cacss_data  * data  = (cacss_data *) (ptr + sizeof(calog_data));
		
		if (!applyFilter(entry))
			continue;

		fprintf(stdout, "0x%016llx %08u 0x%08x 0x%08x\n",
			entry->cookie, entry->tgid, entry->offset,
			data->cg_type);
	} 
	return 0;
}


int list_module()
{
	char * ptr = NULL;

	fprintf(stdout, "\nModule Data Dump\n");
	fprintf(stdout, "=================\n");
	while ((ptr = (char*)calog_get_next_module(&calog, (void**) &ptr)) != NULL) {
		char modName[1024];
		unsigned long long * cookie = (unsigned long long *) ptr;
		unsigned int * length       = (unsigned int *)(ptr + sizeof(unsigned long long));
		char * namePtr              = ptr + sizeof(unsigned long long) + sizeof(unsigned int);

		strncpy(modName, namePtr, *length);
		modName[*length] = '\0';
	
		fprintf(stdout, "0x%016llx : %s\n", *cookie, modName);
	}
	return 0;
}


int dump_calog()
{
	switch (calog.header->data_type) {
	case CALOG_BTA:
		dump_bta();
		break;
	case CALOG_DCMISS:
		dump_dcmiss();
		break;
	case CALOG_CACSS:
		dump_cacss();
		break;
	default:
		break;
	}
	return 0;	
}


int main (int argc, char const * argv[])
{
	int ret = 0;
	
	// Parse options
	if (0 != (ret = do_options(argc, argv))) {
		fprintf(stderr,"calog_report: Error, parsing option.\n");
		goto out;
	}

	if (!input)
		fprintf(stderr,"calog_report: Error, Please specify input file\n");

	if (calog_process_init(&calog, input) == -1)
		exit (-1);

	if (printHdr > 0)
		print_header();
	
	if (listMod > 0) {
		list_module();
	} 

	dump_calog();

	calog_process_deinit(&calog);	
out:
	exit (ret);	
}
