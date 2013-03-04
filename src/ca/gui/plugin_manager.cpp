#include "stdafx.h"

#include "plugin_manager.h"
#include "atuneoptions.h"


#define DEFAULT_OP_PLUGIN OP_091_PLUGIN

using namespace std;

PlugInManager::PlugInManager()
{
	create_opdata = NULL;
	delete_opdata = NULL;
	opdata_handle = NULL;
	ca_output_handle = NULL;
}


PlugInManager::~PlugInManager()
{
	//unload_opdata_plugin((opdata_handler*)opdata_handle);
	//unload_opoutput_plugin((output*)ca_output_handle);
}

void PlugInManager::unload_opdata_plugin(opdata_handler * opdata)
{
	if (NULL != opdata_handle) {
		if ((NULL != opdata) && (NULL != delete_opdata))
			delete_opdata(opdata);
		dlclose(opdata_handle);
	}
	create_opdata = NULL;
	delete_opdata = NULL;
	opdata_handle = NULL;
	opdata = NULL;
}

bool PlugInManager::load_opdata_plugin()
{ 
	CATuneOptions ao;
	QString so_name = "";

	ao.getOPPlugIn(so_name);

	if (so_name.isEmpty())
		so_name = DEFAULT_OP_PLUGIN;

	opdata_handle = dlopen(so_name.data(), RTLD_NOW);
	if (NULL == opdata_handle) {
		show_error(so_name.data());
		return false;
	}

	create_opdata =
		(create_opdata_t *)dlsym(opdata_handle, "create_opdata");
	delete_opdata = 
		(delete_opdata_t *)dlsym(opdata_handle, "delete_opdata");     
	if (NULL == delete_opdata || NULL == create_opdata) {
		show_error(so_name.data());
		return false;
	}

#ifdef _DEBUG_
	qDebug("create_opdata is %x, delete_opdata is %x", create_opdata,
		delete_opdata);
#endif

	return true;
}

void PlugInManager::unload_opoutput_plugin(output * output_handle)
{
	if (NULL != ca_output_handle) {
		if (NULL != output_handle)
			;//delete_ca_output(output_handle);
		dlclose(ca_output_handle);
	}

	ca_output_handle = NULL;
	create_opdata = NULL;
	delete_opdata = NULL;
	output_handle = NULL;
}



bool PlugInManager::load_output_plugin()
{
	string so_name = "lib_tbp_output.so";
	QString msg;


	ca_output_handle = dlopen(so_name.c_str(),  RTLD_NOW);
	if (NULL == ca_output_handle) {
		show_error(so_name.c_str());
		return false;
	}
/*
	create_ca_output =
		(create_ca_output_t *)dlsym(ca_output_handle, "create_ca_output");
	delete_ca_output = 
		(delete_ca_output_t *)dlsym(ca_output_handle, "delete_ca_output");     

	if (NULL == delete_ca_output || NULL == create_ca_output) {
		show_error(so_name.c_str());
		return false;
	}
*/

#ifdef _DEBUG_
	qDebug("create_opdata is %x, delete_opdata is %x", create_ca_output,
		delete_ca_output);
#endif

	return true;
}

void PlugInManager::show_error(const char * so_name)
{
	QString msg;
	msg = QString(so_name) + ": " + dlerror();
	QMessageBox::critical (qApp->activeWindow(), "CodeAnalyst error", msg);
}

