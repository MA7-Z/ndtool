/////////////////////////////////////////////////////////////////////////////////////////
// mlib.cpp
/////////////////////////////////////////////////////////////////////////////////////////
// #define _WIN32_DCOM
#include "mlib.h"
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "version.lib")

// VC++6.0
#if _MSC_VER > 1200
#pragma warning(disable:4312)
#pragma warning(disable:4244)
#pragma warning(disable:4996)
#endif

#define MTK_S_PS_INSWND			TEXT("UNK_cb9dcf9e57555b590a67ec27c6e15b92b66dfee219c4ba70a862225bd0accdc1")

bool MBase::m_bCoInit = false;

BOOL MBase::OpenFileName(HWND hWnd, LPTSTR lpFileName, DWORD dwLen, LPCTSTR lpTitle,
	DWORD dwFlg, LPCTSTR lpFilter, LPCTSTR lpInitialDir,
	LPCTSTR lpDefExt)
{
	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = lpFilter;
	// ofn.lpstrFilter = TEXT("CSVファイル(*.csv)\0*.csv\0テキストファイル(*.txt)\0*.txt\0すべてのファイル(*.*)\0*.*\0\0");
	ofn.lpstrCustomFilter = NULL;					// フィルター
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = lpFileName;
	ofn.nMaxFile = dwLen;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = lpInitialDir;
	ofn.lpstrTitle = lpTitle;
	ofn.Flags = dwFlg;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = lpDefExt;
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	if (::GetOpenFileName(&ofn)) {
		lpFileName[dwLen - 1] = TEXT('\0');
		return TRUE;
	}
	return FALSE;
}

BOOL MBase::SaveFileName(HWND hWnd, LPTSTR lpFileName, DWORD dwLen, LPCTSTR lpTitle,
	DWORD dwFlg, LPCTSTR lpFilter, LPCTSTR lpInitialDir,
	LPCTSTR lpDefExt)
{
	OPENFILENAME ofn;
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = lpFilter;
	// ofn.lpstrFilter = TEXT("CSVファイル(*.csv)\0*.csv\0テキストファイル(*.txt)\0*.txt\0すべてのファイル(*.*)\0*.*\0\0");
	ofn.lpstrCustomFilter = NULL;					// フィルター
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = lpFileName;
	ofn.nMaxFile = dwLen;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrInitialDir = lpInitialDir;
	ofn.lpstrTitle = lpTitle;
	ofn.Flags = dwFlg;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = NULL;
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	if (::GetSaveFileName(&ofn)) {
		lpFileName[dwLen - 1] = TEXT('\0');
		return TRUE;
	}
	return FALSE;
}

bool MBase::SelectFolder(HWND hWnd, LPTSTR lpDir, int nLen,
	LPCTSTR lpTitle, UINT uFlags)
{
	BROWSEINFO bi;
	ITEMIDLIST* idl;
	// LPMALLOC lpMalloc;
	TCHAR szDir[MAX_PATH];
	bool ret;

	ret = false;

	// CoInitializeEx(NULL, COINIT_MULTITHREADED);
	CoInit();

	// IMallocインタフェースの取得
	// ::SHGetMalloc(&lpMalloc);

	bi.hwndOwner = hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szDir;
	bi.lpszTitle = lpTitle;
	bi.ulFlags = uFlags;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;

	idl = ::SHBrowseForFolder(&bi);
	if (idl != NULL) {
		if (!::SHGetPathFromIDList(idl, szDir)) szDir[0] = TEXT('\0');
		::CoTaskMemFree(idl);
		::lstrcpyn(lpDir, szDir, nLen);
		ret = true;
	}
	// ::CoUninitialize();

	return ret;
}

