/*****************************************************************************
2022/5/5   章鹏
	实现较智能的存档算法

2022/5/7 章鹏
	【JsonArchiveAlgorithm参数存档思路】
	1、确认要存档的参数类型；
	2、如果参数不是结构体和类，则直接调用ARCHIVE_SAVE_VALUE()和ARCHIVE_LOAD_VALUE()存档和获取档案即可；
	3、如果参数是结构体或类，则需要在类和结构体定义的地方，使用ADD_PARAM_CLASS_TO_TEMPLATE将结构体或类添加到模板中（详见Demo）；
	4、调用ARCHIVE_OPEN_WRITER()初始化存档资源；
	5、调用ARCHIVE_SAVE_VALUE()将数据添加到JSON中；
	6、调用ARCHIVE_CLOSE_WRITER()释放存档资源并将Json数据保存到文件中；
	7、从文件中读取json值思路跟存档思路类似.
	＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
	【JsonArchiveAlgorithm出现异常的几种情况】
	异常1：结构体或类中添加了新的成员，却没有被存档
	问题原因：新成员没有在结构体对应的ADD_PARAM_CLASS_TO_TEMPLATE后面加入新添加的成员；
	解决方法：在对应结构体的ADD_PARAM_CLASS_TO_TEMPLATE后面追加新的成员变量即可；
	-------------------------------------------------------------------------
	异常2：在结构体或类对应的ADD_PARAM_CLASS_TO_TEMPLATE添加成员变量后，编译报错；
	问题原因：被添加的成员变量是结构体或类，且这个类没有使用ADD_PARAM_CLASS_TO_TEMPLATE进行处理；
	解决方法：在新添加的成员变量类型定义那里，使用ADD_PARAM_CLASS_TO_TEMPLATE进行处理即可.

*****************************************************************************/
#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <vector>
#include <regex>
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <mutex>

#if defined(_WIN32) || defined(WIN32)
#define snprintf(buf,len, format,...) _snprintf_s(buf, len, len-1, format, __VA_ARGS__)
#endif

namespace zp_archive {
	namespace serialize
	{
		/* 生成json数据 */
		rapidjson::StringBuffer *m_pstream = nullptr;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> *m_pwriter = nullptr;
		std::string *m_pfilename = nullptr;
		std::mutex m_writer_mux;

		template<typename ty, class enable = void>
		struct serialize_impl;

		template<>
		struct serialize_impl<int>;
		
		class JsonWriter{};

		template<class ty>
		inline void wirte_json(const char* key, ty& val)
		{
			//std::cout << key << ":" << val << "\n";
			/* 这里用重载区分类型比较麻烦，暂时采用条件判断 */
			if (typeid(signed char) == typeid(val)
				|| (typeid(signed int) == typeid(val))
				|| (typeid(signed short) == typeid(val))
#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
#else
				|| (typeid(signed long int) == typeid(val))
				|| (typeid(signed long) == typeid(val))
#endif				
				)
			{
				m_pwriter->Key(key); 
				m_pwriter->Int(val);
			}
			else if (typeid(bool) == typeid(val))
			{
				m_pwriter->Key(key);
				m_pwriter->Bool(val);
			}			
			else if (typeid(unsigned char) == typeid(val)
				|| (typeid(unsigned int) == typeid(val))
				|| (typeid(unsigned short) == typeid(val))
#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
#else
				|| (typeid(unsigned long int) == typeid(val))
				|| (typeid(unsigned long ) == typeid(val))				
#endif
				)
			{
				m_pwriter->Key(key);
				m_pwriter->Uint(val);
			}
			else if (typeid(int64_t) == typeid(val)
#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
				|| (typeid(signed long int) == typeid(val))
				|| (typeid(signed long ) == typeid(val))
#endif
				|| (typeid(signed long long int) == typeid(val))
				|| (typeid(signed long long ) == typeid(val))
				)
			{
				m_pwriter->Key(key);
				m_pwriter->Int64(val);
			}
			else if (typeid(uint64_t) == typeid(val)
#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
				|| (typeid(unsigned long int) == typeid(val))
				|| (typeid(unsigned long ) == typeid(val))
#endif
				|| (typeid(unsigned long long int) == typeid(val))
				|| (typeid(unsigned long long ) == typeid(val))
				)
			{
				m_pwriter->Key(key);
				m_pwriter->Uint64(val);
			}
			else if (typeid(float) == typeid(val)
				|| (typeid(double) == typeid(val)))
			{
				m_pwriter->Key(key);
				m_pwriter->Double(val);
			}
		}

