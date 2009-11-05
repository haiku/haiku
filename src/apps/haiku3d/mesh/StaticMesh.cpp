/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#include "StaticMesh.h"

#include <Application.h>
#include <Resources.h>
#include <Roster.h>
#include <File.h>
#include <String.h>

#include <stdlib.h>
#include <stdio.h>

StaticMesh::StaticMesh(const char* name)
	:
	fFaces(NULL),
	fFaceCount(0)
{
	// TODO : move that in a utility class
	//BString fileName;
	//fileName << "data/" << name << ".hk3d";
	//_LoadText(fileName);

	//fileName << ".bin";
	//_WriteBinary(fileName);

	_ReadResource(name);
}


StaticMesh::~StaticMesh()
{
	delete [] fFaces;
}


void
StaticMesh::_ReadText(const char* fileName)
{
	FILE* f = fopen(fileName, "r");
	if (f == NULL) {
		printf("Mesh::_ReadText, error accessing %s\n", fileName);
		return;
	}

	fscanf(f, "%lu", &fFaceCount);
	fFaces = new Face[fFaceCount];

	for (uint32 i = 0; i < fFaceCount; i++) {

		uint32 vertexCount = 0;
		fscanf(f, "%lu", &vertexCount);
		fFaces[i].vertexCount = vertexCount;

		for (uint32 vi = 0; vi < vertexCount; vi++) {
			float x, y, z, u, v;
			fscanf(f, "%f %f %f %f %f",	&x,	&y,	&z,	&v,	&u);
			fFaces[i].v[vi].p.setValue(x, y, z);
			fFaces[i].v[vi].u = v;
			fFaces[i].v[vi].v = 1.0 - u;
		}
	}

	fclose(f);
	printf("Mesh::_ReadText, loaded %s (%lu faces)\n", fileName, fFaceCount);
}


void
StaticMesh::_WriteBinary(const char* fileName)
{
	BFile file(fileName, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

	if (file.InitCheck() != B_OK) {
		printf("Mesh::_WriteBinary, error accessing %s\n", fileName);
		return;
	}

	file.Write(&fFaceCount, sizeof(uint32));
	for (uint32 i = 0; i < fFaceCount; i++) {
		file.Write(&fFaces[i].vertexCount, sizeof(uint16));
		for (uint32 vi = 0; vi < fFaces[i].vertexCount; vi++) {
			file.Write(&fFaces[i].v[vi], sizeof(Vertex));
		}
	}
	printf("Mesh::_WriteBinary, wrote %s (%lu faces)\n", fileName, fFaceCount);
}


void
StaticMesh::_ReadBinary(const char* fileName)
{
	BFile file(fileName, B_READ_ONLY);

	if (file.InitCheck() != B_OK) {
		printf("Mesh::_ReadBinary, error accessing %s\n", fileName);
		return;
	}

	file.Read(&fFaceCount, sizeof(uint32));
	fFaces = new Face[fFaceCount];
	for (uint32 i = 0; i < fFaceCount; i++) {
		file.Read(&fFaces[i].vertexCount, sizeof(uint16));
		for (uint32 vi = 0; vi < fFaces[i].vertexCount; vi++) {
			file.Read(&fFaces[i].v[vi], sizeof(Vertex));
		}
	}
	printf("Mesh::_ReadBinary, loaded %s (%lu faces)\n", fileName, fFaceCount);
}


void
StaticMesh::_ReadResource(const char* resourceName)
{
	// TODO: factorize with _ReadBinary
	app_info info;
	be_app->GetAppInfo(&info);
	BFile file(&info.ref, B_READ_ONLY);

	BResources res;
	if (res.SetTo(&file) != B_OK) {
		printf("Mesh::_ReadResource, error accessing resources data\n");
		return;
	}

	size_t size;
	const void* data = res.LoadResource(B_RAW_TYPE, resourceName, &size);
	if (data == NULL) {
		printf("Mesh::_ReadResource, can't access resource %s\n", resourceName);
		return;
	}

	BMemoryIO memoryIO(data, size);

	memoryIO.Read(&fFaceCount, sizeof(uint32));
	fFaces = new Face[fFaceCount];
	for (uint32 i = 0; i < fFaceCount; i++) {
		memoryIO.Read(&fFaces[i].vertexCount, sizeof(uint16));
		for (uint32 vi = 0; vi < fFaces[i].vertexCount; vi++) {
			memoryIO.Read(&fFaces[i].v[vi], sizeof(Vertex));
		}
	}
	printf("Mesh::_ReadResource, loaded %s (%lu faces)\n", resourceName,
		fFaceCount);
}


Face&
StaticMesh::GetFace(uint32 index) const
{
	return fFaces[index];
}


uint32
StaticMesh::FaceCount() const
{
	return fFaceCount;
}
