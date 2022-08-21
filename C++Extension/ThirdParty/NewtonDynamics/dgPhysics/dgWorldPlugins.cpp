/* Copyright (c) <2003-2019> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dgPhysicsStdafx.h"
#include "dgWorld.h"
#include "dgWorldPlugins.h"


#if defined(DG_USE_PLUGINS) && defined(__linux__)
#include <dlfcn.h>
#include <dirent.h>
#endif
	

dgWorldPluginList::dgWorldPluginList(dgMemoryAllocator* const allocator)
	:dgList<dgWorldPluginModulePair>(allocator)
	,m_currentPlugin(NULL)
	,m_preferedPlugin(NULL)
{
}

dgWorldPluginList::~dgWorldPluginList()
{
}


void dgWorldPluginList::LoadVisualStudioPlugins(const char* const plugInPath)
{
#if defined(DG_USE_PLUGINS) && defined(_MSC_VER)
	char rootPathInPath[2048];
	sprintf(rootPathInPath, "%s/*.dll", plugInPath);

	dgInt32 score = 0;
	dgWorld* const world = (dgWorld*) this;

	// scan for all plugins in this folder
	//_finddata_t data;
	//intptr_t handle = _findfirst(rootPathInPath, &data);

	WIN32_FIND_DATAA data;
	HANDLE handle = FindFirstFile(rootPathInPath, &data);

	if (handle != INVALID_HANDLE_VALUE ) {
		do {
			//sprintf(rootPathInPath, "%s/%s", plugInPath, data.name);
			sprintf(rootPathInPath, "%s/%s", plugInPath, data.cFileName);
			HMODULE module = LoadLibrary(rootPathInPath);

			if (module) {
				// get the interface function pointer to the Plug in classes
				InitPlugin initModule = (InitPlugin)GetProcAddress(module, "GetPlugin");
				if (initModule) {
					dgWorldPlugin* const plugin = initModule(world, GetAllocator ());
					if (plugin) {
						dgWorldPluginModulePair entry(plugin, module);
						dgListNode* const node = Append(entry);
						dgInt32 pluginValue = plugin->GetScore();
						bool wasMoved = false;
						for (dgListNode* ptr = GetLast()->GetPrev(); ptr; ptr = ptr->GetPrev()) {
							dgInt32 value = ptr->GetInfo().m_plugin->GetScore();
							if (value > pluginValue) {
								InsertAfter (ptr, node);
								wasMoved = true;
								break;
							}
						}
						if (!wasMoved) {
							InsertBefore (GetFirst(), node);
						}
						
						if (pluginValue > score) {
							score = pluginValue;
							m_preferedPlugin = node; 
						}
					} else {
						FreeLibrary(module);
					}
				} else {
					FreeLibrary(module);
				}
			}

		//} while (_findnext(handle, &data) == 0);
		 } while (FindNextFile(handle, &data));

		//_findclose(handle);
		FindClose(handle);
	}
#endif
}

void dgWorldPluginList::LoadLinuxPlugins(const char* const plugInPath)
{
#if defined(DG_USE_PLUGINS) && defined(__linux__)
	char rootPathInPath[2048];
	DIR* directory;
	dirent* dirEntry;
	directory = opendir(plugInPath);

	dgInt32 score = 0;
	dgWorld* const world = (dgWorld*) this;
	
	if(directory != NULL) {
		while((dirEntry = readdir(directory)) != NULL) {
			const char* const ext = strrchr(dirEntry->d_name, '.');
			if(!strcmp(ext, ".so")) {
				sprintf(rootPathInPath, "%s/%s", plugInPath, dirEntry->d_name);
				void* module = dlopen(rootPathInPath, RTLD_LAZY);
				auto err = dlerror();
 				if(module) {
					InitPlugin initModule = (InitPlugin)dlsym(module, "GetPlugin");
					if(initModule) {
						dgWorldPlugin* const plugin = initModule(world, GetAllocator ());
						if (plugin) {
							dgWorldPluginModulePair entry(plugin, module);
							dgListNode* const node = Append(entry);
							dgInt32 pluginValue = plugin->GetScore();
							if (pluginValue > score) {
								score = pluginValue;
								m_preferedPlugin = node; 
							}
						} else {
							dlclose(module);
						}
					}
				}
			}
		}
		closedir(directory);
	}
#endif
}

void dgWorldPluginList::LoadPlugins(const char* const path)
{
	UnloadPlugins();
	#ifdef _MSC_VER
		LoadVisualStudioPlugins(path);
	#elif __linux__
		LoadLinuxPlugins(path);
	#endif
}

void dgWorldPluginList::UnloadPlugins()
{
#if defined(DG_USE_PLUGINS)
	#ifdef _MSC_VER
		dgWorldPluginList& pluginsList = *this;
		for (dgWorldPluginList::dgListNode* node = pluginsList.GetFirst(); node; node = node->GetNext()) {
			HMODULE module = (HMODULE)node->GetInfo().m_module;
			FreeLibrary(module);
		}
	#elif __linux__
		dgWorldPluginList& pluginsList = *this;
		for (dgWorldPluginList::dgListNode* node = pluginsList.GetFirst(); node; node = node->GetNext()) {
			void* module = node->GetInfo().m_module;
			dlclose(module);
		}
	#endif
#endif
	m_currentPlugin = NULL;
	m_preferedPlugin = NULL;
}

dgWorldPluginList::dgListNode* dgWorldPluginList::GetCurrentPlugin()
{
	return m_currentPlugin;
}

dgWorldPluginList::dgListNode* dgWorldPluginList::GetpreferedPlugin()
{
	return m_preferedPlugin;
}

dgWorldPluginList::dgListNode* dgWorldPluginList::GetFirstPlugin()
{
	dgWorldPluginList& list = *this;
	return list.GetFirst();
}

dgWorldPluginList::dgListNode* dgWorldPluginList::GetNextPlugin(dgListNode* const plugin)
{
	return plugin->GetNext();
}

const char* dgWorldPluginList::GetPluginId(dgListNode* const pluginNode)
{
	dgWorldPluginModulePair entry(pluginNode->GetInfo());
	dgWorldPlugin* const plugin = entry.m_plugin;
	return plugin->GetId();
}

void dgWorldPluginList::SelectPlugin(dgListNode* const plugin)
{
	m_currentPlugin = plugin;
}
