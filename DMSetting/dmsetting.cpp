///////////////////////////////////////////////////////////////////////////
//
// dmsetting.cpp
// DMSetting ���C��
//
///////////////////////////////////////////////////////////////////////////
#include <tchar.h>
#include <string>
#include <Lmcons.h>
#include "dmsetting.h"
#include "resource.h"

#ifdef UNICODE
#define tstring wstring
#else
#define tstring string
#endif

using namespace std;

// int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPTSTR lpCmdLine, int nShowCmd)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
	// ���d�N���h�~
	SECURITY_DESCRIPTOR sd;
	if(!::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION)){
		::MessageBox(NULL, TEXT("�Z�L�����e�B�L�q�q�̏������Ɏ��s�I"),TEXT("DMSetting"),MB_ICONERROR|MB_OK);
		return 2;
	}
	if(!::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE)){
		::MessageBox(NULL, TEXT("DACL�̐ݒ�Ɏ��s�I"),TEXT("DMSetting"),MB_ICONERROR|MB_OK);
		return 2;
	}
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE;
	HANDLE hMutex = ::CreateMutex(&sa, FALSE, DM_SETTING_MUTEX);
	if(hMutex == NULL){
		::MessageBox(NULL, TEXT("Mutex�I�u�W�F�N�g�̎擾�Ɏ��s�I"),TEXT("DMSetting"),MB_ICONERROR|MB_OK);
		return 2;
	}else if(::GetLastError() == ERROR_ALREADY_EXISTS){
		// ::MessageBox(NULL, TEXT("���ɋN�����Ă��܂��I"),TEXT("DMSetting"),MB_ICONERROR|MB_OK);
		::ReleaseMutex(hMutex);
		::CloseHandle(hMutex);
		HWND hWnd = ::FindWindow(TEXT("#32770"), TEXT("�l�b�g���[�N�h���C�u�ڑ��ݒ� - DrvMount"));
		if(hWnd != NULL){
			::ShowWindow(hWnd, SW_RESTORE);
			::SetForegroundWindow(hWnd);
		}
		return 2;
	}
	//////

	int result = DMApp::getInstance(hInstance, lpCmdLine, nShowCmd)->Run();

	// �~���[�e�b�N�X�I�u�W�F�N�g��j���I�I
	if(hMutex != NULL){
		::ReleaseMutex(hMutex);
		::CloseHandle(hMutex);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////
//
// MainDlg �N���X
//
//////////////////////////////////////////////////////////////////////////////////////
TCHAR MainDlg::m_cLetter = 0;

INT_PTR	MainDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg){
		case WM_DEVICECHANGE:
			ListViewRepaint();
		return TRUE;
		case WM_INITDIALOG:
			OnInit(wParam, lParam);
		return TRUE;

		case WM_COMMAND:
			OnCommand(wParam, lParam);
		return TRUE;

		case WM_CLOSE:
			if(m_bChange && m_bNosave && MsgBox(m_hWnd,MB_YESNO|MB_ICONQUESTION,TEXT("�m�F"),
				TEXT("�ύX���e���ۑ�����Ă��܂���B\n�I�����Ă����ł����H"))==IDNO) return TRUE;
			OnExit(wParam, lParam);
			::EndDialog(m_hWnd, 0);
		return TRUE;

		case WM_NOTIFY:
			OnNotify(wParam, lParam);
		return TRUE;
	}
	return FALSE;
}

void MainDlg::OnInit(WPARAM wParam, LPARAM lParam)
{
	DMApp *pApp = DMApp::getInstance();
	m_pDlg = new EntryDlg(m_hWnd);
	m_bNosave = true;

	SendMessage(WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(::LoadIcon(pApp->GetAppInstance(), MAKEINTRESOURCE(IDI_ICON1))));
	SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(::LoadIcon(pApp->GetAppInstance(), MAKEINTRESOURCE(IDI_ICON1))));

	InitListView();
	InitListViewMenu();

	// �o�[�W�������
	WORD v1,v2,v3,v4;
	TCHAR szVersion[64];
	pApp->GetAppVersion(&v1, &v2, &v3, &v4);
	::wsprintf(szVersion, TEXT("Ver. %u.%u.%u (%u)"), v1, v2, v3, v4);
	::SetDlgItemText(m_hWnd, IDC_STATIC_VERSION, szVersion);

	// ini�t�@�C���Ǎ�
	if(!DrvMount::LoadCommonInfo(DMApp::getInstance()->GetIniFileName(), m_dc)){
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("�G���["), TEXT("�ݒ���̓Ǎ��Ɏ��s���܂���"));
	}
	for(int i=0; i<=('Z'-'A'); ++i){
		DMINFO& di = pApp->GetDriveInfo(i);
		di.cDRVLTR = TEXT('A') + i;
		if(!DrvMount::LoadDriveInfo(DMApp::getInstance()->GetIniFileName(),di)){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("�G���["), TEXT("�h���C�u���̓Ǎ��Ɏ��s���܂���"));
		}
	}

	// �_�C�A���O�l������
	if(DrvMount::IsStartupShortcut(m_dc.uALLUSSC==BST_CHECKED)){
		::CheckDlgButton(m_hWnd, IDC_CHECK_STARTUP, BST_CHECKED);
	}else{
		::CheckDlgButton(m_hWnd, IDC_CHECK_STARTUP, BST_UNCHECKED);
	}
	if(m_dc.uERRNOTIFY == BST_CHECKED) ::CheckDlgButton(m_hWnd, IDC_CHECK_ERRNOTIFY, BST_CHECKED);

	ListViewRepaint();

	ButtonEnable();

	m_bChange = false;
}

void MainDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam)){
		case IDC_BUTTON_EXECSTART:
			if(MsgBox(m_hWnd,MB_YESNO|MB_ICONQUESTION,TEXT("�m�F"),
				TEXT("�N�����Ɏ����ڑ����܂��B\n")
				TEXT("���e��ύX�����ꍇ�͕ۑ����Ȃ��Ɣ��f����܂���B\n")
				TEXT("��낵���ł����H"))==IDYES){
				tstring tstr(TEXT("\""));
				tstr += DMApp::getInstance()->GetModuleDirectory();
				tstr += TEXT("\\");
				tstr += DM_DMSTARTNAME;
				tstr += TEXT("\"");
				if(!DrvMount::Exec(tstr.c_str())){
					MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("�G���["), TEXT("�����ڑ��̋N���Ɏ��s�I"));
				}else{
					// ::Sleep(m_dc.uCREFWAIT);
					ListViewRepaint();
				}
			}
		break;

		case IDC_BUTTON_HELP:
			{
				tstring tstr(TEXT("\""));
				tstr += DMApp::getInstance()->GetModuleDirectory();
				tstr += TEXT("\\");
				tstr += DM_DMHELPFILE;
				tstr += TEXT("\"");
				if(!DrvMount::Exec(tstr.c_str())){
					MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("�G���["), TEXT("�w���v���J���܂���ł����B"));
				}
			}
		break;

		case IDC_BUTTON_ALLDEL:
			if(MsgBox(m_hWnd,MB_YESNO|MB_ICONQUESTION,TEXT("�m�F"),
				TEXT("�S�Ă̓o�^�����폜���܂��B\n��낵���ł����H"))==IDNO) break;
			else{
				DeleteAll();
				ListViewRepaint();
			}
		break;

		case IDC_BUTTON_ADD:
			m_cLetter = 0;
			OpenEntryDlg();
		break;

		case IDC_BUTTON_UPD:
		case ID_LISTMENU_UPD:
			OpenUpdateEntry();
		break;

		case IDC_BUTTON_DEL:
		case ID_LISTMENU_DEL:
			DeleteEntry();
		break;

		case IDC_BUTTON_SAVE:
			m_dc.uERRNOTIFY = ::IsDlgButtonChecked(m_hWnd,IDC_CHECK_ERRNOTIFY);
			if(SaveAllSetting()){
				m_bChange = false;
				MsgBox(m_hWnd,MB_ICONINFORMATION|MB_OK, TEXT("�ʒm"), TEXT("�ݒ��ۑ����܂����B"));
			}
		break;

		case ID_LISTMENU_CON:
			Connect();
		break;

		case ID_LISTMENU_DCO:
			Disconnect();
		break;

		case ID_LISTMENU_NEW:
			m_cLetter = 0;
			OpenEntryDlg();
		break;

		case ID_LISTMENU_OPEN:
			OpenDrive();
		break;

		case IDC_CHECK_STARTUP:
		case IDC_CHECK_ERRNOTIFY:
			if(HIWORD(wParam)==BN_CLICKED) m_bChange = true;
		break;

		case IDOK:
		{
			m_bNosave = false;
			m_dc.uERRNOTIFY = ::IsDlgButtonChecked(m_hWnd,IDC_CHECK_ERRNOTIFY);
			if(!SaveAllSetting()) break;
			SendMessage(WM_CLOSE, 0, 0);
		}
		break;

		case IDCANCEL:
			m_bNosave = true;
			SendMessage(WM_CLOSE, 0, 0);
		break;
	}
}

void MainDlg::OnExit(WPARAM wParam, LPARAM lParam)
{
	delete m_pDlg;

	if(m_hListMenu != NULL){
		::DestroyMenu(m_hListMenu);
	}
	if(m_hImgList != NULL){
		::ImageList_Destroy(m_hImgList);
	}
}

BOOL MainDlg::OnNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
	LPNMLVKEYDOWN lpnmlvkeydown = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
	LPNMLISTVIEW lpnmlistview = reinterpret_cast<LPNMLISTVIEW>(lParam);

	switch(lpnmhdr->code){
		case NM_RCLICK:	// �E�I
			ListMenuOpen();
		return TRUE;

		case NM_DBLCLK:
			// �_�u���N���b�N
			OpenUpdateEntry();
		return TRUE;

		case LVN_KEYDOWN: // DEL�L�[
			switch(lpnmlvkeydown->wVKey){
				case VK_DELETE:
					DeleteEntry();
				break;

				case VK_F2:
					// �_�u���N���b�N�Ɠ�������
					OpenUpdateEntry();
				break;

				case VK_F5:
					ListViewRepaint();
				break;
				case VK_APPS:
					ListMenuOpen(true);
				break;
			}
		return TRUE;

		case LVN_ITEMCHANGED:
			ButtonEnable();
			/*
			if((lpnmlistview->uNewState & LVIS_FOCUSED) && (lpnmlistview->uNewState & LVIS_SELECTED)){
				// �I�����ꂽ��index(0�N�_)���Alpnmlistview->iItem�ɓ���
			}else{
				// �����I������ĂȂ��ꍇ�H
			}
			MsgBox(m_hWnd, MB_OK, TEXT("debug"), TEXT("iItem=%d\niSubItem=%d\nuNewState=%u\nuChanged=%u\nuOldState=%u"),
				lpnmlistview->iItem,
				lpnmlistview->iSubItem,
				lpnmlistview->uNewState,
				lpnmlistview->uChanged,
				lpnmlistview->uOldState);
			*/
		return TRUE;
	}
	return FALSE;
}

// �ύX���[�h�Ńh���C�u�G���g���[�_�C�A���O�I�[�v��
void MainDlg::OpenUpdateEntry()
{
	TCHAR szDrv[8];
	int nItem = ListView_GetNextItem(::GetDlgItem(m_hWnd,IDC_LIST_CON), -1, LVNI_ALL | LVNI_SELECTED);
	if(nItem == -1) return;
	ListView_GetItemText(::GetDlgItem(m_hWnd,IDC_LIST_CON), nItem, 0, szDrv, sizeof(szDrv)/sizeof(TCHAR)-1);
	m_cLetter = szDrv[0];
	OpenEntryDlg();
}

// �h���C�u���̍폜
void MainDlg::DeleteEntry()
{
	TCHAR szDrv[8];
	int nItem = ListView_GetNextItem(::GetDlgItem(m_hWnd,IDC_LIST_CON), -1, LVNI_ALL | LVNI_SELECTED);
	if(nItem == -1) return;
	ListView_GetItemText(::GetDlgItem(m_hWnd,IDC_LIST_CON), nItem, 0, szDrv, sizeof(szDrv)/sizeof(TCHAR)-1);
	DeleteDrive(szDrv[0]);
	ListViewRepaint();
	m_bChange = true;
}

