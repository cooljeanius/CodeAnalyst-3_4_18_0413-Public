//$Id: taskinfo.cpp,v 1.3 2006/08/28 21:00:32 ssuthiku Exp $

/*
// CodeAnalyst for Open Source
// Copyright 2002 . 2005 Advanced Micro Devices, Inc.
// You may redistribute this program and/or modify this program under the terms
// of the GNU General Public License as published by the Free Software 
// Foundation; either version 2 of the License, or (at your option) any later 
// version.
//
// This program is distributed WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED 
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  See the 
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA.
*/


#include <qdir.h>
#include <qstring.h>

#include "taskinfo.h"

const int _MAX_PATH = 256;

#ifndef __x86_64__
const unsigned long KERNEL_START_RANGE = 0xc0100000;
#else
const unsigned long KERNEL_START_RANGE = 0xffffffff80100000;
//Kernel usually starts at ffffffff80100000
#endif



CTaskInfo::CTaskInfo()
{
    m_TIReader = NULL;
    m_jnc_counter = 0;
}

CTaskInfo::~CTaskInfo()
{
    if (NULL != m_TIReader) {
        delete m_TIReader;
        m_TIReader = NULL;
    }

    JNCLIST_MAP::iterator it =  m_tiJncMap.begin();
    JNCLIST_MAP::iterator itend =  m_tiJncMap.begin();
#ifdef _DEBUG_
    qDebug("in CTaskInfo destructor");
#endif
    for (;it != itend; it++) {
        if (it != NULL) {
            (*it)->clear();
            delete *it;
        }
    }
    m_tiJncMap.clear();
}

bool CTaskInfo::ReadTaskInfoFile(string filename)
{
    bool ret = false;

    // todo Remove this
    m_TIFileName = filename; 

    m_TIReader = new  cTaskInfoReader(m_TIFileName.c_str());
    m_TIReader->openTIFile();

    if (NULL != m_TIReader) {
        if ((ret = m_TIReader->readHeader(m_TIHeader))) {
           ret = m_TIReader->readTIFile(m_TIVector); 
        } else {
#ifdef _DEBUG_
            qDebug("failed to read TI headr\n");
#endif
        }
    }

    return ret;
}


void CTaskInfo::OnProcessStart(task_info_record & record)
{
//    ModuleKey key(record.pid, record.start_addr + record.offset, record.time_stamp);
//    ModuleValue value;
//
//    value.moduleBaseAddr = record.start_addr; 
//    value.moduleSize = record.module_len;
//    value.moduleUnloadTime = 0;
//    value.moduleName = record.module_name;
//    value.bNameConverted = false;
//    value.bJitModule = false;
//
//    m_tiModMap.insert(ModuleMap::value_type(key, value));
}


void CTaskInfo::OnModuleMap(task_info_record & record)
{
//    ModuleKey key(record.pid, record.start_addr + record.offset, record.time_stamp);
//    ModuleValue value;
//
//    value.moduleBaseAddr = record.start_addr; 
//    value.moduleSize = record.module_len;
//    value.moduleUnloadTime = 0;
//    value.moduleName = record.module_name;
//    value.bNameConverted = false;
//    value.bJitModule = false;
//
//    m_tiModMap.insert(ModuleMap::value_type(key, value));
}



void CTaskInfo::OnProcessEnd(task_info_record & record)
{
//    ModuleKey key(record.pid, record.start_addr, record.time_stamp);
//    ModuleMap::iterator it = m_tiModMap.find(key);
//    ModuleMap::const_iterator itEnd = m_tiModMap.end();
//
//
//    if (it != itEnd) {
//       for (; (it != itEnd) && (record.pid == (*it).first.processId); it++)
//       {
//           (*it).second.moduleUnloadTime = record.time_stamp;
//       }
//    }
}


void CTaskInfo::AddkernelModule(bfd_vma start_addr, bfd_vma size, QString & name)
{
    // Kernel PID = 0, loadTime = 0
    ModuleKey key(0, start_addr, 0); 
    ModuleValue value;

    value.moduleBaseAddr = start_addr; 
    value.moduleSize = size;
    value.moduleUnloadTime = m_TIHeader.stop_time;
    value.moduleName = string(name.data());
    value.bNameConverted = false;
    value.bJitModule = false;



    m_tiModMap.insert(ModuleMap::value_type(key, value));
}