bool MBase::CreateShortcut(LPCTSTR lpShortcutName, LPCTSTR lpLinkPath, LPCTSTR lpWorkDir,
	LPCTSTR lpIconFile, int nIconIndex, LPCTSTR lpArguments,
	LPCTSTR lpDescription, int nShow, WORD wHotkey)
{
	bool bRet = true;
	IShellLink* pShellLink = NULL;
	IPersistFile* pPersistFile = NULL;
#ifndef UNICODE
	WCHAR wszPath[MAX_PATH + 1];
#endif

	CoInit();
	if (::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID*>(&pShellLink)) == S_OK) {
		// リンク先パス
		if (bRet && pShellLink->SetPath(lpLinkPath) != S_OK) bRet = false;

		// 作業フォルダ
		if (bRet && lpWorkDir != NULL) {
			if (pShellLink->SetWorkingDirectory(lpWorkDir) != S_OK) bRet = false;
		}

		// アイコンファイル名
		if (bRet && lpIconFile != NULL) {
			if (pShellLink->SetIconLocation(lpIconFile, nIconIndex) != S_OK) bRet = false;
		}

		// 引数
		if (bRet && lpArguments != NULL) {
			if (pShellLink->SetArguments(lpArguments) != S_OK) bRet = false;
		}

		// コメント
		if (bRet && lpDescription != NULL) {
			if (pShellLink->SetDescription(lpDescription) != S_OK) bRet = false;
		}

		// ウインドウの大きさ
		if (bRet && pShellLink->SetShowCmd(nShow) != S_OK) bRet = false;

		// ホットキー
		if (bRet && pShellLink->SetHotkey(wHotkey) != S_OK) bRet = false;

		// IPersistFileインタフェース
		if (bRet && pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<LPVOID*>(&pPersistFile)) != S_OK) bRet = false;

#ifndef UNICODE
		if (bRet && !::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpShortcutName, -1, wszPath, sizeof(wszPath) / sizeof(WCHAR))) bRet = false;
		if (bRet && pPersistFile->Save(wszPath, TRUE) != S_OK) bRet = false;
#else
		if (bRet && pPersistFile->Save(lpShortcutName, TRUE) != S_OK) bRet = false;
#endif
	}
	else {
		bRet = false;
	}

	if (pPersistFile != NULL) pPersistFile->Release();
	if (pShellLink != NULL) pShellLink->Release();
	// ::CoUninitialize();
	return bRet;
}


int MBase::MsgBox(HWND hWnd, UINT uType, LPCTSTR lpCaption, LPCTSTR lpMsg, ...)
{
	static		TCHAR szMsg[MTK_MSGBUFSIZE];
	va_list		list;

	va_start(list, lpMsg);
	_vsntprintf(szMsg, MTK_MSGBUFSIZE, lpMsg, list);
	va_end(list);

	szMsg[MTK_MSGBUFSIZE > 0 ? (MTK_MSGBUFSIZE - 1) : 0] = TEXT('\0');

	return ::MessageBox(hWnd, szMsg, lpCaption, uType);
}

int MBase::ErrBox(HWND hWnd, DWORD dwMsgID, LPCTSTR lpCaption, UINT uType)
{
	LPVOID	lpMsgBuf;
	int		result;

	::FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwMsgID,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	result = ::MessageBox(hWnd, (LPCTSTR)lpMsgBuf, lpCaption, uType);

	::LocalFree(lpMsgBuf);

	return result;
}

int MBase::GetErrMsg(DWORD dwMsgID, LPTSTR lpMsgBuffer, int nMsgLen)
{
	LPVOID	lpMsgBuf;
	int		len;

	::FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwMsgID,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	if (lpMsgBuffer == NULL) {
		len = ::lstrlen(reinterpret_cast<LPCTSTR>(lpMsgBuf));
	}
	else {
		::lstrcpyn(lpMsgBuffer, reinterpret_cast<LPCTSTR>(lpMsgBuf), nMsgLen);
		len = ::lstrlen(lpMsgBuffer);
	}

	::LocalFree(lpMsgBuf);

	return len;
}

HRESULT MBase::CoInit()
{
	if (!m_bCoInit) {
		m_bCoInit = true;
		return ::CoInitialize(NULL);
	}
	return S_FALSE;
}

void MBase::CoUninit()
{
	if (m_bCoInit) {
		::CoUninitialize();
		m_bCoInit = false;
	}
}

int MBase::ToWideChar(LPCSTR lpMulti, LPWSTR lpWide, int widelen, UINT CodePage)
{
	if (lpWide != NULL && widelen < 1) return 0;
	int result = ::MultiByteToWideChar(CodePage, 0, lpMulti, -1, lpWide, widelen);
	lpWide[widelen - 1] = L'\0';
	return result;
}

int MBase::ToMultiByte(LPCWSTR lpWide, LPSTR lpMulti, int buflen, UINT CodePage)
{
	if (lpMulti != NULL && buflen < 1) return 0;
	int result = ::WideCharToMultiByte(CodePage, 0, lpWide, -1, lpMulti, buflen, NULL, NULL);
	lpMulti[buflen - 1] = '\0';
	return result;
}