		template<class ty>
		inline void wirte_common_arry_to_json(ty& val)
		{
			/* 这里用重载区分类型比较麻烦，暂时采用条件判断 */
			if (typeid(signed char) == typeid(val)
				|| (typeid(signed int) == typeid(val))
				|| (typeid(signed short) == typeid(val))
#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
#else				
				|| (typeid(signed long int) == typeid(val))
				|| (typeid(signed long ) == typeid(val))				
#endif
			)
			{
				m_pwriter->Int(val);
			}
			else if (typeid(bool) == typeid(val))
			{
				m_pwriter->Bool(val);
			}			
			else if (typeid(unsigned char) == typeid(val)
				|| (typeid(unsigned int) == typeid(val))
				|| (typeid(unsigned short) == typeid(val))
#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
#else				
				|| (typeid(unsigned long int) == typeid(val))
				|| (typeid(unsigned long ) == typeid(val))				
#endif
			)
			{
				m_pwriter->Uint(val);
			}
			else if (typeid(int64_t) == typeid(val)
#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
				|| (typeid(signed long int) == typeid(val))
				|| (typeid(signed long ) == typeid(val))
#else				
#endif			
				|| (typeid(signed long long int) == typeid(val))
				|| (typeid(signed long long ) == typeid(val))
			)
			{
				m_pwriter->Int64(val);
			}
			else if (typeid(uint64_t) == typeid(val)
#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
				|| (typeid(unsigned long int) == typeid(val))
				|| (typeid(unsigned long ) == typeid(val))
#else				
#endif			
				|| (typeid(unsigned long long int) == typeid(val))
				|| (typeid(unsigned long long ) == typeid(val))
			)
			{
				m_pwriter->Uint64(val);
			}
			else if (typeid(float) == typeid(val)
				|| (typeid(double) == typeid(val)))
			{
				m_pwriter->Double(val);
			}
		}

		template<class ty>
		inline void write_array_val(const char* key, ty& val, typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			wirte_json(key, val);
		}

		template<class ty>
		inline void write_array_val(const char* key, ty& val, typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			typedef typename std::remove_cv<ty>::type rty;
			serialize_impl<rty>::write(key, val);
		}

		template<class ty, int len>
		inline void write_val(const char* key, ty (&val)[len], typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			if (typeid(ty) == typeid(char))
			{
				m_pwriter->Key(key); 
				m_pwriter->String(reinterpret_cast<const char*>(val));
			}
			else
			{
				m_pwriter->Key(key);
				m_pwriter->StartArray();
				for (int i = 0; i < len; i++)
				{
					if (std::is_enum<ty>::value)
					{
						m_pwriter->Int(val[i]);
					}
					else
					{
						wirte_common_arry_to_json(val[i]);
					}					
				}
				m_pwriter->EndArray();
			}				
		}

		template<class ty, int len>
		inline void write_val(const char* key, ty(&val)[len], typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			m_pwriter->Key(key);
			m_pwriter->StartArray();
			for (int i = 0; i < len; i++)
			{
				m_pwriter->StartObject();
				write_array_val(key, val[i]);
				m_pwriter->EndObject();
			}
			m_pwriter->EndArray();
		}

		template<class ty>
		inline void write_val(const char* key, ty& val, typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			if (std::is_enum<ty>::value)
			{
				m_pwriter->Key(key);
				m_pwriter->Int(val);
			}
			else
			{
				wirte_json(key, val);
			}			
		}

		template<class ty>
		inline void write_val(const char* key, ty& val, typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			m_pwriter->Key(key);
			m_pwriter->StartObject();
			typedef typename std::remove_cv<ty>::type rty;
			serialize_impl<rty>::write(key, val);
			m_pwriter->EndObject();
		}

		// 最终递归函数
		void  write_members(std::vector<std::string>& szValueName, int& iSzIndx)
		{
			return;
		}

		// 展开函数
		template<typename head, typename... args>
		void  write_members(std::vector<std::string>& szValueName, int& iSzIndx, head& h, args& ... args_)
		{
			serialize::write_val(szValueName[iSzIndx].c_str(), h);
			if (sizeof...(args))
			{
				iSzIndx++;
				write_members(szValueName, iSzIndx, args_...);
			}
		}
		
		/* 加载数据 */
		std::string *m_pjson_content = nullptr;
		rapidjson::Document m_dom;
		std::mutex m_reader_mux;
		bool bParseSuccess;

