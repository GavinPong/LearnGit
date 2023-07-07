#include "JsonArchiveAlgorithm.h"
#include <typeinfo>
#include <stdint.h>

#ifdef _WIN32
#pragma warning(disable:4996) 
#endif

using namespace std;


struct coordinates{
	int i32X;
	int i32Y;
	int i32Z;
};
ADD_PARAM_CLASS_TO_TEMPLATE(coordinates, i32X, i32Y, i32Z)

enum grade_e {
	GRAGE_PERFECT,
	GRAGE_GOOD,
	GRAGE_GENERAL,
};

struct studentRank{
	int i32Ranking;
	enum grade_e enGrade;
};
ADD_PARAM_CLASS_TO_TEMPLATE(studentRank, i32Ranking, enGrade)

struct subjectAchievement {
	int i32Score[7];
	struct studentRank stRank;
};
ADD_PARAM_CLASS_TO_TEMPLATE(subjectAchievement, i32Score, stRank)

class Person {
public:
	char m_strName[32];
	int m_i32Age;
	Person():m_i32Age(16)
	{
		snprintf(m_strName, sizeof(m_strName), "San Zhang");
	}
};
ADD_PARAM_CLASS_TO_TEMPLATE(Person, m_strName, m_i32Age)

class Student : public Person {
public:
	char m_strSchool[64];
	struct subjectAchievement stSubjectAchieve;
	struct coordinates stSchoolCoordinates;
	Student()
	{
		#ifdef _WIN32
		_snprintf(m_strSchool, sizeof(m_strSchool), "%s", "Peking University");
		#else
		snprintf(m_strSchool, sizeof(m_strSchool), "%s", "Peking University");
		#endif
		for (uint32_t i = 0; i < sizeof(stSubjectAchieve.i32Score) / sizeof(stSubjectAchieve.i32Score[0]); i++)
		{
			stSubjectAchieve.i32Score[i] = 76 + i;
		}
		stSubjectAchieve.stRank.i32Ranking = 2;
		stSubjectAchieve.stRank.enGrade = GRAGE_GOOD;
		stSchoolCoordinates.i32X = 90;
		stSchoolCoordinates.i32Y = 180;
		stSchoolCoordinates.i32Z = 270;
	}
};
/* 基类的属性，如果想存档，也要添加到模板，如m_strName, m_i32Age继承于Person*/
ADD_PARAM_CLASS_TO_TEMPLATE(Student, m_strSchool, stSubjectAchieve, stSchoolCoordinates, m_strName, m_i32Age)


typedef enum chn_type_e{
	CHN_TYPE_LOCAL,
	CHN_TYPE_FILE,
	CHN_TYPE_REMOTE,
}chn_type_e;


#if 0 //可以将所有需要添加到模板的结构体，放在同一处，也可以在各个类和结构体定义的下面添加
namespace zp_archive
{
	ADD_PARAM_CLASS_TO_TEMPLATE(rank, i32Ranking, enGrade)
	ADD_PARAM_CLASS_TO_TEMPLATE(subjectAchievement, i32Score, stRank)
	ADD_PARAM_CLASS_TO_TEMPLATE(Person, m_strName, m_i32Age)
	ADD_PARAM_CLASS_TO_TEMPLATE(Student, m_strSchool, stSubjectAchieve, stPoint, m_strName, m_i32Age)
}
#endif

