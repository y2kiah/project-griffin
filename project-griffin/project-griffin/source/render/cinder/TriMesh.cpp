/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include <algorithm>
#include <functional>
#include "TriMesh.h"
#include <glm/geometric.hpp>

using std::vector;
using namespace glm;

namespace griffin {

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// TriMesh
	void TriMesh::clear()
	{
		mVertices.clear();
		mNormals.clear();
		mColorsRGB.clear();
		mColorsRGBA.clear();
		mTexCoords.clear();
		mIndices.clear();
	}

	void TriMesh::appendVertices(const vec3 *verts, size_t num)
	{
		mVertices.insert(mVertices.end(), verts, verts + num);
	}

	void TriMesh::appendVertices(const vec4 *verts, size_t num)
	{
		for (size_t v = 0; v < num; ++v)
			mVertices.push_back(vec3((float)verts[v].x, (float)verts[v].y, (float)verts[v].z));
	}

	void TriMesh::appendIndices(const uint32_t *indices, size_t num)
	{
		mIndices.insert(mIndices.end(), indices, indices + num);
	}

	void TriMesh::appendNormals(const vec3 *normals, size_t num)
	{
		mNormals.insert(mNormals.end(), normals, normals + num);
	}

	void TriMesh::appendNormals(const vec4 *normals, size_t num)
	{
		for (size_t v = 0; v < num; ++v)
			mNormals.push_back(vec3((float)normals[v].x, (float)normals[v].y, (float)normals[v].z));
	}

	void TriMesh::appendColorsRgb(const Color *rgbs, size_t num)
	{
		mColorsRGB.insert(mColorsRGB.end(), rgbs, rgbs + num);
	}

	void TriMesh::appendColorsRgba(const ColorA *rgbas, size_t num)
	{
		mColorsRGBA.insert(mColorsRGBA.end(), rgbas, rgbas + num);
	}

	void TriMesh::appendTexCoords(const vec2 *texcoords, size_t num)
	{
		mTexCoords.insert(mTexCoords.end(), texcoords, texcoords + num);
	}

	void TriMesh::getTriangleVertices(size_t idx, vec3 *a, vec3 *b, vec3 *c) const
	{
		*a = mVertices[mIndices[idx * 3]];
		*b = mVertices[mIndices[idx * 3 + 1]];
		*c = mVertices[mIndices[idx * 3 + 2]];
	}

	AxisAlignedBox3f TriMesh::calcBoundingBox() const
	{
		if (mVertices.empty())
			return AxisAlignedBox3f(vec3(0), vec3(0));

		vec3 min(mVertices[0]), max(mVertices[0]);
		for (size_t i = 1; i < mVertices.size(); ++i) {
			if (mVertices[i].x < min.x)
				min.x = mVertices[i].x;
			else if (mVertices[i].x > max.x)
				max.x = mVertices[i].x;
			if (mVertices[i].y < min.y)
				min.y = mVertices[i].y;
			else if (mVertices[i].y > max.y)
				max.y = mVertices[i].y;
			if (mVertices[i].z < min.z)
				min.z = mVertices[i].z;
			else if (mVertices[i].z > max.z)
				max.z = mVertices[i].z;
		}

		return AxisAlignedBox3f(min, max);
	}