MWException::MWException(LPCTSTR lpMsg, int nLine, LPCTSTR lpFile)
{
	M_SetMsg(lpMsg, nLine, lpFile);
}

int MWException::M_SetMsg(LPCTSTR lpMsg, int nLine, LPCTSTR lpFile)
{
	int			ret;
	LPCTSTR		lpChar = NULL;

	if (lpFile != NULL) {
		for (lpChar = lpFile + ::lstrlen(lpFile); lpChar > lpFile; lpChar = ::CharPrev(lpFile, lpChar)) {
			if (*lpChar == TEXT('\\')) {
				lpChar = ::CharNext(lpChar);
				break;
			}
		}
	}

	if (nLine && lpChar != NULL) ret = _sntprintf(m_szMsg, MTK_MSGBUFSIZE, TEXT("%s(%d) : %s"), lpChar, nLine, lpMsg);
	else if (!nLine && lpChar == NULL) ret = _sntprintf(m_szMsg, MTK_MSGBUFSIZE, TEXT("%s : %s"), lpChar, lpMsg);
	else if (nLine) ret = _sntprintf(m_szMsg, MTK_MSGBUFSIZE, TEXT("(%d) : %s"), nLine, lpMsg);
	else if (lpChar != NULL) ret = _sntprintf(m_szMsg, MTK_MSGBUFSIZE, TEXT("%s : %s"), lpChar, lpMsg);
	else ret = _sntprintf(m_szMsg, MTK_MSGBUFSIZE, TEXT("%s"), lpMsg);

	m_szMsg[MTK_MSGBUFSIZE > 0 ? (MTK_MSGBUFSIZE - 1) : 0] = TEXT('\0');
	return ret;
}
/*
MWException* MWException::M_Set(LPCTSTR lpMsg, int nLine, LPCTSTR lpFile)
{
	M_SetMsg(lpMsg, nLine, lpFile);
	return this;
}
*/

MApp::MApp(HINSTANCE hInst, LPTSTR lpCmdLine, int nShowCmd) :
	m_hInst(hInst), m_lpCmdLine(lpCmdLine),
	m_nShowCmd(nShowCmd), m_hLog(INVALID_HANDLE_VALUE)
{
	TCHAR	szModuleFile[MAX_PATH * 2 + 1];
	LPTSTR	lpChar;
	INITCOMMONCONTROLSEX iccx;

	::InitializeCriticalSection(&m_csLog);

	iccx.dwSize = sizeof(iccx);
	// iccx.dwICC = ICC_WIN95_CLASSES;	// 0x7ffでほぼ全部
	iccx.dwICC = ICC_WIN95_CLASSES | ICC_DATE_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES | ICC_INTERNET_CLASSES | ICC_PAGESCROLLER_CLASS;
	if (!::InitCommonControlsEx(&iccx)) {
		::MessageBox(NULL, TEXT("InitCommonControlsEx"),
			TEXT("コモンコントロールの初期化に失敗しました！"),
			MB_ICONERROR | MB_OK);
	}

	m_szModuleFile[0] = m_szModuleDir[0] = TEXT('\0');
	::GetModuleFileName(m_hInst, szModuleFile, sizeof(szModuleFile) / sizeof(TCHAR) - 1);
	szModuleFile[sizeof(szModuleFile) / sizeof(TCHAR) - 1] = TEXT('\0');
	for (lpChar = szModuleFile + ::lstrlen(szModuleFile); lpChar > szModuleFile; lpChar = ::CharPrev(szModuleFile, lpChar)) {
		if (*lpChar == TEXT('\\')) {
			::lstrcpyn(m_szModuleFile, ::CharNext(lpChar), sizeof(m_szModuleFile) / sizeof(TCHAR));
			*lpChar = TEXT('\0');
			::lstrcpyn(m_szModuleDir, szModuleFile, sizeof(m_szModuleDir) / sizeof(TCHAR));
			break;
		}
	}
	CoInit();
}

MApp::~MApp()
{
	CoUninit();
	CloseLog();
	::DeleteCriticalSection(&m_csLog);
}