Student objectStudent;
int32_t i32Value = 20220504;
chn_type_e enChnType = CHN_TYPE_FILE;
char szName[32] = "This is an array of characters";
int szi32Value[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
double szDValue[8] = { 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8 };
struct coordinates szstCoordinates[2] = { {80, 69, 53}, {120, 109, 280} };
bool bSuccess = false;
bool bszSuccess[8] = {false, true, false, true};

void modifyData()
{
	std::cout << "\n-------------------------\nStart parse json file:\n-------------------------\n";
	//为验证读取的数据是否正确，先修改存储在内存中的数据
	memset(szName, 0, sizeof(szName));
	memset(szi32Value, 0, sizeof(szi32Value));
	std::cout << szName << "\n";
	enChnType = CHN_TYPE_REMOTE;
	i32Value = 100000;

	#ifdef _WIN32
	_snprintf(objectStudent.m_strSchool, sizeof(objectStudent.m_strSchool), "%s", "Do3Think");
	#else
	snprintf(objectStudent.m_strSchool, sizeof(objectStudent.m_strSchool), "%s", "Do3Think");
	#endif
	for (uint32_t i = 0; i < sizeof(objectStudent.stSubjectAchieve.i32Score) / sizeof(objectStudent.stSubjectAchieve.i32Score[0]); i++)
	{
		objectStudent.stSubjectAchieve.i32Score[i] = 1 + i;
	}
	objectStudent.stSubjectAchieve.stRank.i32Ranking = 50;
	objectStudent.stSubjectAchieve.stRank.enGrade = GRAGE_GENERAL;
	objectStudent.stSchoolCoordinates.i32X = 0;
	objectStudent.stSchoolCoordinates.i32Y = 0;
	objectStudent.stSchoolCoordinates.i32Z = 0;
	#ifdef _WIN32
	_snprintf(objectStudent.m_strName, sizeof(objectStudent.m_strName), "%s", "DoThinkKey");
	#else
	snprintf(objectStudent.m_strName, sizeof(objectStudent.m_strName), "%s", "DoThinkKey");
	#endif
	objectStudent.m_i32Age = 88;
}

void printLoadedData()
{
	std::cout << "i32Value:" << i32Value << "\n";
	std::cout << "enChnType:" << enChnType << "\n";

	std::cout << "objectStudent{\n";
	std::cout << "\tm_strSchool:" << objectStudent.m_strSchool << "\n";
	std::cout << "\tsubjectAchievement{\n";
	std::cout << "\t\ti32Score[" << "\n";
	for (uint32_t i = 0; i < sizeof(objectStudent.stSubjectAchieve.i32Score) / sizeof(objectStudent.stSubjectAchieve.i32Score[0]); i++)
	{
		std::cout << "\t\t\t" << objectStudent.stSubjectAchieve.i32Score[i] << "\n";
	}
	std::cout << "\t\t]" << "\n";
	std::cout << "\t\tstRank{\n";
	std::cout << "\t\t\ti32Ranking:" << objectStudent.stSubjectAchieve.stRank.i32Ranking << "\n";
	std::cout << "\t\t\tenGrade:" << objectStudent.stSubjectAchieve.stRank.enGrade << "\n";
	std::cout << "\t\t\tstSchoolCoordinates{\n";
	std::cout << "\t\t\t\ti32X:" << objectStudent.stSchoolCoordinates.i32X << "\n";
	std::cout << "\t\t\t\ti32Y:" << objectStudent.stSchoolCoordinates.i32Y << "\n";
	std::cout << "\t\t\t\ti32Z:" << objectStudent.stSchoolCoordinates.i32Z << "\n";
	std::cout << "\t\t\t}\n";
	std::cout << "\t\t}\n";
	std::cout << "\t}\n";
	std::cout << "\tm_strName:" << objectStudent.m_strName << "\n";
	std::cout << "\tm_i32Age:" << objectStudent.m_i32Age << "\n";
	std::cout << "}\n";
	std::cout << "szName:" << szName << "\n";
	std::cout << "szi32Value[" << "\n";
	for (uint32_t i = 0; i < sizeof(szi32Value) / sizeof(szi32Value[0]); i++)
	{
		std::cout << "\t" << szi32Value[i] << "\n";
	}
	std::cout << "]" << "\n";
	std::cout << "szDValue[" << "\n";
	for (uint32_t i = 0; i < sizeof(szDValue) / sizeof(szDValue[0]); i++)
	{
		std::cout << "\t" << szDValue[i] << "\n";
	}
	std::cout << "]" << "\n";
	std::cout << "szstCoordinates[\n";
	for (uint32_t i = 0; i < sizeof(szstCoordinates) / sizeof(szstCoordinates[0]); i++)
	{
		std::cout << "\t{\n";
		std::cout << "\t\ti32X:" << szstCoordinates[i].i32X << "\n";
		std::cout << "\t\ti32Y:" << szstCoordinates[i].i32Y << "\n";
		std::cout << "\t\ti32Z:" << szstCoordinates[i].i32Z << "\n";
		std::cout << "\t}\n";
	}
	std::cout << "]\n";
	std::cout << "bSuccess:" << bSuccess << "\n";
	std::cout << "bszSuccess[" << "\n";
	for (uint32_t i = 0; i < sizeof(bszSuccess) / sizeof(bszSuccess[0]);i++)
	{
		std::cout << "\t" << bszSuccess[i] << "\n";
	}
	std::cout << "]\n";
}

void archiveTest(const char *pstrJsonFile)
{	
	/* 测试存档案 */
	ARCHIVE_OPEN_WRITER(pstrJsonFile);
	ARCHIVE_SAVE_VALUE(i32Value);
	ARCHIVE_SAVE_VALUE(enChnType);
	#if 1
	ARCHIVE_SAVE_VALUE(objectStudent);
	#endif
	ARCHIVE_SAVE_VALUE(szName);
	ARCHIVE_SAVE_VALUE(szi32Value);
	ARCHIVE_SAVE_VALUE(szDValue);
	ARCHIVE_SAVE_VALUE(szstCoordinates);
	ARCHIVE_SAVE_VALUE(bSuccess);
	ARCHIVE_SAVE_VALUE(bszSuccess);
	ARCHIVE_CLOSE_WRITER();

	/* 测试读档案 */
	modifyData();//为检验读取的数据是否正确，先修改内存中的数据

	ARCHIVE_OPEN_READER(pstrJsonFile);
	ARCHIVE_LOAD_VALUE(bSuccess);
	ARCHIVE_LOAD_VALUE(bszSuccess);
	ARCHIVE_LOAD_VALUE(i32Value);
	ARCHIVE_LOAD_VALUE(enChnType);
	ARCHIVE_LOAD_VALUE(objectStudent);
	ARCHIVE_LOAD_VALUE(szName);
	ARCHIVE_LOAD_VALUE(szi32Value);
	ARCHIVE_LOAD_VALUE(szDValue);
	ARCHIVE_LOAD_VALUE(szstCoordinates);
	ARCHIVE_CLOSE_READER();	

	//打印读取后的数据	
	printLoadedData();
}