// �h���C�u���J��
void MainDlg::OpenDrive()
{
	TCHAR szDrv[8];
	int nItem = ListView_GetNextItem(::GetDlgItem(m_hWnd,IDC_LIST_CON), -1, LVNI_ALL | LVNI_SELECTED);
	if(nItem == -1) return;
	ListView_GetItemText(::GetDlgItem(m_hWnd,IDC_LIST_CON), nItem, 0, szDrv, sizeof(szDrv)/sizeof(TCHAR)-1);
	::lstrcat(szDrv, TEXT("\\"));
	::ShellExecute(NULL, TEXT("open"), TEXT("explorer"), szDrv, NULL, SW_SHOWNORMAL);
}

// ���X�g�r���[������
void MainDlg::InitListView()
{
	// �C���[�W���X�g
	if((m_hImgList=::ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,4,1))==NULL){
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("�C���[�W���X�g�����G���["));
		SendMessage(WM_CLOSE, 0, 0L);
		return;
	}

	DMApp* pApp = DMApp::getInstance();
	m_imgid[0] = ::ImageList_AddIcon(m_hImgList, ::LoadIcon(pApp->GetAppInstance(), MAKEINTRESOURCE(IDI_ONLINE1)));
	m_imgid[1] = ::ImageList_AddIcon(m_hImgList, ::LoadIcon(pApp->GetAppInstance(), MAKEINTRESOURCE(IDI_ONLINE2)));
	m_imgid[2] = ::ImageList_AddIcon(m_hImgList, ::LoadIcon(pApp->GetAppInstance(), MAKEINTRESOURCE(IDI_OFFLINE)));
	m_imgid[3] = ::ImageList_AddIcon(m_hImgList, ::LoadIcon(pApp->GetAppInstance(), MAKEINTRESOURCE(IDI_DISABLE)));

	for(int i=0; i<4; ++i){
		if(m_imgid[i] < 0){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("�C���[�W���X�g�o�^�G���["));
			SendMessage(WM_CLOSE, 0, 0L);
			return;
		}
	}

	DWORD dwStyle = ListView_GetExtendedListViewStyle(::GetDlgItem(m_hWnd,IDC_LIST_CON));
	dwStyle |= (LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	ListView_SetExtendedListViewStyle(::GetDlgItem(m_hWnd,IDC_LIST_CON), dwStyle);

	LVCOLUMN lvCol;

	// �h���C�u���^�[
	lvCol.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = 50;
	lvCol.pszText = TEXT("�h���C�u");
	lvCol.cchTextMax = ::lstrlen(lvCol.pszText);
	lvCol.iSubItem = 0;
	if(ListView_InsertColumn(::GetDlgItem(m_hWnd, IDC_LIST_CON), 0, &lvCol) == -1) {
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("ListView_InsertColumn (%d) �G���["), lvCol.iSubItem);
		SendMessage(WM_CLOSE, 0, 0L);
		return;
	}

	// �p�X
	lvCol.cx = 160;
	lvCol.pszText = TEXT("�p�X");
	lvCol.cchTextMax = ::lstrlen(lvCol.pszText);
	lvCol.iSubItem = 1;
	if(ListView_InsertColumn(::GetDlgItem(m_hWnd, IDC_LIST_CON), 1, &lvCol) == -1) {
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("ListView_InsertColumn (%d) �G���["), lvCol.iSubItem);
		SendMessage(WM_CLOSE, 0, 0L);
		return;
	}

	// ���[�U�[
	lvCol.cx = 100;
	lvCol.pszText = TEXT("���[�U�[");
	lvCol.cchTextMax = ::lstrlen(lvCol.pszText);
	lvCol.iSubItem = 2;
	if(ListView_InsertColumn(::GetDlgItem(m_hWnd, IDC_LIST_CON), 2, &lvCol) == -1) {
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("ListView_InsertColumn (%d) �G���["), lvCol.iSubItem);
		SendMessage(WM_CLOSE, 0, 0L);
		return;
	}

	// �����ڑ�
	lvCol.cx = 50;
	lvCol.pszText = TEXT("�����ڑ�");
	lvCol.cchTextMax = ::lstrlen(lvCol.pszText);
	lvCol.iSubItem = 3;
	if(ListView_InsertColumn(::GetDlgItem(m_hWnd, IDC_LIST_CON), 3, &lvCol) == -1) {
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("ListView_InsertColumn (%d) �G���["), lvCol.iSubItem);
		SendMessage(WM_CLOSE, 0, 0L);
		return;
	}

	// �����ڑ�
	lvCol.cx = 50;
	lvCol.pszText = TEXT("�����ڑ�");
	lvCol.cchTextMax = ::lstrlen(lvCol.pszText);
	lvCol.iSubItem = 4;
	if(ListView_InsertColumn(::GetDlgItem(m_hWnd, IDC_LIST_CON), 4, &lvCol) == -1) {
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("ListView_InsertColumn (%d) �G���["), lvCol.iSubItem);
		SendMessage(WM_CLOSE, 0, 0L);
		return;
	}

	// �R�}���h���s
	lvCol.cx = 50;
	lvCol.pszText = TEXT("�R�}���h���s");
	lvCol.cchTextMax = ::lstrlen(lvCol.pszText);
	lvCol.iSubItem = 5;
	if(ListView_InsertColumn(::GetDlgItem(m_hWnd, IDC_LIST_CON), 5, &lvCol) == -1) {
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("ListView_InsertColumn (%d) �G���["), lvCol.iSubItem);
		SendMessage(WM_CLOSE, 0, 0L);
		return;
	}

	// ���g���C��
	lvCol.cx = 50;
	lvCol.fmt = LVCFMT_RIGHT;
	lvCol.pszText = TEXT("���g���C��");
	lvCol.cchTextMax = ::lstrlen(lvCol.pszText);
	lvCol.iSubItem = 6;
	if(ListView_InsertColumn(::GetDlgItem(m_hWnd, IDC_LIST_CON), 6, &lvCol) == -1) {
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("DMSetting"), TEXT("ListView_InsertColumn (%d) �G���["), lvCol.iSubItem);
		SendMessage(WM_CLOSE, 0, 0L);
		return;
	}

	// ���X�g�r���[�փC���[�W���X�g��ݒ�
	ListView_SetImageList(::GetDlgItem(m_hWnd,IDC_LIST_CON), m_hImgList, LVSIL_SMALL);
}