		template<class ty = bool>
		inline void read_json(bool& val, const rapidjson::Value& jsonObj)
		{
			val = static_cast<bool>(jsonObj.GetBool());
		}

		template<class ty = signed char>
		inline void read_json(signed char& val, const rapidjson::Value& jsonObj)
		{
			val = static_cast<signed char>(jsonObj.GetInt());
		}

		template<class ty = signed short>
		inline void read_json(signed short& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetInt();
		}
		
		template<class ty= signed int>
		inline void read_json(signed int& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetInt();
		}

		template<class ty = signed long long int>
		inline void read_json(signed long long int& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetInt64();
		}

#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
		template<class ty = signed long int>
		inline void read_json(signed long int& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetInt64();
		}
#else
		template<class ty = signed long int>
		inline void read_json(signed long int& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetInt();
		}		
#endif

		template<class ty = unsigned char>
		inline void read_json(unsigned char& val, const rapidjson::Value& jsonObj)
		{
			val = static_cast<unsigned char>(jsonObj.GetUint());
		}

		template<class ty = unsigned short>
		inline void read_json(unsigned short& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetUint();
		}

		template<class ty = unsigned int>
		inline void read_json(unsigned int& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetUint();
		}

		template<class ty = unsigned long long int>
		inline void read_json(unsigned long long int& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetUint64();
		}		

#if defined(WIN64) || defined(_WIN64) || defined(__x86_x64__) || defined(_M_ARM64) || defined(__BIT64__)
		template<class ty = unsigned long int>
		inline void read_json(unsigned long int& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetUint64();
		}
#else
		template<class ty = unsigned long int>
		inline void read_json(unsigned long int& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetUint();
		}
#endif
		template<class ty = float>
		inline void read_json(float& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetDouble();
		}

		template<class ty = double>
		inline void read_json(double& val, const rapidjson::Value& jsonObj)
		{
			val = jsonObj.GetDouble();
		}

		template<class ty>
		inline void read_json(ty& val, const rapidjson::Value& jsonObj)
		{
		}

		template<class ty>
		inline void load_array_val(const char* key, ty& val, const rapidjson::Value& jsonObj, typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			read_json(key, val, jsonObj);
		}

		template<class ty>
		inline void load_array_val(const char* key, ty& val, const rapidjson::Value& jsonObj, typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			typedef typename std::remove_cv<ty>::type rty;
			//const rapidjson::Value& obj = jsonObj[key];
			serialize_impl<rty>::read(key, val, jsonObj);
		}

		template<int len>
		inline void load_val(const char* key, char(&val)[len], const rapidjson::Value& jsonObj)
		{
			//if (typeid(ty) == typeid(char))
			{
				snprintf(val, len, "%s", jsonObj.GetString());
			}
		}

		template<class ty, int len>
		inline void load_val(const char* key, ty(&val)[len], const rapidjson::Value& jsonObj, typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			int32_t i32ArraySize = jsonObj.Size() > len ? len : jsonObj.Size();
			for (int i = 0; i < i32ArraySize; i++)
			{
				read_json(val[i], jsonObj[i]);
			}
		}

		template<class ty, int len>
		inline void load_val(const char* key, ty(&val)[len], const rapidjson::Value& jsonObj, typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			int32_t i32ArraySize = jsonObj.Size() > len ? len : jsonObj.Size();
			for (int i = 0; i < i32ArraySize; i++)
			{
				load_array_val(key, val[i], jsonObj[i]);
			}
		}

		template<class ty = void*>
		inline void read_enum_json(void* val, const rapidjson::Value& jsonObj)
		{
			*(reinterpret_cast<int *>(val)) = jsonObj.GetInt();
		}

		template<class ty>
		inline void load_val(const char* key, ty& val, const rapidjson::Value& jsonObj, typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			if (std::is_enum<ty>::value)
			{
				read_enum_json((void*)&val, jsonObj);
			}
			else
			{
				read_json(val, jsonObj);
			}
		}

		template<class ty>
		inline void load_val(const char* key, ty& val, const rapidjson::Value& jsonObj, typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			typedef typename std::remove_cv<ty>::type rty;
			serialize_impl<rty>::read(key, val, jsonObj);
		}

		template<class ty, int len>
		inline void read_val(const char* key, ty(&val)[len], typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			if(m_dom.HasMember(key))
			{
				const rapidjson::Value& obj = m_dom[key];
				load_val(key, val, obj);
			}
			else
			{

			}
		}

