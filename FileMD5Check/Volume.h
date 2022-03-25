#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <Winioctl.h>
#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <afx.h>
#include <regex>
#include <shlwapi.h>
#include <afxinet.h>
#include <sstream>
#include "ThreadPool.h"
#include <thread>
#include "afxmt.h"
#include "sqlite3.h"
#pragma comment(lib,"sqlite3.lib")

#define SQL_CREATE_TABLE "Create table if not exists file_info(ID INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,fileName string, filePath string, fileMD5 string, fileRisk string, fileSize string, fileChangeTime string)" 
#define SQL_INSERT_DATA "Insert into file_info (fileName, filePath, fileMD5, fileRisk, fileSize, fileChangeTime) values ('%s', '%s', '%s', '%s', '%s', '%s');"
#define SQL_QUERY_DATA "Select * from file_info"
#define SQL_SELECT_DATA_1 "Select * from file_info where fileName LIKE '%s'"
#define SQL_SELECT_DATA_2 "Select * from file_info where fileName LIKE '%s' AND filePath LIKE '%s'"
#define SQL_SELECT_DATA_3 "Select * from file_info where filePath LIKE '%s'"
#define SQL_CREATE_RISK_TABLE "Create table if not exists file_risk(fileMD5 string PRIMARY KEY, fileRisk string)"
#define SQL_INSERT_RISK_DATA "Insert into file_risk (fileMD5, fileRisk) values ('%s', '%s');"
#define SQL_UPDATE_FILE_DATA "UPDATE file_info SET fileMD5 = '%s',fileRisk = '%s' WHERE ID = %d;"
#define SQL_QUERY_RISK_DATA "Select * from file_risk"
#define SQL_QUERY_SORT " ORDER BY %s %s"

using namespace std;
std::string Unicode2Utf8(const std::wstring& widestring);
sqlite3 *m_pDB;
sqlite3 *risk_pDB;
char file_db_path[255];
BOOL isCreateDBSucc = true;
ThreadPool insert_pool(thread::hardware_concurrency() * 2);
void insert_fileInfo(int beginIndex, int endIndex, LPVOID pParam);
CCriticalSection g_insertSection;
CCriticalSection g_frnSection;
BOOL is_import = false;

LARGE_INTEGER nFreq;
LARGE_INTEGER tStart;
LARGE_INTEGER tMid;
LARGE_INTEGER tEnd;

typedef struct _name_cur {
	CString filename;
	DWORDLONG pfrn;
}Name_Cur;

typedef struct _file_info {
	CString fileName;
	CString filePath;
	CString fileMD5;
	CString fileRisk;
	int ID;
	CString fileSize;
	CString fileChangeTime;
}File_Info;

typedef struct _pfrn_name {
	DWORDLONG pfrn;
	CString filename;
}Pfrn_Name;
typedef map<DWORDLONG, Pfrn_Name> Frn_Pfrn_Name_Map;

struct sqlite3_value {
	union MemValue {
		double r;           /* Real value used when MEM_Real is set in flags */
	} u;
	unsigned short int flags;          /* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
	unsigned char  enc;            /* SQLITE_UTF8, SQLITE_UTF16BE, SQLITE_UTF16LE */
	unsigned char  eSubtype;       /* Subtype for this value */
	int n;              /* Number of characters in string value, excluding '\0' */
	char *z;            /* String or BLOB value */
	/* ShallowCopy only needs to copy the information above */
	char *zMalloc;      /* Space to hold MEM_Str or MEM_Blob if szMalloc>0 */
	int szMalloc;       /* Size of the zMalloc allocation */
	unsigned int uTemp;          /* Transient storage for serial_type in OP_MakeRecord */
	sqlite3 *db;        /* The associated database connection */
	void(*xDel)(void*);/* Destructor for Mem.z - only valid if MEM_Dyn */
#ifdef SQLITE_DEBUG
	Mem *pScopyFrom;    /* This Mem is a shallow copy of pScopyFrom */
	u16 mScopyFlags;    /* flags value immediately after the shallow copy */
#endif
};