// ���X�g�r���[�p�̃��j���[
void MainDlg::InitListViewMenu()
{
	MENUITEMINFO mii;
	::ZeroMemory(&mii, sizeof(mii));

	m_hListMenu = ::CreatePopupMenu();

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = TEXT("�C��");
	mii.wID = ID_LISTMENU_UPD;
	::InsertMenuItem(m_hListMenu, 0, TRUE, &mii); 

	mii.dwTypeData = TEXT("�폜");
	mii.wID = ID_LISTMENU_DEL;
	::InsertMenuItem(m_hListMenu, 1, TRUE, &mii);

	mii.dwTypeData = TEXT("-");
	mii.wID = 0;
	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_SEPARATOR;
	::InsertMenuItem(m_hListMenu, 2, TRUE, &mii);

	mii.fType = MFT_STRING;
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.dwTypeData = TEXT("�ڑ�");
	mii.wID = ID_LISTMENU_CON;
	::InsertMenuItem(m_hListMenu, 3, TRUE, &mii);

	mii.dwTypeData = TEXT("�ؒf");
	mii.wID = ID_LISTMENU_DCO;
	::InsertMenuItem(m_hListMenu, 4, TRUE, &mii); 

	mii.dwTypeData = TEXT("-");
	mii.wID = 0;
	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_SEPARATOR;
	::InsertMenuItem(m_hListMenu, 5, TRUE, &mii);

	mii.fType = MFT_STRING;
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.dwTypeData = TEXT("�ŐV�̏��ɍX�V(F5)");
	mii.wID = ID_LISTMENU_NEW;
	::InsertMenuItem(m_hListMenu, 6, TRUE, &mii);

	mii.dwTypeData = TEXT("-");
	mii.wID = 0;
	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_SEPARATOR;
	::InsertMenuItem(m_hListMenu, 7, TRUE, &mii);

	mii.fType = MFT_STRING;
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.dwTypeData = TEXT("�J��");
	mii.wID = ID_LISTMENU_OPEN;
	::InsertMenuItem(m_hListMenu, 8, TRUE, &mii);
}

// ���X�g�r���[���e�ĕ\��
void MainDlg::ListViewRepaint()
{
	DMApp* pApp = DMApp::getInstance();
	HWND hList = ::GetDlgItem(m_hWnd, IDC_LIST_CON);

	// �N���A
	ListView_DeleteAllItems(hList);

	DWORD dwDrives = ::GetLogicalDrives();
	LVITEM lvItem;
	TCHAR szBuf[64];

	int count = 0;
	for(int i=0; i<=('Z'-'A'); ++i){
		DMINFO& di = pApp->GetDriveInfo(i);
		if(!di.szPATH[0] || di.cDRVLTR < TEXT('A') || di.cDRVLTR > TEXT('Z')) continue;

		::ZeroMemory(&lvItem, sizeof(lvItem));

		// �h���C�u���^�[
		lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
		lvItem.iItem = count;
		lvItem.iSubItem = 0;
		lvItem.iImage = m_imgid[GetImageNo(TEXT('A')+i, dwDrives)];
		::wsprintf(szBuf, TEXT("%c:"), TEXT('A')+i);
		lvItem.pszText = szBuf;
		if(ListView_InsertItem(hList,&lvItem) == -1){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("ListView"), TEXT("���X�g�A�C�e���ǉ��G���[(%s)"), szBuf);
			break;
		}

		// �p�X
		lvItem.iSubItem = 1;
		lvItem.pszText = di.szPATH;
		if(ListView_SetItem(hList, &lvItem) == -1){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("ListView"), TEXT("���X�g�A�C�e���Z�b�g�G���[(%s)"), di.szPATH);
			break;
		}

		// ���[�U�[
		lvItem.iSubItem = 2;
		lvItem.pszText = di.szUSER;
		if(ListView_SetItem(hList, &lvItem) == -1){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("ListView"), TEXT("���X�g�A�C�e���Z�b�g�G���[(%s)"), di.szUSER);
			break;
		}

		// �����ڑ�
		lvItem.iSubItem = 3;
		lvItem.pszText = di.uSCON==BST_CHECKED ? TEXT("����") : TEXT("���Ȃ�");
		if(ListView_SetItem(hList, &lvItem) == -1){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("ListView"), TEXT("���X�g�A�C�e���Z�b�g�G���[(�����ڑ�)"));
			break;
		}

		// �����ڑ�
		lvItem.iSubItem = 4;
		lvItem.pszText = di.uFORCE==BST_CHECKED ? TEXT("����") : TEXT("���Ȃ�");
		if(ListView_SetItem(hList, &lvItem) == -1){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("ListView"), TEXT("���X�g�A�C�e���Z�b�g�G���[(�����ڑ�)"));
			break;
		}

		// �R�}���h���s
		lvItem.iSubItem = 5;
		lvItem.pszText = di.uEXEC==BST_CHECKED ? TEXT("����") : TEXT("���Ȃ�");
		if(ListView_SetItem(hList, &lvItem) == -1){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("ListView"), TEXT("���X�g�A�C�e���Z�b�g�G���[(�R�}���h���s)"));
			break;
		}

		// ���g���C��
		if(di.uRETRY) ::wsprintf(szBuf,TEXT("%u��"),di.uRETRY);
		else ::lstrcpyn(szBuf,TEXT("������"),sizeof(szBuf)/sizeof(TCHAR));
		lvItem.iSubItem = 6;
		lvItem.pszText = szBuf;
		if(ListView_SetItem(hList, &lvItem) == -1){
			MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("ListView"), TEXT("���X�g�A�C�e���Z�b�g�G���[(���g���C��)"));
			break;
		}
		++count;
	}

}

