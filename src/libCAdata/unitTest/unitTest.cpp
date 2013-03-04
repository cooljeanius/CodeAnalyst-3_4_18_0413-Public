// unitTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>

#include "libCAdata.h"
#include "CaProfileReader.h"
#include "CaProfileWriter.h"


std::wstring stringToWstring(const string str)
{
	wstring ws;
	ws.assign(str.begin(), str.end());
	return ws;
}

int main(int argc, char* argv[])
{
	if (argc < 3) {
		fprintf(stderr,"Error: Please specify input and output file.\n");
		exit(1);
	}
	
	wstring profileFile = stringToWstring(string(argv[1]));	
	wstring tbpFileOut = stringToWstring(string(argv[1]));	

	///////////////////////////////////////////////
	/* Read TBP */
	CaProfileReader reader;
	fprintf(stderr,".... open TBP/EBP file to read (%s) ... ", profileFile.c_str());
	if (!reader.open(profileFile)) {
		fprintf(stderr,"FAILED\n");
		exit(1);
	} else {
		fprintf(stderr,"OK\n");
	}

	/* Get ProfileInfo */
	CaProfileInfo * env = reader.getProfileInfo();

	/* Get ProcessMap */
	fprintf(stderr,".... getProcessMap ... ");
	PidProcessMap * procMap = reader.getProcessMap();		
	if (!procMap) {
		fprintf(stderr,"FAILED\n");
		exit(1);
	} else {
		fprintf(stderr,"OK\n");
	}

	/* Get Module map */
	fprintf(stderr,".... getModuleMap ... ");
	NameModuleMap * modMap  = reader.getModuleMap();
	if (!modMap) {
		fprintf(stderr,"FAILED\n");
		exit(1);
	} else {
		fprintf(stderr,"OK\n");
	}

	/* Get Module Detail */
	fprintf(stderr,".... getModuleDetail ... ");
	NameModuleMap::iterator m_it = modMap->begin();
	NameModuleMap::iterator m_end = modMap->end();
	for (; m_it != m_end; m_it++) {
		fprintf(stderr,"\n Module (id:%u) = %s\n", m_it->second.getImdIndex(), 
										m_it->first.c_str());
		if(!reader.getModuleDetail(m_it->first)){
			fprintf(stderr,"FAILED\n");
			exit(1);
		} 
	}
	fprintf(stderr,"OK\n");


	/////////////////////////////////////////////////////////////
	/* Write TBP */
	wstring profileOut = stringToWstring(string(argv[2]));	
	CaProfileWriter writer;

	if (!writer.write( profileOut, env, procMap, modMap)) {
		fprintf(stderr," CaProfileWriter::write(): Failed\n");
		exit (1);
	}
	fprintf(stderr," CaProfileWriter::write(): Success\n");
					
#if 0
	fprintf(stderr,".... open TBP/EBP file to write (%s) ... ", profileOut.c_str());

	if (!writer.open(profileOut)) {
		fprintf(stderr,"FAILED\n");
		exit(1);
	} else {
		fprintf(stderr,"OK\n");
	}

	fprintf(stderr,".... writeProfileInfo ... ");
	if (!writer.writeProfileInfo(env)) {
		fprintf(stderr,"FAILED\n");
		exit(1);
	} else {
		fprintf(stderr,"OK\n");
	}

	fprintf(stderr,".... writeProcSection ... ");
	if (!writer.writeProcSection(procMap)) {
		fprintf(stderr,"FAILED\n");
		exit(1);
	} else {
		fprintf(stderr,"OK\n");
	}

	fprintf(stderr,".... writeModSectionAndImd ... ");
	if (!writer.writeModSectionAndImd(modMap)) {
		fprintf(stderr,"FAILED\n");
		exit(1);
	} else {
		fprintf(stderr,"OK\n");
	}
#endif

	return 0;
}

