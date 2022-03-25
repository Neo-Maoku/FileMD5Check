// MainDialog.cpp: 实现文件
//

#include "pch.h"
#include "FileMD5Check.h"
#include "MainDialog.h"
#include "afxdialogex.h"
#include "Volume.h"
#include "md5.h"

ThreadPool pool(thread::hardware_concurrency() * 2);
void checkFileMD5(vector<File_Info>::iterator file, MainDialog *dialog);
BOOL request(CString content);
void insertRiskData();
void updateFileInfoDB();
CString getInputText(CString lpszStringBuf, CString *dir);
bool AdjustPrivileges();
wstring string2wstring(string str);
string wstring2string(wstring wstr);
string unicode2accsi(wstring wstr);
static int CALLBACK MyCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

struct MyData
{
	CListCtrl* List;
	bool asc;
	int iSubItem;
};

IMPLEMENT_DYNAMIC(MainDialog, CDialogEx)

MainDialog::MainDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{
	m_DlgRect.SetRect(0, 0, 0, 0);
	hAccKey = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1));
}

BOOL MainDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	HICON m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	is_have_db = initdata.init();
	num = initdata.getJ();
	m_pro.SetRange(0, num * 100);

	init_ui();

	CString str[] = {TEXT("文件名"), TEXT("文件路径") ,TEXT("大小"),TEXT("修改时间"),TEXT("MD5"), TEXT("信誉值")};
	int length[] = {300-25, 500-25, 115, 150, 325, 70};
	
	for (int i = 0; i < 6; i++)
	{
		m_list.InsertColumn(i, str[i], LVCFMT_LEFT, length[i]);
	}
	m_list.SetExtendedStyle(m_list.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);//整行+网格

	pool.init();

	pThread = AfxBeginThread(getRiskData, (LPVOID)this);

	return TRUE;
}

UINT MainDialog::showData(LPVOID pParam)
{
	MainDialog * pObj = (MainDialog*)pParam;

	pObj->setDialogIsEnabled(false);

	pObj->GetDlgItem(IDC_SHOWPRO)->SetWindowText(_T("数据加载中..."));

	int length = initdata.sql_select(TEXT(""), TEXT(""), TEXT(""));
	pObj->m_list.SetItemCount(length);
	CString temp;
	temp.Format(_T("共有%d项\n"), length);
	pObj->GetDlgItem(IDC_SHOWPRO)->SetWindowText(temp);

	pObj->GetDlgItem(IDC_BUTTON1)->ShowWindow(true);
	pObj->GetDlgItem(IDC_BUTTON4)->ShowWindow(false);

	pObj->setDialogIsEnabled(true);

	return 0;
}

UINT MainDialog::getRiskData(LPVOID pParam)
{
	initdata.getRiskDataFormDB();

	return 0;
}

void MainDialog::init_ui()
{
	CRect temprect(0, 0, 1440+100, 810+30);
	CWnd::SetWindowPos(NULL, 0, 0, temprect.Width(), temprect.Height(), SWP_NOZORDER | SWP_NOMOVE);

	CRect m_rect;
	GetClientRect(&m_rect);

	int height = 50;
	int top = 30;
	int btn_width = 130;
	int margin = 30;
	int label_width = 150;
	
	setWndPos(IDC_STATIC, CRect(margin, top, label_width, height));

	setWndPos(IDC_BUTTON1, CRect(m_rect.right - 3*margin - 3*btn_width, top, btn_width, height));
	setWndPos(IDC_BUTTON4, CRect(m_rect.right - 3 * margin - 3 * btn_width, top, btn_width, height));

	setWndPos(IDC_BUTTON2, CRect(m_rect.right - 2*margin - 2*btn_width, top, btn_width, height));

	setWndPos(IDC_BUTTON3, CRect(m_rect.right - margin - btn_width, top, btn_width, height));

	setWndPos(IDC_STR, CRect(2* margin + label_width, top, m_rect.Width() - 6*margin - 3* btn_width- label_width, height));

	setWndPos(IDC_LIST1, CRect(margin, 2*top + height, m_rect.Width() - 2 * margin, m_rect.Height() - height - 3*margin));

	setWndPos(IDC_SHOWPRO, CRect(margin, m_rect.Height() - margin, label_width+50, margin));

	setWndPos(IDC_PROGRESS1, CRect(margin + label_width+50, m_rect.Height() - margin, label_width, margin));

	if (is_have_db) {
		GetDlgItem(IDC_BUTTON1)->ShowWindow(true);
		GetDlgItem(IDC_BUTTON4)->ShowWindow(false);
	}
	else
	{
		GetDlgItem(IDC_BUTTON1)->ShowWindow(FALSE);
		GetDlgItem(IDC_BUTTON4)->ShowWindow(TRUE);
		CEdit* edit = (CEdit *)GetDlgItem(IDC_STR);
		edit->SetReadOnly(false);
	}
}

