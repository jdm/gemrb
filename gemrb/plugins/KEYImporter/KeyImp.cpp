/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/KEYImporter/KeyImp.cpp,v 1.26 2003/12/07 10:02:28 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "KeyImp.h"
#include "../../includes/globals.h"
#include "../Core/FileStream.h"
#include "../Core/Interface.h"
#include "../Core/ArchiveImporter.h"
#include "../Core/AnimationMgr.h"
#ifndef WIN32
#include <unistd.h>
#include <dirent.h>
#endif

static char overridesubfolder[9]="override";

#ifndef WIN32
char * FindInDir(char * Dir, char * Filename)
{ 
	char * fn = NULL; 
	DIR * dir = opendir(Dir); 
	if(dir == NULL) 
		return NULL; 
	struct dirent * de = readdir(dir); 
	if(de == NULL) 
		return NULL;
	do { 
		if(strnicmp(de->d_name, Filename, sizeof(Filename)) == 0) {
			fn = (char*)malloc(strlen(de->d_name)+1);
			strcpy(fn, de->d_name);
			break;
		}
	} while((de = readdir(dir)) != NULL);
	closedir(dir);  //No other files in the directory, close it
	return fn;
}
#endif

KeyImp::KeyImp(void)
{
	if(core->CaseSensitive) {
		char path[_MAX_PATH];
		strcpy(path, core->GamePath);
		strcat(path, overridesubfolder);
		if(!dir_exists(path) ) {
			overridesubfolder[0]=toupper(overridesubfolder[0]);
		}
	}
}

KeyImp::~KeyImp(void)
{
	for(unsigned int i = 0; i < biffiles.size(); i++) {
		free(biffiles[i].name);
	}
}

bool KeyImp::LoadResFile(const char * resfile)
{
	char fn[_MAX_PATH] = {0};
#ifndef WIN32
	if(core->CaseSensitive) {
		ExtractFileFromPath(fn, resfile);
		char * newname = FindInDir(core->GamePath, fn);
		if(newname) {
			strcpy(fn, core->GamePath);
			strcat(fn, newname);
			free(newname);
		}
	}
	else
#endif
	strcpy(fn,resfile);
	printf("[KEY Importer]: Opening %s...", fn);
	FileStream * f = new FileStream();
	if(!f->Open(fn)) {
		printf("[ERROR]\nCannot open Chitin.key\n");
		delete(f);
		return false;
	}
	printf("[OK]\nChecking file type...");
	char Signature[8];
	f->Read(Signature, 8);
	if(strncmp(Signature, "KEY V1  ", 8) != 0) {
		printf("[ERROR]\nFile has an Invalid Signature.\n");
		delete(f);
		return false;
	}
	printf("[OK]\nReading Resources...\n");
	unsigned long BifCount, ResCount, BifOffset, ResOffset;
	f->Read(&BifCount, 4);
	f->Read(&ResCount, 4);
	f->Read(&BifOffset, 4);
	f->Read(&ResOffset, 4);
	printf("BIF Files Count: %ld (Starting at %ld Bytes)\nRES Count: %ld (Starting at %ld Bytes)\n", BifCount, BifOffset, ResCount, ResOffset);
	f->Seek(BifOffset, GEM_STREAM_START);
	unsigned long BifLen, ASCIIZOffset;
	unsigned short ASCIIZLen;
	for(unsigned int i = 0; i < BifCount; i++) {
		BIFEntry be;
		f->Seek(BifOffset + (12*i), GEM_STREAM_START);
		f->Read(&BifLen, 4);
		f->Read(&ASCIIZOffset, 4);
		f->Read(&ASCIIZLen, 2);
		f->Read(&be.BIFLocator, 2);
		be.name = (char*)malloc(ASCIIZLen);
		f->Seek(ASCIIZOffset, GEM_STREAM_START);
		f->Read(be.name, ASCIIZLen);
#ifndef WIN32
		for(int p = 0; p < ASCIIZLen; p++) {
		  if(be.name[p] == '\\')
		    be.name[p] = '/';
		}
		if(core->CaseSensitive) {
			char fullPath[_MAX_PATH], tmpPath[_MAX_PATH] = {0}, fn[_MAX_PATH] = {0};
			strncpy( tmpPath , be.name, (strrchr(be.name, PathDelimiter)+1)-be.name);
			strcpy(fullPath, core->GamePath);
			strcat(fullPath, tmpPath);
			ExtractFileFromPath(fn, be.name);
			char * newname = FindInDir(fullPath, fn);
			if(newname) {
				strcpy(be.name, tmpPath);
				strcat(be.name, newname);
				free(newname);
			}
		}
#endif
		biffiles.push_back(be);
	}
	f->Seek(ResOffset, GEM_STREAM_START);
        resources.InitHashTable(ResCount);
	for(unsigned int i = 0; i < ResCount; i++) {
		RESEntry re;
		f->Read(re.ResRef, 8);
		f->Read(&re.Type, 2);
		f->Read(&re.ResLocator, 4);
		char *key;
		key=new char[8];
		for(int j=0;j<8;j++) key[j]=toupper(re.ResRef[j]);
		resources.SetAt(key, re.Type, re.ResLocator);
	}
	printf("Resources Loaded Succesfully.\n");
	delete(f);
	return true;
}

