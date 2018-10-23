#include "MeshFactory.h"
#include "Utility.h"
#include "DirectXTex.h"
#include "DirectXTexExr.h"

void CMeshFactory::CreateUVSphereMesh(int segments, int rings, std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indees)
{
	if (segments >= 3 && rings >= 3)
	{
		for (int i = 1; i < rings; ++i)
		{
			float phi = XM_PI * i / rings;
			for (int j = 0; j < segments; ++j)
			{
				float theta = XM_2PI * j / segments;
				XMFLOAT3 p;
				p.x = std::sinf(phi) * std::cosf(theta);
				p.y = std::cosf(phi);
				p.z = std::sinf(phi) * std::sinf(theta);
				positions.push_back(p);
			}
		}
		positions.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
		positions.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));

		for (int i = 1; i < rings - 1; ++i)
		{
			for (int j = 0; j < segments - 1; ++j)
			{
				int p00 = (i - 1) * segments + j;		int p01 = (i - 1) * segments + j + 1;
				int p10 = (i - 1 + 1) * segments + j;	int p11 = (i - 1 + 1) * segments + j + 1;

				indees.push_back(p00);
				indees.push_back(p10);
				indees.push_back(p11);

				indees.push_back(p00);
				indees.push_back(p11);
				indees.push_back(p01);
			}

			int p00 = (i - 1) * segments + segments - 1;		int p01 = (i - 1) * segments;
			int p10 = (i - 1 + 1) * segments + segments - 1;	int p11 = (i - 1 + 1) * segments ;

			indees.push_back(p00);
			indees.push_back(p10);
			indees.push_back(p11);

			indees.push_back(p00);
			indees.push_back(p11);
			indees.push_back(p01);
		}

		int top = (int)positions.size() - 2;
		for (int j = 0; j < segments; ++j)
		{
			indees.push_back(top);
			indees.push_back(j);

			if (j == segments - 1)
			{
				indees.push_back(0);
			}
			else
			{
				indees.push_back(j + 1);
			}
		}
		
		int bottom = (int)positions.size() - 1;
		int bottomUp = segments * (rings - 2);
		for (int j = 0; j < segments; ++j)
		{
			indees.push_back(bottom);
			indees.push_back(bottomUp + j);
			if (j == segments - 1)
			{
				indees.push_back(bottomUp);
			}
			else
			{
				indees.push_back(bottomUp + j + 1);
			}
		}
	}
}

void CMeshFactory::CreateUVSphereMesh(int segments, int rings, MeshData & mesh)
{
	if (segments >= 3 && rings >= 3)
	{
		for (int i = 1; i < rings; ++i)
		{
			float phi = XM_PI * i / rings;
			for (int j = 0; j < segments; ++j)
			{
				float theta = XM_2PI * j / segments;
				XMFLOAT3 p;
				p.x = std::sinf(phi) * std::cosf(theta);
				p.y = std::cosf(phi);
				p.z = std::sinf(phi) * std::sinf(theta);
				mesh.Positions.push_back(p);
			}
		}
		mesh.Positions.push_back(XMFLOAT3(0.0f, 1.0f, 0.0f));
		mesh.Positions.push_back(XMFLOAT3(0.0f, -1.0f, 0.0f));

		for (int i = 1; i < rings - 1; ++i)
		{
			for (int j = 0; j < segments - 1; ++j)
			{
				int p00 = (i - 1) * segments + j;		int p01 = (i - 1) * segments + j + 1;
				int p10 = (i - 1 + 1) * segments + j;	int p11 = (i - 1 + 1) * segments + j + 1;
				
				mesh.Indices.push_back(p00);
				mesh.Indices.push_back(p10);
				mesh.Indices.push_back(p11);

				mesh.Indices.push_back(p00);
				mesh.Indices.push_back(p11);
				mesh.Indices.push_back(p01);
			}

			int p00 = (i - 1) * segments + segments - 1;		int p01 = (i - 1) * segments;
			int p10 = (i - 1 + 1) * segments + segments - 1;	int p11 = (i - 1 + 1) * segments;

			mesh.Indices.push_back(p00);
			mesh.Indices.push_back(p10);
			mesh.Indices.push_back(p11);

			mesh.Indices.push_back(p00);
			mesh.Indices.push_back(p11);
			mesh.Indices.push_back(p01);
		}

		int top = (int)mesh.Positions.size() - 2;
		for (int j = 0; j < segments - 1; ++j)
		{
			mesh.Indices.push_back(top);
			mesh.Indices.push_back(j);
			mesh.Indices.push_back(j + 1);
		}
		mesh.Indices.push_back(top);
		mesh.Indices.push_back(segments);
		mesh.Indices.push_back(0);

		int bottom = (int)mesh.Positions.size() - 1;
		int bottomUp = segments * (rings - 2);
		for (int j = 0; j < segments - 1; ++j)
		{
			mesh.Indices.push_back(bottom);
			mesh.Indices.push_back(bottomUp + j + 1);
			mesh.Indices.push_back(bottomUp + j);
		}
	}
}

void CMeshFactory::CreateBox(std::vector<XMFLOAT3>& positions, std::vector<UINT16>& indices)
{
	positions =
	{
		XMFLOAT3(-1.0f, -1.0f, -1.0f),
		XMFLOAT3(-1.0f, +1.0f, -1.0f),
		XMFLOAT3(+1.0f, +1.0f, -1.0f),
		XMFLOAT3(+1.0f, -1.0f, -1.0f),
		XMFLOAT3(-1.0f, -1.0f, +1.0f),
		XMFLOAT3(-1.0f, +1.0f, +1.0f),
		XMFLOAT3(+1.0f, +1.0f, +1.0f),
		XMFLOAT3(+1.0f, -1.0f, +1.0f)
	};

	indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};
}