// ���j���[���J��
void MainDlg::ListMenuOpen(bool bKeyBord)
{
	TCHAR szDrive[16];
	int nSelItem;
	DWORD dwDrv;
	POINT po;

	nSelItem = ListView_GetNextItem(::GetDlgItem(m_hWnd,IDC_LIST_CON), -1, LVNI_ALL | LVNI_SELECTED);
	if(nSelItem < 0){
		::EnableMenuItem(m_hListMenu, ID_LISTMENU_UPD, MF_BYCOMMAND | MF_GRAYED);
		::EnableMenuItem(m_hListMenu, ID_LISTMENU_DEL, MF_BYCOMMAND | MF_GRAYED);
		::EnableMenuItem(m_hListMenu, ID_LISTMENU_CON, MF_BYCOMMAND | MF_GRAYED);
		::EnableMenuItem(m_hListMenu, ID_LISTMENU_DCO, MF_BYCOMMAND | MF_GRAYED);
		::EnableMenuItem(m_hListMenu, ID_LISTMENU_OPEN, MF_BYCOMMAND | MF_GRAYED);
		m_cLetter = 0;
	}else{
		::EnableMenuItem(m_hListMenu, ID_LISTMENU_UPD, MF_BYCOMMAND | MF_ENABLED);
		::EnableMenuItem(m_hListMenu, ID_LISTMENU_DEL, MF_BYCOMMAND | MF_ENABLED);
		ListView_GetItemText(::GetDlgItem(m_hWnd,IDC_LIST_CON), nSelItem, 0, szDrive, sizeof(szDrive)/sizeof(TCHAR)-2);
		m_cLetter = szDrive[0];
		dwDrv = ::GetLogicalDrives();
		if(dwDrv & (1<<(m_cLetter-'A'))){
			::EnableMenuItem(m_hListMenu, ID_LISTMENU_OPEN,
							MF_BYCOMMAND | MF_ENABLED);
			::EnableMenuItem(m_hListMenu, ID_LISTMENU_CON,
							MF_BYCOMMAND | MF_GRAYED);
			::lstrcat(szDrive, TEXT("\\"));
			switch(::GetDriveType(szDrive)){
				case DRIVE_REMOTE:
				case DRIVE_FIXED:
					::EnableMenuItem(m_hListMenu, ID_LISTMENU_DCO, MF_BYCOMMAND | MF_ENABLED);
				break;
				default:
					::EnableMenuItem(m_hListMenu, ID_LISTMENU_DCO, MF_BYCOMMAND | MF_GRAYED);
				break;
			}
		}else{
			::EnableMenuItem(m_hListMenu, ID_LISTMENU_CON, MF_BYCOMMAND | MF_ENABLED);
			::EnableMenuItem(m_hListMenu, ID_LISTMENU_DCO, MF_BYCOMMAND | MF_GRAYED);
			::EnableMenuItem(m_hListMenu, ID_LISTMENU_OPEN, MF_BYCOMMAND | MF_GRAYED);
		}
	}
	::GetCursorPos(&po);
	if(bKeyBord){
		RECT rc;
		::GetWindowRect(::GetDlgItem(m_hWnd,IDC_LIST_CON),&rc);
		if(po.x < rc.left || po.x > rc.right || po.y < rc.top || po.y > rc.bottom){
			po.x = (rc.left + ((rc.right - rc.left) / 10)) ;
			po.y = (rc.top + ((rc.bottom - rc.top) / 10));
			// po.x = rc.left + 16;
			// po.y = rc.top + 16;
		}
	}
	::TrackPopupMenuEx(m_hListMenu, TPM_RIGHTBUTTON | TPM_TOPALIGN, po.x, po.y, m_hWnd, NULL);
}

// �h���C�u�ݒ�����폜
void MainDlg::DeleteDrive(TCHAR cLetter)
{
	if(cLetter < TEXT('A') || cLetter > TEXT('Z')) return;

	DMINFO& di = DMApp::getInstance()->GetDriveInfo(cLetter - TEXT('A'));
	::ZeroMemory(&di, sizeof(di));
	di.cDRVLTR = cLetter;
}

// ���X�g�r���[�C���[�W�̍X�V
void MainDlg::ImageRepaint(int nItem, LPTSTR lpDrive)
{
	LVITEM lvItem;
	DWORD dwDrives = ::GetLogicalDrives();
	::ZeroMemory(&lvItem, sizeof(LVITEM));
	lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
	lvItem.iItem = nItem;
	lvItem.iSubItem = 0;
	lvItem.iImage = m_imgid[GetImageNo(lpDrive[0], dwDrives)];
	lvItem.pszText = lpDrive;
	ListView_SetItem(::GetDlgItem(m_hWnd,IDC_LIST_CON), &lvItem);
}

// �h���C�u�ڑ�
void MainDlg::Connect()
{
	TCHAR szDrv[8];

	int nItem = ListView_GetNextItem(::GetDlgItem(m_hWnd,IDC_LIST_CON), -1, LVNI_ALL | LVNI_SELECTED);
	if(nItem == -1) return;

	// �A�C�R���̍����v��
	HCURSOR cur = ::SetCursor(::LoadCursor(NULL,IDC_WAIT));

	ListView_GetItemText(::GetDlgItem(m_hWnd,IDC_LIST_CON), nItem, 0, szDrv, sizeof(szDrv)/sizeof(TCHAR)-1);
	DMApp* pApp = DMApp::getInstance();
	DMINFO& di = pApp->GetDriveInfo(szDrv[0]-TEXT('A'));

	if(!DrvMount::Mount(di)){
		// �����I
		ImageRepaint(nItem, szDrv);
	}else{
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("�G���["),
			TEXT("%c�h���C�u�̃}�E���g�Ɏ��s\n�p�X��p�X���[�h�����������m�F���Ă��������B"), di.cDRVLTR);
	}

	// �A�C�R���̍����v������
	::SetCursor(cur);
}

