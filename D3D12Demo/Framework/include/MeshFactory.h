#pragma once
#include "stdafx.h"
#include "StaticMesh.h"
#include "GPUResource.h"

class CMeshFactory
{
public:
	// �ֱ�����ҪС�� 128 x 64
	static void CreateUVSphereMesh(int segments, int rings, std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indees);

	static void CreateUVSphereMesh(int segments, int rings, MeshData& mesh);

	static void CreateBox(std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indices);
};
