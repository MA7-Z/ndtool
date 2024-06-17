#ifndef __INC_MLIB_H__
#define __INC_MLIB_H__
/////////////////////////////////////////////////////////////////////////////////////////
// mlib.h
/////////////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <shlobj.h>

#define MLIB_VERSION			0x0900

#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE		0x0040
#endif

#define MTK_MSGBUFSIZE			4096
#define MTK_LOG_NEW				1
#define MTK_LOG_ADD				2
#define MTK_LOG_TCMSG			TEXT("...(ry")

#define MTKMB(msg)				::MessageBox(NULL,(msg),TEXT("MTKMB"),MB_OK)
#define MTKEB(msg)				::MessageBox(NULL,(msg),TEXT("error!"),MB_ICONERROR|MB_OK)
#define masizeof(a)				(sizeof(a)/sizeof(a[0]))

typedef struct _MTKAPPINFO {
	TCHAR szComments[256];
	TCHAR szInternalName[256];
	TCHAR szProductName[256];
	TCHAR szCompanyName[256];
	TCHAR szLegalCopyright[256];
	TCHAR szProductVersion[64];
	TCHAR szFileDescription[256];
	TCHAR szLegalTrademarks[256];
	TCHAR szPrivateBuild[64];
	TCHAR szFileVersion[64];
	TCHAR szOriginalFilename[256];
	TCHAR szSpecialBuild[256];
} MTKAPPINFO, * PMTKAPPINFO;

class MBase {
	static bool				m_bCoInit;
public:
	MBase() {}
	virtual ~MBase() {}
	virtual BOOL OpenFileName(HWND hWnd, LPTSTR lpFileName, DWORD dwLen, LPCTSTR lpTitle = NULL,
		DWORD dwFlg = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		LPCTSTR lpFilter = TEXT("すべてのファイル(*.*)\0*.*\0\0"),
		LPCTSTR lpInitialDir = NULL, LPCTSTR lpDefExt = NULL);
	virtual BOOL SaveFileName(HWND hWnd, LPTSTR lpFileName, DWORD dwLen, LPCTSTR lpTitle = NULL,
		DWORD dwFlg = OFN_OVERWRITEPROMPT,
		LPCTSTR lpFilter = TEXT("すべてのファイル(*.*)\0*.*\0\0"),
		LPCTSTR lpInitialDir = NULL, LPCTSTR lpDefExt = NULL);
	virtual bool SelectFolder(HWND hWnd, LPTSTR lpDir, int nLen,
		LPCTSTR lpTitle = TEXT("フォルダの参照"),
		UINT uFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE);
	virtual bool CreateShortcut(LPCTSTR lpShortcutName, LPCTSTR lpLinkPath, LPCTSTR lpWorkDir = NULL,
		LPCTSTR lpIconFile = NULL, int nIconIndex = 0, LPCTSTR lpArguments = NULL,
		LPCTSTR lpDescription = NULL, int nShow = SW_SHOWNORMAL, WORD wHotkey = 0);
	static int MsgBox(HWND hWnd, UINT uType, LPCTSTR lpCaption, LPCTSTR lpMsg, ...);
	static int ErrBox(HWND hWnd, DWORD dwMsgID, LPCTSTR lpCaption = TEXT("Error!"), UINT uType = MB_ICONERROR | MB_OK);
	static int GetErrMsg(DWORD dwMsgID, LPTSTR lpMsgBuffer, int nMsgLen);
	static HRESULT CoInit();
	static void CoUninit();
	static int ToWideChar(LPCSTR lpMulti, LPWSTR lpWide, int widelen, UINT CodePage = CP_ACP);
	static int ToMultiByte(LPCWSTR lpWide, LPSTR lpMulti, int buflen, UINT CodePage = CP_ACP);
};

class MWException : public MBase {
protected:
	TCHAR			m_szMsg[MTK_MSGBUFSIZE + 1];			// メッセージ
	int M_SetMsg(LPCTSTR lpMsg, int nLine = 0, LPCTSTR lpFile = NULL);
public:
	MWException() { m_szMsg[0] = TEXT('\0'); }
	MWException(LPCTSTR lpMsg, int nLine = 0, LPCTSTR lpFile = NULL);
	~MWException() {}
	// 	int M_SetMsg(LPCTSTR lpMsg, int nLine=0, LPCTSTR lpFile=NULL);
	// 	LPCTSTR GetMsg()const{ return m_szMsg; }
	LPCTSTR what()const { return m_szMsg; }
	// 	virtual MWException* M_Set(LPCTSTR lpMsg, int nLine, LPCTSTR lpFile);
	// #define MSetMsg(msg)			M_SetMsg((msg), __LINE__, TEXT(__FILE__));
	// #define MSet(msg)				M_Set((msg), __LINE__, TEXT(__FILE__));
	static void create(LPCTSTR msg, int line = __LINE__, LPCTSTR lpFile = TEXT(__FILE__)) { throw MWException(msg, line, lpFile); }
};

