
#pragma once
#include <string>
#include <vector>
#include "StaticMesh.h"

// ����������ʱ���Ǵ�1��ʼ��������0
// �����ݼ��У�����ʱһ�����ݣ�������һ�����ݣ�uv��һ�����ݣ����Ե�������������������ԭʼ���ݼ��У����ܻὫ��ͬ�ķ��߶��ϲ�����ֻ��һ������
// ��ע���face˳����� f v/vt/vn
// ��Ҫ���һ���Ƿ��������棬���ǵĻ�����ǰ�˳�
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
	std::vector<XMUINT2> m_MeshRanges; // ���ļ����е�λ��
};

void parse_float_in_string(std::string text, std::vector<float>& data);
void parse_uint_in_string(std::string text, std::vector<UINT>& data);