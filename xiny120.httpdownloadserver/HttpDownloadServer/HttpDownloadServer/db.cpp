
#include "db.h"
#include <stdio.h>
#include <tchar.h>
#include "output.h"

NS_XINY120_BEGIN

#define SETPARAM(name,type,dir,size,val) cmd->Parameters->Append(cmd->CreateParameter((name), (type), (dir), (size), (val)))

static db* dbobj = NULL;
static std::string strcon;
db::db() :isopen(false){}
db::~db(){}
void db::setupdb(const std::string& str){ strcon = str; }
db* db::me(){ if (dbobj == NULL){ dbobj = new db(); dbobj->open(); }return dbobj; }
bool db::open(){
	CoInitialize(NULL);	_lockg lock(mt);
	isopen = false;
	try{
		con.CreateInstance(__uuidof(Connection));
		con->Open(_bstr_t(strcon.c_str()), L"", L"", -1);
		ver = con->Version;
		isopen = true;
	}
	catch (_com_error e){
		otprint("sql error: %d 0x%08x,%s %s", e.WCode(), e.Error(), _bstr_t(e.ErrorMessage()).operator char *(), e.Description().operator char *());
		close();
		return false;
	}
	catch (...){
		close();
		return false;
	}
	return isopen;
}

void db::close(){
	_lockg lock(mt);
	try{ if (con != NULL){ con->Close(); con = NULL; } }
	catch (...){}
	isopen = false;
	CoUninitialize();
}

bool db::testopen(){
	_lockg lock(mt);
	return true;
}

tm db::localtime(const int64_t& _now){
	int64_t now = _now;	if (now < 0 || now >= 0x793406fffi64) now = 0;
	return *::_localtime64(&now);
}

std::string db::getfile(const std::string& key, int32_t& userid){
	_CommandPtr	cmd; HRESULT hr; _RecordsetPtr rs; std::string ret;
	if (SUCCEEDED(hr = cmd.CreateInstance(__uuidof(Command)))){
		try{
			SETPARAM("@keyId", adVarChar, adParamInput, key.length(), key.c_str());
			cmd->CommandText = _bstr_t("httpdownloadserver_getfile");			// 设定存储过程名
			cmd->CommandType = adCmdStoredProc;
			cmd->ActiveConnection = con;								// 绑定连接
			_lockg lock(mt);
			rs = cmd->Execute(NULL, NULL, adCmdStoredProc);				// 一定要用这种方式
			try{
				if (rs != NULL){
					if (!(rs->BOF && rs->adoEOF)){
						_bstr_t b = rs->GetCollect("filePath").operator _bstr_t();
						ret = b;
						userid = rs->GetCollect("userid").operator int();
					}
					rs->Close();
				}
			}
			catch (...){}
		}
		catch (_com_error e){
			otprint("sql error: %d %s 0x%08x,%s %s", int32_t(e.WCode()),key.c_str(), int32_t(e.Error()), _bstr_t(e.ErrorMessage()).operator char *(), e.Description().operator char *());
			
			//close(); open();
			return ret;
		}catch (...){ 
			//close(); open(); 
		}
	}
	return ret;
}

bool db::checklogin(const char* un, const char* pwd, int64_t& userid, int64_t& i117, int64_t& ivcd40, int64_t& igroupid, int64_t& ivagroupid, int64_t& ret){
	_CommandPtr	cmd; HRESULT hr; int64_t tmp = 0; ret = 3;
	if (SUCCEEDED(hr = cmd.CreateInstance(__uuidof(Command)))){
		try{
			SETPARAM("@szUserName", adVarChar, adParamInput, strlen(un), un);
			SETPARAM("@szPassword", adVarChar, adParamInput, strlen(pwd), pwd);
			/*
			@szUserName nvarchar(50),
			@szPassword nvarchar(50),
			@i117		bigint OUTPUT,
			@iVcd40	bigint OUTPUT,
			@iUserId	bigint OUTPUT,
			@iGroupId	bigint OUTPUT,
			@iVaGroupId	bigint OUTPUT,
			@iReturn	bigint OUTPUT
			*/
			SETPARAM("@i117", adBigInt, adParamOutput, sizeof(int64_t), tmp);
			SETPARAM("@iVcd40", adBigInt, adParamOutput, sizeof(int64_t), tmp);
			SETPARAM("@iUserId", adBigInt, adParamOutput, sizeof(int64_t), tmp);
			SETPARAM("@iGroupId", adBigInt, adParamOutput, sizeof(int64_t), tmp);
			SETPARAM("@iVaGroupId", adBigInt, adParamOutput, sizeof(int64_t), tmp);
			SETPARAM("@iReturn", adBigInt, adParamOutput, sizeof(int64_t), tmp);

			cmd->CommandText = _bstr_t("tserver2_login");			// 设定存储过程名
			cmd->CommandType = adCmdStoredProc;
			cmd->ActiveConnection = con;								// 绑定连接
			_lockg lock(mt);
			cmd->Execute(NULL, NULL, adCmdStoredProc);					// 一定要用这种方式   
			ret = (int64_t)cmd->Parameters->GetItem("@iReturn")->GetValue();					// 取结果 
			i117 = (int64_t)cmd->Parameters->GetItem("@i117")->GetValue();
			ivcd40 = (int64_t)cmd->Parameters->GetItem("@iVcd40")->GetValue();
			userid = (int64_t)cmd->Parameters->GetItem("@iUserId")->GetValue();
			igroupid = (int64_t)cmd->Parameters->GetItem("@iGroupId")->GetValue();
			ivagroupid = (int64_t)cmd->Parameters->GetItem("@iVaGroupId")->GetValue();

			return true;
		}
		catch (_com_error e){
			close(); open();
		}
		catch (...){ close(); open(); }
	}
	return false;
}

