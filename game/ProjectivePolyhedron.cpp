#include "ProjectivePolyhedron.hpp"
#include "Stereographic.hpp"

static const auto phi = (1.0f + sqrt(5.0f)) / 2.0f;

ProjectivePolyhedron ProjectivePolyhedron::makeHemiIcosahedron() {
	// To make a hemi-<shape> take a regular <shape> and take choose only the vertices that lie on the bottom hemisphere.
	ProjectivePolyhedron p;

	// https://en.wikipedia.org/wiki/Hemi-icosahedron
	// https://en.wikipedia.org/wiki/Regular_icosahedron#Construction
	// Chose all the vertices so that they lie on the bottom hemisphere (z <= 0.0f).
	p.addV(0.0f, -1.0f, -phi); // 0
	p.addV(0.0f, 1.0f, -phi); // 1
	p.addV(1.0f, phi, 0.0f); // 2
	p.addV(-1.0f, phi, 0.0f); // 3
	p.addV(-phi, 0.0f, -1.0f); // 4
	p.addV(phi, 0.0f, -1.0f); // 5

	const auto currentCenter = Vec3(0.0f, 0.0f, -1.0f);

	for (auto& vertex : p.vertices) {
		vertex = vertex.normalized();
	}

	p.addS(0, 1, false);

	p.addS(1, 5, false);
	p.addS(1, 4, false);

	p.addS(1, 2, false);
	p.addS(1, 3, false);
	p.addS(0, 2, true);
	p.addS(0, 3, true);

	p.addS(2, 3, false);
	p.addS(5, 4, true);

	p.addS(5, 2, false);
	p.addS(4, 3, false);
	p.addS(4, 2, true);
	p.addS(5, 3, true);

	p.addS(3, 2, false);

	p.addS(0, 5, false);
	p.addS(0, 4, false);

	return p;
}

ProjectivePolyhedron ProjectivePolyhedron::makeHemiDodecahedron() {
	ProjectivePolyhedron p;
	const auto iphi = 1.0f / phi;
	// https://en.wikipedia.org/wiki/Hemi-dodecahedron
	// https://en.wikipedia.org/wiki/Regular_dodecahedron#Relation_to_the_golden_ratio
	p.addV(1.0f, 1.0f, -1.0f); // 0
	p.addV(-1.0f, 1.0f, -1.0f); // 1
	p.addV(1.0f, -1.0f, -1.0f); // 2
	p.addV(-1.0f, -1.0f, -1.0f); // 3

	p.addV(0.0f, phi, -iphi); // 4
	p.addV(0.0f, -phi, -iphi); // 5

	p.addV(iphi, 0.0f, -phi); // 6
	p.addV(-iphi, 0.0f, -phi); // 7

	p.addV(phi, iphi, 0.0f); // 8
	p.addV(-phi, iphi, 0.0f); // 9

	for (auto& vertex : p.vertices) {
		vertex = vertex.normalized();
	}

	p.addS(1, 7, false);
	p.addS(0, 6, false);

	p.addS(1, 4, false);
	p.addS(0, 4, false);

	p.addS(3, 7, false);
	p.addS(2, 6, false);

	p.addS(3, 5, false);
	p.addS(2, 5, false);

	p.addS(7, 6, false);

	p.addS(5, 4, true);

	p.addS(1, 9, false);
	p.addS(3, 8, true);
	p.addS(9, 8, true);

	p.addS(0, 8, false);
	p.addS(2, 9, true);

	return p;
}

void ProjectivePolyhedron::addV(f32 x, f32 y, f32 z) {
	vertices.push_back(Vec3(x, y, z));
}

void ProjectivePolyhedron::addS(i32 i0, i32 i1, bool connectedThroughHemisphere) {
	segments.push_back(Segment{ i0, i1, connectedThroughHemisphere });
}

static const auto currentCenter = Vec3(0.0f, 0.0f, -1.0f);

Quat makeHemiIcosahedronVertexToCenter(const ProjectivePolyhedron& icosahedron) {
	const auto vertex = icosahedron.vertices[5];
	const auto bringPentagonToCenter = Quat(
		sphericalDistance(currentCenter, vertex),
		Vec3(0.0f, 1.0f, 0.0f)
	);
	return Quat(PI<f32> / 2.0f, Vec3(0.0f, 0.0f, 1.0f)) * bringPentagonToCenter;
}

Quat makeHemiIcosahedronFaceToCenter(const ProjectivePolyhedron& icosahedron) {
	const auto centerOfTriangle140 = icosahedron.vertices[1] + icosahedron.vertices[4] + icosahedron.vertices[0];
	return
		Quat(PI<f32> / 2.0f, Vec3(0.0f, 0.0f, 1.0f)) *
		Quat(sphericalDistance(centerOfTriangle140, currentCenter), Vec3(0.0f, 1.0f, 0.0f));
}

Quat makeHemiDodecahedronVertexToCenter(const ProjectivePolyhedron& dodecahedron) {
	const auto vertex = dodecahedron.vertices[6];
	const auto bringVertexToCenter = Quat(sphericalDistance(currentCenter, vertex), Vec3(0.0f, 1.0f, 0.0f));
	return Quat(PI<f32> / 2.0f, Vec3(0.0f, 0.0f, 1.0f)) * bringVertexToCenter;
}

Quat makeHemiDodecahedronFaceToCenter(const ProjectivePolyhedron& dodecahedron) {
	const auto pentagonCenter = dodecahedron.vertices[4] + dodecahedron.vertices[0] + dodecahedron.vertices[6] + dodecahedron.vertices[7] + dodecahedron.vertices[1];
	return Quat(sphericalDistance(currentCenter, pentagonCenter), Vec3(1.0f, 0.0f, 0.0f));
}


const ProjectivePolyhedron ProjectivePolyhedron::hemiIcosahedron = ProjectivePolyhedron::makeHemiIcosahedron();
const Quat ProjectivePolyhedron::hemiIcosahedronVertexToCenter = makeHemiIcosahedronVertexToCenter(ProjectivePolyhedron::hemiIcosahedron);
const Quat ProjectivePolyhedron::hemiIcosahedronFaceToCenter = makeHemiIcosahedronFaceToCenter(ProjectivePolyhedron::hemiIcosahedron);

const ProjectivePolyhedron ProjectivePolyhedron::hemiDodecahedron = ProjectivePolyhedron::makeHemiDodecahedron();
const Quat ProjectivePolyhedron::hemiDodecahedronVertexToCenter = makeHemiDodecahedronVertexToCenter(ProjectivePolyhedron::hemiDodecahedron);
const Quat ProjectivePolyhedron::hemiDodecahedronFaceToCenter = makeHemiDodecahedronFaceToCenter(ProjectivePolyhedron::hemiDodecahedron);