bool CTaskInfo::ReadJitInformation(const char * directory)
{
    bool ret = true;

    //for each process id directory in the given directory
    QDir search_dir (directory);
    const QFileInfoList *di_list = search_dir.entryInfoList();
    QFileInfoListIterator d_it( *di_list );
    QFileInfo *di;
    search_dir.setFilter( QDir::Dirs );

    //while there are directories left,
    for (; ((di = d_it.current()) != 0 ); ++d_it) {
#ifdef _DEBUG_
		qDebug("ReadJitInformation: processing dir %s", di->dirPath().data());
#endif
        unsigned long pid = di->fileName().toULong();
 
        if (0 == pid) {
 	        continue;
        }

    	JNC_LIST * jnc_list = new JNC_LIST();
        if (NULL == jnc_list) {
            ret = false;
            break;
        }
        
		JIT_Load_ELEMENT jit_block;
		char java_app[_MAX_PATH];
        ModuleKey t_modKey(pid, 0, 0);
        ModuleValue t_modValue;

        QString jcl_file = directory + QString ("/") + di->fileName();
		jcl_file += "/" + di->fileName();
		jcl_file += ".jcl";

    	//read the jcl file
#ifdef _DEBUG_
		qDebug("Before reading jcl file");
#endif 
		JCLReader jcl_reader (jcl_file.data());
		JIT_RECORDS jit_record_type;
#ifdef _DEBUG_
		qDebug("After reading jcl file");
#endif 

		//for each function in the jcl, add jnc data to the list
		jnc_list->setAutoDelete (true);

		if (!jcl_reader.readHeader(java_app)) {
#ifdef _DEBUG_
            qDebug("readHeader %s failed", jcl_file.data());
#endif
            continue;
        }

        t_modValue.moduleName = java_app;
#ifdef _DEBUG_
        qDebug("readind java_app %s", java_app);
#endif
        t_modValue.bJitModule = true;
        m_tiModMap.insert (ModuleMap::value_type(t_modKey, t_modValue));

		while (jcl_reader.readNextRecordType (&jit_record_type)) {

#ifdef _DEBUG_
					qDebug("reading next jcl record");
#endif
				if (JIT_LOAD == jit_record_type) {
					if (!jcl_reader.readLoadRecord (&jit_block)) {
						break;
					}

			        JNC_VALUE * temp_jnc = new JNC_VALUE();
			        temp_jnc->blockEndAddr = jit_block.blockEndAddr;
			        temp_jnc->blockStartAddr = jit_block.blockStartAddr;
			        temp_jnc->loadTimestamp = jit_block.loadTimestamp;
			        temp_jnc->threadID = jit_block.threadID;
			        temp_jnc->jncFile = jit_block.jncFileName.c_str();
					temp_jnc->functionName = jit_block.classFunctionName.c_str();
		            temp_jnc->javaSrcFile = jit_block.javaSrcFileName.c_str();
		            jnc_list->append(temp_jnc);
				} else {
					JIT_Unload_ELEMENT jit_unload_block;
					if (!jcl_reader.readUnLoadRecord (&jit_unload_block))
						break;

					//find old record module, and alter unload time.
					JNC_LIST::iterator it = jnc_list->begin();
					for (;it != jnc_list->end(); ++it) {
#ifdef _DEBUG_
						qDebug("looping the jnc_list");
#endif

						JNC_VALUE * itv = *it;
						if (jit_unload_block.blockStartAddr != itv->blockStartAddr)	
							continue;
						if (itv->loadTimestamp >= jit_unload_block.unloadTimestamp)
							continue;
							
						itv->unloadTimestamp = jit_unload_block.unloadTimestamp;	
					}
				}	
		}
     	//add to map
        m_tiJncMap[pid] = jnc_list;
   } 

    PrintJITMap();

    return ret;
}


void CTaskInfo::PrintJITMap()
{
#ifdef _DEBUG_
    JNCLIST_MAP::iterator it = m_tiJncMap.begin();
    JNCLIST_MAP::iterator it_end = m_tiJncMap.end();

    for (;it != it_end; it++)
    {
        JNC_LIST * list = it.data();

        JNC_VALUE * value = list->first();
        if (value == NULL)
            qDebug("value is NULL");
        for (value = list->first(); value; value = list->next())
        {
            qDebug("jit function %s", value->functionName.data());         
        }
    }
#endif    
}


