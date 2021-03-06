// Test.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <regex>
#include <iostream>
#include <map>
#include <Windows.h>

void TEST_regex();
void TEST_map();
void TEST_unique_ptr();
void TEST_shared_ptr();

int main()
{
	//TEST_regex();
	//TEST_map();

	//TEST_unique_ptr();
	TEST_shared_ptr();

	getchar();

    return 0;
}

void TEST_regex()
{
	// https://www.cnblogs.com/cycxtz/p/4804115.html
	// https://blog.csdn.net/mark555/article/details/22379757
	std::regex reg0(".*<.*>.*</.*>");
	std::string text0 = "XML <tag>hello</tag>";
	bool r = std::regex_match(text0, reg0);
	std::cout << r << std::endl;
	
	std::smatch search;
	std::regex reg1(".*<(.*)>(.*)</(\\1)>");
 	r = std::regex_search(text0, search, reg1);

	// Parse OBJ
	std::smatch s;
	std::string posText = "v 1.000000 -1.000000 -1.000000";
	std::regex regPos("v\\s+(-?\\d+\\.\\d+)\\s+(-?\\d+\\.\\d+)\\s+(-?\\d+\\.\\d+)$");
	r = std::regex_match(posText, s , regPos);

	// 

	std::string faceText = "f 287/280/22 44/472/22 116/281/22";
	std::regex reg_face("f(\\s+[0-9]+/[0-9]+/[0-9]+){3}");
	std::regex reg_face1("([0-9]+)/([0-9]+)/([0-9]+)");

	std::string test = faceText;
	while (std::regex_search(test, s, reg_face1))
	{
		for (int i = 0; i < s.size(); ++i)
		{
			std::cout << s[i] << " ";
		}
		std::cout << std::endl;
		test = s.suffix().str();
	}
	
	r = std::regex_match(faceText, s, reg_face);

	int a = 0;
}

void TEST_map()
{
	struct DataID
	{
		int x;
		int y;
		int z;

		bool operator < (const DataID& id) const
		{
			return	 this->x < id.x ? true : (this->x == id.x ? 
					(this->y < id.y ? true : (this->y == id.y ? 
					(this->z < id.z ? true : false) : false)) : false);

			/*if (this->x < id.x)
			{
				return true;
			}
			else if (this->x == id.x)
			{
				if (this->y < id.y)
				{
					return true;
				}
				else if (this->y == id.y)
				{
					if (this->z < id.z)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}*/
		}
	};

	std::map<DataID, int> testMap;

	DataID key0 = { 0, 0, 1};
	DataID key1 = { 0, 10, 1 };
	DataID key2 = { 0, 0, 1 };

	testMap[key0] = 1;
	testMap[key1] = 2;
	//testMap[key2] = 3;

	int a = 1;

}

void TEST_unique_ptr()
{
	// 测试Move操作时的 析构过程，在Move执行时，会将其原来引用的对象析构掉
	class Foo
	{
	public:
		Foo()
		{
			std::cout << "Construction." << std::endl;
		}
		~Foo()
		{
			std::cout << "Deconstruction." << id << std::endl;
		}

	public:
		int id = 0;
	};

	auto f1 = std::make_unique<Foo>();
	f1->id = 1;
	auto f2 = std::make_unique<Foo>();
	f2->id = 2;

	f2 = std::move(f1);

	int a = 1;
}

class Foo
{
public:
	Foo()
	{
		std::cout << "Construction. Foo" << std::endl;
		ptr = new int(2);
	}
	~Foo()
	{
		std::cout << "Deconstruction. Foo" << id << std::endl;
		delete ptr;
		ptr = nullptr;
	}

	
public:
	int id = 0;
	int* ptr = nullptr;
};

void test222(std::shared_ptr<Foo> dd)
{
	int a = 1;
}


void TEST_shared_ptr()
{
	{
		std::vector<std::shared_ptr<Foo>> libs;
		for (int i = 0; i < 10; ++i)
		{
			std::shared_ptr<Foo> foo = std::make_shared<Foo>();
			foo->id = i;
			libs.push_back(foo);
			//Sleep(10);
		}
		//Sleep(10);

		auto foo = libs[0];

		test222(foo);

		int a = 2;
	}

	int a = 1;


}