int MApp::Run()
{
	int nResult = 0;
	TCHAR szErrMsg[MTK_MSGBUFSIZE];
	try {
		nResult = Main();
	}
	catch (MWException* e) {
		PutLog(TEXT("<<例外>> %s"), e->what());
		::MessageBox(NULL, e->what(), TEXT("MWException"), MB_OK | MB_ICONERROR);
		nResult = 2;
	}
	catch (const MWException& e) {
		PutLog(TEXT("<<例外>> %s"), e.what());
		::MessageBox(NULL, e.what(), TEXT("MWException"), MB_OK | MB_ICONERROR);
		nResult = 2;
	}
	catch (LPCTSTR lpMsg) {
		PutLog(TEXT("<<例外>> %s"), lpMsg);
		::MessageBox(NULL, lpMsg, TEXT("例外発生"), MB_OK | MB_ICONERROR);
		nResult = 2;
	}
	catch (...) {
		GetErrMsg(::GetLastError(), szErrMsg, sizeof(szErrMsg) / sizeof(TCHAR));
		MsgBox(NULL, MB_ICONERROR | MB_OK, TEXT("例外発生"), TEXT("Error = %s"), szErrMsg);
		nResult = 2;
	}
	return nResult;
}

bool MApp::OpenLog(LPCTSTR lpLogFile, int nMode, int CodePage)
{
	m_logCodePage = CodePage;
	if (m_hLog != INVALID_HANDLE_VALUE) ::CloseHandle(m_hLog);
	m_hLog = ::CreateFile(lpLogFile,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		nMode == MTK_LOG_NEW ? CREATE_ALWAYS : OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (m_hLog == INVALID_HANDLE_VALUE) {
		MsgBox(NULL, MB_ICONERROR | MB_OK,
			TEXT("Error!"),
			TEXT("ログファイル名=%s\nログファイルのオープンに失敗！！\nログは記録されません。"),
			lpLogFile);
		return false;
	}
	else {
		if (nMode == MTK_LOG_ADD) {
			// 追記する
			LONG lHigh = 0;
			::SetFilePointer(m_hLog, 0, &lHigh, FILE_END);
		}
	}
	return true;
}

void MApp::CloseLog()
{
	::EnterCriticalSection(&m_csLog);
	if (m_hLog != INVALID_HANDLE_VALUE) ::CloseHandle(m_hLog);
	m_hLog = INVALID_HANDLE_VALUE;
	::LeaveCriticalSection(&m_csLog);
}

int MApp::PutLog(LPCTSTR lpString, ...)
{
	static TCHAR szMsg[MTK_MSGBUFSIZE];
	static char szWriteMsg[MTK_MSGBUFSIZE], szTime[64], szTruncMsg[128], szWriteModule[MAX_PATH + 1];
	va_list vl;
	int wc, ret;
	DWORD dwWrite;
	SYSTEMTIME st;
	bool bTrunc = false;

	LPTSTR lpChar;
	for (lpChar = m_szModuleFile + ::lstrlen(m_szModuleFile); lpChar > m_szModuleFile; lpChar = ::CharPrev(m_szModuleFile, lpChar)) {
		if (*lpChar == TEXT('\\')) {
			++lpChar;
			break;
		}
	}

#ifdef UNICODE
	if (!ToMultiByte(MTK_LOG_TCMSG, szTruncMsg, sizeof(szTruncMsg), m_logCodePage)) szTruncMsg[0] = '\0';
	if (!ToMultiByte(lpChar, szWriteModule, sizeof(szWriteModule), m_logCodePage)) szWriteModule[0] = '\0';
#else
	if (m_logCodePage != CP_ACP) {
		WCHAR szWide[MTK_MSGBUFSIZE];
		if (!ToWideChar(MTK_LOG_TCMSG, szWide, masizeof(szWide), CP_ACP)) szWide[0] = L'\0';
		if (!ToMultiByte(szWide, szTruncMsg, sizeof(szTruncMsg), m_logCodePage)) szTruncMsg[0] = '\0';
		if (!ToWideChar(lpChar, szWide, masizeof(szWide), CP_ACP)) szWide[0] = L'\0';
		if (!ToMultiByte(szWide, szWriteModule, sizeof(szWriteModule), m_logCodePage)) szWriteModule[0] = '\0';
	}
	else {
		::lstrcpynA(szTruncMsg, MTK_LOG_TCMSG, sizeof(szTruncMsg));
		::lstrcpynA(szWriteModule, lpChar, sizeof(szWriteModule));
	}
#endif
	::EnterCriticalSection(&m_csLog);

	va_start(vl, lpString);
	wc = _vsntprintf(szMsg, sizeof(szMsg) / sizeof(TCHAR), lpString, vl) * sizeof(TCHAR);
	va_end(vl);

	szMsg[MTK_MSGBUFSIZE > 0 ? (MTK_MSGBUFSIZE - 1) : 0] = TEXT('\0');
	ret = 0;
	::GetLocalTime(&st);
	::wsprintfA(szTime, "%04u/%02u/%02u %02u:%02u:%02u\t",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	if (m_hLog != INVALID_HANDLE_VALUE) {
		if (wc < 0) {
			wc = sizeof(szMsg) - sizeof(TCHAR);
			ret = 1;
			bTrunc = true;
		}

#ifdef UNICODE
		if (!ToMultiByte(szMsg, szWriteMsg, sizeof(szWriteMsg), m_logCodePage)) szWriteMsg[0] = '\0';
#else
		if (m_logCodePage != CP_ACP) {
			WCHAR szWide[MTK_MSGBUFSIZE];
			if (!ToWideChar(szMsg, szWide, sizeof(szWide) / sizeof(WCHAR), CP_ACP)) szWide[0] = L'\0';
			if (!ToMultiByte(szWide, szWriteMsg, sizeof(szWriteMsg), m_logCodePage)) szWriteMsg[0] = '\0';
		}
		else {
			::lstrcpynA(szWriteMsg, szMsg, sizeof(szWriteMsg));
		}
#endif
		if (!::WriteFile(m_hLog, szTime, ::lstrlenA(szTime), &dwWrite, NULL)) ret = 2;
		if (!::WriteFile(m_hLog, szWriteModule, ::lstrlenA(szWriteModule), &dwWrite, NULL)) ret = 2;
		if (!::WriteFile(m_hLog, "\t", 1, &dwWrite, NULL)) ret = 2;
		if (!::WriteFile(m_hLog, szWriteMsg, ::lstrlenA(szWriteMsg), &dwWrite, NULL)) ret = 2;
		if (bTrunc && !::WriteFile(m_hLog, szTruncMsg, ::lstrlenA(szTruncMsg), &dwWrite, NULL)) ret = 2;
		if (!::WriteFile(m_hLog, "\r\n", 2, &dwWrite, NULL)) ret = 2;
		::FlushFileBuffers(m_hLog);
	}
	else {
		ret = 3;
	}
	::LeaveCriticalSection(&m_csLog);
	return ret;
}

void MApp::GetAppVersion(PWORD pV1, PWORD pV2, PWORD pV3, PWORD pV4)
{
	DWORD dwTemp;
	UINT uSize;
	VS_FIXEDFILEINFO vfi;
	TCHAR szFile[MAX_PATH + 1];
	LPTSTR lpBuf;
	LPVOID lpvBuf;

	::GetModuleFileName(m_hInst, szFile, sizeof(szFile) / sizeof(TCHAR));
	szFile[sizeof(szFile) / sizeof(TCHAR) - 1] = TEXT('\0');

	uSize = ::GetFileVersionInfoSize(szFile, &dwTemp) + 1;
	try { lpBuf = reinterpret_cast<LPTSTR>(new BYTE[uSize]); }
	catch (...) { lpBuf = NULL; }

	if (lpBuf == NULL) {
		if (pV1 != NULL) *pV1 = 0;
		if (pV2 != NULL) *pV2 = 0;
		if (pV3 != NULL) *pV3 = 0;
		if (pV4 != NULL) *pV4 = 0;
		return;
	}

	if (::GetFileVersionInfo(szFile, 0 /* 無視される */, uSize, lpBuf)) {
		::VerQueryValue(lpBuf, TEXT("\\"), &lpvBuf, &uSize);
		CopyMemory(&vfi, lpvBuf, sizeof(vfi));
		if (pV1 != NULL) *pV1 = static_cast<WORD>(vfi.dwFileVersionMS >> 16);
		if (pV2 != NULL) *pV2 = static_cast<WORD>(vfi.dwFileVersionMS & 0x0000FFFF);
		if (pV3 != NULL) *pV3 = static_cast<WORD>(vfi.dwFileVersionLS >> 16);
		if (pV4 != NULL) *pV4 = static_cast<WORD>(vfi.dwFileVersionLS & 0x0000FFFF);
	}
	else {
		if (pV1 != NULL) *pV1 = 0;
		if (pV2 != NULL) *pV2 = 0;
		if (pV3 != NULL) *pV3 = 0;
		if (pV4 != NULL) *pV4 = 0;
	}
	delete[] lpBuf;
}

bool MApp::GetAppVersionInfo(LPCTSTR lpModule, MTKAPPINFO& mai, WORD wLanguage, WORD wCodePage)
{
	DWORD dwTemp;
	UINT uSize;
	LPTSTR lpBuf;
	LPVOID lpvBuf;
	UINT  vlen;
	bool bResult = true;

	uSize = ::GetFileVersionInfoSize(lpModule, &dwTemp) + 1;
	if (!uSize) return false;
	lpBuf = reinterpret_cast<LPTSTR>(new BYTE[uSize]);

	if (!::GetFileVersionInfo(lpModule, 0, uSize, lpBuf)) bResult = false;
	::ZeroMemory(&mai, sizeof(MTKAPPINFO));

	TCHAR szSubBlock[256];

	// コメント
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\Comments"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szComments, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szComments) < vlen ? masizeof(mai.szComments) : vlen);
	}

	// 内部名
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\InternalName"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szInternalName, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szInternalName) < vlen ? masizeof(mai.szInternalName) : vlen);
	}

	// 製品名
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\ProductName"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szProductName, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szProductName) < vlen ? masizeof(mai.szProductName) : vlen);
	}

	// 会社名
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\CompanyName"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szCompanyName, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szCompanyName) < vlen ? masizeof(mai.szCompanyName) : vlen);
	}

	// 著作権
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\LegalCopyright"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szLegalCopyright, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szLegalCopyright) < vlen ? masizeof(mai.szLegalCopyright) : vlen);
	}

	// 製品バージョン
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\ProductVersion"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szProductVersion, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szProductVersion) < vlen ? masizeof(mai.szProductVersion) : vlen);
	}

	// 説明
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szFileDescription, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szFileDescription) < vlen ? masizeof(mai.szFileDescription) : vlen);
	}

	// 商標
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\LegalTrademarks"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szLegalTrademarks, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szLegalTrademarks) < vlen ? masizeof(mai.szLegalTrademarks) : vlen);
	}

	// プライベートビルド番号
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\PrivateBuild"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szPrivateBuild, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szPrivateBuild) < vlen ? masizeof(mai.szPrivateBuild) : vlen);
	}

	// ファイルバージョン
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\FileVersion"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szFileVersion, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szFileVersion) < vlen ? masizeof(mai.szFileVersion) : vlen);
	}

	// 正式ファイル名
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\OriginalFilename"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szOriginalFilename, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szOriginalFilename) < vlen ? masizeof(mai.szOriginalFilename) : vlen);
	}

	// スペシャルビルド情報
	::wsprintf(szSubBlock, TEXT("\\StringFileInfo\\%04x%04x\\SpecialBuild"), wLanguage, wCodePage);
	if (bResult && ::VerQueryValue(lpBuf, szSubBlock, &lpvBuf, &vlen)) {
		::lstrcpyn(mai.szSpecialBuild, reinterpret_cast<LPCTSTR>(lpvBuf),
			masizeof(mai.szSpecialBuild) < vlen ? masizeof(mai.szSpecialBuild) : vlen);
	}

	delete[] lpBuf;

	return bResult;
}

