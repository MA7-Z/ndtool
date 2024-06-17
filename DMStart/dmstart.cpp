#include "dmstart.h"
#include <tchar.h>
#include <string>
#include <vector>

#ifdef UNICODE
#define tstring wstring
#else
#define tstring string
#endif

using namespace std;


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nShowCmd)
{
	// 多重起動防止
	SECURITY_DESCRIPTOR sd;
	if(!::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION)){
		::MessageBox(NULL, TEXT("セキュリティ記述子の初期化に失敗！"),TEXT("DMStart"),MB_ICONERROR|MB_OK);
		return 2;
	}
	if(!::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE)){
		::MessageBox(NULL, TEXT("DACLの設定に失敗！"),TEXT("DMStart"),MB_ICONERROR|MB_OK);
		return 2;
	}
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE;
	HANDLE hMutex = ::CreateMutex(&sa, FALSE, DM_START_MUTEX);
	if(hMutex == NULL){
		::MessageBox(NULL, TEXT("Mutexオブジェクトの取得に失敗！"),TEXT("DMStart"),MB_ICONERROR|MB_OK);
		return 2;
	}else if(::GetLastError() == ERROR_ALREADY_EXISTS){
		::MessageBox(NULL, TEXT("既に起動しています！"),TEXT("DMStart"),MB_ICONERROR|MB_OK);
		::ReleaseMutex(hMutex);
		::CloseHandle(hMutex);
		return 2;
	}

	DM_OpenLog(hInstance);

	// ::MessageBox(NULL, TEXT("起動確認"), TEXT("Debug"), MB_OK);

	DM_WriteLog(TEXT("自動接続を開始します。"));
	int result = DM_AutoConnect(hInstance);
	DM_WriteLog(TEXT("自動接続を終了します。"));
	
	// ミューテックスオブジェクトを破棄！！
	if(hMutex != NULL){
		::ReleaseMutex(hMutex);
		::CloseHandle(hMutex);
	}

	DM_CloseLog();

	return result;
}

// 接続処理
int DM_AutoConnect(HINSTANCE hInst)
{
	TCHAR szMsg[1024];
	TCHAR szIniFile[MAX_PATH+1];
	if(!DrvMount::GetIniFileName(hInst, szIniFile, sizeof(szIniFile)/sizeof(TCHAR))){
		::MessageBox(NULL, TEXT("設定ファイルの取得に失敗"),TEXT("エラー"),MB_ICONERROR|MB_OK);
		DM_WriteLog(TEXT("設定ファイル名の取得に失敗しました。"));
		return 2;
	}

	DMCOMMON dc;
	if(!DrvMount::LoadCommonInfo(szIniFile,dc)){
		::MessageBox(NULL, TEXT("共通設定情報の読込に失敗"),TEXT("エラー"),MB_ICONERROR|MB_OK);
		DM_WriteLog(TEXT("共通設定情報の読込に失敗しました。"));
		return 2;
	}

	DMINFO di['Z'-'A'+1];
	for(int i=0; i<(sizeof(di)/sizeof(DMINFO)); ++i){
		::ZeroMemory(&di[i], sizeof(DMINFO));
		di[i].cDRVLTR = TEXT('A') + i;
		if(!DrvMount::LoadDriveInfo(szIniFile,di[i])){
			::MessageBox(NULL, TEXT("ドライブ情報の読込に失敗"),TEXT("エラー"),MB_ICONERROR|MB_OK);
			::wsprintf(szMsg, TEXT("ドライブ情報の読込に失敗:%c"), di[i].cDRVLTR);
			DM_WriteLog(szMsg);
			return 2;
		}
	}

	DrvConnect* dcon;
	vector<DrvConnect*> vDCon;
	DrvConnect::ResetErrorCount();
	vDCon.clear();
	for(int i=0; i<(sizeof(di)/sizeof(DMINFO)); ++i){
		if(di[i].uSCON==BST_CHECKED && di[i].szPATH[0]){
			dcon = new DrvConnect(di[i]);
			dcon->Start();
			vDCon.push_back(dcon);
		}
	}

	while(vDCon.size() && (DrvConnect::GetRunningCount() || !DrvConnect::IsExec())) ::Sleep(500);

	int result = 0;
	if(DrvConnect::GetErrorCount()){
		if(dc.uERRNOTIFY == BST_CHECKED) ::MessageBox(NULL, DrvConnect::GetErrorMessages(), TEXT("DrvMount"), MB_ICONERROR|MB_OK);
		result = 1;
	}
	LPCTSTR lpCmdErr = DrvConnect::GetExecErrMessages();
	if(*lpCmdErr){
		if(dc.uERRNOTIFY == BST_CHECKED) ::MessageBox(NULL, lpCmdErr, TEXT("DrvMount"), MB_ICONERROR|MB_OK);
		result = 1;
	}

	for(vector<DrvConnect*>::size_type i=0; i<vDCon.size(); ++i) delete vDCon[i];
	vDCon.clear();

	return result;
}