void MainDialog::setWndPos(UINT id, CRect rect)
{
	CWnd *wnd = NULL;
	wnd = GetDlgItem(id);

	rect.left += 0;
	rect.right += (rect.left);
	rect.top += 0;
	rect.bottom += (rect.top);
	wnd->MoveWindow(&rect);
}

UINT MainDialog::initThread(LPVOID pParam) {
	MainDialog * pObj = (MainDialog*)pParam;
	if (pObj) {
		return pObj->realThread(NULL);
	}
	return false;
}

UINT MainDialog::realThread(LPVOID pParam) {
	char *pvol = initdata.getVol();

	setDialogIsEnabled(false);
	m_pro.ShowWindow(SW_SHOW);

	for (int i = 0; i < num; ++i) {
		CString showpro(_T("正在统计"));
		showpro += pvol[i];
		showpro += _T("盘文件...");
		GetDlgItem(IDC_SHOWPRO)->SetWindowText(showpro);
		initdata.initvolumelist((LPVOID)pvol[i]);
		showProsess((i + 1) * 100);
	}

	m_pro.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SHOWPRO)->SetWindowText(_T("统计完毕,加载数据中"));

	int length = initdata.sql_select(TEXT(""), TEXT(""), TEXT(""));
	
	CString temp;
	temp.Format(_T("共有%d项\n"), length);
	GetDlgItem(IDC_SHOWPRO)->SetWindowText(temp);

	GetDlgItem(IDC_BUTTON1)->ShowWindow(true);
	GetDlgItem(IDC_BUTTON4)->ShowWindow(false);

	m_list.SetItemCount(length);
	setDialogIsEnabled(true);

	return 0;
}

void MainDialog::setDialogIsEnabled(BOOL isEnabled)
{
	if (isEnabled)
	{
		search_btn.EnableWindow(true);
		md5Check_btn.EnableWindow(true);
		disk_file_btn.EnableWindow(true);
		import_btn.EnableWindow(true);
		CEdit* edit = (CEdit *)GetDlgItem(IDC_STR);
		edit->SetReadOnly(false);
	}
	else
	{
		search_btn.EnableWindow(false);
		md5Check_btn.EnableWindow(false);
		disk_file_btn.EnableWindow(false);
		import_btn.EnableWindow(false);
		CEdit* edit = (CEdit *)GetDlgItem(IDC_STR);
		edit->SetReadOnly(true);
	}
}

void MainDialog::showProsess(int end) {
	for (int i = 0; i <= end; i += 5) {
		m_pro.SetPos(i);
	}
}

MainDialog::~MainDialog()
{
	initdata.clear();
	pool.shutdown();
}

void MainDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STR, m_str);
	DDX_Control(pDX, IDC_PROGRESS1, m_pro);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_BUTTON1, search_btn);
	DDX_Control(pDX, IDC_BUTTON2, md5Check_btn);
	DDX_Control(pDX, IDC_SHOWPRO, hintText);
	DDX_Control(pDX, IDC_BUTTON4, disk_file_btn);
	DDX_Control(pDX, IDC_BUTTON3, import_btn);
}