int MApp::MessageLoop(HWND hWnd)
{
	// メッセージループ
	BOOL bRet;
	MSG msg;
	while ((bRet = ::GetMessage(&msg, NULL, 0, 0))) {
		if (bRet == -1) {
			MsgBox(NULL,
				MB_ICONERROR | MB_OK,
				TEXT("GetMessage"),
				TEXT("Message Loop Error!!"));
			break;
		}
		if (!::IsDialogMessage(hWnd == NULL ? msg.hwnd : hWnd, &msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		// ::TranslateMessage(&msg);
		// ::DispatchMessage(&msg);
	}
	return msg.wParam;
}

MWinBase::~MWinBase()
{
	if (m_hWnd != NULL) ::RemoveProp(m_hWnd, MTK_S_PS_INSWND);
}


HWND MWinBase::SetWnd(HWND hWnd)
{
	HWND hPreWnd = m_hWnd;
	m_hWnd = hWnd;
	return hPreWnd;
}

LRESULT CALLBACK MWinBase::CallWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static MWin* pw;
	static LRESULT ret;
	if (msg == WM_NCCREATE) {
		pw = reinterpret_cast<MWin*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);
		if (pw != NULL) {
			pw->m_hWnd = hWnd;
			if (!::SetProp(hWnd, MTK_S_PS_INSWND, pw)) MWException::create(TEXT("window create error!!"));
		}
	}
	else if (msg == WM_INITDIALOG) {
		pw = reinterpret_cast<MWin*>(lParam);
		if (pw->m_hWnd == NULL) {
			pw->m_hWnd = hWnd;
			if (!::SetProp(hWnd, MTK_S_PS_INSWND, pw)) MWException::create(TEXT("dialog create error!!"));
		}
	}
	pw = reinterpret_cast<MWin*>(::GetProp(hWnd, MTK_S_PS_INSWND));
	if (pw != NULL) {
		ret = pw->WndProc(msg, wParam, lParam);
		if (msg == WM_DESTROY) ::RemoveProp(pw->m_hWnd, MTK_S_PS_INSWND);
		return ret;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

INT_PTR CALLBACK MWinBase::CallDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static MDlg* pd;
	static INT_PTR ret;
	if (msg == WM_INITDIALOG) {
		pd = reinterpret_cast<MDlg*>(lParam);
		pd->m_hWnd = hWnd;
		if (!::SetProp(hWnd, MTK_S_PS_INSWND, pd)) MWException::create(TEXT("dialog create error!!"));
	}
	pd = reinterpret_cast<MDlg*>(::GetProp(hWnd, MTK_S_PS_INSWND));
	if (pd != NULL) {
		ret = pd->DlgProc(msg, wParam, lParam);
		if (msg == WM_DESTROY) ::RemoveProp(pd->m_hWnd, MTK_S_PS_INSWND);
		return ret;
	}
	return FALSE;
}

LRESULT CALLBACK MWinBase::CallSubProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static MSubWin* ps;
	static LRESULT ret;
	ps = reinterpret_cast<MSubWin*>(::GetProp(hWnd, MTK_S_PS_INSWND));
	if (ps != NULL) {
		return ps->WndProc(msg, wParam, lParam);
	}
	return 0;
}