// extern MWException				mexception;

// #define makeException(msg) throw MWException(msg, __LINE__, TEXT(__FILE__))

// アプリ
class MApp : public MBase {
	CRITICAL_SECTION m_csLog;
	HANDLE			m_hLog;
	int				m_logCodePage;
protected:
	HINSTANCE		m_hInst;
	LPTSTR			m_lpCmdLine;
	int				m_nShowCmd;
	TCHAR			m_szModuleDir[MAX_PATH + 1];
	TCHAR			m_szModuleFile[MAX_PATH + 1];
	virtual int Main() = 0;
	virtual HANDLE GetLogHandle()const { return m_hLog; }
public:
	MApp(HINSTANCE hInst, LPTSTR lpCmdLine, int nShowCmd);
	virtual ~MApp();
	virtual int Run();
	virtual bool OpenLog(LPCTSTR lpLogFile, int nMode = MTK_LOG_ADD, int CodePage = CP_ACP);
	virtual void CloseLog();
	virtual int PutLog(LPCTSTR lpString, ...);
	virtual void GetAppVersion(PWORD pV1 = NULL, PWORD pV2 = NULL, PWORD pV3 = NULL, PWORD pV4 = NULL);
	static bool GetAppVersionInfo(LPCTSTR lpModule, MTKAPPINFO& mai, WORD wLanguage = 0x411,
#ifdef UNICODE
		WORD wCodePage = 0x4B0);
#else
		WORD wCodePage = 0x4B0);
	// 		WORD wCodePage=0x3A4);
#endif
	virtual int MessageLoop(HWND hWnd = NULL);
};

class MWinBase : public MBase {
protected:
	HWND								m_hWnd;
	virtual HWND SetWnd(HWND hWnd);
public:
	MWinBase(HWND hWnd = NULL) : m_hWnd(hWnd) {}
	virtual ~MWinBase();
	virtual HWND GetWnd()const { return m_hWnd; }
	virtual LRESULT SendMessage(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::SendMessage(m_hWnd, msg, wParam, lParam);
	}
	virtual BOOL PostMessage(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ::PostMessage(m_hWnd, msg, wParam, lParam);
	}
	static LRESULT CALLBACK CallWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK CallDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK CallSubProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

class MWin : public MWinBase {
protected:
	virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
public:
	MWin(HWND hWnd = NULL);
	virtual ~MWin();
	virtual bool Create(LPCTSTR lpTitle, WNDCLASSEX* pwcx, HINSTANCE hInst, HWND hOwn = NULL);
	virtual BOOL Show(int nShowCmd)const { return ::ShowWindow(m_hWnd, nShowCmd); }
	virtual BOOL Update()const { return ::UpdateWindow(m_hWnd); }
	friend class MWinBase;
};

class MWinDlg : public MWin {
public:
	MWinDlg(HWND hWnd = NULL);
	virtual ~MWinDlg();
	virtual bool Create(LPCTSTR lpTemplate, WNDCLASSEX* pwcx, HINSTANCE hInst, HWND hOwn = NULL);
};

class MDlg : public MWinBase {
protected:
	virtual INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);
public:
	MDlg(HWND hWnd = NULL);
	virtual int Create(HINSTANCE hInst, LPCTSTR lpTemplate, HWND hParent = NULL);
	virtual LRESULT SendDlgItemMessage(int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
	virtual ~MDlg();
	friend class MWinBase;
};

class MSubWin : public MWinBase {
protected:
	WNDPROC								m_pfnOrgProc;
	virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
public:
	MSubWin(HWND hWnd = NULL);
	virtual ~MSubWin();
	virtual HWND SetWnd(HWND hWnd);
	virtual WNDPROC ChangeWindowProc(WNDPROC pfnWndProc = MWinBase::CallSubProc);
	virtual WNDPROC ReturnWindowProc();
	virtual WNDPROC GetOrgProc()const { return m_pfnOrgProc; }
	friend class MWinBase;
};

#endif	// __INC_MLIB_H__
