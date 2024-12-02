#pragma once

#include <engine/Math/Vec3.hpp>
#include <vector>
#include <engine/Math/Quat.hpp>

struct ProjectivePolyhedron {
	struct Segment {
		// Vertex indices
		i32 endpoints[2];
		bool connectedThroughHemisphere;
	};

	// // To make a hemi-<shape> take a regular <shape> and take choose only the vertices that lie on the bottom hemisphere.
	static ProjectivePolyhedron makeHemiIcosahedron();
	static ProjectivePolyhedron makeHemiDodecahedron();

	std::vector<Vec3> vertices;
	std::vector<Segment> segments;

	void addV(f32 x, f32 y, f32 z);
	void addS(i32 i0, i32 i1, bool connectedThroughHemisphere);

	static const ProjectivePolyhedron hemiIcosahedron;
	static const Quat hemiIcosahedronVertexToCenter;
	static const Quat hemiIcosahedronFaceToCenter;

	static const ProjectivePolyhedron hemiDodecahedron;
	static const Quat hemiDodecahedronVertexToCenter;
	static const Quat hemiDodecahedronFaceToCenter;
};

Quat makeHemiIcosahedronVertexToCenter(const ProjectivePolyhedron& icosahedron);
Quat makeHemiIcosahedronFaceToCenter(const ProjectivePolyhedron& icosahedron);

Quat makeHemiDodecahedronVertexToCenter(const ProjectivePolyhedron& dodecahedron);
Quat makeHemiDodecahedronFaceToCenter(const ProjectivePolyhedron& dodecahedron);