bool db::isvalidfile(const int64_t& pubid, const int64_t& year, const int64_t& filetime){
	_CommandPtr	cmd; HRESULT hr; int64_t tmp = 0;
	if (SUCCEEDED(hr = cmd.CreateInstance(__uuidof(Command)))){
		try{
			SETPARAM("pubid", adBigInt, adParamInput, sizeof(int64_t), pubid);
			SETPARAM("year", adBigInt, adParamInput, sizeof(int64_t), year);
			SETPARAM("filetime", adBigInt, adParamInput, sizeof(int64_t), filetime);
			SETPARAM("ret", adBigInt, adParamOutput, sizeof(int64_t), tmp);

			cmd->CommandText = _bstr_t("tserver2_isvalidfile");			// 设定存储过程名
			cmd->CommandType = adCmdStoredProc;
			cmd->ActiveConnection = con;								// 绑定连接
			_lockg lock(mt);
			cmd->Execute(NULL, NULL, adCmdStoredProc);					// 一定要用这种方式   
			tmp = (int64_t)cmd->Parameters->GetItem("ret")->GetValue();
			return filetime == tmp;				// 取结果 
		}
		catch (_com_error e){
			close(); open();
		}
		catch (...){ close(); open(); }
	}
	return false;
}


int64_t db::canpalygs(const int64_t& pubid, const int64_t& year, const int64_t& filetime, const char* un){
	HRESULT hr = S_FALSE; int64_t iReturn = 0; _CommandPtr	cmd; tm tm1 = localtime(filetime); char* pszMd5 = "34eef343test";
	try{
		hr = cmd.CreateInstance(__uuidof(Command)); char szUpdateTime[512] = { 0 };
		sprintf_s(szUpdateTime, sizeof(szUpdateTime), "%04d-%02d-%02d %02d:%02d:%02d",
			tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
		SETPARAM("szUserName", adVarChar, adParamInput, strlen(un), un);
		SETPARAM("szVcdTime", adVarChar, adParamInput, strlen(szUpdateTime), szUpdateTime);
		SETPARAM("md5_c", adVarChar, adParamInput, strlen(pszMd5), pszMd5);
		SETPARAM("szUpdateTime", adVarChar, adParamInput, strlen(szUpdateTime), szUpdateTime);
		SETPARAM("iReturn", adBigInt, adParamOutput, sizeof(int64_t), iReturn);
		SETPARAM("iTemp", adBigInt, adParamOutput, sizeof(int64_t), iReturn);
		SETPARAM("szOut", adVarChar, adParamInput, 1000, 0);
		cmd->CommandText = _bstr_t("tserver2_gongshibuy");						//设定存储过程名
		cmd->CommandType = adCmdStoredProc;
		cmd->ActiveConnection = con;							//绑定连接
		_lockg lock(mt);
		cmd->Execute(NULL, NULL, adCmdStoredProc);//一定要用这种方式   
		iReturn = (long)cmd->Parameters->GetItem("iReturn")->GetValue();//取结果 
		if (iReturn != 333){
		}
	}
	catch (_com_error e){
		close(); open();
		return -1;
	}
	catch (...){ close(); open(); }

	return iReturn;
}

int64_t db::canplaygp2014(const int64_t& pubid, const int64_t& year, const int64_t& filetime, const char* un){
	HRESULT hr = S_FALSE; char* pszMd5 = "34eef343test"; int64_t iReturn = 0; _CommandPtr cmd; tm tm1 = localtime(filetime);
	try{
		hr = cmd.CreateInstance(__uuidof(Command));
		char szUpdateTime[512] = { 0 };
		sprintf_s(szUpdateTime, sizeof(szUpdateTime), "%04d-%02d-%02d %02d:%02d:%02d",
			tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
		SETPARAM("szUserName", adVarChar, adParamInput, strlen(un), un);
		SETPARAM("szVcdTime", adVarChar, adParamInput, strlen(szUpdateTime), szUpdateTime);
		SETPARAM("tVcdTime", adBigInt, adParamInput, sizeof(int64_t), filetime);
		SETPARAM("szPublisherId", adBigInt, adParamInput, sizeof(int64_t), pubid);
		SETPARAM("iReturn", adBigInt, adParamOutput, sizeof(int64_t), iReturn);
		SETPARAM("iTemp", adBigInt, adParamOutput, sizeof(int64_t), iReturn);
		cmd->CommandText = _bstr_t("tserver2_mzgpbuy_2014");						//设定存储过程名
		cmd->CommandType = adCmdStoredProc;
		cmd->ActiveConnection = con;							//绑定连接
		_lockg lock(mt);
		cmd->Execute(NULL, NULL, adCmdStoredProc);//一定要用这种方式   
		iReturn = (long)cmd->Parameters->GetItem("iReturn")->GetValue();//取结果 
	}
	catch (...){ close(); open(); }
	return iReturn;
}

int64_t db::canplaygp_old(const int64_t& pubid, const int64_t& year, const int64_t& filetime, const char* un){
	HRESULT hr = S_FALSE; int64_t iReturn = 0; _CommandPtr	cmd; tm tm1 = localtime(filetime);
	try{
		hr = cmd.CreateInstance(__uuidof(Command));
		CHAR* pszMd5 = "34eef343test";
		CHAR szUpdateTime[512] = { 0 };
		sprintf_s(szUpdateTime, sizeof(szUpdateTime), "%04d-%02d-%02d %02d:%02d:%02d",
			tm1.tm_year + 1900, tm1.tm_mon + 1, tm1.tm_mday, tm1.tm_hour, tm1.tm_min, tm1.tm_sec);
		SETPARAM("szUserName", adVarChar, adParamInput, strlen(un), un);
		SETPARAM("szVcdTime", adVarChar, adParamInput, strlen(szUpdateTime), szUpdateTime);
		SETPARAM("md5_c", adVarChar, adParamInput, strlen(pszMd5), pszMd5);
		SETPARAM("szUpdateTime", adVarChar, adParamInput, strlen(szUpdateTime), szUpdateTime);
		SETPARAM("iReturn", adBigInt, adParamOutput, sizeof(int64_t), iReturn);
		SETPARAM("iTemp", adBigInt, adParamOutput, sizeof(int64_t), iReturn);
		SETPARAM("szOut", adVarChar, adParamInput, 1000, 0);
		cmd->CommandText = _bstr_t("tserver2_mzgpbuy_old");						//设定存储过程名
		cmd->CommandType = adCmdStoredProc;
		cmd->ActiveConnection = con;							//绑定连接
		cmd->Execute(NULL, NULL, adCmdStoredProc);//一定要用这种方式   
		iReturn = (long)cmd->Parameters->GetItem("iReturn")->GetValue();//取结果 
	}
	catch (...){ close(); open(); }
	return iReturn;
}

bool db::getfileidx(const int64_t& pubid, const int64_t& year, const int64_t& filetime, int64_t* ptimes, int64_t& len){
	_CommandPtr	cmd; HRESULT hr; _RecordsetPtr	rs; bool ret = false; int64_t idx = 0;
	if (SUCCEEDED(hr = cmd.CreateInstance(__uuidof(Command)))){
		try{
			SETPARAM("pubid", adBigInt, adParamInput, sizeof(int64_t), pubid);
			SETPARAM("year", adBigInt, adParamInput, sizeof(int64_t), year);
			SETPARAM("filetime", adBigInt, adParamInput, sizeof(int64_t), filetime);
			cmd->CommandText = _bstr_t("tserver2_getfileidx");			// 设定存储过程名
			cmd->CommandType = adCmdStoredProc;
			cmd->ActiveConnection = con;								// 绑定连接
			_lockg lock(mt);
			rs = cmd->Execute(NULL, NULL, adCmdStoredProc);				// 一定要用这种方式
			idx = 0;
			while (!rs->adoEOF){
				*ptimes = rs->GetCollect(_T("tNow")).llVal; ptimes++;
				if (idx >= len) break;
				rs->MoveNext();
				idx++;
			}
			rs->Close();
			len = idx;
			ret = true;
		}
		catch (_com_error e){
			otprint("sql error: %d 0x%08x,%s %s", e.WCode(), e.Error(), _bstr_t(e.ErrorMessage()).operator char *(), e.Description().operator char *());
			close(); open();
			return false;
		}
		catch (...){ close(); open(); }
	}
	return ret;
}

NS_XINY120_END