BEGIN_MESSAGE_MAP(MainDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &MainDialog::OnBnClickedFind)
	ON_BN_CLICKED(IDC_BUTTON2, &MainDialog::OnMD5CheckBnClicked)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST1, GetDispInfo)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &MainDialog::OnNMRClickList1)
	ON_COMMAND(ID_32772, &MainDialog::OnCopyMD5)
	ON_COMMAND(ID_32774, &MainDialog::OnCopyLine)
	ON_COMMAND(ID_32778, &MainDialog::OnOpenFilePath)
	ON_COMMAND(ID_32775, &MainDialog::OnExportAllData)
	ON_COMMAND(ID_32776, &MainDialog::OnExportRiskData)
	ON_WM_SIZE()
	ON_COMMAND(ID_32782, &MainDialog::OnFileAttr)
	ON_BN_CLICKED(IDC_BUTTON3, &MainDialog::OnImportClicked)
	ON_BN_CLICKED(IDC_BUTTON4, &MainDialog::OnGetDiskFileClicked)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, &MainDialog::OnLvnColumnclickList1)
END_MESSAGE_MAP()

void MainDialog::GetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &(pDispInfo)->item;
	int iItemIndx = pItem->iItem;

	if (pItem->mask & LVIF_TEXT)
	{
		switch (pItem->iSubItem)
		{

		case 0:
			lstrcpyn(pItem->pszText, initdata.files[iItemIndx].fileName, pItem->cchTextMax);
			break;
		case 1:
			lstrcpyn(pItem->pszText, initdata.files[iItemIndx].filePath, pItem->cchTextMax);
			break;
		case 2:
			lstrcpyn(pItem->pszText, initdata.files[iItemIndx].fileSize, pItem->cchTextMax);
			break;
		case 3:
			lstrcpyn(pItem->pszText, initdata.files[iItemIndx].fileChangeTime, pItem->cchTextMax);
			break;
		case 4:
			lstrcpyn(pItem->pszText, initdata.files[iItemIndx].fileMD5, pItem->cchTextMax);
			break;
		case 5:
			lstrcpyn(pItem->pszText, initdata.files[iItemIndx].fileRisk, pItem->cchTextMax);
			break;
		default:
			break;
		}
	}

	*pResult = 0;
}

void MainDialog::OnGetDiskFileClicked()
{
	if (!is_have_db) {
		pThread = AfxBeginThread(initThread, (LPVOID)this);
	}
	else {
		AfxBeginThread(showData, (LPVOID)this);
	}
}

CString getInputText(CString lpszStringBuf, CString *dir)
{
	string b = CW2A(lpszStringBuf.GetString());
	string tempstr = "";

	string str("[a-zA-Z]:([\\\\/][^\\s\\\\/:*?<>\"|][^\\\\/:*?<>\"|]*)*([/\\\\])?");
	
	smatch result;
	regex reg(str);
	string::const_iterator iterStart = b.begin();
	string::const_iterator iterEnd = b.end();
	if (regex_search(iterStart, iterEnd, result, reg)) {
		*dir = result.str(0).c_str();
		string prefixStr = result.prefix().str();
		prefixStr.erase(prefixStr.find_last_not_of(" ") + 1);
		if (prefixStr != "")
			tempstr = prefixStr;

		string suffixStr = result.suffix().str();
		suffixStr.erase(0, suffixStr.find_first_not_of(" "));
		if (suffixStr != "")
			tempstr = suffixStr;
	}
	else
	{
		tempstr = b;
	}

	return CA2T(tempstr.c_str());
}

