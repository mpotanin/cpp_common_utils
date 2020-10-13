#ifndef __FILESYSTEMFUNCS_
#define __FILESYSTEMFUNCS_

#pragma once

#define _FILE_OFFSET_BITS 64
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <string>
#else
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

#include <stdint.h>
#include <thread>
#include <mutex>
#include <future>
#include <chrono>
#include <regex>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

#include "stringfuncs.h"

#ifdef _WIN32
#define GMXFSWIN32
#endif

class MPLFileSys
{
public:
	static bool	WriteToTextFile(string strFileName, string strText)
	{
		FILE *fp = OpenFile(strFileName, "w");
		if (!fp) return false;
		fprintf(fp, "%s", strText.data());
		fclose(fp);
		return true;
	};


	static string	GetPath(string strFileName)
	{
		if (strFileName.find_last_of("/") != string::npos) return strFileName.substr(0, strFileName.find_last_of("/") + 1);
		else return "";
	}


	static bool FileExists(string strFileName)
	{
#ifdef GMXFSWIN32
		wstring strFileNameW;
		MPLString::utf8toWStr(strFileNameW, strFileName);

		return !(GetFileAttributesW(strFileNameW.c_str()) == INVALID_FILE_ATTRIBUTES); //GetFileAttributes ->stat
#else
		if (FILE* fp = fopen(strFileName.c_str(), "r"))
		{
			fclose(fp);
			return true;
		}
		else if (DIR* psDIR = opendir(strFileName.c_str()))
		{
			closedir(psDIR);
			return true;
		}
		else return false;
#endif
	}


	static bool IsDirectory(string strPath)
	{
		if (!FileExists(strPath)) return false;
#ifdef GMXFSWIN32
		wstring strPathW;
		MPLString::utf8toWStr(strPathW, strPath);
		return (GetFileAttributesW(strPathW.c_str()) & FILE_ATTRIBUTE_DIRECTORY);
#else
		if (DIR* psDIR = opendir(strPath.c_str()))
		{
			closedir(psDIR);
			return true;
		}
		else return false;
#endif
	}

	static string GetRuntimeModulePath()
	{
#ifdef GMXFSWIN32
		wchar_t exe_filename_w[_MAX_PATH + 1];
		GetModuleFileNameW(0, exe_filename_w, _MAX_PATH);
		string exe_filename = MPLString::wstrToUtf8(exe_filename_w);
		exe_filename = MPLString::ReplaceAll(exe_filename, "\\", "/");
		return GetPath(exe_filename);
#else
		return "";
#endif
	}



	static string	RemoveEndingSlash(string	strFolderName)
	{
		return	(strFolderName == "") ? "" :
			(strFolderName[strFolderName.length() - 1] == '/') ? strFolderName.substr(0, strFolderName.length() - 1) : strFolderName;
	}


	static string	GetAbsolutePath(string strBasePath, string strRelativePath)
	{
		return strBasePath + "/" + strRelativePath;

		strBasePath = RemoveEndingSlash(strBasePath);
		regex regUpLevel("^(\\.| )*\\/.+");

		while (regex_match(strRelativePath, regUpLevel))
		{
			if (strBasePath.find('/') == string::npos) break;
			strRelativePath = strRelativePath.substr(strRelativePath.find('/') + 1);
			strBasePath = strBasePath.substr(0, strBasePath.rfind('/'));
		}

		return strBasePath == "" ? strRelativePath : strBasePath + "/" + strRelativePath;
	}


	static string RemovePath(string strFileName)
	{
		return  (strFileName.find_last_of("/") != string::npos) ?
			strFileName.substr(strFileName.find_last_of("/") + 1) :
			strFileName;
	}


	static string RemoveExtension(string strFileName)
	{
		int nPointPos = strFileName.rfind(L'.');
		int nSlashPos = strFileName.rfind(L'/');
		return (nPointPos > nSlashPos) ? strFileName.substr(0, nPointPos) : strFileName;
	}