		template<class ty, int len>
		inline void read_val(const char* key, ty(&val)[len], typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			if(m_dom.HasMember(key))
			{
				const rapidjson::Value& obj = m_dom[key];
				load_val(key, val, obj);
			}
			else
			{

			}
		}

		template<class ty>
		inline void read_val(const char* key, ty& val, typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			if(m_dom.HasMember(key))
			{
				const rapidjson::Value& obj = m_dom[key];
				load_val(key, val, obj);
			}
			else
			{

			}
		}

		template<class ty>
		inline void read_val(const char* key, ty& val, typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			if(m_dom.HasMember(key))
			{
				const rapidjson::Value& obj = m_dom[key];
				load_val(key, val, obj);
			}
			else
			{

			}
		}

		////
		template<class ty, int len>
		inline void read_member_val(const char* key, ty(&val)[len], const rapidjson::Value& jsonObj, typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			if(jsonObj.HasMember(key))
			{
				const rapidjson::Value& obj = jsonObj[key];
				load_val(key, val, obj);
			}
			else
			{

			}
		}

		template<class ty, int len>
		inline void read_member_val(const char* key, ty(&val)[len], const rapidjson::Value& jsonObj, typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			if(jsonObj.HasMember(key))
			{			
				const rapidjson::Value& obj = jsonObj[key];
				load_val(key, val, obj);
			}
			else
			{

			}
		}

		template<class ty>
		inline void read_member_val(const char* key, ty& val, const rapidjson::Value& jsonObj, typename std::enable_if<!std::is_class<ty>::value, void>::type* dump = 0)
		{
			if(jsonObj.HasMember(key))
			{			
				const rapidjson::Value& obj = jsonObj[key];
				load_val(key, val, obj);
			}
			else
			{

			}
		}

		template<class ty>
		inline void read_member_val(const char* key, ty& val, const rapidjson::Value& jsonObj, typename std::enable_if<std::is_class<ty>::value, void>::type* dump = 0)
		{
			if(jsonObj.HasMember(key))
			{			
				const rapidjson::Value& obj = jsonObj[key];
				load_val(key, val, obj);
			}
			else
			{

			}
		}

		// 最终递归函数
		void  load_members(std::vector<std::string>& szValueName, int& iSzIndx, const rapidjson::Value& jsonObj)
		{
			return;
		}

		// 展开函数
		template<typename head, typename... args>
		void  load_members(std::vector<std::string>& szValueName, int& iSzIndx, const rapidjson::Value& jsonObj, head& h, args& ... args_)
		{
			serialize::read_member_val(szValueName[iSzIndx].c_str(), h, jsonObj);
			if (sizeof...(args))
			{
				iSzIndx++;
				load_members(szValueName, iSzIndx, jsonObj, args_...);
			}
		}
	}
}

/* 启动参数存档 ，只有调用了他，存档才有效*/
#define ARCHIVE_OPEN_WRITER(filename) do{\
	zp_archive::serialize::m_writer_mux.lock();\
	if(nullptr != zp_archive::serialize::m_pstream \
		|| nullptr != zp_archive::serialize::m_pwriter \
		|| nullptr != zp_archive::serialize::m_pfilename \
		)\
	{\
	}\
	else\
	{\
		zp_archive::serialize::m_pstream = new rapidjson::StringBuffer; \
		zp_archive::serialize::m_pwriter = new rapidjson::PrettyWriter<rapidjson::StringBuffer>(*(zp_archive::serialize::m_pstream));\
		zp_archive::serialize::m_pfilename = new std::string(filename);\
		zp_archive::serialize::m_pwriter->StartObject(); \
	}\
}while(0)

/* 关闭参数存档，会释放资源，并将档案写入文件 */
#define ARCHIVE_CLOSE_WRITER() do{\
	if(zp_archive::serialize::m_pwriter)\
	{\
		zp_archive::serialize::m_pwriter->EndObject();\
		std::ofstream outfile;\
		outfile.open(zp_archive::serialize::m_pfilename->c_str());\
		if (outfile.is_open()) {\
			const char* json_content = zp_archive::serialize::m_pstream->GetString();\
			outfile << json_content << std::endl;\
			std::cout << "-----------------------\narchive json:\n-----------------------\n" << json_content << "\n";\
			outfile.close();\
			outfile.flush();\
		}\
		\
		delete zp_archive::serialize::m_pwriter;\
		zp_archive::serialize::m_pwriter = nullptr;\
	}\
	if (zp_archive::serialize::m_pstream)\
	{\
		delete zp_archive::serialize::m_pstream;\
		zp_archive::serialize::m_pstream = nullptr;\
	}\
	if (zp_archive::serialize::m_pfilename)\
	{\
		delete zp_archive::serialize::m_pfilename;\
		zp_archive::serialize::m_pfilename = nullptr;\
	}\
	zp_archive::serialize::m_writer_mux.unlock();\
}while(0)