void MainDialog::OnBnClickedFind()
{
	CString lpszStringBuf;

	GetDlgItem(IDC_SHOWPRO)->SetWindowText(_T("正在检索..."));
	setDialogIsEnabled(false);

	m_str.GetWindowText(lpszStringBuf);
	lpszStringBuf.TrimLeft();
	lpszStringBuf.TrimRight();

	CString dir;
	CString searchText = getInputText(lpszStringBuf, &dir);

	int length = initdata.sql_select(searchText, dir, TEXT(""));
	m_list.SetItemCount(length);
	CString temp;
	temp.Format(_T("共有%d项\n"), length);
	GetDlgItem(IDC_SHOWPRO)->SetWindowText(temp);
	setDialogIsEnabled(true);
}

void MainDialog::OnMD5CheckBnClicked()
{
	setDialogIsEnabled(false);

	listSumCount = initdata.files.size();
	listFinishCount = 0;

	pThread = AfxBeginThread(checkMD5, (LPVOID)this);
	//CloseHandle(pThread);
}

void MainDialog::OnImportClicked()
{
	CFileDialog fileOpenDlg(TRUE, _T("*.xlsx"), NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,
		_T("csv file(*.csv)|*.csv|Microsoft Excel 工作表(*.xlsx)|*.xlsx|All files (*.*)|*.*||"), NULL);
	fileOpenDlg.m_ofn.lpstrTitle = _T("Open File");
	if (fileOpenDlg.DoModal() == IDOK)//导入对话框打开
	{
		CString szLine;
		CStdioFile file;
		if (file.Open(fileOpenDlg.GetPathName(), CFile::modeRead))//打开选中的文件
		{
			is_import = true;
			initdata.files.clear();
			std::regex reg("\"(.*?)\",\"(.*?)\",\"(.*?)\",(.*?),(.*?),(.*?)$");
			smatch result;

			file.ReadString(szLine);

			while (file.ReadString(szLine))
			{
				string temp = unicode2accsi((wstring)(szLine));
				string::const_iterator iterStart = temp.begin();
				string::const_iterator iterEnd = temp.end();

				regex_search(iterStart, iterEnd, result, reg);
				File_Info temp1;

				temp1.fileName = UTF8ToUnicode(result.str(1).c_str());
				temp1.filePath = UTF8ToUnicode(result.str(2).c_str());
				temp1.fileMD5 = CString(result.str(5).c_str());
				temp1.fileRisk = CString(result.str(6).c_str());
				temp1.fileSize = CString(result.str(3).c_str());
				temp1.fileChangeTime = CString(result.str(4).c_str());

				initdata.files.push_back(temp1);
			}

			m_list.SetItemCount(initdata.files.size());
			CString temp;
			temp.Format(_T("共有%d项\n"), initdata.files.size());
			GetDlgItem(IDC_SHOWPRO)->SetWindowText(temp);
		}
	}
}