	static int FindFilesByPattern(list<string> &listFiles, string strSearchPattern)
	{
#ifdef GMXFSWIN32
		WIN32_FIND_DATAW owinFindFileData;
		HANDLE powinFind;

		wstring strSearchPatternW;
		MPLString::utf8toWStr(strSearchPatternW, strSearchPattern);

		powinFind = FindFirstFileW(strSearchPatternW.data(), &owinFindFileData);

		if (powinFind == INVALID_HANDLE_VALUE)
		{
			if (powinFind) FindClose(powinFind);
			return 0;
		}

		while (true)
		{
			if (owinFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
			{
				string strFoundFile = MPLString::wstrToUtf8(owinFindFileData.cFileName);
				listFiles.push_back(GetAbsolutePath(GetPath(strSearchPattern), strFoundFile));
			}
			if (!FindNextFileW(powinFind, &owinFindFileData)) break;
		}

		if (powinFind) FindClose(powinFind);
#else
		string strBasePath = GetPath(strSearchPattern);
		string strFileNamePattern = RemovePath(strSearchPattern);
		MPLString::ReplaceAll(strFileNamePattern, "*", ".*");
		regex regFileNamePattern(strFileNamePattern);

		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir(strBasePath == "" ? "." : strBasePath.c_str())) != 0)
		{
			while ((ent = readdir(dir)) != 0)
			{
				if ((regex_match(ent->d_name, regFileNamePattern)) && (!IsDirectory(ent->d_name)))
					listFiles.push_back(GetAbsolutePath(strBasePath, ent->d_name));
			}
			closedir(dir);
		}
		else return 0;
#endif

		listFiles.sort();
		return listFiles.size();
	}


	static int FindFilesByExtensionRecursive(list<string> &listFiles, string strFolder, string	strExtension)
	{
		strExtension = (strExtension[0] == '.') ? strExtension.substr(1, strExtension.length() - 1) : strExtension;
		regex regFileNamePattern((strExtension == "") ? ".*" : (".*\\." + strExtension + "$"));

#ifdef GMXFSWIN32
		WIN32_FIND_DATAW owinFindFileData;
		HANDLE powinFind;

		wstring strPathW;
		MPLString::utf8toWStr(strPathW, GetAbsolutePath(strFolder, "*"));
		powinFind = FindFirstFileW(strPathW.c_str(), &owinFindFileData);

		if (powinFind == INVALID_HANDLE_VALUE)
		{
			FindClose(powinFind);
			return 0;
		}

		while (true)
		{
			string strFoundFile = MPLString::wstrToUtf8(owinFindFileData.cFileName);
			if ((owinFindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) && (owinFindFileData.dwFileAttributes != 48))
			{
				if (regex_match(strFoundFile, regFileNamePattern))
					listFiles.push_back(GetAbsolutePath(strFolder, strFoundFile));
			}
			else
			{
				if ((strFoundFile != ".") && (strFoundFile != ".."))
					FindFilesByExtensionRecursive(listFiles, GetAbsolutePath(strFolder, strFoundFile), strExtension);
			}
			if (!FindNextFileW(powinFind, &owinFindFileData)) break;
		}

		if (powinFind) FindClose(powinFind);
		return listFiles.size();
#else

		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir(strFolder == "" ? "." : strFolder.c_str())) != 0)
		{
			while ((ent = readdir(dir)) != 0)
			{
				if ((ent->d_name[0] == '.') && ((ent->d_name[1] == 0) || (ent->d_name[2] == 0))) continue;
				if (IsDirectory(GetAbsolutePath(strFolder, ent->d_name)))
					FindFilesByExtensionRecursive(listFiles, GetAbsolutePath(strFolder, ent->d_name), strExtension);
				else if (regex_match(ent->d_name, regFileNamePattern))
					listFiles.push_back(GetAbsolutePath(strFolder, ent->d_name));
			}
			closedir(dir);
		}
		else return 0;

		return listFiles.size();
