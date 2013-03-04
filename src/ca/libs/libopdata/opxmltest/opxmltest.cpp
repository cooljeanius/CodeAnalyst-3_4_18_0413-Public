#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>

#include "opxmldata_handler.h"


double diffTime(struct timeval & start, struct timeval & stop)
{
	double msec_st, msec_sp;
	msec_st =  (start.tv_usec / 1000);
	msec_st += (start.tv_sec * 1000);

	msec_sp =  (stop.tv_usec / 1000);
	msec_sp += (stop.tv_sec * 1000);

	return (msec_sp - msec_st);
}

int main(int argc , char * argv [])
{
	struct timeval start, stop;
	opxmldata_handler xmlHandler;


	gettimeofday(&start,NULL);
	xmlHandler.init();
	xmlHandler.readXML(argv[1]);
	gettimeofday(&stop,NULL);

	fprintf(stdout,"Parsing time = %f msec\n",diffTime(start,stop));

	gettimeofday(&start,NULL);

	xmlHandler.processXMLData();
	gettimeofday(&stop,NULL);
	
	fprintf(stdout,"Attribute time = %f msec\n",diffTime(start,stop));
	exit(0);
}
