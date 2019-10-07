#include <cstdlib>
#include<iostream>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
using namespace std;
typedef int(*CmdProcFunc)(void);
typedef struct
{
  char *pszCmd;
  CmdProcFunc fpCmd;
}CMD_PROC;
#define MOCK_FUNC(funcName)\
    int funcName(void){printf(" Enter "#funcName"!\n");return 0;}
#define CMD_ENTRY(cmdStr,func)  {cmdStr,func}
#define CMD_ENTRY_END           {NULL,NULL}
#define CMD_MAP_NUM     (sizeof(gCmdMap)/sizeof(CMD_PROC))-1
MOCK_FUNC(ShowMeInfo);
MOCK_FUNC(SetLogCtrl);
MOCK_FUNC(TestBatch);
MOCK_FUNC(TestEndianOper);
static CMD_PROC gCmdMap[]=
{
 CMD_ENTRY("ShowMeInfo",       ShowMeInfo),
 CMD_ENTRY("SetLogCtrl",       SetLogCtrl),
 CMD_ENTRY("TestBatch",        TestBatch),
 CMD_ENTRY("TestEndian",       TestEndianOper),

 CMD_ENTRY_END
};
static char *GetCmdByIndex(unsigned int dwCmdIndex)
{
  if(dwCmdIndex>= CMD_MAP_NUM)
    return NULL;
  return gCmdMap[dwCmdIndex].pszCmd;
}
static int ExecCmd(char *pszCmdLine)
{
   if(NULL==pszCmdLine)
     return -1;
   unsigned int dwCmdIndex=0;
   for(; dwCmdIndex<CMD_MAP_NUM;dwCmdIndex++)
    {
      if(!strcmp(pszCmdLine,gCmdMap[dwCmdIndex].pszCmd))
        break;
    }
   if(CMD_MAP_NUM==dwCmdIndex)
     return -1;
   gCmdMap[dwCmdIndex].fpCmd();
return 0;
}
