/**
 * @file calog_types.h
 *
 */

#ifndef CALOG_TYPES_H
#define CALOG_TYPES_H

#include <sys/types.h> 

enum calog_type {
	CALOG_BTA = 1,
	CALOG_DCMISS,
	CALOG_CACSS,
};

typedef struct {
	char            signature[8];
	unsigned int    ver_major;
	unsigned int    ver_minor;
	unsigned int    calogdata_begin;
	unsigned int    calogdata_end;
	unsigned int    moddata_begin;
	unsigned int    moddata_end;
	unsigned int	data_type;
	unsigned int	data_size;
} calog_header;
 
typedef struct calog{
	calog_header 	* header;
	void		* modmap;
	void 		* file;
	int		fd;
	unsigned int	size;
} calog_t;


typedef struct {
	unsigned long long	cookie;
	unsigned long long	app_cookie;
	unsigned int		cpu;
	unsigned int		tgid;
	unsigned int		tid;
	unsigned int		offset;
	unsigned int		cnt;
} calog_data; //36


typedef struct {
	unsigned long long	bta;
} bta_data;

typedef struct {
	unsigned long long	phy;
	unsigned long long	lin;
	unsigned int		ld;
	unsigned int		lat;
} dcmiss_data; // 24

typedef struct {
	unsigned int		cg_type; //1: self, 0 cg inclusive;
} cacss_data; // 4 

#endif /* CALOG_TYPES_H*/
