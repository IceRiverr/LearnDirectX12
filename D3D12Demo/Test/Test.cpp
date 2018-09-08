// Test.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <regex>
#include <iostream>
#include <map>

void TEST_regex();
void TEST_map();

int main()
{
	//TEST_regex();
	TEST_map();


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