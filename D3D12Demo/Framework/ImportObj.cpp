#include "ImportObj.h"
#include <fstream>
#include <regex>
#include <map>

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

CImportor_Obj::~CImportor_Obj()
{
	Clear();
}

void CImportor_Obj::SetPath(std::string path)
{
	m_ObjFilePath = path;
}

void CImportor_Obj::Import()
{
	ImportObjMesh();
	ProcessObjData();
}

void CImportor_Obj::Clear()
{
	m_ObjFilePath = "";

	for (UINT i = 0; i < m_MeshObjs.size(); ++i)
	{
		MeshData* pMesh = m_MeshObjs[i];
		delete pMesh; pMesh = nullptr;
	}
	m_MeshObjs.clear();

	for (UINT i = 0; i < m_ObjDatas.size(); ++i)
	{
		ObjData* pObj = m_ObjDatas[i];
		delete pObj; pObj = nullptr;
	}
	m_ObjDatas.clear();
	
	m_MeshNames.clear();
	m_MeshRanges.clear();
}

bool CImportor_Obj::ImportObjMesh()
{
	std::fstream fs;
	fs.open(m_ObjFilePath, std::ios::in);

	char buffer[128];

	// 先确定文件中有多少个模型，以及范围，在通过 getline() 读取数据时，多算了一个换行符，不知道原因
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
	
	// fs.eof() 结束后，文件出错，需要重置
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
	
	// 拆分数据
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
				//std::regex reg_vtx("^v(\\s+-?\\d+\\.\\d+){3}$");
				//if (std::regex_match(line, reg_vtx))
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
				//std::regex reg_vt("^vt(\\s+-?\\d+\\.\\d+){2}$");
				//if (std::regex_match(line, reg_vt))
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
				//std::regex reg_vtx("^vn(\\s+-?\\d+\\.\\d+){3}$");
				//if (std::regex_match(line, reg_vtx))
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
	// 重新组织数据
	for (UINT obj = 0; obj < m_ObjDatas.size(); ++obj)
	{
		std::map<FaceEntry, UINT> FaceIndexMap;

		ObjData* pObjData = m_ObjDatas[obj];
		MeshData* pMeshData = new MeshData();
		m_MeshObjs.push_back(pMeshData);

		for (UINT i = 0; i < pObjData->Faces.size(); ++i)
		{
			auto it = FaceIndexMap.find(pObjData->Faces[i]);
			if (it == FaceIndexMap.end())
			{
				const FaceEntry& fe = pObjData->Faces[i];
				pMeshData->Positions.push_back(pObjData->Positions[fe.posID - 1]);
				pMeshData->UVs.push_back(pObjData->UVs[fe.uvID - 1]);
				pMeshData->Normals.push_back(pObjData->Normals[fe.nmlID - 1]);

				UINT index = (UINT)pMeshData->Positions.size() - 1;
				pMeshData->Indices.push_back(index);
				FaceIndexMap.emplace(fe, index);
			}
			else
			{
				UINT index = it->second;
				pMeshData->Indices.push_back(index);
			}
		}
	}
}

bool CImportor_Obj::FaceEntry::operator<(const FaceEntry& f) const
{
	return	 this->posID < f.posID ? true : (this->posID == f.posID ?
			(this->uvID < f.uvID ? true : (this->uvID == f.uvID ?
			(this->nmlID < f.nmlID ? true : false) : false)) : false);
}