UINT MainDialog::checkMD5(LPVOID pParam) {
	MainDialog * pObj = (MainDialog*)pParam;
	
	int length = initdata.files.size();
	std::future<void> *future = new std::future<void>[length];

	bool hasFuture = false;
	int index = 0;
	for (vector<File_Info>::iterator vstrit = initdata.files.begin();
		vstrit != initdata.files.end(); ++vstrit) {
		future[index++] = pool.submit(checkFileMD5, vstrit, pObj);
		hasFuture = true;
	}

	if (hasFuture)
	{
		for (int i = 0; i < length; i++)
		{
			future[i].get();
		}
		
		hasFuture = false;
	}

	delete[] future;
	pObj->m_list.SetItemCount(pObj->listSumCount);
	pObj->GetDlgItem(IDC_SHOWPRO)->SetWindowText(_T("文件信誉请求中..."));

	length = ceil(initdata.fileRisks.size() / 1000.0);
	std::future<BOOL> *future1 = new std::future<BOOL>[length];
	int count = 0;
	int pool_index = 0;
	CString content;
	for (vector<vector<File_Info>::iterator>::iterator vstrit = initdata.fileRisks.begin();
		vstrit != initdata.fileRisks.end(); ++vstrit) {
		vector<File_Info>::iterator temp = vstrit[0];

		if (count >= 1000)
		{
			future1[pool_index++] = pool.submit(request, content);
			count = 0;
			content = _TEXT("");
			hasFuture = true;
		}
		else {
			if (count == 0)
				content += temp->fileMD5;
			else
				content = content + _T("\n") + temp->fileMD5;
			count++;
		}
	}
	if (count > 0)
	{
		future1[pool_index] = pool.submit(request, content);
		hasFuture = true;
	}

	if (hasFuture)
	{
		BOOL flag;
		for (int i = 0; i <= pool_index; i++)
		{
			flag = future1[i].get();
		}
		hasFuture = false;

		if (!flag)
		{
			AfxMessageBox(_T("无法连接服务器，请检查网络环境"));
		}
	}

	delete[] future1;

	for (vector<vector<File_Info>::iterator>::iterator vstrit = initdata.fileRisks.begin();
		vstrit != initdata.fileRisks.end(); ++vstrit) {
		vstrit[0]->fileRisk = initdata.file_risk[vstrit[0]->fileMD5];
	}
	pObj->m_list.SetItemCount(pObj->listSumCount);
	CString temp;
	temp.Format(_T("共有%d项\n"), pObj->listSumCount);
	pObj->GetDlgItem(IDC_SHOWPRO)->SetWindowText(temp);
	pObj->setDialogIsEnabled(true);

	//更新数据库
	pool.submit(insertRiskData);
	if (!is_import)
		pool.submit(updateFileInfoDB);

	return false;
}

void insertRiskData()
{
	initdata.insertRiskData();
}

void updateFileInfoDB()
{
	initdata.updateFileInfoDB();
}

BOOL request(CString content)
{
	return initdata.file_risk_request(content);
}

void checkFileMD5(vector<File_Info>::iterator file, MainDialog *dialog)
{
	if (file->fileRisk != "")
		int a = 0;
	else if (file->fileMD5 != "")
	{
		initdata.g_criSection.Lock();
		initdata.fileRisks.push_back(file);
		initdata.g_criSection.Unlock();
		return;
	}
	else {
		string b = CW2A((file->filePath + file->fileName).GetString());
		CString s(MD5Sum((char*)(b.c_str())));
		if (s != "")
		{
			file->fileMD5 = s.MakeUpper();
			CString risk = initdata.file_risk[file->fileMD5];
			if (risk != "")
				file->fileRisk = risk;
			else {
			initdata.g_criSection.Lock();
			initdata.fileRisks.push_back(file);
			initdata.g_criSection.Unlock();
			}
		}
		else {
			file->fileMD5 = "unknown";
		}
	}
	
	initdata.g_countSection.Lock();

	dialog->listFinishCount++;
	CString text;
	text.Format(_T("文件MD5计算中...%d/%d"), dialog->listFinishCount, dialog->listSumCount);
	dialog->hintText.SetWindowText(text);

	initdata.g_countSection.Unlock();
}

void MainDialog::OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (initdata.files.size() == 0)
		return;

	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	POINT pt;
	GetCursorPos(&pt);
	int x = m_list.GetSelectionMark();

	CMenu menu;
	menu.LoadMenu(IDR_MENU2);
	CMenu * pop = menu.GetSubMenu(0);
	pop->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this);

	*pResult = 0;
}

void MainDialog::OnCopyMD5()
{
	CString str;
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos)
	{
		int nItem = m_list.GetNextSelectedItem(pos);
		str = str + initdata.files[nItem].fileMD5 + TEXT("\n");
	}

	copyContext2Clipboard(str);
}

