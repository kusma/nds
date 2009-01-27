// collada_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FCollada.h"
#include "FUtils/FUAssert.h"
#include "FUtils/FUTestBed.h"

#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDMaterial.h"
#include "FCDocument/FCDEffectParameter.h"

#include "FCDocument/FCDGeometry.h"
#include "FCDocument/FCDGeometryMesh.h"
#include "FCDocument/FCDGeometrySource.h"
#include "FCDocument/FCDGeometryPolygons.h"
#include "FCDocument/FCDGeometryPolygonsInput.h"
#include "FCDocument/FCDGeometryPolygonsTools.h"

#include <FreeImage.h>

#include "gfxfifo.h"
#include <cassert>
#include <algorithm>
#include "compress_texture.h"

// FMVector3
void findFloatRange(FCDGeometrySource *source, int components, float *min, float *max)
{
	for (int j = 0; j < components; ++j)
	{
		min[j] = FLT_MAX;
		max[j] = FLT_MIN;
	}
	
	for (size_t i = 0; i < source->GetValueCount(); ++i)
	{
		const float *data = source->GetValue(i);
		for (int j = 0; j < components; ++j)
		{
			min[j] = std::min(min[j], data[j]);
			max[j] = std::max(max[j], data[j]);
		}
	}
}

void dumpMesh(FCDGeometryMesh *mesh, std::string filename)
{
	gfx_fifo::GfxFifoStream fifoStream;
	
	printf("\tid: \"%s\"\n\tpoly groups: %d\n", mesh->GetDaeId().c_str(), mesh->GetPolygonsCount());
	for (size_t j = 0; j < mesh->GetPolygonsCount(); ++j)
	{
		FCDGeometryPolygons *polys = mesh->GetPolygons(j);
		if (FCDGeometryPolygons::POLYGONS == polys->GetPrimitiveType())
		{
			FCDGeometryPolygonsTools::Triangulate(polys, false);
			fifoStream.begin(gfx_fifo::TRIANGLES);
		}
		
		fstring material = polys->GetMaterialSemantic();
		printf("\tmat: %s\n", material.c_str());
		
		bool normalizePositions = true;
		FCDGeometryPolygonsInput *positionIndices = polys->FindInput(FUDaeGeometryInput::POSITION);
		assert(NULL != positionIndices);
		
		// default identity transform
		FMVector3 posTrans(0.0f, 0.0f, 0.0f);
		FMVector3 posScale(1.0f, 1.0f, 1.0f);
		if (normalizePositions)
		{
			float posMin[3];
			float posMax[3];
			findFloatRange(positionIndices->GetSource(), 3, posMin, posMax);
			FMVector3 posRange(posMax[0] - posMin[0], posMax[1] - posMin[1], posMax[2] - posMin[2]);
			posTrans = FMVector3(-posMin[0], -posMin[1], -posMin[2]) - posRange / 2;
			posScale.x = ((1 << 16) - 1) / posRange.x;
			posScale.y = ((1 << 16) - 1) / posRange.y;
			posScale.z = ((1 << 16) - 1) / posRange.z;
		}
		
		printf("\tscale: %f %f %f\n", posScale.x, posScale.y, posScale.z);
		printf("\ttrans: %f %f %f\n", posTrans.x, posTrans.y, posTrans.z);
		printf("\tinv scale: %f %f %f\n", 1.0f / posScale.x, 1.0f / posScale.y, 1.0f / posScale.z);
		printf("\tinv trans: %f %f %f\n", -posTrans.x, -posTrans.y, -posTrans.z);

/*				FMMatrix44 transform;
		transform.Recompose(posScale, FMVector3(0, 0, 0), posTrans); */
		
		bool dumpNormals = true; // TODO: inspect material
		FCDGeometryPolygonsInput *normalIndices = NULL;
		if (dumpNormals) normalIndices = polys->FindInput(FUDaeGeometryInput::NORMAL);

		FCDGeometryPolygonsInputList texCoordsList;
		polys->FindInputs(FUDaeGeometryInput::TEXCOORD, texCoordsList);
		printf("\ttexcoords: %d\n", texCoordsList.size());
		
		bool dumpTexcoords = true; // true;
		FCDGeometryPolygonsInput *texcoordIndices = NULL;
		if (dumpTexcoords) texcoordIndices = polys->FindInput(FUDaeGeometryInput::TEXCOORD);
		
		bool dumpColors = false;
		FCDGeometryPolygonsInput *colorIndices = NULL;
		if (dumpColors) colorIndices = polys->FindInput(FUDaeGeometryInput::COLOR);
		
		printf("\tface count: %d\n\tindex count: %d\n", polys->GetFaceCount(), positionIndices->GetIndexCount() / 3);
		
		size_t index = 0;
		for (size_t f = 0; f < polys->GetFaceCount(); ++f)
		{
			if (FCDGeometryPolygons::TRIANGLE_STRIPS == polys->GetPrimitiveType())
			{
				fifoStream.begin(gfx_fifo::TRIANGLE_STRIP);
			}
			
			for (size_t k = 0; k < polys->GetFaceVertexCount(f); ++k)
			{
				if (texcoordIndices != NULL)
				{
					assert(index < texcoordIndices->GetIndexCount());
					int texcoordIndex = texcoordIndices->GetIndices()[index];
					const float *texcoordData = texcoordIndices->GetSource()->GetValue(texcoordIndex);
					int u = int(0.0f + texcoordData[0] * (1 << 9));
					int v = int(0.0f + texcoordData[1] * (1 << 9));
					fifoStream.texCoord(u, v);
				}
				
				if (normalIndices != NULL)
				{
					assert(index < normalIndices->GetIndexCount());
					int normalIndex = normalIndices->GetIndices()[index];
					const float *normalData = normalIndices->GetSource()->GetValue(normalIndex);
					int nx = int(normalData[0] * ((1 << 9) - 1));
					int ny = int(normalData[1] * ((1 << 9) - 1));
					int nz = int(normalData[2] * ((1 << 9) - 1));
					fifoStream.normal(nx, ny, nz);
				}
				
				if (colorIndices != NULL)
				{
					assert(index < colorIndices->GetIndexCount());
					int colorIndex = colorIndices->GetIndices()[index];
					const float *colorData = colorIndices->GetSource()->GetValue(colorIndex);
					int r = int(colorData[0] * ((1 << 5) - 1));
					int g = int(colorData[1] * ((1 << 5) - 1));
					int b = int(colorData[2] * ((1 << 5) - 1));
					fifoStream.color(r, g, b);
				}
				
				assert(index < positionIndices->GetIndexCount());
				int positionIndex = positionIndices->GetIndices()[index];
				const float *positionData = positionIndices->GetSource()->GetValue(positionIndex);
				FMVector3 pos(positionData[0], positionData[1], positionData[2]);
				pos += posTrans;
				pos = FMVector3(pos.x * posScale.x, pos.y * posScale.y, pos.z * posScale.z);
				int x = int(pos.x);
				int y = int(pos.y);
				int z = int(pos.z);
				fifoStream.position16(x, y, z);
				
				index++;
			}
			
			if (FCDGeometryPolygons::TRIANGLE_STRIPS == polys->GetPrimitiveType())
			{
				fifoStream.end();
			}
		}
		
		if (FCDGeometryPolygons::POLYGONS == polys->GetPrimitiveType())
		{
			fifoStream.end();
		}
	}
	
	// dump to file
	std::vector<unsigned int> result = fifoStream.finalize();
	{
		FILE *fp = fopen(filename.c_str(), "wb");
		fwrite(&result[0], sizeof(result[0]), result.size(), fp);
		fclose(fp);
		fp = NULL;
	}
}