void CTaskInfo::ProcessTaskInfoVector()
{
    TIVector::iterator it = m_TIVector.begin();

    for(; it != m_TIVector.end(); it++) {
        task_info_record & record =  *it;
        switch(record.record_type) {
            case PROCESS_START:
                OnProcessStart(record);
                break;
            case PROCESS_EXIT:
                OnProcessEnd(record);
                break;
            case MODULE_MAP:
                OnModuleMap(record);
                break;
            case MODULE_UNMAP:
                /*
                 * According to Reeja, the driver does not 
                 * receive this event.  
                 */
            default:
                break;
        }
    }

    AdjustModuleUnloadTime();
}


// If a module is still loaded by the time the profile session
// ends, it would unload time being 0.  Need to adjust for the
// module lookup to work.
void CTaskInfo::AdjustModuleUnloadTime()
{
    ModuleMap::iterator it = m_tiModMap.begin();
    ModuleMap::const_iterator itEnd = m_tiModMap.end();

    if (it != itEnd) {
       for (; (it != itEnd); it++)
       {
            if (0 == (*it).second.moduleUnloadTime)
                (*it).second.moduleUnloadTime = m_TIHeader.stop_time;
       }
    }
}


bool CTaskInfo::GetModuleInfo(TI_MODULE_INFO * mod_info, 
    unsigned int novmlinux)
{
#ifdef _DEBUG_
	if (mod_info->sampleAddr < KERNEL_START_RANGE) {
    	qDebug("--------");
		qDebug("prd has processID %d", mod_info->processID);
    	qDebug("prd has addr %llx", mod_info->sampleAddr);
    	qDebug("prd has time stamp %llx", mod_info->timestamp);
	}


    ModuleMap::iterator j; 
    for (j = m_tiModMap.begin(); j != m_tiModMap.end(); ++j) {
        ModuleMap::value_type &item = *j;
		qDebug("module is %s pid is %d", item.second.moduleName.c_str(),
		 item.first.processId);
	}

#endif
    bool ret = false;

    // check map status
    if (0 == m_tiModMap.size()) {
#ifdef _DEBUG_
        qDebug("mod map empty");
#endif
        return ret;
    }

    // Kernel range sample
    // If user does not specify the path to vmlinux
    // Set the modulename to no-vmlinux by default.
    if (mod_info->sampleAddr > KERNEL_START_RANGE )
    {
        mod_info->processID = 0;
        if (1 == novmlinux) {
            strcpy(mod_info->pModulename, "Anonymous Samples");
            mod_info->ModuleStartAddr = KERNEL_START_RANGE;
            
            return true;
        }
    }

    ModuleMap::iterator i; 
    for (i = m_tiModMap.lower_bound(ModuleKey(mod_info->processID, 0, 0)); i != m_tiModMap.end(); ++i)
    {
        ModuleMap::value_type &item = *i;

        /* 
         * Note: The map structure is identical to what we do on Windows.
         * The only differences are:
         * 1. We use TSC instead of system time  on Linux.  No time conversion is needed here. 
         * 2. We don't distinguish between kernel/user modules in Linux.
         * 3. There is no need to convert the name.
         */

        if (!item.second.bJitModule) {
            // different process
            if (item.first.processId != mod_info->processID)
		        break;

#ifdef _DEBUG_
            qDebug("module pid %d", item.first.processId);
            qDebug("module load addr %llx", item.first.moduleLoadAddr);
            qDebug("module unloadload addr %llx", item.first.moduleLoadAddr + item.second.moduleSize);
            qDebug("module addr is %llx", mod_info->sampleAddr);
            qDebug("module load time %llx", item.first.moduleLoadTime);
            qDebug("module unload time %llx", item.second.moduleUnloadTime);
            qDebug("module time is %llx", mod_info->timestamp);
#endif

            // since the module map is sorted by the process id, module address and time.
    		// if module load address is greater than sample address, we don't need 
    		// go farther.
			if (item.first.moduleLoadAddr > mod_info->sampleAddr) {
    			break;
			}

            // if module upper boundary is less than the sample address, try next module.
	    	if (item.first.moduleLoadAddr + item.second.moduleSize <= mod_info->sampleAddr) {
	        	continue;
			}

			// if load time is not in module load frame, try next module
	    	if (item.first.moduleLoadTime >= mod_info->timestamp) {
				continue;
        	}

            // if load time is not in module load frame, try next module
	    	if (item.second.moduleUnloadTime < mod_info->timestamp) {
				continue;
        	}
	    	mod_info->ModuleStartAddr = item.first.moduleLoadAddr;
       } else {
#ifdef _DEBUG_
            qDebug("searching for JIT module");
            qDebug("module pid %d", item.first.processId);
            qDebug("module load addr %llx", item.first.moduleLoadAddr);
            qDebug("module load time %llx", item.first.moduleLoadTime);
            qDebug("module unload time %llx", item.second.moduleUnloadTime);
#endif

            //check functions for pid.
	    JNC_VALUE * jncIt = NULL;

            //check jit area map 
	    JNCLIST_MAP::Iterator jnc_list = m_tiJncMap.find (mod_info->processID);

		if (m_tiJncMap.end() == jnc_list)
        	break;

		for (jncIt = jnc_list.data()->first();  jncIt;  jncIt = jnc_list.data()->next()) {
			if (jncIt->blockStartAddr > mod_info->sampleAddr) {
#ifdef _DEBUG_
				qDebug("blockStartAddr %llx > sample Addr %llx",
					jncIt->blockStartAddr, mod_info->sampleAddr);
#endif
				continue;
			}
			if (jncIt->blockEndAddr < mod_info->sampleAddr) {
#ifdef _DEBUG_
				qDebug("blockStartAddr %llx < sample Addr %llx",
					jncIt->blockStartAddr, mod_info->sampleAddr);
#endif
		    	continue;
			}
			 
			if ((jncIt->loadTimestamp > mod_info->timestamp) ||
				(jncIt->unloadTimestamp < mod_info->timestamp)) {
#ifdef _DEBUG_
				qDebug("incorrect loadtime/unloadtime");
				if (jncIt->loadTimestamp > mod_info->timestamp)
					qDebug("smaple has timestamp earlier than JIT load time");
				if (jncIt->unloadTimestamp < mod_info->timestamp)
					qDebug("sample has timestampe later than JIT unload time");
					

				qDebug("jncIt has load time %llx, mod_info has time %llx",
					jncIt->loadTimestamp, mod_info->timestamp);
				qDebug("jncIt has unload time %llx, mod_info has time %llx",
					jncIt->unloadTimestamp, mod_info->timestamp);

#endif
				continue;
			}
			
			

			//translate jnc name here...
			if ( jncIt->translatedJncFile.isEmpty())
		    	jncIt->translatedJncFile.sprintf ("jnc_%d.jnc",m_jnc_counter++);

			break;
		}

            //if it wasn't in the recorded jit areas for the module, keep going
	    if (! jncIt)
	        continue;

            mod_info->funSize =  jncIt->functionName.length();
	        strncpy (mod_info->pFunctionName,  jncIt->functionName.data(), mod_info->funSize);
	        mod_info->FunStartAddr =  jncIt->blockStartAddr;
	        mod_info->ModuleStartAddr = jncIt->blockStartAddr;
	        mod_info->jncSize =  jncIt->translatedJncFile.length();
	        strncpy (mod_info->pJncName,  jncIt->translatedJncFile.data(), mod_info->jncSize);
            strncpy(mod_info->pJavaSrcName, jncIt->javaSrcFile.data(),
                jncIt->javaSrcFile.length());
        }
        

        // Here, we found the module.
	mod_info->JitModule = item.second.bJitModule;
	mod_info->Modulesize = item.second.moduleSize;

        // todo: verify that there is no need to convert the map
        // if the module name is not converted yet, convert it and update it in the map.
        /*
		if (!item.second.bJitModule) {
		    if (!item.second.bNameConverted) {
		        item.second.moduleName = ConvertName(item.second.moduleName.c_str());
				item.second.bNameConverted = TRUE;
			}
	    } else {
		    item.second.bNameConverted = TRUE;
	    }
        */

		item.second.bNameConverted = TRUE;

        if (item.second.moduleName.size() < mod_info->namesize)
		{
#ifdef _DEBUG_
				qDebug("Found in JNC");
#endif

		    strcpy(mod_info->pModulename, item.second.moduleName.c_str());
			ret = true;
	    }
		else 
		{
#ifdef _DEBUG_
				qDebug("Found in JNC");
#endif

		    ret = true;
		}

        break;
    }

#ifdef _DEBUG_
    qDebug("GetModuleInfo  PID %d returns %d\n", mod_info->processID, ret);
#endif
    return ret;
}