void MainDialog::copyContext2Clipboard(CString Context)
{
	if (OpenClipboard())
	{
		EmptyClipboard(); // 清空剪切板
		USES_CONVERSION;
		string b = CW2A(Context.GetString());

		HGLOBAL hMem = GlobalAlloc(GHND, b.length()); // 分配全局内存
		char *pmen = (char *)GlobalLock(hMem);    // 锁定全局内存
		memcpy(pmen, b.c_str(), b.length());    // 向全局内存中拷贝数据
		SetClipboardData(CF_TEXT, hMem);        // 设置数据到剪切板
		CloseClipboard();                        // 关闭剪切板
		GlobalFree(hMem);                    // 释放全局内存
	}
}

void MainDialog::OnCopyLine()
{
	CString str;
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	while (pos)
	{
		int nItem = m_list.GetNextSelectedItem(pos);
		File_Info vstrit = initdata.files[nItem];
		CString temp;
		temp.Format(_T("%s	%s	%s	%s	%s	%s\n"), vstrit.fileName, vstrit.filePath, vstrit.fileSize, vstrit.fileChangeTime, vstrit.fileMD5, vstrit.fileRisk);
		str += temp;
	}

	copyContext2Clipboard(str);
}

void MainDialog::OnOpenFilePath()
{
	if (is_import)
	{
		AfxMessageBox(_T("导入文件的路径不可打开"));
		return;
	}

	POSITION pos = m_list.GetFirstSelectedItemPosition();

	while (pos)
	{
		int nItem = m_list.GetNextSelectedItem(pos);
		CString filePath = initdata.files[nItem].filePath;
		struct stat buffer;

		if (stat(CW2A(filePath.GetString()), &buffer) == 0)
			ShellExecute(NULL, NULL, _T("explorer"), filePath, NULL, SW_SHOW);
		else
			AfxMessageBox(_T("文件路径不存在"));
	}
}

void MainDialog::OnExportAllData()
{
	CFileDialog dlg(FALSE, _T("*.csv"), _T("file_info_data.csv"));//FALSE表示为“另存为”对话框，否则为“打开”对话框
	if (dlg.DoModal() == IDOK)
	{
		CString strFile = dlg.GetPathName();//获取完整路径
		CStdioFile file;
		if (file.Open(strFile, CStdioFile::modeCreate | CStdioFile::modeWrite| CFile::typeBinary))
		{
			//WORD unicodeFlag = 0xFEFF; //文件采用unicode格式
			//file.Write((void*)&unicodeFlag, sizeof(WORD));

			char szUTF_8BOM[4] = { 0xEF, 0xBB, 0xBF, 0}; //utf-8
			file.Write(szUTF_8BOM, 4);

			CString txt = _T("文件路径,大小,修改时间,MD5, 信誉值\n");
			file.Write((Unicode2Utf8((wstring)txt)).c_str(), (Unicode2Utf8((wstring)txt)).length());

			CString temp;
			for (vector<File_Info>::iterator vstrit = initdata.files.begin();
				vstrit != initdata.files.end(); ++vstrit) {
				if (vstrit->fileMD5 != "unknown")
				{
					temp.Format(_T("\"%s\",\"%s\",\"%s\",%s,%s,%s\n"), vstrit->filePath, vstrit->fileName, vstrit->fileSize, vstrit->fileChangeTime, vstrit->fileMD5, vstrit->fileRisk);
					
					file.Write((Unicode2Utf8((wstring)temp)).c_str(), (Unicode2Utf8((wstring)temp)).length());
				}
			}
			file.Close();
		}
	}
}