LRESULT MWin::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(m_hWnd, msg, wParam, lParam);
}


MWin::MWin(HWND hWnd) : MWinBase(hWnd)
{
}

MWin::~MWin()
{
}

bool MWin::Create(LPCTSTR lpTitle, WNDCLASSEX* pwcx, HINSTANCE hInst, HWND hOwn)
{
	WNDCLASSEX wcx, * px;

	if (pwcx == NULL) {
		ZeroMemory(&wcx, sizeof(wcx));
		wcx.cbSize = sizeof(wcx);
		wcx.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
		wcx.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wcx.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
		wcx.hIconSm = ::LoadIcon(NULL, IDI_WINLOGO);
		wcx.hInstance = hInst;
		wcx.lpfnWndProc = MWinBase::CallWndProc;
		wcx.lpszClassName = TEXT("mlib_Window_ClassName");
		wcx.lpszMenuName = NULL;
		px = &wcx;
	}
	else {
		px = pwcx;
	}
	// ウィンドウクラス登録
	if (!::RegisterClassEx(px)) {
		ErrBox(NULL, ::GetLastError());
		return false;
	}

	// ウィンドウ生成
	if (::CreateWindowEx(0,
		px->lpszClassName,
		lpTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		hOwn,
		NULL,
		px->hInstance,
		this) == NULL)
	{
		ErrBox(NULL, ::GetLastError());
		return false;
	}

	return true;
}