/* 将数据存档 */
#define ARCHIVE_SAVE_VALUE(value) do{\
	zp_archive::serialize::write_val(#value, value);\
}while(0)


/* 初始化读档案时使用的资源 */
#define ARCHIVE_OPEN_READER(filename) do{\
	zp_archive::serialize::m_reader_mux.lock();\
	if(nullptr != zp_archive::serialize::m_pjson_content \
		)\
	{\
	}\
	else\
	{\
		std::ifstream in(filename);\
		if (!in.is_open()) {\
			fprintf(stderr, "fail to read json file: %s\n", filename);\
		}\
		else\
		{\
			zp_archive::serialize::m_pjson_content = new std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());\
			in.close();\
			zp_archive::serialize::bParseSuccess = !(zp_archive::serialize::m_dom.Parse(zp_archive::serialize::m_pjson_content->c_str()).HasParseError());\
		}\
	}\
}while(0)

/* 释放读档案时使用的资源 */
#define ARCHIVE_CLOSE_READER() do{\
	zp_archive::serialize::bParseSuccess = false;\
	if(nullptr != zp_archive::serialize::m_pjson_content)\
	{\
		delete zp_archive::serialize::m_pjson_content;\
		zp_archive::serialize::m_pjson_content = nullptr;\
	}\
	zp_archive::serialize::m_reader_mux.unlock();\
}while(0)

/* 读档案中的具体数据 */
#define ARCHIVE_LOAD_VALUE(value) do{\
	if(zp_archive::serialize::bParseSuccess)\
	{\
		zp_archive::serialize::read_val(#value, value);\
	}\
}while(0)

/* 每一个要存档的结构体，都需要使用这个宏将各个成员变量加入存档表单，否则对结构体进行存档时，会报错 */
#define ADD_PARAM_CLASS_TO_TEMPLATE(TYPE,...) \
namespace zp_archive {\
	namespace serialize\
	{\
		template<>\
		struct serialize_impl < TYPE , void >\
		{\
			struct serialize_helper : public TYPE\
			{\
				inline void write_(const char* key)\
				{\
					int i32MemberIndx = 0;\
					std::string str(#__VA_ARGS__);\
					earseSpace(str);\
					std::vector<std::string> szValueName = Split(str, ",");\
					write_members(szValueName, i32MemberIndx, __VA_ARGS__);\
				}\
				inline void read_(const char* key, const rapidjson::Value& jsonObj)\
				{\
					int i32MemberIndx = 0;\
					std::string str(#__VA_ARGS__);\
					earseSpace(str);\
					std::vector<std::string> szValueName = Split(str, ",");\
					load_members(szValueName, i32MemberIndx, jsonObj, __VA_ARGS__);\
				}\
				vector<string> Split(const string& in, const string delim)\
				{\
					std::vector<string> ret;\
					try\
					{\
						regex re{ delim };\
						return vector<string>{\
							std::sregex_token_iterator(in.begin(), in.end(), re, -1),\
							std::sregex_token_iterator()\
						};\
					}\
					catch (const std::exception& e)\
					{\
						cout << "error:" << e.what() << std::endl;\
					}\
					return ret;\
				}\
				void earseSpace(std::string &str)\
				{\
					uint64_t index = 0; \
					if (!str.empty())\
					{\
						std::regex newlines_re("\r\n+");\
						str = std::regex_replace(str, newlines_re, "");\
						while ((index = str.find(' ', index)) != std::string::npos)\
						{\
							str.erase(index, 1); \
						}\
					}\
				}\
			};\
			static inline void write(const char *key, TYPE & v)\
			{\
				reinterpret_cast<serialize_helper &>(v).write_(key);\
			}\
			static inline void read(const char *key, TYPE & v, const rapidjson::Value& jsonObj)\
			{\
				reinterpret_cast<serialize_helper &>(v).read_(key, jsonObj);\
			}\
		};\
	}\
}