void MainDialog::OnExportRiskData()
{
	CFileDialog dlg(FALSE, _T("*.csv"), _T("risk_info_data.csv"));//FALSE表示为“另存为”对话框，否则为“打开”对话框
	if (dlg.DoModal() == IDOK)
	{
		CString strFile = dlg.GetPathName();//获取完整路径
		CStdioFile file;

		if (file.Open(strFile, CStdioFile::modeCreate | CFile::typeBinary | CStdioFile::modeWrite))
		{
			char szUTF_8BOM[4] = { 0xEF, 0xBB, 0xBF, 0 }; //utf-8
			file.Write(szUTF_8BOM, 4);

			CString txt = _T("文件路径,大小,修改时间,MD5, 信誉值\n");
			file.Write((Unicode2Utf8((wstring)txt)).c_str(), (Unicode2Utf8((wstring)txt)).length());

			CString temp;
			for (vector<File_Info>::iterator vstrit = initdata.files.begin();
				vstrit != initdata.files.end(); ++vstrit) {
				if (vstrit->fileRisk != "" && (vstrit->fileRisk) >= _T("40")) {
					temp.Format(_T("\"%s\",\"%s\",\"%s\",%s,%s,%s\n"), vstrit->filePath, vstrit->fileName, vstrit->fileSize, vstrit->fileChangeTime, vstrit->fileMD5, vstrit->fileRisk);

					file.Write((Unicode2Utf8((wstring)temp)).c_str(), (Unicode2Utf8((wstring)temp)).length());
				}
			}
			file.Close();
		}
	}
}

BOOL MainDialog::PreTranslateMessage(MSG* pMsg)
{
	if (TranslateAccelerator(m_hWnd, hAccKey, pMsg)) {
		CWnd* a= GetDlgItem(IDC_STR);
		if (*a != m_str)
			return true;
	}
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && pMsg->wParam) {
		OnBnClickedFind();
		return true;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void MainDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (0 == m_DlgRect.left && 0 == m_DlgRect.right
		&& 0 == m_DlgRect.top && 0 == m_DlgRect.bottom)//第一次启动对话框时的大小变化不作处理
	{
	}
	else
	{
		if (0 == cx && 0 == cy)//若是是按下了最小化，则触发条件，这时不保存对话框数据
		{
			return;
		}
		CRect rectDlgChangeSize;
		GetClientRect(&rectDlgChangeSize);//存储对话框大小改变后对话框大小数据

		HWND  hwndChild = ::GetWindow(m_hWnd, GW_CHILD);  //列出所有控件
		CRect Rect;
		int woc;
		while (hwndChild) {
			woc = ::GetDlgCtrlID(hwndChild);//取得ID  
			repaint(woc, m_DlgRect.Width(), rectDlgChangeSize.Width(), m_DlgRect.Height(), rectDlgChangeSize.Height());//重绘函数，用以更新对话框上控件的位置和大小
			hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
		}
	}
	GetClientRect(&m_DlgRect); //save size of dialog
	Invalidate();//更新窗口
}

void MainDialog::repaint(UINT id, int last_Width, int now_Width, int last_Height, int now_Height)//更新控件位置和大小函数，能够根据须要自行修改
{
	CRect rect;
	CWnd *wnd = NULL;
	wnd = GetDlgItem(id);
	if (NULL == wnd)
	{
		MessageBox(_T("相应控件不存在"));
	}
	wnd->GetWindowRect(&rect);
	ScreenToClient(&rect);

	double proportion_x = now_Width / (double)last_Width;
	double proportion_y = now_Height / (double)last_Height;

	rect.left = (long)(rect.left*proportion_x + 0.5);
	rect.right = (long)(rect.right*proportion_x + 0.5);
	rect.top = (long)(rect.top *proportion_y + 0.5);
	rect.bottom = (long)(rect.bottom *proportion_y + 0.5);
	wnd->MoveWindow(&rect);
}

void MainDialog::OnFileAttr()
{
	CString path;
	POSITION pos = m_list.GetFirstSelectedItemPosition();
	int nItem = m_list.GetNextSelectedItem(pos);
	File_Info vstrit = initdata.files[nItem];

	path = vstrit.filePath + vstrit.fileName;

	SHELLEXECUTEINFO sei;
	ZeroMemory(&sei, sizeof(sei)); sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_INVOKEIDLIST;
	sei.lpVerb = _T("properties");
	sei.lpFile = path;
	sei.nShow = SW_SHOWDEFAULT;
	ShellExecuteEx(&sei);
}