class Volume {
public:
	Volume(char vol) {
		this->vol = vol;
	}
	~Volume() {
	}

	BOOL GetFileAttri(struct _stati64 *st, CString filePath)
	{
		if (0 != _wstati64(filePath, st))
			return FALSE;

		return TRUE;
	}

	CString ToThousandsNum(int num)
	{
		CString temp;
		temp.Format(_T("%d"), num);

		int length = temp.GetLength();
		if (length > 6)
		{
			temp.Insert(length - 6, _T(","));
			temp.Insert(length - 2, _T(","));
		}
		else if(length > 3)
			temp.Insert(length-3, _T(","));

		temp += _T("KB");

		return temp;
	}

	BOOL getUSNJournal()
	{
		CString driveletter(_T("\\\\.\\c:"));
		driveletter.SetAt(4, vol);
		HANDLE hVol = CreateFile(driveletter, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		USN_JOURNAL_DATA journalData;
		PUSN_RECORD usnRecord;
		DWORD dwBytes;
		DWORD dwRetBytes;
		char buffer[USN_PAGE_SIZE];
		BOOL bDioControl = DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &journalData, sizeof(journalData), &dwBytes, NULL);
		if (!bDioControl)
		{
			CloseHandle(hVol);
			return false;
		}
		MFT_ENUM_DATA_V0 med;
		med.StartFileReferenceNumber = 0;
		med.LowUsn = 0;
		med.HighUsn = journalData.NextUsn;

		CString tmp(_T("C:"));
		tmp.SetAt(0, vol);
		frnPfrnNameMap[0x5000000000005].filename = tmp;
		frnPfrnNameMap[0x5000000000005].pfrn = 0;
		
		while (dwBytes > sizeof(USN))
		{
			memset(buffer, 0, sizeof(USN_PAGE_SIZE));
			bDioControl = DeviceIoControl(hVol, FSCTL_ENUM_USN_DATA, &med, sizeof(med), &buffer, USN_PAGE_SIZE, &dwBytes, NULL);
			if (!bDioControl)
				break;

			dwRetBytes = dwBytes - sizeof(USN);
			usnRecord = (PUSN_RECORD)(((PUCHAR)buffer) + sizeof(USN));
			while (dwRetBytes > 0)
			{
				CString CfileName(usnRecord->FileName, usnRecord->FileNameLength / 2);

				pfrnName.filename = nameCur.filename = CfileName;
				pfrnName.pfrn = nameCur.pfrn = usnRecord->ParentFileReferenceNumber;
				VecNameCur.push_back(nameCur);

				frnPfrnNameMap[usnRecord->FileReferenceNumber] = pfrnName;

				dwRetBytes -= usnRecord->RecordLength;
				usnRecord = (PUSN_RECORD)(((PCHAR)usnRecord) + usnRecord->RecordLength);
			}
			med.StartFileReferenceNumber = *(DWORDLONG*)buffer;
		}
		CloseHandle(hVol);

		int length = VecNameCur.size();
		int index = ceil(length / 50000.0);
		std::future<void> *future = new std::future<void>[index];

		for (int i = 0; i < index; i++)
		{
			int beginIndex = i * 50000;
			int endIndex = (i + 1) * 50000;
			if (i == (index - 1))
				endIndex = length;
			future[i] = insert_pool.submit(insert_fileInfo, beginIndex, endIndex, (LPVOID)this);
		}

		for (int i = 0; i < index; i++)
		{
			future[i].get();
		}

		isCreateDBSucc = true;

		return true;
	}

	bool initVolume() {
		return getUSNJournal();
	}
	
	CString getPath(DWORDLONG frn, CString &path);
	char vol;
	vector<Name_Cur> VecNameCur;		// 查找1
	Name_Cur nameCur;
	Pfrn_Name pfrnName;
	Frn_Pfrn_Name_Map frnPfrnNameMap;	// 查找2
	map<DWORDLONG, CString> frnPfrnPathMap;
	USN_JOURNAL_DATA ujd;
	CREATE_USN_JOURNAL_DATA cujd;
};
	
