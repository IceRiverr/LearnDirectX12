
#pragma once
#include <string>
#include <vector>
#include "StaticMesh.h"

// 在索引顶点时，是从1开始，而不是0
// 在数据集中，顶点时一个数据，法线是一个数据，uv是一个数据，各自单独进行索引，所以在原始数据集中，可能会将相同的法线都合并，而只有一个法线
// 标注你的face顺序就是 f v/vt/vn
// 需要检测一下是否是三角面，不是的话，提前退出
// https://www.opengl.org/discussion_boards/showthread.php/126212-Interpreting-a-Wavefront-Obj
// https://nccastaff.bournemouth.ac.uk/jmacey/RobTheBloke/www/source/obj.html
// http://www.opengl-tutorial.org/miscellaneous/useful-tools-links/ 

class CImportor_Obj
{
public:
	class FaceEntry
	{
	public:
		UINT posID;
		UINT uvID;
		UINT nmlID;

		bool operator < (const FaceEntry& f) const;
	};

	struct ObjData
	{
		std::vector<XMFLOAT3> Positions;
		std::vector<XMFLOAT2> UVs;
		std::vector<XMFLOAT3> Normals;
		std::vector<FaceEntry> Faces;
	};

	~CImportor_Obj();

	void SetPath(std::string path);
	void Import();
	void Clear();

	std::vector<MeshData*> m_MeshObjs;
	std::vector<ObjData*> m_ObjDatas;

private:
	bool ImportObjMesh();
	void ProcessObjData();

private:
	std::string m_ObjFilePath;
	std::vector<std::string> m_MeshNames;
	std::vector<XMUINT2> m_MeshRanges; // 在文件流中的位置
};

void parse_float_in_string(std::string text, std::vector<float>& data);
void parse_uint_in_string(std::string text, std::vector<UINT>& data);