// �h���C�u�ؒf
void MainDlg::Disconnect()
{
	TCHAR szDrv[8];

	int nItem = ListView_GetNextItem(::GetDlgItem(m_hWnd,IDC_LIST_CON), -1, LVNI_ALL | LVNI_SELECTED);
	if(nItem == -1) return;

	// �A�C�R���̍����v��
	HCURSOR cur = ::SetCursor(::LoadCursor(NULL,IDC_WAIT));

	ListView_GetItemText(::GetDlgItem(m_hWnd,IDC_LIST_CON), nItem, 0, szDrv, sizeof(szDrv)/sizeof(TCHAR)-1);
	DMApp* pApp = DMApp::getInstance();
	DMINFO& di = pApp->GetDriveInfo(szDrv[0]-TEXT('A'));

	if(!DrvMount::Unmount(di)){
		// �����I
		ImageRepaint(nItem, szDrv);
	}else{
		MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("�G���["), TEXT("%c�h���C�u�̃A���}�E���g�Ɏ��s"), di.cDRVLTR);
	}

	// �A�C�R���̍����v������
	::SetCursor(cur);
}

// �o�^�p�_�C�A���O���J��
int MainDlg::OpenEntryDlg()
{
	int result = m_pDlg->Create(DMApp::getInstance()->GetAppInstance(), MAKEINTRESOURCE(IDD_ENTRYDLG), m_hWnd);
	if(result == 1){
		m_bChange = true;
		ListViewRepaint();
	}
	return result;
}

// �{�^���̗L������
void MainDlg::ButtonEnable()
{
	if(ListView_GetNextItem(::GetDlgItem(m_hWnd,IDC_LIST_CON),
			-1, LVNI_ALL | LVNI_SELECTED) < 0){
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_UPD), FALSE);
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_DEL), FALSE);
	}else{
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_UPD), TRUE);
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_DEL), TRUE);
	}
}

// �A�C�R���C���[�W�̗v�f�ԍ�
int MainDlg::GetImageNo(TCHAR drvltr, DWORD dwEnableDrives)
{
	TCHAR szDrv[8];

	if(!(dwEnableDrives & (1<<(drvltr-TEXT('A'))))) return 2;

	::wsprintf(szDrv, TEXT("%c:\\"), drvltr);
	switch(::GetDriveType(szDrv)){
		case DRIVE_REMOVABLE:
		case DRIVE_CDROM:
		case DRIVE_RAMDISK:
		return 3;	// �g���Ȃ��h���C�u

		case DRIVE_FIXED:
		return 1;	// �Œ�h���C�u�Ƃ��ăI�����C���i����������Ɖ��z�h���C�u�����j

		case DRIVE_REMOTE:
		return 0;	// �l�b�g���[�N�h���C�u�ڑ���
	}

	return 3; // ����ȊO�͎g���Ȃ��h���C�u�Ƃ���
}

// �S�Ă̐ݒ��ۑ�
bool MainDlg::SaveAllSetting()
{
	DMApp* pApp = DMApp::getInstance();

	// �X�^�[�g�A�b�v�V���[�g�J�b�g
	if(::IsDlgButtonChecked(m_hWnd,IDC_CHECK_STARTUP)==BST_CHECKED){
		// �o�^����Ƀ`�F�b�N����������o�^
		if(!DrvMount::IsStartupShortcut(m_dc.uALLUSSC==BST_CHECKED)){
			// �V���[�g�J�b�g�̐���
			if(!DrvMount::MakeStartupShortcut(this,m_dc.uALLUSSC==BST_CHECKED,pApp->GetModuleDirectory())){
				MsgBox(m_hWnd, MB_OK|MB_ICONERROR,TEXT("�G���["),TEXT("�X�^�[�g�A�b�v�ɃV���[�g�J�b�g�����܂���ł���"));
				return false;
			}
		}
	}else{
		// �o�^����Ƀ`�F�b�N���Ȃ���΍폜
		if(DrvMount::IsStartupShortcut(m_dc.uALLUSSC==BST_CHECKED)){
			// �V���[�g�J�b�g�̍폜
			TCHAR szLink[MAX_PATH+1];
			if(!DrvMount::GetStartupShortcut(szLink,sizeof(szLink)/sizeof(TCHAR),m_dc.uALLUSSC==BST_CHECKED)){
				MsgBox(m_hWnd, MB_OK|MB_ICONERROR,TEXT("�G���["),TEXT("�X�^�[�g�A�b�v�̃p�X���擾�ł��܂���"));
				return false;
			}
			if(!::DeleteFile(szLink)){
				MsgBox(m_hWnd, MB_OK|MB_ICONERROR,TEXT("�G���["),TEXT("�X�^�[�g�A�b�v�V���[�g�J�b�g���폜�ł��܂���"));
				return false;
			}
		}
	}

	if(!DrvMount::SaveCommonInfo(pApp->GetIniFileName(),m_dc)){
		MsgBox(m_hWnd, MB_OK|MB_ICONERROR,TEXT("�G���["),TEXT("���ʐݒ���̏����Ɏ��s�I�I"));
		return false;
	}

	for(int i=0; i<=('Z'-'A'); ++i){
		DMINFO& di = pApp->GetDriveInfo(i);
		di.cDRVLTR = static_cast<TCHAR>(TEXT('A') + i);
		if(!DrvMount::SaveDriveInfo(pApp->GetIniFileName(),di)){
			MsgBox(m_hWnd, MB_OK|MB_ICONERROR,TEXT("�G���["),TEXT("�h���C�u�ݒ���̏����Ɏ��s�I�I"));
			return false;
		}
	}
	m_bChange = false;

	return true;
}

// ���ׂĂ̐ݒ�����폜
void MainDlg::DeleteAll()
{
	DMApp* pApp = DMApp::getInstance();
	for(int i=0; i<('Z'-'A'+1); ++i){
		DMINFO& di=pApp->GetDriveInfo(i);
		::ZeroMemory(&di, sizeof(di));
		di.cDRVLTR = TEXT('A') + i;
	}
	m_bChange = true;
}


