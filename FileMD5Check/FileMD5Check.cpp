
// FileMD5Check.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "FileMD5Check.h"
#include "MainFrm.h"
#include "MainDialog.h"
#include "Shlobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFileMD5CheckApp


// CFileMD5CheckApp 构造

CFileMD5CheckApp::CFileMD5CheckApp() noexcept
{
	// TODO: 将以下应用程序 ID 字符串替换为唯一的 ID 字符串；建议的字符串格式
	//为 CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("FileMD5Check.AppID.NoVersion"));

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}

// 唯一的 CFileMD5CheckApp 对象

CFileMD5CheckApp theApp;


// CFileMD5CheckApp 初始化

BOOL CFileMD5CheckApp::InitInstance()
{
	if (IsUserAnAdmin() == FALSE)
	{
		TCHAR path[MAX_PATH] = { 0 }; // 需要初始化
		DWORD dwPathSize = MAX_PATH;
		QueryFullProcessImageName(GetCurrentProcess(), 0,
			path,
			&dwPathSize);

		// 2 调用创建进程的API运行本进程.
		ShellExecute(NULL,            // 窗口句柄,没有则填NULL
			_T("runas"),   // 以管理员身份运行的重要参数
			path,            // 所有运行的程序的路径(这里是本进程)
			NULL,            // 命令行参数
			NULL,            // 新进程的工作目录的路径
			SW_SHOW           // 创建后的显示标志(最小化,最大化, 显示,隐藏等)
		);

		// 退出本进程
		ExitProcess(0);
	}

	CreateMutex(NULL, TRUE, TEXT("CFileMD5CheckApp")); //避免程序的多开  xxxx为信号量的名字 可随意
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CWnd* cwnd = CWnd::FindWindow(NULL, TEXT("FileMD5Check"));//windowname为你的主窗体的标题,当然你也可以通过进程来找到主窗体。
		if (cwnd)//显示原先的主界面
		{
			cwnd->ShowWindow(SW_SHOWNORMAL);
			cwnd->SetForegroundWindow();
		}
		return FALSE;
	}
	CWinApp::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// 使用 RichEdit 控件需要 AfxInitRichEdit2()
	// AfxInitRichEdit2();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));


	// 若要创建主窗口，此代码将创建新的框架窗口
	// 对象，然后将其设置为应用程序的主窗口对象
	MainDialog aboutDlg;
	aboutDlg.DoModal();
	return TRUE;
}

int CFileMD5CheckApp::ExitInstance()
{
	//TODO: 处理可能已添加的附加资源
	return CWinApp::ExitInstance();
}

// CFileMD5CheckApp 消息处理程序