CString Volume::getPath(DWORDLONG frn, CString &path) {
	// 查找2
	Frn_Pfrn_Name_Map::iterator it = frnPfrnNameMap.find(frn);
	if (it != frnPfrnNameMap.end()) {
		if ( 0 != it->second.pfrn ) {
			getPath(it->second.pfrn, path);
		}
		path += it->second.filename;
		path += ( _T("\\") );
	}

	return path;
}

std::string Unicode2Utf8(const std::wstring& widestring) {
	int utf8size = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
	if (utf8size == 0)
	{
		throw std::exception("Error in conversion.");
	}
	std::vector<char> resultstring(utf8size);
	int convresult = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, &resultstring[0], utf8size, NULL, NULL);
	if (convresult != utf8size)
	{
		throw std::exception("La falla!");
	}
	return std::string(&resultstring[0]);
}

CString UTF8ToUnicode(const CStringA& utf8Str)
{
	UINT codepage = CP_UTF8;

	int nSourceLen = utf8Str.GetLength();
	int nWideBufLen = MultiByteToWideChar(codepage, 0, utf8Str, -1, NULL, 0);

	wchar_t* pWideBuf = new wchar_t[nWideBufLen + 1];
	memset(pWideBuf, 0, (nWideBufLen + 1) * sizeof(wchar_t));

	MultiByteToWideChar(codepage, 0, utf8Str, -1, (LPWSTR)pWideBuf, nWideBufLen);

	CString strTarget(pWideBuf);
	delete[] pWideBuf;

	return strTarget;
}

void insert_fileInfo(int beginIndex, int endIndex, LPVOID pParam)
{
	Volume *vol = (Volume*)pParam;
	CString sql;
	CString sqlSum = TEXT("BEGIN;");
	int count = 0;
	struct _stati64 st;

	for (int i = beginIndex; i < endIndex; i++)
	{
		Name_Cur cit = vol->VecNameCur[i];
		CString path;

		g_frnSection.Lock();
		if (vol->frnPfrnPathMap[cit.pfrn] != "") {
			path = vol->frnPfrnPathMap[cit.pfrn];
		}
		else {
			vol->getPath(cit.pfrn, path);
			
			vol->frnPfrnPathMap[cit.pfrn] = path;
		}
		g_frnSection.Unlock();

		CString fileSize;
		CString fileChangeTime;
		bool xx = vol->GetFileAttri(&st, path + cit.filename);
		if (xx)
		{
			if (GetFileAttributes((path + cit.filename)) != FILE_ATTRIBUTE_DIRECTORY)
				fileSize = vol->ToThousandsNum(st.st_size / 8);
			char tmp[64];
			strftime(tmp, sizeof(tmp), "%Y/%m/%d %H:%M", localtime(&st.st_mtime));
			fileChangeTime = CA2T(tmp);
		}

		cit.filename.Replace(TEXT("'"), TEXT("''"));
		path.Replace(TEXT("'"), TEXT("''"));
		sql.Format(TEXT(SQL_INSERT_DATA), cit.filename, path, _T(""), _T(""), fileSize, fileChangeTime);

		sqlSum += sql;
	}
	sqlSum += "COMMIT;";

	g_insertSection.Lock();
	sqlite3_exec(m_pDB, Unicode2Utf8((wstring)sqlSum).c_str(), 0, 0, NULL);
	g_insertSection.Unlock();
}

class InitData {
public:	
	bool checkNTFS(char* name);

	list<Volume> volumelist;
	CCriticalSection g_criSection;
	CCriticalSection g_countSection;
	vector<File_Info> files;
	vector<vector<File_Info>::iterator> fileRisks;
	map<CString, CString> file_risk;
	map<CString, CString> file_insert_risk;
	UINT initvolumelist(LPVOID vol) {
		char c = (char)vol;
		Volume volume(c);
		volume.initVolume();
		volumelist.push_back(volume);

		return 1;
	}
	