MWinDlg::MWinDlg(HWND hWnd) : MWin(hWnd)
{
}

MWinDlg::~MWinDlg()
{
}

bool MWinDlg::Create(LPCTSTR lpTemplate, WNDCLASSEX* pwcx, HINSTANCE hInst, HWND hOwn)
{
	WNDCLASSEX wcx, * px;

	if (pwcx == NULL) {
		ZeroMemory(&wcx, sizeof(wcx));
		wcx.cbSize = sizeof(wcx);
		wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);;
		wcx.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wcx.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
		wcx.hIconSm = ::LoadIcon(NULL, IDI_WINLOGO);
		wcx.hInstance = hInst;
		wcx.lpfnWndProc = MWinBase::CallWndProc;
		wcx.lpszClassName = TEXT("mlib_DlgWindow_ClassName");
		wcx.lpszMenuName = NULL;
		wcx.cbWndExtra = DLGWINDOWEXTRA;
		px = &wcx;
	}
	else {
		px = pwcx;
	}
	// ウィンドウクラス登録
	if (!::RegisterClassEx(px)) {
		ErrBox(NULL, ::GetLastError());
		return false;
	}

	// ウィンドウ生成
	if (::CreateDialogParam(hInst,
		lpTemplate,
		hOwn,
		reinterpret_cast<DLGPROC>(MWinBase::CallWndProc),
		reinterpret_cast<LPARAM>(this)) == NULL)
	{
		ErrBox(NULL, ::GetLastError());
		return false;
	}

	return true;
}