bool AdjustPrivileges() {
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tp;
	TOKEN_PRIVILEGES oldtp;
	DWORD dwSize = sizeof(TOKEN_PRIVILEGES);
	LUID luid;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken);

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
		CloseHandle(hToken);
		OutputDebugString(TEXT("提升权限失败,LookupPrivilegeValue"));
		return false;
	}
	ZeroMemory(&tp, sizeof(tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	/* Adjust Token Privileges */
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize)) {
		CloseHandle(hToken);
		OutputDebugString(TEXT("提升权限失败 AdjustTokenPrivileges"));
		return false;
	}
	// close handles
	CloseHandle(hToken);
	return true;
}

wstring string2wstring(string str)
{
	wstring result;
	//获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
	TCHAR* buffer = new TCHAR[len + 1];
	//多字节编码转换成宽字节编码  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);
	buffer[len] = '\0';             //添加字符串结尾  
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

string wstring2string(wstring wstr)
{
	string result;
	//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	//宽字节编码转换成多字节编码  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

string unicode2accsi(wstring wstr)
{
	//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	string buffer;
	//宽字节编码转换成多字节编码  
	//WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	DWORD addr = (DWORD)(&wstr)+4;
	for (int i = 0; i < len *2; i=i+2)
	{
		DWORD tt = *(DWORD *)addr;
		char ch = *(CHAR *)(tt + i);
		buffer += ch;
	}

	buffer[len] = '\0';

	return buffer;
}


void MainDialog::OnLvnColumnclickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (isSorting)
		return;

	isSorting = true;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	LPNMLISTVIEW listView = (LPNMLISTVIEW)pNMHDR;

	GetDlgItem(IDC_SHOWPRO)->SetWindowText(_T("正在排序中..."));

	int index = listView->iSubItem;//将子列索引增加到排序器中

	int length = initdata.sql_select_sort(index, asc[index]);

	m_list.SetItemCount(length);

	CString temp;
	temp.Format(_T("共有%d项\n"), length);
	GetDlgItem(IDC_SHOWPRO)->SetWindowText(temp);

	//MyData mydata;
	//mydata.asc = this->asc[index];
	//mydata.iSubItem = index;//将项目索引增加到排序器中
	//mydata.List = &m_list;
	//m_list.SortItems((PFNLVCOMPARE)MyCompareProc, (DWORD_PTR)&mydata);
	asc[index] = !asc[index];
	isSorting = false;

	*pResult = 0;
}

//虚表不能使用该方式排序
static int CALLBACK MyCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	MyData* data = (MyData*)lParamSort;
	CListCtrl* list = data->List;
	LVFINDINFO fi;
	fi.flags = LVFI_PARAM;
	fi.lParam = lParam1;
	LVFINDINFO fi1;
	fi1.flags = LVFI_PARAM;
	fi1.lParam = lParam2;
	int index1 = list->FindItem(&fi);//从LVITEM结构中的lParam成员来查找对应项的索引号
	int index2 = list->FindItem(&fi1);
	CString text1 = list->GetItemText(index1, data->iSubItem);//得到指定行和列号的单元格文本
	CString text2 = list->GetItemText(index2, data->iSubItem);
	LPTSTR txt1 = text1.GetBuffer();//以下七句用于将CString转化成const char*
	LPTSTR txt2 = text2.GetBuffer();
	USES_CONVERSION;
	const   char*   pSth1 = T2A(txt1);
	const   char*   pSth2 = T2A(txt2);
	text1.ReleaseBuffer();
	text2.ReleaseBuffer();
	if (data->asc)
	{
		return strcmp(pSth2, pSth1);
	}
	else
	{
		return strcmp(pSth1, pSth2);
	}
	return 0;
}

