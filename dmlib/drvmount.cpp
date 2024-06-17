#include "drvmount.h"
#include "md5.h"
#include "base64.h"
#include <string>
#include <shlobj.h>
#include <Lmcons.h>
#include "mcsv.h"
#include <tchar.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#ifdef UNICODE
#define tstring wstring
#else
#define tstring string
#endif

#pragma comment(lib, "mpr.lib")

using namespace std;

bool DrvConnect::m_exec = false;
int DrvConnect::m_count = 0;
int DrvConnect::m_err = 0;
MSyncObject DrvConnect::m_sync;

static tstring s_errmsg;
static tstring s_execerr;

static FILE *s_pf = NULL;
static MSyncObject s_sync;

void DM_OpenLog(HINSTANCE hInst)
{
	s_sync.Lock();
	if(s_pf != NULL){
		return;
		s_sync.Unlock();
	}

	HANDLE hFile;
	int fd;
	TCHAR szFile[MAX_PATH+1];

	DrvMount::GetLogFileName(hInst, szFile, sizeof(szFile)/sizeof(TCHAR));

	hFile = ::CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE){
		s_sync.Unlock();
		return;
	}
	fd = _open_osfhandle((intptr_t)hFile, 0);
	if(fd < 0){
		::CloseHandle(hFile);
	}else if((s_pf=_tfdopen(fd,TEXT("wb"))) == NULL){
		_close(fd);
	}else{
		if(sizeof(TCHAR)>sizeof(char)){
			unsigned short bom = 0xfeff;
			fwrite(&bom, sizeof(bom), 1, s_pf);
		}
	}
	s_sync.Unlock();
}

void DM_WriteLog(LPCTSTR lpMsg)
{
	SYSTEMTIME st;
	::GetLocalTime(&st);
	s_sync.Lock();
	if(s_pf != NULL){
		_ftprintf(s_pf, TEXT("%04u/%02u/%02u %02u:%02u:%02u\t%s\r\n"),
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
			lpMsg);
		fflush(s_pf);
	}
	s_sync.Unlock();
}

void DM_CloseLog()
{
	s_sync.Lock();
	if(s_pf != NULL){
		fclose(s_pf);
		s_pf = NULL;
	}
	s_sync.Unlock();
}

void DrvConnect::Run()
{
	SetResult(0);
	if(m_di.uSCON != BST_CHECKED) return;
	TCHAR szMsg[256];

	m_sync.Lock();
	if(!m_exec) m_exec = true;
	++m_count;
	m_sync.Unlock();

	int state;
	UINT trycnt = 0;

	while((state=DrvMount::Mount(m_di)) < 2){
		if(!state) break;
		if(m_di.uRETRY){
			++trycnt;
			if(trycnt > m_di.uRETRY){
				::wsprintf(szMsg, TEXT("[%c]リトライ回数が%u回を超えたため処理を中断します。"), m_di.cDRVLTR, m_di.uRETRY);
				DM_WriteLog(szMsg);
				break;
			}else if(trycnt == m_di.uRETRY){
				::wsprintf(szMsg, TEXT("[%c]%u回目失敗。"), m_di.cDRVLTR, trycnt);
				DM_WriteLog(szMsg);
			}else{
				::wsprintf(szMsg, TEXT("[%c]%u回目失敗 %u秒待機します。"), m_di.cDRVLTR, trycnt, m_di.uRTWAIT);
				DM_WriteLog(szMsg);
				::Sleep(m_di.uRTWAIT*1000);
			}
		}
	}

	m_sync.Lock();
	if(state){
		++m_err;
		SetResult(2);
		if(s_errmsg.length()){
			s_errmsg += TEXT("\n");
			s_errmsg += m_di.cDRVLTR;
			s_errmsg += TEXT("ドライブ");
		}else{
			s_errmsg = TEXT("次のドライブはマウントが出来ませんでした。\n");
			s_errmsg += m_di.cDRVLTR;
			s_errmsg += TEXT("ドライブ");
		}
		if(state > 1){
			::wsprintf(szMsg, TEXT("[%c]ドライブのマウントが不可能と判断したため処理を中断しました。"), m_di.cDRVLTR);
			DM_WriteLog(szMsg);
		}
	}else{
		::wsprintf(szMsg, TEXT("[%c]ドライブが正常にマウントされました。"), m_di.cDRVLTR);
		DM_WriteLog(szMsg);
		// 接続成功時にコマンド指定してあればコマンド実行
		if(m_di.uEXEC==BST_CHECKED && m_di.szEXECPATH[0]){
			if(!DrvMount::Exec(m_di.szEXECPATH)){
				state = 3;
				SetResult(3);
				::wsprintf(szMsg, TEXT("[%c]ドライブに設定されているコマンドの実行に失敗しました。"), m_di.cDRVLTR);
				DM_WriteLog(szMsg);
				if(s_execerr.length()){
					s_execerr += TEXT("\n");
					s_execerr += m_di.cDRVLTR;
					s_execerr += TEXT(": \"");
					s_execerr += m_di.szEXECPATH;
					s_execerr += TEXT("\"");
				}else{
					s_execerr = TEXT("次のドライブに設定されているコマンドが失敗しました。\n");
					s_execerr += m_di.cDRVLTR;
					s_execerr += TEXT(": \"");
					s_execerr += m_di.szEXECPATH;
					s_execerr += TEXT("\"");
				}
			}else{
				::wsprintf(szMsg, TEXT("[%c]ドライブに設定されたコマンドが正常に実行されました。"), m_di.cDRVLTR);
				DM_WriteLog(szMsg);
			}
		}
	}
	--m_count;
	m_sync.Unlock();
}