INT_PTR MDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
}

MDlg::MDlg(HWND hWnd) : MWinBase(hWnd)
{
}

int MDlg::Create(HINSTANCE hInst, LPCTSTR lpTemplate, HWND hParent)
{
	return ::DialogBoxParam(hInst, lpTemplate, hParent,
		reinterpret_cast<DLGPROC>(MWinBase::CallDlgProc),
		reinterpret_cast<LPARAM>(this));
}

LRESULT MDlg::SendDlgItemMessage(int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	return ::SendDlgItemMessage(m_hWnd, nIDDlgItem, Msg, wParam, lParam);
}

MDlg::~MDlg()
{
}

LRESULT MSubWin::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	// オーバーライド用
	return ::CallWindowProc(m_pfnOrgProc, m_hWnd, msg, wParam, lParam);
}

MSubWin::MSubWin(HWND hWnd) : MWinBase(hWnd)
{
	if (m_hWnd != NULL) {
		// VC++6.0
#if _MSC_VER > 1200
		m_pfnOrgProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(m_hWnd, GWLP_WNDPROC));
#else
		m_pfnOrgProc = reinterpret_cast<WNDPROC>(::GetWindowLong(m_hWnd, GWL_WNDPROC));
#endif
		if (m_pfnOrgProc == 0) MWException::create(TEXT("ウィンドウプロシージャの取得に失敗"));
		if (!::SetProp(m_hWnd, MTK_S_PS_INSWND, this)) MWException::create(TEXT("subclass error!!"));
	}
	else {
		m_pfnOrgProc = NULL;
	}
}

MSubWin::~MSubWin()
{
	ReturnWindowProc();
	if (m_hWnd != NULL) ::RemoveProp(m_hWnd, MTK_S_PS_INSWND);
	m_pfnOrgProc = NULL;
	m_hWnd = NULL;
}

HWND MSubWin::SetWnd(HWND hWnd)
{
	WNDPROC pfnProc;

	if (hWnd != NULL) {
		// VC++6.0
#if _MSC_VER > 1200
		pfnProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hWnd, GWLP_WNDPROC));
#else
		pfnProc = reinterpret_cast<WNDPROC>(::GetWindowLong(hWnd, GWL_WNDPROC));
#endif
		if (pfnProc == 0) MWException::create(TEXT("ウィンドウプロシージャの取得に失敗"));
	}
	else {
		pfnProc = NULL;
	}
	if (m_hWnd != NULL) {
		::RemoveProp(m_hWnd, MTK_S_PS_INSWND);
		ReturnWindowProc();
	}
	m_pfnOrgProc = pfnProc;
	if (hWnd != NULL) {
		if (!::SetProp(hWnd, MTK_S_PS_INSWND, this)) MWException::create(TEXT("subclass error!!"));
	}
	return MWinBase::SetWnd(hWnd);
}

WNDPROC MSubWin::ChangeWindowProc(WNDPROC pfnWndProc)
{
	return reinterpret_cast<WNDPROC>(
		// VC++6.0
#if _MSC_VER > 1200
		::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pfnWndProc))
#else
		::SetWindowLong(m_hWnd, GWL_WNDPROC, reinterpret_cast<LONG>(pfnWndProc))
#endif
		);
}

WNDPROC MSubWin::ReturnWindowProc()
{
	if (m_hWnd == NULL) return NULL;

	return reinterpret_cast<WNDPROC>(
		// VC++6.0
#if _MSC_VER > 1200
		::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_pfnOrgProc))
#else
		::SetWindowLong(m_hWnd, GWL_WNDPROC, reinterpret_cast<LONG>(m_pfnOrgProc))
#endif
		);
}