#endif
	}


	static bool WriteWLDFile(string strRasterFile, double dblULX, double dblULY, double dblRes)
	{
		if (strRasterFile.length() > 1)
		{
			strRasterFile[strRasterFile.length() - 2] = strRasterFile[strRasterFile.length() - 1];
			strRasterFile[strRasterFile.length() - 1] = 'w';
		}

		FILE *fp = OpenFile(strRasterFile, "w");
		if (!fp) return false;

		fprintf(fp, "%.10lf\n", dblRes);
		fprintf(fp, "%lf\n", 0.0);
		fprintf(fp, "%lf\n", 0.0);
		fprintf(fp, "%.10lf\n", -dblRes);
		fprintf(fp, "%.10lf\n", dblULX + 0.5*dblRes);
		fprintf(fp, "%.10lf\n", dblULY - 0.5*dblRes);

		fclose(fp);

		return true;
	}


	static FILE* OpenFile(string	strFileName, string strMode)
	{
#ifdef GMXFSWIN32
		return _wfopen(MPLString::utf8toWStr(strFileName).c_str(),
			MPLString::utf8toWStr(strMode).c_str());
#else
		return fopen(strFileName.c_str(), strMode.c_str());
#endif
	}


	static bool		RenameFile(string strOldPath, string strNewPath)
	{
#ifdef GMXFSWIN32
		return 0 == _wrename(MPLString::utf8toWStr(strOldPath).c_str(),
			MPLString::utf8toWStr(strNewPath).c_str());
#else
		return 0 == rename(strOldPath.c_str(), strNewPath.c_str());
#endif
	}


	static bool		FileDelete(string strPath)
	{
#ifdef GMXFSWIN32
		return ::DeleteFileW(MPLString::utf8toWStr(strPath).c_str());
#else
		return (0 == remove(strPath.c_str()));
#endif
	}


	static bool		CreateDir(string strPath)
	{
#ifdef GMXFSWIN32
		return (_wmkdir(MPLString::utf8toWStr(strPath).c_str()) == 0);
#else
		return (mkdir(strPath.c_str(), 0777) == 0);
#endif
	}


	static bool	SaveDataToFile(string strFileName, void *pabData, int nSize)
	{
		FILE *fp;
		if (!(fp = OpenFile(strFileName, "wb"))) return false;
		fwrite(pabData, sizeof(char), nSize, fp);
		fclose(fp);

		return true;
	}


	static string		GetExtension(string strPath)
	{
		int n1 = strPath.rfind('.');
		if (n1 == string::npos) return "";
		int n2 = strPath.rfind('/');

		if (n2 == string::npos || n1 > n2) return strPath.substr(n1 + 1);
		else return "";
	}

	static string  ReadTextFile(string strFileName)
	{
		FILE *fp = OpenFile(strFileName, "r");
		if (!fp) return "";

		string strFileContent;
		char c;
		while (1 == fscanf(fp, "%c", &c))
			strFileContent += c;
		fclose(fp);

		return strFileContent;
	}

	static bool ReadTextFile(string strFileName, list<string> &listLines)
	{
		std::ifstream infile(strFileName.c_str());
		if (infile.eof()) return false;

		string strLine;
		while (getline(infile, strLine))
			listLines.push_back(strLine);
		infile.close();

		return true;
	}



	static bool ReadBinaryFile(string strFileName, void *&pabData, int &nSize)
	{
		FILE *fp = OpenFile(strFileName, "rb");
		if (!fp) return false;
		Fseek64(fp, 0, SEEK_END);
		nSize = ftell(fp);
		Fseek64(fp, 0, SEEK_SET);

		pabData = new char[nSize];
		fread(pabData, sizeof(char), nSize, fp);

		fclose(fp);
		return true;
	}

	static int Fseek64(FILE* poFile, uint64_t nOffset, int nOrigin)
	{
#ifdef GMXFSWIN32
		return _fseeki64(poFile, nOffset, nOrigin);
#else
		return fseeko(poFile, nOffset, nOrigin);
#endif
	}



	
};

class GMXThreading
{
public:
	static std::launch GetLaunchPolicy()
	{
#ifdef GMXFSWIN32
		return std::launch::async | std::launch::deferred;
#else
		return std::launch::async;
#endif
	}
};

//#include "filesystemfuncs.cpp"
#endif