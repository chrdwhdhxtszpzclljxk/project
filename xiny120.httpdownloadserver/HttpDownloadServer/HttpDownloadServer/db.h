#ifndef _DB_H
#define _DB_H
#include "HDS_ns.h"


#include <mutex>
#include <string>

#ifdef _WIN64
#pragma warning (disable : 4146)
#import "c:\Program Files\Common Files\SYSTEM\ADO\msado15.dll" \
        no_namespace \
        rename ("EOF", "adoEOF") \
        rename ("LockTypeEnum", "adoLockTypeEnum") \
        rename ("DataTypeEnum", "adoDataTypeEnum") \
        rename ("FieldAttributeEnum", "adoFieldAttributeEnum") \
        rename ("EditModeEnum", "adoEditModeEnum") \
        rename ("RecordStatusEnum", "adoRecordStatusEnum") \
		rename ("Field", "adoField") \
        rename ("ParameterDirectionEnum", "adoParameterDirectionEnum")
#pragma warning (default : 4146)
#else
#pragma warning (disable : 4146)
#import "C:/Program Files (x86)/Common Files/System/ado/msado15.dll" \
        no_namespace \
        rename ("EOF", "adoEOF") \
        rename ("LockTypeEnum", "adoLockTypeEnum") \
        rename ("DataTypeEnum", "adoDataTypeEnum") \
        rename ("FieldAttributeEnum", "adoFieldAttributeEnum") \
        rename ("EditModeEnum", "adoEditModeEnum") \
        rename ("RecordStatusEnum", "adoRecordStatusEnum") \
		rename ("Field", "adoField") \
        rename ("ParameterDirectionEnum", "adoParameterDirectionEnum")
#pragma warning (default : 4146)
#endif  /* _WIN64 */


#pragma warning (disable : 4996)

NS_XINY120_BEGIN


class db{
public:
	typedef std::lock_guard<std::recursive_mutex> _lockg;
public:
	static void setupdb(const std::string&);
	static db* me();
	std::string getfile(const std::string& key, int32_t& userid);
	bool checklogin(const char* un, const char* pwd, int64_t& userid, int64_t& i117, int64_t& ivcd40, int64_t& igroupid, int64_t& ivagroupid, int64_t&);
	bool isvalidfile(const int64_t& pubid, const int64_t& year, const int64_t& filetime);
	int64_t canpalygs(const int64_t& pubid, const int64_t& year, const int64_t& filetime, const char* un);
	int64_t canplaygp2014(const int64_t& pubid, const int64_t& year, const int64_t& filetime, const char* un);
	int64_t canplaygp_old(const int64_t& pubid, const int64_t& year, const int64_t& filetime, const char* un);
	bool getfileidx(const int64_t& pubid, const int64_t& year, const int64_t& filetime, int64_t* ptimes, int64_t& dwLen);
protected:
	db();
	~db();
	bool open();
	void close();
private:
	bool isopen;
	_ConnectionPtr con;
	std::wstring ver;
	std::recursive_mutex mt;
public:
	static tm localtime(const int64_t& _now);
};

NS_XINY120_END

#endif