class MainDialog : public CDialogEx
{
	DECLARE_DYNAMIC(MainDialog)

public:
	MainDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~MainDialog();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif
public:
	int num;
	CWinThread* pThread;
	CWinThread* pThread1;
	CProgressCtrl m_pro;
	CEdit m_str;
	int listSumCount;
	int listFinishCount;
	bool asc[6];
	bool isSorting;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	static UINT initThread(LPVOID pParam);
	static UINT checkMD5(LPVOID pParam);
	static UINT getRiskData(LPVOID pParam);
	static UINT showData(LPVOID pParam);
	UINT realThread(LPVOID pParam);
	void showProsess(int i);
	afx_msg void OnBnClickedFind();
	CListCtrl m_list;
	afx_msg void OnMD5CheckBnClicked();
	void MainDialog::GetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCopyMD5();
	void copyContext2Clipboard(CString Context);
	afx_msg void OnCopyLine();
	afx_msg void OnOpenFilePath();
	afx_msg void OnExportAllData();
	afx_msg void OnExportRiskData();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	CRect m_DlgRect;
	bool is_have_db;
private:
	HACCEL hAccKey;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CButton search_btn;
	CButton md5Check_btn;
	void repaint(UINT id, int last_Width, int now_Width, int last_Height, int now_Height);
	void init_ui();
	void setWndPos(UINT id, CRect rect);
	void setDialogIsEnabled(BOOL isEnabled);
	CStatic hintText;
	afx_msg void OnFileAttr();
	afx_msg void OnImportClicked();
	afx_msg void OnGetDiskFileClicked();
	CButton disk_file_btn;
	CButton import_btn;
	afx_msg void OnLvnColumnclickList1(NMHDR *pNMHDR, LRESULT *pResult);
};
