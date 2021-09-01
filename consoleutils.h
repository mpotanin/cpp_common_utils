#pragma once

#ifndef __CONSOLEUTILS_
#define __CONSOLEUTILS_

#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <stdio.h>

#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "vrtdataset.h"
#include "gdalwarper.h"
//#include "gd.h"
#include "sqlite3.h"

using namespace std;


class MPLGDALDelayLoader
{
public:
  static string ReadPathFromConfigFile(string strConfigFile);
  static bool Load (string strExecPath);

protected:
  static void SetWinEnvVars (string strGDALPath);
  static bool LoadWinDll (string strGDALDir, string strDllVer);
  

protected:
  static string strGDALWinVer;
};

typedef struct 
{
  string strOptionName;
  bool bIsBoolean;
  int nOptionValueType; //0 - single, 1 - multiple, 2 - multiple key=value
  bool bIsObligatory;
  string strUsage;
} MPLOptionDescriptor;


class MPLOptionParser
{
public:
  static bool InitCmdLineArgsFromFile (string strFileName, 
                                      vector<string> &vecArgs, 
                                      string strExeFilePath="");

public:
  static void PrintUsage (const list<MPLOptionDescriptor> listDescriptors,
                          const list<string> listExamples);
    
  bool Init (const list<MPLOptionDescriptor> listDescriptors, vector<string> &vecArgs);
    
  string GetOptionValue (string strOptionName);
  list<string> GetValueList (string strMultipleOptionName);
  map<string,string> GetKeyValueCollection (string strMultipleKVOptionName);

private:
  void Clear();

private:
  map<string,MPLOptionDescriptor> m_mapDescriptors;
  map<string,string> m_mapSingleOptions;
  map<string,map<string,string>> m_mapMultipleKVOptions;
  map<string,list<string>> m_mapMultipleOptions;
};

#include "consoleutils.cpp"

#endif