//////////////////////////////////////////////////////////////////////////////////////
//
// EntryDlg �N���X
//
//////////////////////////////////////////////////////////////////////////////////////

// �_�C�A���O�v���V�[�W��
INT_PTR	EntryDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg){
		case WM_INITDIALOG:
			OnInit(wParam, lParam);
		return TRUE;

		case WM_COMMAND:
			OnCommand(wParam, lParam);
		return TRUE;

		case WM_CLOSE:
			OnExit(wParam, lParam);
			::EndDialog(m_hWnd, m_result);
		return TRUE;
	}
	return FALSE;
}

// ������
void EntryDlg::OnInit(WPARAM wParam, LPARAM lParam)
{
	m_result = 0;
	DMApp* pApp = DMApp::getInstance();

	SendMessage(WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(::LoadIcon(pApp->GetAppInstance(), MAKEINTRESOURCE(IDI_ICON1))));
	SendMessage(WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(::LoadIcon(pApp->GetAppInstance(), MAKEINTRESOURCE(IDI_ICON1))));

	TCHAR szDrv[16];
	for(int i=TEXT('A'); i<=TEXT('Z'); ++i){
		::wsprintf(szDrv, TEXT("%c:"), i);
		::SendDlgItemMessage(m_hWnd, IDC_COMBO_DRIVE, CB_INSERTSTRING, i-TEXT('A'), reinterpret_cast<LPARAM>(szDrv));
	}

	// ���������͐���
	::SendDlgItemMessage(m_hWnd, IDC_EDIT_PATH, EM_SETLIMITTEXT, DM_EL_PATH, 0);
	::SendDlgItemMessage(m_hWnd, IDC_EDIT_USER, EM_SETLIMITTEXT, DM_EL_USER, 0);
	::SendDlgItemMessage(m_hWnd, IDC_EDIT_PWD, EM_SETLIMITTEXT, DM_EL_PWD, 0);
	::SendDlgItemMessage(m_hWnd, IDC_EDIT_RETRY, EM_SETLIMITTEXT, DM_EL_RETRY, 0);
	::SendDlgItemMessage(m_hWnd, IDC_EDIT_RTWAIT, EM_SETLIMITTEXT, DM_EL_RTWAIT, 0);
	::SendDlgItemMessage(m_hWnd, IDC_EDIT_EXECPATH, EM_SETLIMITTEXT, DM_EL_EXECPATH, 0);

	if(MainDlg::m_cLetter >= TEXT('A') && MainDlg::m_cLetter <= TEXT('Z')){
		::SendDlgItemMessage(m_hWnd, IDC_COMBO_DRIVE, CB_SETCURSEL, MainDlg::m_cLetter-TEXT('A'), 0);
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_COMBO_DRIVE), FALSE);
		SetDlgInfo();
	}else{
		::SendDlgItemMessage(m_hWnd, IDC_COMBO_DRIVE, CB_SETCURSEL, -1, 0);
		::SetDlgItemInt(m_hWnd, IDC_EDIT_RETRY, DM_DEF_RETRY, FALSE);
		::SetDlgItemInt(m_hWnd, IDC_EDIT_RTWAIT, DM_DEF_RTWAIT, FALSE);
	}

	ExecPathEnable();
}

// �R�}���h�C�x���g
void EntryDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(HIWORD(wParam)){
		case CBN_SELCHANGE:
			if(LOWORD(wParam) == IDC_COMBO_DRIVE){
				SetDlgInfo();
			}
		break;

		case BN_CLICKED:
			if(LOWORD(wParam) == IDC_CHECK_EXEC) ExecPathEnable();
		break;

		case EN_CHANGE:
			if(LOWORD(wParam) == IDC_EDIT_EXECPATH) ExecPathEnable();
		break;
	}

	switch(LOWORD(wParam)){
		case IDOK:
		{
			int cur = static_cast<int>(::SendDlgItemMessage(m_hWnd, IDC_COMBO_DRIVE, CB_GETCURSEL, 0, 0));
			if(cur == CB_ERR){
				MsgBox(m_hWnd, MB_ICONERROR|MB_OK, TEXT("�o�^�s��"), TEXT("�o�^����h���C�u���^�[���I������Ă��܂���B"));
				break;
			}
			DMINFO& di = DMApp::getInstance()->GetDriveInfo(cur);
			int failcnt = 0;
			::ZeroMemory(&di, sizeof(di));
			BOOL bTran;
			di.cDRVLTR = TEXT('A') + cur;
			if(!::GetDlgItemText(m_hWnd, IDC_EDIT_PATH, di.szPATH, sizeof(di.szPATH)/sizeof(TCHAR)-1) && ::GetLastError()) ++failcnt;
			if(!::GetDlgItemText(m_hWnd, IDC_EDIT_USER, di.szUSER, sizeof(di.szUSER)/sizeof(TCHAR)-1) && ::GetLastError()) ++failcnt;
			if(!::GetDlgItemText(m_hWnd, IDC_EDIT_PWD, di.szPWD, sizeof(di.szPWD)/sizeof(TCHAR)-1) && ::GetLastError()) ++failcnt;
			di.uRETRY = ::GetDlgItemInt(m_hWnd, IDC_EDIT_RETRY, &bTran, FALSE); if(!bTran) ++failcnt;
			di.uRTWAIT = ::GetDlgItemInt(m_hWnd, IDC_EDIT_RTWAIT, &bTran, FALSE); if(!bTran) ++failcnt;
			di.uSCON = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_AUTOCON);
			di.uFORCE = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_FORCE);
			di.uEXEC = ::IsDlgButtonChecked(m_hWnd, IDC_CHECK_EXEC);
			if(!::GetDlgItemText(m_hWnd, IDC_EDIT_EXECPATH, di.szEXECPATH, sizeof(di.szEXECPATH)/sizeof(TCHAR)-1) && ::GetLastError()) ++failcnt;
			if(failcnt) MsgBox(m_hWnd, MB_OK|MB_ICONERROR, TEXT("�G���["), TEXT("�_�C�A���O���ǎ�G���[ %d"), failcnt);
			m_result = 1;
			SendMessage(WM_CLOSE, 0, 0);
		}
		break;
		case IDCANCEL:
			SendMessage(WM_CLOSE, 0, 0);
		break;
		case IDC_BUTTON_EXECPATH:
		{
			TCHAR szFile[DM_EXECPATHLEN]=TEXT("");
			tstring strFile;
			// ::GetDlgItemText(m_hWnd, IDC_EDIT_EXECPATH, szFile, sizeof(szFile)/sizeof(TCHAR));
			// szFile[sizeof(szFile)/sizeof(TCHAR)-1] = TEXT('\0');
			if(OpenFileName(m_hWnd,szFile,sizeof(szFile)/sizeof(TCHAR),TEXT("�v���O�����܂��̓t�@�C����I��"))){
				strFile = szFile;
				if(strFile.find(TEXT(' ')) != tstring::npos){
					strFile = TEXT("\"");
					strFile += szFile;
					strFile += TEXT("\"");
				}
				::SetDlgItemText(m_hWnd, IDC_EDIT_EXECPATH, strFile.c_str());
			}
		}
		break;
		case IDC_BUTTON_PATH:
		{
			TCHAR szDir[MAX_PATH+1];
			if(SelectFolder(m_hWnd, szDir,sizeof(szDir)/sizeof(TCHAR),TEXT("���蓖�Ă�t�H���_�[��I��"))){
				::SetDlgItemText(m_hWnd, IDC_EDIT_PATH, szDir);
			}
		}
		break;
		case IDC_BUTTON_EXECUTE:
		{
			TCHAR szFile[DM_EXECPATHLEN];
			::GetDlgItemText(m_hWnd, IDC_EDIT_EXECPATH, szFile, sizeof(szFile)/sizeof(TCHAR));
			szFile[sizeof(szFile)/sizeof(TCHAR)-1] = TEXT('\0');
			if(!DrvMount::Exec(szFile)){
				MsgBox(m_hWnd,MB_ICONERROR|MB_OK, TEXT("�G���["), TEXT("�R�}���h�̎��s�Ɏ��s���܂����B\n%s"), szFile);
			}
		}
		break;
	}
}