void CTaskInfo::PrintTIVector()
{
#ifdef _DEBUG_
    TIVector::iterator it = m_TIVector.begin();

    printf("size of TIVector is %d\n", m_TIVector.size());

    for(; it != m_TIVector.end(); it++) {

        task_info_record & record =  *it;
        printf("/***********************/\n");
        printf("pid: %d\n", record.pid);
        printf("tgid: %d\n", record.tgid);
        printf("start_addr: %llx\n", record.start_addr);
        printf("module len: %d\n", record.module_len);
        printf("offset: %d\n", record.offset);
        //printf("time_stamp: %d\n", record.time_stamp);
        printf("module name len: %d\n", record.module_name_len);
        cout << "modulenaem: " << record.module_name << endl;
        //printf("modulename: %s\n", record.module_name);
        printf("record type: %d\n\n", record.record_type);
    }
#endif
}


bool CTaskInfo::copyJNCFile(QString & sessionDir, QString & jncPath)
{
    bool ret = true;

    JNCLIST_MAP::Iterator map_it = m_tiJncMap.begin();
    JNCLIST_MAP::Iterator map_it_end = m_tiJncMap.end();

    for (; map_it != map_it_end; map_it++)
    {
        JNC_VALUE * temp;
        JNC_LIST * jnc_list = map_it.data();
        //check jit area map
        for (temp = jnc_list->first(); temp; temp = jnc_list->next()) {
            //translate jnc name here...
	    	if (!temp->translatedJncFile.isEmpty()) {
	        	//copy from temp->jncFile to temp->translatedJncFile
	      		QString new_jnc;

	      		new_jnc.sprintf ("%s/%s",sessionDir.data(), temp->translatedJncFile.data());
				QString old_file_name = temp->jncFile;

				if (!temp->jncFile.contains(jncPath)) {
					/* 
					 * If the jitted block does not contain the jnc path,
					 * this must be a import session.  We must rename the file
					 * to be copied from
					 */
					QString java = "/Java/";
					int index = temp->jncFile.find(java) + java.length();

					old_file_name =  jncPath + 
						temp->jncFile.right(temp->jncFile.length() - index);
				}

	      		QFile new_file (new_jnc);
	      		QFile old_file (old_file_name);


	      		if (!old_file.open(QIODevice::ReadOnly)) {
					#ifdef _DEBUG_
		  				qDebug("failed to open old file %s", temp->jncFile.data());
					#endif
		  			return false;
                }

	      		if (!new_file.open (QIODevice::WriteOnly ))
		  			return false;

	      		const QByteArray temp_buffer = old_file.readAll();
	      		new_file.writeBlock (temp_buffer);

	      		old_file.close();
	      		new_file.close();
			}	
		}
	}
}