	bool init() {
		DWORD dwSize = 254;
		GetTempPathA(dwSize, file_db_path);
		strcat(file_db_path, "fileinfo.db");
		BOOL FLAG = PathFileExistsA(file_db_path);
		sqlite3_open(file_db_path, &m_pDB);
		insert_pool.init();

		if (!FLAG) {
			isCreateDBSucc = false;
			sqlite3_exec(m_pDB, SQL_CREATE_TABLE, NULL, NULL, NULL);
		}
		else {
			sqlite3_stmt* stmt;
			sqlite3_prepare_v2(m_pDB, "Select count(*) from file_info", -1, &stmt, NULL);
			sqlite3_step(stmt);
			int sum = atoi((CStringA)(sqlite3_column_text(stmt, 0)));
			if (sum == 0)
				isCreateDBSucc = false;
		}

		if (!isCreateDBSucc)
		{
			int bufLen = GetLogicalDriveStrings(0, NULL);
			char driverBuf[1024];
			memset(driverBuf, 0, 1024);
			GetLogicalDriveStringsA(bufLen, driverBuf);
			char* driverName = driverBuf;
			while (*driverName != '\0')
			{
				if (checkNTFS(driverName))
				{
					vol[j++] = driverName[0];
				}
				driverName += strlen(driverName) + 1;
			}
		}

		BOOL IsHadRiskDb = PathFileExistsA("file_risk.db");
		sqlite3_open("file_risk.db", &risk_pDB);

		if (!IsHadRiskDb)
			sqlite3_exec(risk_pDB, SQL_CREATE_RISK_TABLE, NULL, NULL, NULL);

		return isCreateDBSucc;
	}

	void getRiskDataFormDB()
	{
		sqlite3_stmt* stmt;
		CString select;

		sqlite3_prepare_v2(risk_pDB, SQL_QUERY_RISK_DATA, -1, &stmt, NULL);
		int rc = sqlite3_step(stmt);
		while (rc == SQLITE_ROW)
		{
			CStringA a = CStringA(sqlite3_column_text(stmt, 0));
			CStringA b = CStringA(sqlite3_column_text(stmt, 1));
			CString fileMD5 = UTF8ToUnicode(a);
			CString fileRisk = UTF8ToUnicode(b);

			file_risk[fileMD5] = fileRisk;

			rc = sqlite3_step(stmt);
		}
		sqlite3_finalize(stmt);

		return ;
	}

	BOOL file_risk_request(CString content)
	{
		//通过 http POST 协议来发送命令给服务器
		CInternetSession session;
		session.SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 1000 * 20);
		session.SetOption(INTERNET_OPTION_CONNECT_BACKOFF, 1000);
		session.SetOption(INTERNET_OPTION_CONNECT_RETRIES, 1);

		CHttpConnection* pConnection = session.GetHttpConnection(TEXT("qup.b.qianxin-inc.cn"),
			(INTERNET_PORT)80);
		CHttpFile* pFile = pConnection->OpenRequest(CHttpConnection::HTTP_VERB_POST,
			TEXT("/file_health_info.php"),
			NULL,
			1,
			NULL,
			TEXT("HTTP/1.1"),
			INTERNET_FLAG_RELOAD);

		//需要提交的数据
		CString cstrBoundary = _T("--24a1efafe07bc0a5675ff76402ba1fa2");
		CString szHeaders = _T("Content-Type:multipart/form-data;");
		CString strFormData;
		CString temp;
		temp.Format(_T("%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s"),
			cstrBoundary, _T("product"), _T("internal"));
		strFormData += temp;
		temp.Format(_T("\r\n%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s"),
			cstrBoundary, _T("combo"), _T("internal"));
		strFormData += temp;
		
		temp.Format(_T("\r\n%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s"),
			cstrBoundary, _T("md5s"), content);
		strFormData += temp;
		temp.Format(_T("\r\n%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s"),
			cstrBoundary, _T("v"), _T("2"));
		strFormData += temp;
		temp.Format(_T("\r\n%s\r\nContent-Disposition: form-data; name=\"%s\"\r\n\r\n%s"),
			cstrBoundary, _T("en"), _T("1"));
		strFormData += temp;
		temp.Format(_T("\r\n%s--"),
			cstrBoundary);
		strFormData += temp;
		string b = CW2A((strFormData).GetString());
		pFile->AddRequestHeaders(_T("Connection: keep-alive"));
		try {
			pFile->SendRequest(szHeaders,
				szHeaders.GetLength(),
				(LPVOID)b.c_str(),
				b.length());
		}
		catch (CInternetException* pEx)
		{
			session.Close();
			pFile->Close();
			delete pFile;
			
			return false;
		}

