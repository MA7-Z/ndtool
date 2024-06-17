#ifndef __DRVMOUNT_H__
#define __DRVMOUNT_H__
#include "mlib.h"
#include "mthread.h"

#define DM_APPSECTION							TEXT("DrvMount")
#define DM_INIFILE								TEXT("_DM.ini")
#define DM_DMSTARTNAME							TEXT("DMStart.exe")
#define DM_DMSTARTUPNAME						TEXT("DMStart.lnk")
#define DM_DMHELPFILE							TEXT("drvmount.chm")
#define DM_LOGFILE								TEXT("_DM.log")
#define DM_IK_ERRNOTIFY							TEXT("ERRNOTIFY")
#define DM_IK_ALLUSSC							TEXT("ALLUSSC")
#define DM_IK_CREFWAIT							TEXT("CREFWAIT")
#define DM_IKS_PATH								TEXT("PATH")
#define DM_IKS_USER								TEXT("USER")
#define DM_IKS_PWD								TEXT("PWD")
#define DM_IKS_RETRY							TEXT("RETRY")
#define DM_IKS_RTWAIT							TEXT("RTWAIT")
#define DM_IKS_SCON								TEXT("SCON")
#define DM_IKS_FORCE							TEXT("FORCE")
#define DM_IKS_EXEC								TEXT("EXEC")
#define DM_IKS_EXECPATH							TEXT("EXECPATH")

#define DM_MAXPATH								MAX_PATH
#define DM_USERLEN								128
#define DM_PWDLEN								128
#define DM_EXECPATHLEN							(DM_MAXPATH*4+1)
#define DM_EL_PATH								DM_MAXPATH
#define DM_EL_USER								DM_USERLEN
#define DM_EL_PWD								DM_PWDLEN
#define DM_EL_RETRY								4
#define DM_EL_RTWAIT							4
#define DM_EL_EXECPATH							DM_EXECPATHLEN
#define DM_DEF_RETRY							10
#define DM_DEF_RTWAIT							30
#define DM_DEF_CREFWAIT							1500
#define DM_MAX_CREFWAIT							9999

typedef struct _DMCOMMON{
	UINT				uERRNOTIFY;
	UINT				uALLUSSC;
	UINT				uCREFWAIT;
} DMCOMMON,*PDMCOMMON;


typedef struct _DMINFO{
	TCHAR					cDRVLTR;
	TCHAR					szPATH[DM_MAXPATH+1];
	TCHAR					szUSER[DM_USERLEN];
	TCHAR					szPWD[DM_PWDLEN];
	UINT					uRETRY;
	UINT					uRTWAIT;
	UINT					uSCON;
	UINT					uFORCE;
	UINT					uEXEC;
	TCHAR					szEXECPATH[DM_EXECPATHLEN];
}DMINFO,*PDMINFO;

extern void DM_OpenLog(HINSTANCE hInst);
extern void DM_WriteLog(LPCTSTR lpMsg);
extern void DM_CloseLog();

class DrvConnect : public MThread{
	DMINFO&					m_di;
	static bool				m_exec;
	static int				m_count;
	static int				m_err;
	static MSyncObject		m_sync;
public:
	DrvConnect(DMINFO& di) : m_di(di){}
	void Run();
	static bool IsExec() { return m_exec; }
	static int GetRunningCount() { return m_count; }
	static void ResetErrorCount();
	static int GetErrorCount(){ return m_err; }
	static LPCTSTR GetErrorMessages();
	static LPCTSTR GetExecErrMessages();
};

class DrvMount{
protected:
public:
	static bool GetLogFileName(HINSTANCE hInst, LPTSTR lpLogFile, int size);
	static bool GetIniFileName(HINSTANCE hInst, LPTSTR lpIniFile, int size);
	static void ShufflePassword(LPTSTR lpPassword, int pwlen, LPCTSTR lpSalt, int saltlen);
	static bool SaveCommonInfo(LPCTSTR lpIniFile, const DMCOMMON& dc);
	static bool LoadCommonInfo(LPCTSTR lpIniFile, DMCOMMON& dc);
	static bool SaveDriveInfo(LPCTSTR lpIniFile, const DMINFO& di);
	static bool LoadDriveInfo(LPCTSTR lpIniFile, DMINFO& di);
	static bool GetStartupShortcut(LPTSTR lpPath, int size, bool common);
	static bool IsStartupShortcut(bool common);
	static bool MakeStartupShortcut(MBase *pBase, bool common, LPCTSTR lpModuleDir);
	static int ConnectDrive(const DMINFO& di);
	static bool AllocateDrive(const DMINFO& di);
	static int DisconnectDrive(const DMINFO& di);
	static bool ReleaseDrive(const DMINFO& di);
	static int Mount(const DMINFO& di);
	static int Unmount(const DMINFO& di);
	static bool Exec(LPCTSTR lpCmd, HWND hWnd=NULL);
};

#endif
