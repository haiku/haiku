/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */

#include "MeshInstance.h"
#include "MathUtils.h"
#include <GL/gl.h>

//#include <stdio.h> // debug


MeshInstance::MeshInstance(Mesh* mesh, Texture* texture,
	const Vector3& position, const Quaternion& orientation, float animOffset)
	:
	fMeshReference(mesh),
	fTextureReference(texture),
	fPosition(position),
	fOrientation(orientation),
	fScale(1.0f),
	fTime(0.0f),
	fAnimOffset(animOffset),
	fDoubleSided(true),
	fDrawNormals(false)
{
}


MeshInstance::~MeshInstance()
{
}


void
MeshInstance::Update(float dt)
{
	fTextureReference->Update(dt);

	float animDuration = 4.0f;

	fTime += dt;
	if (fTime >= fAnimOffset) {
		float animTime = (fTime - fAnimOffset);

		float rotAngle = MathUtils::EaseInOutQuart(animTime, 0,
			3 * 2 * 3.14159, animDuration);

		fOrientation = Quaternion(Vector3(0.0f, 1.0f, 0.0f), rotAngle);
	}

	if (fTime >= fAnimOffset + animDuration) {
		fOrientation = Quaternion(Vector3(0.0f, 1.0f, 0.0f), 0.0);
	}

	if (fTime >= animDuration * 6) {
		fTime = 0.0;
	}
}


void
MeshInstance::Render()
{
	if (fMeshReference->FaceCount() == 0)
		return;

	glPushMatrix();
	glTranslatef(fPosition.x(), fPosition.y(), fPosition.z());
	float mat[4][4];
	fOrientation.toOpenGLMatrix(mat);
	glMultMatrixf((GLfloat*) mat);
	glScalef(fScale, fScale, fScale);
	glBindTexture(GL_TEXTURE_2D, fTextureReference->Id());

	int lastVertexCount = 0;

	for (uint32 i = 0; i < fMeshReference->FaceCount(); i++) {

		const Face& face = fMeshReference->GetFace(i);

		// switch face mode
		if (face.vertexCount != lastVertexCount) {
			if (lastVertexCount != 0)
				glEnd();

			if (face.vertexCount == 3)
				glBegin(GL_TRIANGLES);
			else
				glBegin(GL_QUADS);
		}

		// calculate normal
		Vector3 lu(face.v[0].p - face.v[1].p);
		Vector3 lv(face.v[1].p - face.v[2].p);
		Vector3 normal(lu.cross(lv));
		if (normal.length() <= 0.000001)
			normal.setValue(0, 0, -1.0);
		normal.normalize();

		// draw face
		glNormal3f(normal.x(), normal.y(),  normal.z());
		glTexCoord2f(face.v[0].u, face.v[0].v);
		glVertex3f(face.v[0].p.x(), face.v[0].p.y(), face.v[0].p.z());

		glNormal3f(normal.x(), normal.y(),  normal.z());
		glTexCoord2f(face.v[1].u, face.v[1].v);
		glVertex3f(face.v[1].p.x(), face.v[1].p.y(), face.v[1].p.z());

		glNormal3f(normal.x(), normal.y(),  normal.z());
		glTexCoord2f(face.v[2].u, face.v[2].v);
		glVertex3f(face.v[2].p.x(), face.v[2].p.y(), face.v[2].p.z());

		if (face.vertexCount == 4) {
			glNormal3f(normal.x(), normal.y(), normal.z());
			glTexCoord2f(face.v[3].u, face.v[3].v);
			glVertex3f(face.v[3].p.x(), face.v[3].p.y(), face.v[3].p.z());
		}

		if (fDoubleSided) {
			if (face.vertexCount == 4) {
				glNormal3f(-normal.x(), -normal.y(), -normal.z());
				glTexCoord2f(face.v[3].u, face.v[3].v);
				glVertex3f(face.v[3].p.x(), face.v[3].p.y(), face.v[3].p.z());
			}

			glNormal3f(-normal.x(), -normal.y(), -normal.z());
			glTexCoord2f(face.v[2].u, face.v[2].v);
			glVertex3f(face.v[2].p.x(), face.v[2].p.y(), face.v[2].p.z());

			glNormal3f(-normal.x(), -normal.y(), -normal.z());
			glTexCoord2f(face.v[1].u, face.v[1].v);
			glVertex3f(face.v[1].p.x(), face.v[1].p.y(), face.v[1].p.z());

			glNormal3f(-normal.x(), -normal.y(), -normal.z());
			glTexCoord2f(face.v[0].u, face.v[0].v);
			glVertex3f(face.v[0].p.x(), face.v[0].p.y(), face.v[0].p.z());
		}
		lastVertexCount = face.vertexCount;
	}
	glEnd();

	if (fDrawNormals) {
		glBegin(GL_LINES);
		for (uint32 i = 0; i < fMeshReference->FaceCount(); i++) {

			const Face& face = fMeshReference->GetFace(i);

			if (face.vertexCount == 4) {

				// calculate normal
				Vector3 lu(face.v[0].p - face.v[1].p);
				Vector3 lv(face.v[1].p - face.v[2].p);
				Vector3 normal(lu.cross(lv));
				if (normal.length() <= 0.000001)
					normal.setValue(0, 0, -1.0);
				normal.normalize();

				// center of the face
				Vector3 g;
				if (face.vertexCount == 4)
					g = (face.v[0].p + face.v[1].p + face.v[2].p + face.v[3].p)
						/ 4.0;
				else
					g = (face.v[0].p + face.v[1].p + face.v[2].p) / 3.0;

				Vector3 h(g + normal);

				glVertex3f(g.x(), g.y(), g.z());
				glVertex3f(h.x(), h.y(), h.z());
			}
		}
		glEnd();
	}

	glPopMatrix();
}
