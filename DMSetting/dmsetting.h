#ifndef __DMSETTING_H__
#define __DMSETTING_H__
///////////////////////////////////////////////////////////////////////////
//
// dmsetting.h
// DMSetting ヘッダー
//
///////////////////////////////////////////////////////////////////////////
#include "../dmlib/mlib.h"
#include <commctrl.h>
#include "../dmlib/drvmount.h"

#define DM_WM_THREADEND							(WM_APP+1)
#define DM_SETTING_MUTEX						TEXT("DM_SETTING_20131225_mt")

#define ID_LISTMENU_UPD							10
#define ID_LISTMENU_DEL							11
#define ID_LISTMENU_CON							12
#define ID_LISTMENU_DCO							13
#define ID_LISTMENU_NEW							14
#define ID_LISTMENU_OPEN						15

class EntryDlg;

// メインダイアログ
class MainDlg : public MDlg{
	HIMAGELIST						m_hImgList;
	HMENU							m_hListMenu;
	int								m_imgid[5];
	EntryDlg*						m_pDlg;
	DMCOMMON						m_dc;
	bool							m_bNosave;
	bool							m_bChange;
protected:
	INT_PTR	DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);
	void OnInit(WPARAM wParam, LPARAM lParam);
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void OnExit(WPARAM wParam, LPARAM lParam);
	BOOL OnNotify(WPARAM wParam, LPARAM lParam);
	void OpenUpdateEntry();
	void DeleteEntry();
	void OpenDrive();
	void InitListView();
	void InitListViewMenu();
	void ListViewRepaint();
	void ListMenuOpen(bool bKeyBord=false);
	void DeleteDrive(TCHAR cLetter);
	void ImageRepaint(int nItem, LPTSTR lpDrive);
	void Connect();
	void Disconnect();
	int OpenEntryDlg();
	void ButtonEnable();
	int GetImageNo(TCHAR drvltr, DWORD dwEnableDrives);
	bool SaveAllSetting();
	void DeleteAll();
public:
	static TCHAR	m_cLetter;
	MainDlg(HWND hWnd=NULL) : MDlg(hWnd){}
};

// エントリーダイアログ
class EntryDlg : public MDlg{
	int							m_result;
protected:
	INT_PTR	DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);
	void OnInit(WPARAM wParam, LPARAM lParam);
	void OnCommand(WPARAM wParam, LPARAM lParam);
	void OnExit(WPARAM wParam, LPARAM lParam);
	void SetDlgInfo();
	void ExecPathEnable();
public:
	EntryDlg(HWND hWnd=NULL) : MDlg(hWnd){}
};

// アプリ基本クラス
class DMApp : public MApp{
	static DMApp*				m_pApp;
	LPTSTR						m_lpIniFile;
	int							m_inifnsize;
	DMApp(HINSTANCE hInst, LPSTR lpCmdLine, int nShowCmd);
	DMINFO						m_di['Z'-'A'+1];
protected:
	int Main();
public:
	~DMApp();
	static DMApp* getInstance(HINSTANCE hInst, LPSTR lpCmdLine, int nShowCmd);
	static DMApp* getInstance(){ return m_pApp; }
	HINSTANCE GetAppInstance()const{ return m_hInst; }
	LPCTSTR GetModuleDirectory()const{ return m_szModuleDir; }
	LPCTSTR GetIniFileName()const{ return m_lpIniFile; }
	DMINFO& GetDriveInfo(int idx){ return m_di[idx]; }
	const DMINFO& GetDriveInfo(int idx)const{ return m_di[idx]; }
};

#endif