		DWORD dwRet;
		pFile->QueryInfoStatusCode(dwRet);

		if (dwRet != HTTP_STATUS_OK)
		{
			session.Close();
			pFile->Close();
			delete pFile;
			return false;
		}
		else
		{
			int len = pFile->GetLength();
			char buf[1024*100];
			int numread;
			string content;
			while ((numread = pFile->Read(buf, sizeof(buf) - 1)) > 0)
			{
				buf[numread] = '\0';
				content += buf;
			}
			
			std::regex ip_reg("(<e_level>(.*?)</e_level>)|(<md5>(.*?)</md5>)");

			string::const_iterator iterStart = content.begin();
			string::const_iterator iterEnd = content.end();
			int index = 0;
			smatch result;
			CString fileMD5;
			while (regex_search(iterStart, iterEnd, result, ip_reg))
			{
				if (index % 2 == 1)
				{
					g_criSection.Lock();
					string s = result.str(2);
					if (s.c_str() != "")
					{
						file_risk[fileMD5] = CString(s.c_str());
						file_insert_risk[fileMD5] = CString(s.c_str());
					}
					g_criSection.Unlock();
				}
				else
				{
					string s = result.str(4);
					fileMD5 = CString(s.c_str()).MakeUpper();
				}
				index++;
				iterStart = result[0].second;	//更新搜索起始位置,搜索剩下的字符串
			}
		}