// �I��
void EntryDlg::OnExit(WPARAM wParam, LPARAM lParam)
{
}

// �_�C�A���O���ݒ�
void EntryDlg::SetDlgInfo()
{
	int cur = static_cast<int>(::SendDlgItemMessage(m_hWnd, IDC_COMBO_DRIVE, CB_GETCURSEL, 0, 0));
	if(cur == CB_ERR) return;
	DMINFO& di = DMApp::getInstance()->GetDriveInfo(cur);
	if(!di.szPATH[0]) return;
	::SetDlgItemText(m_hWnd, IDC_EDIT_PATH, di.szPATH);
	::SetDlgItemText(m_hWnd, IDC_EDIT_USER, di.szUSER);
	::SetDlgItemText(m_hWnd, IDC_EDIT_PWD, di.szPWD);
	::SetDlgItemInt(m_hWnd, IDC_EDIT_RETRY, di.uRETRY, FALSE);
	::SetDlgItemInt(m_hWnd, IDC_EDIT_RTWAIT, di.uRTWAIT, FALSE);
	::CheckDlgButton(m_hWnd, IDC_CHECK_AUTOCON, di.uSCON);
	::CheckDlgButton(m_hWnd, IDC_CHECK_FORCE, di.uFORCE);
	::CheckDlgButton(m_hWnd, IDC_CHECK_EXEC, di.uEXEC);
	::SetDlgItemText(m_hWnd, IDC_EDIT_EXECPATH, di.szEXECPATH);
	ExecPathEnable();
}

// ���s�R�}���h���͗�
void EntryDlg::ExecPathEnable()
{
	if(::IsDlgButtonChecked(m_hWnd,IDC_CHECK_EXEC) == BST_UNCHECKED){
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_EDIT_EXECPATH), FALSE);
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_EXECPATH), FALSE);
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_EXECUTE), FALSE);
	}else{
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_EDIT_EXECPATH), TRUE);
		::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_EXECPATH), TRUE);
		if(::GetWindowTextLength(::GetDlgItem(m_hWnd,IDC_EDIT_EXECPATH))){
			::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_EXECUTE), TRUE);
		}else{
			::EnableWindow(::GetDlgItem(m_hWnd,IDC_BUTTON_EXECUTE), FALSE);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////
//
// DMApp �N���X
//
//////////////////////////////////////////////////////////////////////////////////////

DMApp* DMApp::m_pApp = NULL;

// �R���X�g���N�^
DMApp::DMApp(HINSTANCE hInstance, LPSTR lpCmdLine, int nShowCmd) :
MApp(hInstance, lpCmdLine, nShowCmd)
{
	m_inifnsize = sizeof(m_szModuleDir)/sizeof(TCHAR)+1+::lstrlen(DM_INIFILE)+UNLEN+1;
	m_lpIniFile = new TCHAR[m_inifnsize+1];

	::ZeroMemory(m_di, sizeof(m_di));

	if(!DrvMount::GetIniFileName(hInstance,m_lpIniFile,m_inifnsize)){
		*m_lpIniFile = TEXT('\0');
		MsgBox(NULL, MB_ICONERROR|MB_OK, DM_APPSECTION, TEXT("�ݒ���t�@�C���̎擾�Ɏ��s�I�I"));
	}
}

// �f�X�g���N�^
DMApp::~DMApp()
{
	delete [] m_lpIniFile;
}

// �C���X�^���X����
DMApp* DMApp::getInstance(HINSTANCE hInst, LPSTR lpCmdLine, int nShowCmd)
{
	static DMApp dmapp(hInst, lpCmdLine, nShowCmd);
	m_pApp = &dmapp;
	return m_pApp;
}

// ���C��
int DMApp::Main()
{
	if(!(*m_lpIniFile)) return 2;
	MainDlg dlg;
	return dlg.Create(m_hInst, MAKEINTRESOURCE(IDD_MAINDLG));
}