void DrvConnect::ResetErrorCount()
{
	m_sync.Lock();
	s_errmsg = TEXT("");
	s_execerr = TEXT("");
	m_err = 0;
	m_sync.Unlock();
}

LPCTSTR DrvConnect::GetErrorMessages()
{
	return s_errmsg.c_str();
}

LPCTSTR DrvConnect::GetExecErrMessages()
{
	return s_execerr.c_str();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool DrvMount::GetLogFileName(HINSTANCE hInst, LPTSTR lpLogFile, int size)
{
	TCHAR szUser[UNLEN+1];
	DWORD unlen = sizeof(szUser)/sizeof(TCHAR);
	TCHAR szModulePath[DM_MAXPATH+1];
	LPTSTR lpChar;

	if(!::GetUserName(szUser, &unlen)) szUser[0] = TEXT('\0');
	if(!::GetModuleFileName(hInst, szModulePath, sizeof(szModulePath)/sizeof(TCHAR))) return false;
	szModulePath[sizeof(szModulePath)/sizeof(TCHAR)-1] = TEXT('\0');
	for(lpChar=szModulePath+::lstrlen(szModulePath); *lpChar != TEXT('\\') && lpChar>szModulePath; lpChar=::CharPrev(szModulePath,lpChar));
	*lpChar = TEXT('\0');
	tstring path(szModulePath);
	path += TEXT("\\");
	path += szUser;
	path += DM_LOGFILE;
	::lstrcpyn(lpLogFile, path.c_str(), size);

	return (path.length() < static_cast<unsigned>(size));
}

bool DrvMount::GetIniFileName(HINSTANCE hInst, LPTSTR lpIniFile, int size)
{
	TCHAR szUser[UNLEN+1];
	DWORD unlen = sizeof(szUser)/sizeof(TCHAR);
	TCHAR szModulePath[DM_MAXPATH+1];
	LPTSTR lpChar;

	if(!::GetUserName(szUser, &unlen)) szUser[0] = TEXT('\0');
	if(!::GetModuleFileName(hInst, szModulePath, sizeof(szModulePath)/sizeof(TCHAR))) return false;
	szModulePath[sizeof(szModulePath)/sizeof(TCHAR)-1] = TEXT('\0');
	for(lpChar=szModulePath+::lstrlen(szModulePath); *lpChar != TEXT('\\') && lpChar>szModulePath; lpChar=::CharPrev(szModulePath,lpChar));
	*lpChar = TEXT('\0');
	tstring path(szModulePath);
	path += TEXT("\\");
	path += szUser;
	path += DM_INIFILE;
	::lstrcpyn(lpIniFile, path.c_str(), size);

	return (path.length() < static_cast<unsigned>(size));
}

void DrvMount::ShufflePassword(LPTSTR lpPassword, int pwlen, LPCTSTR lpSalt, int saltlen)
{
	uint8 md5v[8*16];
	md5_context md5c;

	for(int i=0; i<8; ++i){
		md5_starts(&md5c);
		md5_update(&md5c, reinterpret_cast<uint8*>(DM_APPSECTION), ::lstrlen(DM_APPSECTION)*sizeof(TCHAR));
		md5_update(&md5c, reinterpret_cast<uint8*>("@"), 1);
		md5_update(&md5c, reinterpret_cast<const uint8*>(lpSalt), saltlen*sizeof(TCHAR));
		md5_update(&md5c, reinterpret_cast<uint8*>(&i), sizeof(int));
		if(i) md5_update(&md5c, &md5v[i*16-16],16);
		md5_finish(&md5c, &md5v[i*16]);
	}

	for(int i=0; i<pwlen; ++i){
		if(i>(8*16)) break;
		lpPassword[i] ^= static_cast<TCHAR>(md5v[i]);
	}
}

bool DrvMount::SaveCommonInfo(LPCTSTR lpIniFile, const DMCOMMON& dc)
{
	bool result = true;
	TCHAR szNum[32];

	::wsprintf(szNum, TEXT("%u"), dc.uERRNOTIFY);
	if(!::WritePrivateProfileString(DM_APPSECTION, DM_IK_ERRNOTIFY, szNum, lpIniFile)){
		if(::GetLastError()) result = false;
	}

	::wsprintf(szNum, TEXT("%u"), dc.uALLUSSC);
	if(!::WritePrivateProfileString(DM_APPSECTION, DM_IK_ALLUSSC, szNum, lpIniFile)){
		if(::GetLastError()) result = false;
	}

	::wsprintf(szNum, TEXT("%u"), dc.uCREFWAIT);
	if(!::WritePrivateProfileString(DM_APPSECTION, DM_IK_CREFWAIT, szNum, lpIniFile)){
		if(::GetLastError()) result = false;
	}

	return result;
}

bool DrvMount::LoadCommonInfo(LPCTSTR lpIniFile, DMCOMMON& dc)
{
	bool result = true;
	dc.uERRNOTIFY = ::GetPrivateProfileInt(DM_APPSECTION, DM_IK_ERRNOTIFY, BST_UNCHECKED, lpIniFile);
	dc.uALLUSSC = ::GetPrivateProfileInt(DM_APPSECTION, DM_IK_ALLUSSC, BST_UNCHECKED, lpIniFile);
	dc.uCREFWAIT = ::GetPrivateProfileInt(DM_APPSECTION, DM_IK_CREFWAIT, DM_DEF_CREFWAIT, lpIniFile);
	if(dc.uCREFWAIT > DM_MAX_CREFWAIT) dc.uCREFWAIT = DM_MAX_CREFWAIT;
	else if(dc.uCREFWAIT < 1) dc.uCREFWAIT = 1;
	return result;
}

bool DrvMount::SaveDriveInfo(LPCTSTR lpIniFile, const DMINFO& di)
{
	bool result=true;
	TCHAR szNum[32];
	TCHAR szDrv[8];
	TCHAR szPwd[sizeof(di.szPWD)/sizeof(TCHAR)];
	TCHAR szSalt[sizeof(di.szUSER)/sizeof(TCHAR)+sizeof(di.szPATH)/sizeof(TCHAR)+16];
	char* pBase64;
	LPTSTR ptBase64;

	if(di.szPWD[0]){
		::wsprintf(szSalt, TEXT("%s@%s:%c"), di.szUSER, di.szPATH, di.cDRVLTR);
		::CopyMemory(szPwd, di.szPWD, sizeof(szPwd));
		ShufflePassword(szPwd, sizeof(szPwd)/sizeof(TCHAR),szSalt, ::lstrlen(szSalt));
		pBase64 = Base64Encode(reinterpret_cast<const char*>(szPwd), sizeof(szPwd));
		ptBase64 = new TCHAR[::lstrlenA(pBase64)+1];
		for(char* p = pBase64; ; ++p){
			ptBase64[p-pBase64] = *p;
			if(!*p) break;
		}
	}else{
		pBase64 = NULL;
		ptBase64 = NULL;
	}

	::wsprintf(szDrv, TEXT("%c"), di.cDRVLTR);

	if(!::WritePrivateProfileString(szDrv, DM_IKS_PATH, di.szPATH, lpIniFile)){
		if(::GetLastError()) result = false;
	}
	if(!::WritePrivateProfileString(szDrv, DM_IKS_USER, di.szUSER, lpIniFile)){
		if(::GetLastError()) result = false;
	}
	if(!::WritePrivateProfileString(szDrv, DM_IKS_PWD, ptBase64==NULL?TEXT(""):ptBase64, lpIniFile)){
		if(::GetLastError()) result = false;
	}
	::wsprintf(szNum, TEXT("%u"), di.uRETRY);
	if(!::WritePrivateProfileString(szDrv, DM_IKS_RETRY, szNum, lpIniFile)){
		if(::GetLastError()) result = false;
	}
	::wsprintf(szNum, TEXT("%u"), di.uRTWAIT);
	if(!::WritePrivateProfileString(szDrv, DM_IKS_RTWAIT, szNum, lpIniFile)){
		if(::GetLastError()) result = false;
	}
	::wsprintf(szNum, TEXT("%u"), di.uSCON);
	if(!::WritePrivateProfileString(szDrv, DM_IKS_SCON, szNum, lpIniFile)){
		if(::GetLastError()) result = false;
	}
	::wsprintf(szNum, TEXT("%u"), di.uFORCE);
	if(!::WritePrivateProfileString(szDrv, DM_IKS_FORCE, szNum, lpIniFile)){
		if(::GetLastError()) result = false;
	}
	::wsprintf(szNum, TEXT("%u"), di.uEXEC);
	if(!::WritePrivateProfileString(szDrv, DM_IKS_EXEC, szNum, lpIniFile)){
		if(::GetLastError()) result = false;
	}
	if(!::WritePrivateProfileString(szDrv, DM_IKS_EXECPATH, di.szEXECPATH, lpIniFile)){
		if(::GetLastError()) result = false;
	}

	delete [] ptBase64;
	Base64Free(pBase64);

	return result;
}

bool DrvMount::LoadDriveInfo(LPCTSTR lpIniFile, DMINFO& di)
{
	bool result = true;
	TCHAR szDrv[8];
	TCHAR szBase64Pwd[sizeof(di.szPWD)*2];

	::wsprintf(szDrv, TEXT("%c"), di.cDRVLTR);

	::GetPrivateProfileString(szDrv, DM_IKS_PATH, TEXT(""), di.szPATH, sizeof(di.szPATH)/sizeof(TCHAR), lpIniFile);
	di.szPATH[sizeof(di.szPATH)/sizeof(TCHAR)-1] = TEXT('\0');
	::GetPrivateProfileString(szDrv, DM_IKS_USER, TEXT(""), di.szUSER, sizeof(di.szUSER)/sizeof(TCHAR), lpIniFile);
	di.szUSER[sizeof(di.szUSER)/sizeof(TCHAR)-1] = TEXT('\0');
	::GetPrivateProfileString(szDrv, DM_IKS_PWD, TEXT(""), szBase64Pwd, sizeof(szBase64Pwd)/sizeof(TCHAR), lpIniFile);
	szBase64Pwd[sizeof(szBase64Pwd)/sizeof(TCHAR)-1] = TEXT('\0');
	di.uRETRY = ::GetPrivateProfileInt(szDrv, DM_IKS_RETRY, DM_DEF_RETRY, lpIniFile);
	di.uRTWAIT = ::GetPrivateProfileInt(szDrv, DM_IKS_RTWAIT, DM_DEF_RTWAIT, lpIniFile);
	di.uSCON = ::GetPrivateProfileInt(szDrv, DM_IKS_SCON, BST_UNCHECKED, lpIniFile);
	di.uFORCE = ::GetPrivateProfileInt(szDrv, DM_IKS_FORCE, BST_UNCHECKED, lpIniFile);
	di.uEXEC = ::GetPrivateProfileInt(szDrv, DM_IKS_EXEC, BST_UNCHECKED, lpIniFile);
	::GetPrivateProfileString(szDrv, DM_IKS_EXECPATH, TEXT(""), di.szEXECPATH, sizeof(di.szEXECPATH)/sizeof(TCHAR), lpIniFile);
	di.szEXECPATH[sizeof(di.szEXECPATH)/sizeof(TCHAR)-1] = TEXT('\0');

	if(szBase64Pwd[0]){
		char base64v[sizeof(szBase64Pwd)/sizeof(TCHAR)];
		char *p = NULL;
		int size;
		TCHAR szSalt[sizeof(di.szUSER)/sizeof(TCHAR)+sizeof(di.szPATH)/sizeof(TCHAR)+16];
		for(int i=0; ; ++i){
			base64v[i] = static_cast<char>(szBase64Pwd[i]);
			if(!base64v[i]) break;
		}

		::ZeroMemory(di.szPWD, sizeof(di.szPWD)/sizeof(TCHAR));
		if((size=Base64Decode(base64v,&p))<0){
			p = NULL;
			result = false;
		}else{
			for(int i=0; i<(sizeof(di.szPWD)/sizeof(TCHAR)); ++i){
				if(i>=size) break;
				::CopyMemory(di.szPWD+i, p+i*sizeof(TCHAR), sizeof(TCHAR));
			}
		}
		Base64Free(p);

		::wsprintf(szSalt, TEXT("%s@%s:%c"), di.szUSER, di.szPATH, di.cDRVLTR);
		ShufflePassword(di.szPWD, sizeof(di.szPWD)/sizeof(TCHAR), szSalt, ::lstrlen(szSalt));
	}else{
		::ZeroMemory(di.szPWD, sizeof(di.szPWD));
	}

	if(di.uRETRY < 0) di.uRETRY = DM_DEF_RETRY;
	if(di.uRTWAIT < 1) di.uRTWAIT = DM_DEF_RTWAIT;
	
	return result;
}

bool DrvMount::GetStartupShortcut(LPTSTR lpPath, int size, bool common)
{
	TCHAR szFileName[DM_MAXPATH+1];
	LPITEMIDLIST pidl;

	if(lpPath == NULL || size == 0) return false;

	::CoInitialize(NULL);
	if(::SHGetSpecialFolderLocation(NULL,common?CSIDL_COMMON_STARTUP:CSIDL_STARTUP,&pidl) != S_OK){
		szFileName[0] = TEXT('\0');
	}else{
		if(!::SHGetPathFromIDList(pidl, szFileName)){
			szFileName[0] = TEXT('\0');
		}
		::CoTaskMemFree(pidl);
	}
	::CoUninitialize();

	if(!szFileName[0]) return false;

	int len = ::lstrlen(szFileName);

	if((len+::lstrlen(DM_DMSTARTUPNAME)+1) >= (sizeof(szFileName)/sizeof(TCHAR))) return false;
	::lstrcat(szFileName, TEXT("\\"));
	::lstrcat(szFileName, DM_DMSTARTUPNAME);

	::lstrcpyn(lpPath, szFileName, size);
	if(size <= ::lstrlen(szFileName)) return false;

	return true;
}

bool DrvMount::IsStartupShortcut(bool common)
{
	WIN32_FIND_DATA w32fd;
	TCHAR szFileName[DM_MAXPATH+1];

	if(!GetStartupShortcut(szFileName, sizeof(szFileName)/sizeof(TCHAR),common)) return false;

	HANDLE hFind = ::FindFirstFile(szFileName, &w32fd);
	if(hFind != INVALID_HANDLE_VALUE){
		::FindClose(hFind);
		return true;
	}
	return false;
}

bool DrvMount::MakeStartupShortcut(MBase *pBase, bool common, LPCTSTR lpModuleDir)
{
	TCHAR szFileName[DM_MAXPATH+1];

	if(!GetStartupShortcut(szFileName, sizeof(szFileName)/sizeof(TCHAR),common)) return false;

	WIN32_FIND_DATA w32fd;
	HANDLE hFind = ::FindFirstFile(szFileName, &w32fd);
	if(hFind == INVALID_HANDLE_VALUE){
		tstring tstr(lpModuleDir);
		tstr += TEXT("\\");
		tstr += DM_DMSTARTNAME;
		if(!pBase->CreateShortcut(szFileName,tstr.c_str(),lpModuleDir)) return false;
		return true;
	}else{
		::FindClose(hFind);
	}
	return false;
}

int DrvMount::ConnectDrive(const DMINFO& di)
{
	TCHAR szDrv[8];
	TCHAR szPath[sizeof(di.szPATH)/sizeof(TCHAR)];
	NETRESOURCE nrc;
	DWORD dwResult;

	::wsprintf(szDrv, TEXT("%c:"), di.cDRVLTR);
	::lstrcpy(szPath, di.szPATH);
	::ZeroMemory(&nrc, sizeof(nrc));
	nrc.dwType = RESOURCETYPE_DISK;
	nrc.lpLocalName = szDrv;
	nrc.lpRemoteName = szPath;
	nrc.lpProvider = NULL;

	dwResult = ::WNetAddConnection2(&nrc, di.szPWD, di.szUSER, 0);

	switch(dwResult){
		case NO_ERROR:
		return 0;

		case ERROR_ALREADY_ASSIGNED:
		case ERROR_ACCESS_DENIED:
		case ERROR_INVALID_PASSWORD:
		return 2;
	}

	return 1;
}

bool DrvMount::AllocateDrive(const DMINFO& di)
{
	DWORD dwDevices = ::GetLogicalDrives();
	if((dwDevices >> static_cast<DWORD>(di.cDRVLTR-TEXT('A')))&0x1) return false;
	TCHAR szDrv[8];
	::wsprintf(szDrv, TEXT("%c:"), di.cDRVLTR);
	return (::DefineDosDevice(0, szDrv, di.szPATH) != 0);
}

int DrvMount::DisconnectDrive(const DMINFO& di)
{
	TCHAR szDrv[8];
	// 切断する
	::wsprintf(szDrv, TEXT("%c:"), di.cDRVLTR);
	DWORD dwResult = ::WNetCancelConnection2(szDrv, 0, TRUE);
	switch(dwResult){
		case NO_ERROR:
		return 0;
		case ERROR_DEVICE_IN_USE:
		case ERROR_OPEN_FILES:
		return 1;
	}
	return 2;
}

bool DrvMount::ReleaseDrive(const DMINFO& di)
{
	TCHAR szDrv[8];
	::wsprintf(szDrv, TEXT("%c:"), di.cDRVLTR);
	return (::DefineDosDevice(DDD_REMOVE_DEFINITION|DDD_EXACT_MATCH_ON_REMOVE, szDrv, di.szPATH) != 0);
}

int DrvMount::Mount(const DMINFO& di)
{
	int result = 0;

	if(!di.szPATH[0]) return 2;
	if(di.uFORCE==BST_CHECKED) Unmount(di);
	if(di.szPATH[0]==TEXT('\\') && di.szPATH[1]==TEXT('\\')){
		// UNCパスならばネットワークドライブとして接続
		result = ConnectDrive(di);
	}else{
		// UNCパスでなければ仮想ドライブとして接続
		result = (AllocateDrive(di)?0:2);
	}

	return result;
}

int DrvMount::Unmount(const DMINFO& di)
{
	TCHAR szDrive[8];

	::wsprintf(szDrive, TEXT("%c:"), di.cDRVLTR);
	switch(::GetDriveType(szDrive)){
		case DRIVE_REMOTE:
		return DisconnectDrive(di);

		case DRIVE_FIXED:
		return (ReleaseDrive(di)?0:2);
	}
	return 2;
}

bool DrvMount::Exec(LPCTSTR lpCmd, HWND hWnd)
{
	MCSVParser<TCHAR> csv;
	csv.setquot(TEXT('"'));
	csv.setsep(TEXT(" "));
	csv.setrecord(lpCmd);

	tstring pg(TEXT("")), para(TEXT(""));
	for(int i=0; i<csv.getcolums(); ++i){
		if(*(csv[i]) == TEXT('\0')) continue;
		if(!pg.length()) pg = csv[i];
		else{
			if(para.length()) para += TEXT(" ");
			if(csv.getquot(i)){
				para += TEXT("\"");
				para += csv[i];
				para += TEXT("\"");
			}else para += csv[i];
		}
	}

	LPTSTR lpWorkDir = new TCHAR[pg.length()+1];
	::lstrcpy(lpWorkDir, pg.c_str());
	for(LPTSTR lpChar=lpWorkDir+::lstrlen(lpWorkDir); ; lpChar=::CharPrev(lpWorkDir,lpChar)){
		if(lpChar<=lpWorkDir){
			*lpChar = TEXT('\0');
			break;
		}else if(*lpChar == TEXT('\\') || *lpChar == TEXT('/')){
			*lpChar = TEXT('\0');
			break;
		}
	}

	INT_PTR result = reinterpret_cast<INT_PTR>(
			::ShellExecute(hWnd, TEXT("open"),
						pg.c_str(),para.c_str(),(*lpWorkDir)?lpWorkDir:NULL,SW_SHOWDEFAULT)
			);
	delete [] lpWorkDir;
	return (result > 32);
}