		session.Close();
		pFile->Close();
		delete pFile;
		return true;
	}

	void insertRiskData()
	{
		CString sql;
		CString sqlSum = TEXT("BEGIN;");
		int count = 0;
		for (map<CString, CString>::iterator risk_info = file_insert_risk.begin();
			risk_info != file_insert_risk.end(); ++risk_info) {

			sql.Format(TEXT(SQL_INSERT_RISK_DATA), risk_info->first, risk_info->second);

			sqlSum += sql;
			if (++count >= 50000)
			{
				sqlSum += "COMMIT;";
				sqlite3_exec(risk_pDB, Unicode2Utf8((wstring)sqlSum).c_str(), 0, 0, NULL);
				sqlSum = "BEGIN;";
				count = 0;
			}
		}
		sqlSum += "COMMIT;";
		int a = sqlite3_exec(risk_pDB, Unicode2Utf8((wstring)sqlSum).c_str(), 0, 0, NULL);

		fileRisks.clear();
		file_insert_risk.clear();
	}

	void updateFileInfoDB()
	{
		CString sql;
		CString sqlSum = TEXT("BEGIN;");
		int count = 0;
		for (vector<File_Info>::iterator file_info = files.begin();
			file_info != files.end(); ++file_info) {

			sql.Format(TEXT(SQL_UPDATE_FILE_DATA), file_info->fileMD5, file_info->fileRisk, file_info->ID);
			
			sqlSum += sql;
			if (++count >= 50000)
			{
				sqlSum += "COMMIT;";
				sqlite3_exec(m_pDB, Unicode2Utf8((wstring)sqlSum).c_str(), 0, 0, NULL);
				sqlSum = "BEGIN;";
				count = 0;
			}
		}
		sqlSum += "COMMIT;";
		
		sqlite3_exec(m_pDB, Unicode2Utf8((wstring)sqlSum).c_str(), 0, 0, NULL);
	}

	int sql_select(CString searchText, CString dir, CString sortStr)
	{
		m_searchText = searchText;
		m_dir = dir;

		sqlite3_stmt* stmt;
		CString select;
		files.clear();
		int replaceCount = searchText.Replace(TEXT("*"), TEXT("%"));
		replaceCount = replaceCount + searchText.Replace(TEXT("?"), TEXT("_"));
		if (dir == "" && searchText == "")
		{
			select = SQL_QUERY_DATA;
		}
		else if (dir == "")
		{
			if (replaceCount == 0)
				searchText = TEXT("%") + searchText + TEXT("%");
			select.Format(TEXT(SQL_SELECT_DATA_1), searchText);
		}
		else if (searchText == "")
		{
			dir += "%";
			select.Format(TEXT(SQL_SELECT_DATA_3), dir);
		}
		else {
			dir += "%";
			if (replaceCount == 0)
				searchText = TEXT("%") + searchText + TEXT("%");
			select.Format(TEXT(SQL_SELECT_DATA_2), searchText, dir);
		}

		select += sortStr;

		sqlite3_prepare_v2(m_pDB, Unicode2Utf8((wstring)select).c_str(), -1, &stmt, NULL);
		
		int rc = sqlite3_step(stmt);
		DWORD p;
		sqlite3_value *pOut;
		
		while (rc == SQLITE_ROW)
		{
			File_Info temp;
			
			p = DWORD(*((LPVOID *)stmt + 0x20));//0x80

			temp.ID = *(PDWORD)p; //第一个字段恰好是自增索引

			pOut = (sqlite3_value *)(p + 40);//sizeof(sqlite3_value)=40
			temp.fileName = UTF8ToUnicode(pOut->z);

			pOut = (sqlite3_value *)(p + 80);
			temp.filePath = UTF8ToUnicode(pOut->z);

			pOut = (sqlite3_value *)(p + 120);
			temp.fileMD5 = pOut->z;

			temp.fileRisk = sqlite3_column_text(stmt, 4);
			//pOut = (sqlite3_value *)(p + 160); //程序会把数据库的string数字转化为整形传输
			//temp.fileRisk = pOut->z;

			pOut = (sqlite3_value *)(p + 200);
			temp.fileSize = pOut->z;

			pOut = (sqlite3_value *)(p + 240);
			temp.fileChangeTime = pOut->z;

			files.push_back(temp);

			rc = sqlite3_step(stmt);
		}

		sqlite3_finalize(stmt);
		
		/*
		QueryPerformanceFrequency(&nFreq);
		QueryPerformanceCounter(&tStart);
		QueryPerformanceCounter(&tMid);
		double t1 = (tMid.QuadPart - tStart.QuadPart) / (double)nFreq.QuadPart * 1000;
		CString s;
		s.Format(_T("%f"), t1);
		AfxMessageBox(s);*/
		is_import = false;
		return files.size();
	}

	int sql_select_sort(int itemIndex, bool sort)
	{
		CString sortStr;
		CString a = _T("ASC");
		if (sort)
			a = _T("DESC");
		
		CString itemName;
		switch (itemIndex)
		{
		case 0:
			itemName = "fileName";
			break;
		case 1:
			itemName = "filePath";
			break;
		case 2:
			itemName = "fileSize";
			break;
		case 3:
			itemName = "fileChangeTime";
			break;
		case 4:
			itemName = "fileMD5";
			break;
		case 5:
			itemName = "fileRisk";
			break;
		default:
			break;
		}

		sortStr.Format(_T(SQL_QUERY_SORT), itemName, a);

		return sql_select(m_searchText, m_dir, sortStr);
	}

	int getJ() {
		return j;
	}
	char * getVol() {
		return vol;
	}
	void clear()
	{
		insert_pool.shutdown();
		sqlite3_close(m_pDB); 
		sqlite3_close(risk_pDB);
		if (isCreateDBSucc == false)
			DeleteFileA(file_db_path);
	}

private:
	char vol[26];
	int i, j;
	CString m_searchText, m_dir;
};

bool InitData::checkNTFS(char* name)//检查是否是NTFS文件系统盘
{
	char nameBuf[15];
	if (GetVolumeInformationA(name, NULL, 0, NULL, NULL, NULL, nameBuf, 15))//获取磁盘信息
		return strcmp(nameBuf, "NTFS") ? 0 : 1;
}

InitData initdata;