#define SearchIn(BasePath, Path, ResRef, Type, foundMessage) \
{ \
	char p[_MAX_PATH], f[_MAX_PATH] = {0}; \
	strcpy(p, BasePath); \
	strcat(p, Path); \
	strcat(p, SPathDelimiter); \
	strncpy(f, ResRef, 8); \
	f[8] = 0; \
	strcat(f, core->TypeExt(Type)); \
	strlwr(f); \
	strcat(p, f); \
	FILE * exist = fopen(p, "rb"); \
	if(exist) { \
		printf(foundMessage); \
		fclose(exist); \
		FileStream * fs = new FileStream(); \
		if(!fs) return NULL; \
		fs->Open(p, true); \
		return fs; \
	} \
}

DataStream * KeyImp::GetResource(const char * resname, SClass_ID type)
{
	char path[_MAX_PATH], BasePath[_MAX_PATH], filename[_MAX_PATH] = {0};
	//Search it in the GemRB override Directory
	strcpy(path, "override"); //this shouldn't change
	strcat(path, SPathDelimiter);
	strcat(path, core->GameType);
	SearchIn(core->GemRBPath, path, resname, type, "[KEYImporter]: Found in GemRB Override...\n");
	SearchIn(core->GamePath, overridesubfolder, resname, type, "[KEYImporter]: Found in Override...\n");
	SearchIn(core->GamePath, "Data", resname, type, "[KEYImporter]: Found in Local CD1 Folder...\n");
	printf("[KEYImporter]: Searching for %.8s%s...\n", resname, core->TypeExt(type));
	unsigned long ResLocator;
	if(resources.Lookup(resname,type,ResLocator) ) {
		if(!core->IsAvailable(IE_BIF_CLASS_ID)) {
			printf("[ERROR]\nAn Archive Plug-in is not Available\n");
			return NULL;
		}
		int bifnum = (ResLocator & 0xFFF00000) >> 20;
		ArchiveImporter * ai = (ArchiveImporter*)core->GetInterface(IE_BIF_CLASS_ID);
		FILE * exist = NULL;
		if(exist == NULL) {
			strcpy(path, core->GamePath);
			strcat(path, biffiles[bifnum].name);
			exist = fopen(path, "rb");
			if(!exist) {
				strcpy(path, core->GamePath);
				strncat(path, biffiles[bifnum].name, strlen(biffiles[bifnum].name)-4);
				strcat(path, ".cbf");
				exist = fopen(path, "rb");
			}
		}
		if(exist == NULL) {
			if((biffiles[bifnum].BIFLocator & (1<<2)) != 0) {
				strcpy(BasePath, core->CD1);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<3)) != 0) {
				strcpy(BasePath, core->CD2);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<4)) != 0) {
				strcpy(BasePath, core->CD3);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<5)) != 0) {
				strcpy(BasePath, core->CD4);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<6)) != 0) {
				strcpy(BasePath, core->CD5);
			}
			else {
				printf("[KEYImporter]: Error: Cannot find %s... Resource unavailable.\n", biffiles[bifnum].name);
				return NULL;
			}
			strcpy(path, BasePath);
			strcat(path, biffiles[bifnum].name);
			exist = fopen(path, "rb");
			if(exist == NULL) {
				//Trying CBF Extension
				strcpy(path, BasePath);
				strncat(path, biffiles[bifnum].name, strlen(biffiles[bifnum].name)-4);
				strcat(path, ".cbf");
				exist = fopen(path, "rb");
				if(!exist) {
					printf("[KEYImporter]: Cannot find %s.", path);
					core->FreeInterface(ai);
					return NULL;
				}
			}
			
			fclose(exist);
		}
		else
			fclose(exist);
		ai->OpenArchive(path);
		DataStream * ret = ai->GetStream(ResLocator, type);
		if(ret == NULL)
			printf("[NOT_FOUND]\n");
		core->FreeInterface(ai);
		if(ret) {
			strncpy(ret->filename, resname,8);
			ret->filename[8]=0;
			strcat(ret->filename, core->TypeExt(type));
		}
		return ret;
	}
	return NULL;
}
void * KeyImp::GetFactoryResource(const char * resname, SClass_ID type, unsigned char mode)
{
	if(type != IE_BAM_CLASS_ID) {
		printf("[KEYImporter]: %s files are not supported.\n", core->TypeExt(type));
		return NULL;
	}
	int fobjindex;
	if((fobjindex = core->GetFactory()->IsLoaded(resname, type)) != -1) {
		printf("[KEYImporter]: Factory Object Found!\n");
		return core->GetFactory()->GetFactoryObject(fobjindex);
	}
	printf("[KEYImporter]: No Factory Object Found, Loading...\n");
	char path[_MAX_PATH], filename[_MAX_PATH] = {0};
	//Search it in the GemRB override Directory
	strcpy(path, core->GemRBPath);
	strcat(path, "override"); //this shouldn't change!
	strcat(path, SPathDelimiter);
	strcat(path, core->GameType);
	strcat(path, SPathDelimiter);
	strncpy(filename, resname, 8);
	filename[8]=0;
	strcat(filename, core->TypeExt(type));
	strlwr(filename);
	strcat(path, filename);
	FILE * exist = fopen(path, "rb");
	if(exist) {
		printf("[KEYImporter]: Found in GemRB Override...\n");
		fclose(exist);
		FileStream * fs = new FileStream();
		if(!fs)
			return NULL;
		fs->Open(path, true);
		return fs;
	}
	strcpy(path, core->GamePath);
	strcat(path, overridesubfolder);
	strcat(path, SPathDelimiter);
	strncat(path, resname, 8);
	strcat(path, core->TypeExt(type));
	exist = fopen(path, "rb");
	if(exist) {
		printf("[KEYImporter]: Found in Override...\n");
		fclose(exist);
		AnimationMgr * ani = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
		if(!ani)
			return NULL;
		FileStream * fs = new FileStream();
		if(!fs)
			return NULL;
		fs->Open(path, true);
		ani->Open(fs, true);
		AnimationFactory * af = ani->GetAnimationFactory(resname, mode);
		core->FreeInterface(ani);
		core->GetFactory()->AddFactoryObject(af);
		return af;
/*
		FileStream * fs = new FileStream();
		if(!fs)
			return NULL;
		fs->Open(path, true);
		return fs;
*/
	}
	/*printf("[KEYImporter]: Searching for %.8s%s...\n", resname, core->TypeExt(type));
	unsigned long ResLocator;
	if(resources.Lookup(resname, type, ResLocator) ) {
		int bifnum = (ResLocator & 0xFFF00000) >> 20;
		ArchiveImporter * ai = (ArchiveImporter*)core->GetInterface(IE_BIF_CLASS_ID);
		FILE * exist = NULL;
		if(exist == NULL) {
			strcpy(path, core->GamePath);
			strcat(path, biffiles[bifnum].name);
			exist = fopen(path, "rb");
		}
		if(exist == NULL) {
			if((biffiles[bifnum].BIFLocator & (1<<2)) != 0) {
				strcpy(path, core->CD1);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<3)) != 0) {
				strcpy(path, core->CD2);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<4)) != 0) {
				strcpy(path, core->CD3);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<5)) != 0) {
				strcpy(path, core->CD4);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<6)) != 0) {
				strcpy(path, core->CD5);
			}
			else {
				printf("[KEYImporter]: Error: Cannot find Bif file... Resource unavailable.\n");
				return NULL;
			}
			strcat(path, biffiles[bifnum].name);
			exist = fopen(path, "rb");
			if(exist == NULL) {
				printf("[KEYImporter]: Cannot find %s.\n", biffiles[bifnum].name);
				core->FreeInterface(ai);
				return NULL;
			}
			fclose(exist);
		}
		else
			fclose(exist);
		ai->OpenArchive(path);
		DataStream * ret = ai->GetStream(ResLocator, type);
		if(ret == NULL)
			printf("[NOT_FOUND]\n");
		core->FreeInterface(ai);
		strncpy(ret->filename, resname, 8);
		ret->filename[8] = 0;
		strcat(ret->filename, core->TypeExt(type));*/
	DataStream * ret = GetResource(resname, type);
	if(ret) {
		AnimationMgr * ani = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
		if(!ani)
			return NULL;
		ani->Open(ret, true);
		AnimationFactory * af = ani->GetAnimationFactory(resname, mode);
		core->FreeInterface(ani);
		core->GetFactory()->AddFactoryObject(af);
		return af;
	}
	return NULL;
}
