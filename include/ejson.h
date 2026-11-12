#pragma once

// 默认情况:强崩溃
#ifndef EJSON_SILENT 
#define EJSONE_COLLAPSE 
#endif

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>



namespace ejson
{
	enum vtype
	{
		INT,//整数
		FLOAT,//浮点
		STRING,//字符串
		ARRAY,//数组
		OBJECT,//对象
		NUL,//空值
		DEL,//删除
		EARRAY,//空数组
		EOBJECT,//空对象
		NIPAIR//新数字键值对
	};

	struct pair;

	class jsonfile;

	class jsonvalue
	{
	private:
		void ToString(std::string&);

		int ToInt();

		double ToFloat();

		void InsertString(const char* data);

		void InsertJString(const char* data);

		void InsertInt(const int& data);

		void InsertFloat(const double& data);
	
		void InsertNull();

		void DeleteString(char* data);

		void DeleteInt(char* data);

		void DeleteFloat(char* data);

		void DeleteArray(char* data);

		void DeleteNull(char* data);

		void DeleteObject(char* data);

		void CheckDelete();

		void DeleteArrayNode();

		void DeleteObjectNode();

		//插入操作
		void InsertNewArray();

		void InsertNewObject();

		void InsertNewPair(const char* key);

		void InsertNewIPair();

		size_t SeekEnd(char* ptr, char check);

		jsonvalue SeekIPair(const int& nindex, const int& index);

		jsonvalue SeekPair(const char* index);

	public:
		jsonvalue() = default;

		jsonvalue(jsonfile* fptr, long offset, vtype type,vtype outer);

		//类型判断
		vtype GetVType();

		//获取大小
		size_t GetArraySize();

		size_t GetObjectSize();

		//尝试访问
		std::pair<bool, jsonvalue> TryAccess(const int& index);

		std::pair<bool, jsonvalue> TryAccess(const char* key);

		//遍历
		bool IPairs(int& key, jsonvalue& value, int step_length = 1);

		bool Pairs(std::string& key, jsonvalue& object);

		//读取操作
		jsonvalue& operator>>(const char** data);

		jsonvalue& operator>>(std::string& data);

		jsonvalue& operator>>(int& data);

		jsonvalue& operator>>(double& data);

		//写入操作
		jsonvalue& operator<<(const int& data);

		jsonvalue& operator<<(const char* data);

		jsonvalue& operator<<(const char** data);

		jsonvalue& operator<<(const double& data);

		//删除,赋空值,插入新的array和object
		jsonvalue& operator<<(const vtype& data);

		jsonvalue& operator<<(const pair& data);

		//访问object
		jsonvalue operator[](const char* key);

		//访问array
		jsonvalue operator[](const int& index);

		void Flush();

	private:
		std::pair<long, vtype> _data;
		jsonfile* _file;
		vtype _outer;
	};

	struct pair
	{
		pair() = default;
		pair(const char* str)
			:k(str)
		{

		}
		std::string k;
		jsonvalue v;
	};

	struct ipair
	{
		int k=0;
		jsonvalue v;
	};

#define NPAIR(key) ejson::pair(key)//新键值对
#define JSTRING(key) ((const char**)(key))//jstring

	class jsonfile
	{
		friend class jsonvalue;
	private:
		std::pair<long, vtype> CheckValidData(long index, char sign=0);

	public:
		jsonfile();

		jsonfile(const char* path);

		jsonvalue GetRootValue();

		void Clear();

		void Flush();

		~jsonfile();
	private:
		FILE* _file;
		std::string _buffer;
		std::string _path;
		long _front;
		long _back;
		long _size;
		bool _Is_Changed;
	};
}

#ifdef EJSON_SILENT

//ejson错误处理
namespace ejson
{
	enum error_code
	{
		NORMAL=0,//正常
		ARRAY_FORMAT_ERROR,//数组格式错误
		OBJECT_FORMAT_ERROR,//对象格式错误
		JSTRING_READ_ERROR,//JSTING读取错误
		OBJECT_ACCESS_ERROR,//对象访问错误
		ARRAY_ACCESS_ERROR,//数组访问错误
		FILE_EMPTY_ERROR,//空文件错误
		ROOT_FORMAT_ERROR,//根节点错误
		STRING_WRITE_ERROR,//字符串写入错误
		INT_WRITE_ERROR,//整形写入错误
		FLOAT_WRITE_ERROR,//浮点型写入错误
		NEW_IPAIR_ERROR,//插入新数字键值对错误
		NEW_PAIR_ERROR,//插入新键值对错误
		OBJECT_OUT_ERROR,//对象越界错误
		ARRAY_OUT_ERROR//数组越界错误
	};

	typedef void (*error_func) (error_code);

	extern thread_local error_code error;

	extern thread_local error_func efunc;

	inline error_code GetError()
	{
		return error;
	}

	inline void SetErrorCallBack(error_func func)
	{
		efunc = func;
	}

	inline const char* GetErrorString(error_code code)
	{
		switch (code)
		{
		case NORMAL:
			break;
		case ARRAY_FORMAT_ERROR:
			return "It's found that json's format is not correct ，finding array members .";
		case OBJECT_FORMAT_ERROR:
			return "It's found that json's format is not correct ，finding object members.";
		case JSTRING_READ_ERROR:
			return "Only the object or array can be read in the way converting jstring.";
		case OBJECT_ACCESS_ERROR:
			return "There is no key-value pair with the key in this object.";
		case ARRAY_ACCESS_ERROR:
			return "There is no key-value pair with the key in this array.";
		case FILE_EMPTY_ERROR:
			return "file is empty.";
		case ROOT_FORMAT_ERROR:
			return "json's format is not correct finding root value .";
		case STRING_WRITE_ERROR:
			return "The jsonvalue type read does not match the string type.";
		case INT_WRITE_ERROR:
			return "The jsonvalue type read does not match the int type.";
		case FLOAT_WRITE_ERROR:
			return "The jsonvalue type read does not match the float type.";
		case NEW_IPAIR_ERROR:
			return "Only objects can be inserted into ipairs.";
		case NEW_PAIR_ERROR:
			return "Only objects can be inserted into pairs.";
		case OBJECT_OUT_ERROR:
			return "Out-of-bounds access to object";
		case ARRAY_OUT_ERROR:
			return "Out-of-bounds access to array";
		}
	}
}

#endif