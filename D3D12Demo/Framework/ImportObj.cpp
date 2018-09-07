#include "ImportObj.h"
#include <fstream>
#include <regex>

void parse_float_in_string(std::string text, std::vector<float>& data)
{
	static std::regex reg_float("(-?\\d+\\.\\d+)");
	std::smatch sm;
	while (std::regex_search(text, sm, reg_float))
	{
		data.push_back(StringToNumber<float>(sm[1].str()));
		text = sm.suffix().str();
	}
}

void parse_uint_in_string(std::string text, std::vector<UINT>& data)
{
	static std::regex reg_uint("([0-9]+)");
	std::smatch sm;
	while (std::regex_search(text, sm, reg_uint))
	{
		data.push_back(StringToNumber<UINT>(sm[1].str()));
		text = sm.suffix().str();
	}
}

bool CImportor_Obj::ImportObjMesh(std::string path)
{
	std::fstream fs;
	fs.open(path, std::ios::in);

	char buffer[128];

	// ��ȷ���ļ����ж��ٸ�ģ�ͣ��Լ���Χ����ͨ�� getline() ��ȡ����ʱ��������һ�����з�����֪��ԭ��
	int lineIndex = 0;
	while (!fs.eof())
	{
		UINT filePos = (UINT)fs.tellg();

		std::memset(buffer, 0, 128);
		fs.getline(buffer, 128);

		std::string line = buffer;
		if (buffer[0] == 'o')
		{
			//o Cube
			std::smatch sm;
			std::regex reg_Name("^o\\s+(\\w+)$");
			std::regex_match(line, sm, reg_Name);
			m_MeshNames.push_back(sm.str());
			m_MeshRanges.push_back(XMUINT2(filePos - lineIndex, 0));
		}
		lineIndex++;
	}
	
	// fs.eof() �������ļ���������Ҫ����
	// https://blog.csdn.net/stpeace/article/details/40693951
	fs.clear();

	if (m_MeshNames.size() == 0)
	{
		fs.close();
		return false;
	}
	else if (m_MeshNames.size() == 1)
	{
		fs.seekg(0, std::ios::end);
		m_MeshRanges[0].y = (UINT)fs.tellg();
	}
	else
	{
		for (int i = 0; i < m_MeshNames.size() - 1; ++i)
		{
			m_MeshRanges[i].y = m_MeshRanges[i + 1].x;
		}
		fs.seekg(0, std::ios::end);
		m_MeshRanges[m_MeshNames.size() - 1].y = (UINT)fs.tellg();
	}

	fs.clear();
	fs.seekg(0, std::ios::beg);
	
	// �������
	for (int i = 0; i < m_MeshNames.size(); ++i)
	{
		//MeshData* pMeshData = new MeshData();
		//m_MeshObjs.push_back(pMeshData);

		ObjData* pObjData = new ObjData();
		m_ObjDatas.push_back(pObjData);

		XMUINT2 range = m_MeshRanges[i];

		fs.seekg(0, std::ios::beg);
		fs.seekg(range.x, std::ios::beg);

		while ((UINT)fs.tellg() < range.y)
		{
			std::memset(buffer, 0, 128);
			fs.getline(buffer, 128);

			std::string line = buffer;

			if (buffer[0] == 'o')
			{
				std::cout << line << std::endl;
			}
			else if (buffer[0] == 'v' && buffer[1] == ' ')
			{
				// "v 1.000000 -1.000000 -1.000000"
				std::regex reg_vtx("^v(\\s+-?\\d+\\.\\d+){3}$");
				if (std::regex_match(line, reg_vtx))
				{
					std::vector<float> data;
					parse_float_in_string(line, data);
					if (data.size() == 3)
					{
						pObjData->Positions.push_back(XMFLOAT3(data[0], data[1], data[2]));
					}
				}
			}
			else if (buffer[0] == 'v' && buffer[1] == 't')
			{
				// "vt 0.833333 0.126139"
				std::regex reg_vt("^vt(\\s+-?\\d+\\.\\d+){2}$");
				if (std::regex_match(line, reg_vt))
				{
					std::vector<float> data;
					parse_float_in_string(line, data);
					if (data.size() == 2)
					{
						pObjData->UVs.push_back(XMFLOAT2(data[0], data[1]));
					}
				}
			}
			else if (buffer[0] == 'v' && buffer[1] == 'n')
			{
				// "vn - 0.9684 0.2114 0.1326"
				std::regex reg_vtx("^vn(\\s+-?\\d+\\.\\d+){3}$");
				if (std::regex_match(line, reg_vtx))
				{
					std::vector<float> data;
					parse_float_in_string(line, data);
					if (data.size() == 3)
					{
						pObjData->Normals.push_back(XMFLOAT3(data[0], data[1], data[2]));
					}
				}
			}
			else if (buffer[0] == 'f' && buffer[1] == ' ')
			{
				// "f 386/1/1 3/2/1 164/3/1"
				std::regex reg_face("^f(\\s+([0-9]+/){2}[0-9]+){3}$");
				if (std::regex_match(line, reg_face))
				{
					std::vector<UINT> data;
					parse_uint_in_string(line, data);
					if (data.size() == 9)
					{
						FaceEntry face;
						face = { data[0], data[1], data[2] };
						pObjData->Faces.push_back(face);

						face = { data[3], data[4], data[5] };
						pObjData->Faces.push_back(face);

						face = { data[6], data[7], data[8] };
						pObjData->Faces.push_back(face);
					}
				}
			}
		}
	}

	fs.close();
	return true;
}

void CImportor_Obj::ProcessObjData()
{
	


}