/*
HRESULT CTaskInfo::WriteJncFiles (const char * directory)
{
	HRESULT hr = S_OK;
    JNCLIST_MAP::Iterator map_it;
    for ( map_it = m_tiJncMap.begin(); map_it != m_tiJncMap.end(); ++map_it ) {
		JNC_VALUE *temp;
		JNC_LIST *jnc_list = map_it.data();
		//check jit area map 
		for ( temp = jnc_list->first(); temp; temp = jnc_list->next() ) {
			//translate jnc name here...
			if (!temp->translatedJncFile.isEmpty()) {
				//copy from temp->jncFile to temp->translatedJncFile
				QString new_jnc;

				new_jnc.sprintf ("%s\\%s",directory, temp->translatedJncFile.data());

				QFile new_file (new_jnc);
				QFile old_file (temp->jncFile);

				if (!old_file.open(IO_ReadOnly))
					return E_FAIL;
				if (!new_file.open (IO_WriteOnly ))
					return E_FAIL;
				const QByteArray temp_buffer = old_file.readAll();
				new_file.writeBlock (temp_buffer);

				old_file.close();
				new_file.close();
			}

		}
    }
 
	return hr;
}
*/


/*
int main()
{
    CTaskInfo ti;

    ti.ReadTaskInfoFile("ca_profile.ti");
    ti.PrintTIVector();

    ti.ProcessTaskInfoVector();

    return 0;
}
*/