int main(int argc, char* argv[])
{
	compressTexture("tex2.jpg");

//	return 0;

	FCollada::Initialize();
	FUErrorSimpleHandler errorHandler;

	FUObjectRef<FCDocument> document = FCollada::NewTopDocument();
	if (!FCollada::LoadDocumentFromFile(document, FC("test.dae")))
	{
		fprintf(stderr, "failed to load document - %d\n", errorHandler.IsSuccessful());
		exit(1);
	}

	FCDMaterialLibrary *matLib = document->GetMaterialLibrary();
	for (size_t i = 0; i < matLib->GetEntityCount(); ++i)
	{
		FCDMaterial *mat = matLib->GetEntity(i);
		printf("material %d : \"%s\"\n", i, mat->GetName().c_str());
//		mat->GetEffect()
		for (size_t j = 0; j < mat->GetEffectParameterCount(); ++j)
		{
			FCDEffectParameter *param = mat->GetEffectParameter(j);
//			param->GetN
		}
	}
	
	FCDGeometryLibrary *geoLib = document->GetGeometryLibrary();
	for (size_t i = 0; i < geoLib->GetEntityCount(); ++i)
	{
		FCDGeometry* geo = geoLib->GetEntity(i);
		printf("mesh %d : \"%s\"\n", i, geo->GetName().c_str());
		
		if (geo->IsMesh())
		{
			std::string filename = std::string(geo->GetName().c_str());
			filename += std::string(".dis");
			dumpMesh(geo->GetMesh(), filename);
		}
	}
	
	return 0;
}