	AxisAlignedBox3f TriMesh::calcBoundingBox(const mat4x4 &transform) const
	{
		if (mVertices.empty())
			return AxisAlignedBox3f(vec3(0), vec3(0));

		vec3 min(transform.transformPointAffine(mVertices[0]));
		vec3 max(min);
		for (size_t i = 0; i < mVertices.size(); ++i) {
			vec3 v = transform.transformPointAffine(mVertices[i]);

			if (v.x < min.x)
				min.x = v.x;
			else if (v.x > max.x)
				max.x = v.x;
			if (v.y < min.y)
				min.y = v.y;
			else if (v.y > max.y)
				max.y = v.y;
			if (v.z < min.z)
				min.z = v.z;
			else if (v.z > max.z)
				max.z = v.z;
		}

		return AxisAlignedBox3f(min, max);
	}

/*
	void TriMesh::read(DataSourceRef dataSource)
	{
		IStreamRef in = dataSource->createStream();
		clear();

		uint8_t versionNumber;
		in->read(&versionNumber);

		uint32_t numVertices, numNormals, numTexCoords, numIndices;
		in->readLittle(&numVertices);
		in->readLittle(&numNormals);
		in->readLittle(&numTexCoords);
		in->readLittle(&numIndices);

		for (size_t idx = 0; idx < numVertices; ++idx) {
			vec3 v;
			in->readLittle(&v.x); in->readLittle(&v.y); in->readLittle(&v.z);
			mVertices.push_back(v);
		}

		for (size_t idx = 0; idx < numNormals; ++idx) {
			vec3 v;
			in->readLittle(&v.x); in->readLittle(&v.y); in->readLittle(&v.z);
			mNormals.push_back(v);
		}

		for (size_t idx = 0; idx < numTexCoords; ++idx) {
			vec2 v;
			in->readLittle(&v.x); in->readLittle(&v.y);
			mTexCoords.push_back(v);
		}

		for (size_t idx = 0; idx < numIndices; ++idx) {
			uint32_t v;
			in->readLittle(&v);
			mIndices.push_back(v);
		}
	}

	void TriMesh::write(DataTargetRef dataTarget) const
	{
		OStreamRef out = dataTarget->getStream();

		const uint8_t versionNumber = 1;
		out->write(versionNumber);

		out->writeLittle(static_cast<uint32_t>(mVertices.size()));
		out->writeLittle(static_cast<uint32_t>(mNormals.size()));
		out->writeLittle(static_cast<uint32_t>(mTexCoords.size()));
		out->writeLittle(static_cast<uint32_t>(mIndices.size()));

		for (vector<vec3>::const_iterator it = mVertices.begin(); it != mVertices.end(); ++it) {
			out->writeLittle(it->x); out->writeLittle(it->y); out->writeLittle(it->z);
		}

		for (vector<vec3>::const_iterator it = mNormals.begin(); it != mNormals.end(); ++it) {
			out->writeLittle(it->x); out->writeLittle(it->y); out->writeLittle(it->z);
		}

		for (vector<vec2>::const_iterator it = mTexCoords.begin(); it != mTexCoords.end(); ++it) {
			out->writeLittle(it->x); out->writeLittle(it->y);
		}

		for (vector<uint32_t>::const_iterator it = mIndices.begin(); it != mIndices.end(); ++it) {
			out->writeLittle(*it);
		}
	}
*/
	void TriMesh::recalculateNormals()
	{
		mNormals.assign(mVertices.size(), vec3(0));

		size_t n = getNumTriangles();
		for (size_t i = 0; i < n; ++i) {
			uint32_t index0 = mIndices[i * 3];
			uint32_t index1 = mIndices[i * 3 + 1];
			uint32_t index2 = mIndices[i * 3 + 2];

			vec3 v0 = mVertices[index0];
			vec3 v1 = mVertices[index1];
			vec3 v2 = mVertices[index2];

			vec3 e0 = v1 - v0;
			vec3 e1 = v2 - v0;
			vec3 normal = normalize(cross(e0, e1));

			mNormals[index0] += normal;
			mNormals[index1] += normal;
			mNormals[index2] += normal;
		}

		std::for_each(mNormals.begin(), mNormals.end(), [](vec3& n){ n = glm::normalize(n); });
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// TriMesh2d
	void TriMesh2d::clear()
	{
		mVertices.clear();
		mColorsRgb.clear();
		mColorsRgba.clear();
		mTexCoords.clear();
		mIndices.clear();
	}

	void TriMesh2d::appendVertices(const vec2 *verts, size_t num)
	{
		mVertices.insert(mVertices.end(), verts, verts + num);
	}

	void TriMesh2d::appendIndices(const uint32_t *indices, size_t num)
	{
		mIndices.insert(mIndices.end(), indices, indices + num);
	}

	void TriMesh2d::appendColorsRgb(const Color *rgbs, size_t num)
	{
		mColorsRgb.insert(mColorsRgb.end(), rgbs, rgbs + num);
	}

	void TriMesh2d::appendColorsRgba(const ColorA *rgbas, size_t num)
	{
		mColorsRgba.insert(mColorsRgba.end(), rgbas, rgbas + num);
	}

	void TriMesh2d::appendTexCoords(const vec2 *texcoords, size_t num)
	{
		mTexCoords.insert(mTexCoords.end(), texcoords, texcoords + num);
	}

	void TriMesh2d::getTriangleVertices(size_t idx, vec2 *a, vec2 *b, vec2 *c) const
	{
		*a = mVertices[mIndices[idx * 3]];
		*b = mVertices[mIndices[idx * 3 + 1]];
		*c = mVertices[mIndices[idx * 3 + 2]];
	}

	Rectf TriMesh2d::calcBoundingBox() const
	{
		if (mVertices.empty())
			return Rectf(vec2(0), vec2(0));

		vec2 min(mVertices[0]), max(mVertices[0]);
		for (size_t i = 1; i < mVertices.size(); ++i) {
			if (mVertices[i].x < min.x)
				min.x = mVertices[i].x;
			else if (mVertices[i].x > max.x)
				max.x = mVertices[i].x;
			if (mVertices[i].y < min.y)
				min.y = mVertices[i].y;
			else if (mVertices[i].y > max.y)
				max.y = mVertices[i].y;
		}

		return Rectf(min, max);
	}

}
