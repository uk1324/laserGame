#include <game/Editor.hpp>
#include <imgui/imgui.h>
#include <game/Constants.hpp>
#include <engine/Input/Input.hpp>
#include <engine/Math/Constants.hpp>
#include <engine/Math/LineSegment.hpp>

EditorGridTool::EllipticIsometry::EllipticIsometry(f32 a0, f32 a1, f32 a2)
	: a{ a0, a1, a2 } {}

Quat EditorGridTool::EllipticIsometry::toQuaternion() const {
	return Quat(a[0], Vec3(0.0f, 0.0f, 1.0f)) *
		Quat(a[1], Vec3(0.0f, 1.0f, 0.0f)) *
		Quat(a[2], Vec3(1.0f, 0.0f, 0.0f));
}

EditorGridTool::EllipticIsometryInput EditorGridTool::EllipticIsometryInput::defaultInit() {
	return EllipticIsometryInput{
		.a = { Ratio(0, 1), Ratio(0, 1), Ratio(0, 1) }
	};
}

EditorGridTool::EllipticIsometry EditorGridTool::EllipticIsometryInput::toEllipticIsometry() const {
	return EllipticIsometry(a[0].toF32() * TAU<f32>, a[1].toF32() * TAU<f32>, a[2].toF32() * TAU<f32>);
}

EditorGridTool::Ratio::Ratio(i32 numerator, i32 denominator) 
	: numerator(numerator)
	, denominator(denominator) {}

f32 EditorGridTool::Ratio::toF32() const {
	return f32(numerator) / f32(denominator);
}

static void transformShape(const std::vector<Vec3>& inputVertices,
	const std::vector<ProjectivePolyhedron::Segment>& inputSegments,
	std::vector<Vec3>& outputVertices,
	std::vector<ProjectivePolyhedron::Segment>& outputSegments,
	Quat transformation) {

	for (const auto& vertex : inputVertices) {
		outputVertices.push_back(vertex * transformation);
	}
	for (const auto& segment : inputSegments) {
		const auto e0Above = outputVertices[segment.endpoints[0]].z > 0.0f;
		const auto e1Above = outputVertices[segment.endpoints[1]].z > 0.0f;
		ProjectivePolyhedron::Segment newSegment = segment;
		if (e0Above != e1Above) {
			newSegment.connectedThroughHemisphere = !newSegment.connectedThroughHemisphere;
		}
		outputSegments.push_back(newSegment);
	}
	for (auto& vertex : outputVertices) {
		const auto above = vertex.z > 0.0f;
		if (above) {
			vertex = -vertex;
		}
	}
}

EditorGridTool::EditorGridTool() 
	: hemiIcosahedronIsometry{ .a = { Ratio(0, 3), Ratio(0, 3), Ratio(0, 3) }}
	, hemiDodecahedronIsometry{ .a = { Ratio(0, 5), Ratio(0, 5), Ratio(0, 5) } }{
	const auto phi = (1.0f + sqrt(5.0f)) / 2.0f;
}

EditorGridTool::SnapCursorResult EditorGridTool::snapCursor(Vec2 cursorPos) {
	auto snapCursorToShape = [&cursorPos](const std::vector<Vec3>& originalVertices, const std::vector<ProjectivePolyhedron::Segment>& originalSegments, Quat isometry) {
		std::vector<Vec3> vertices;
		std::vector<ProjectivePolyhedron::Segment> segments;
		transformShape(originalVertices, originalSegments, vertices, segments, isometry);
		return snapCursorToShapeGrid(cursorPos, vertices, segments);
	};

	switch (gridType) {
		using enum GridType;
	case POLAR: return snapCursorToPolarGrid(cursorPos);
	case HEMI_ICOSAHEDRAL:
		return snapCursorToShape(ProjectivePolyhedron::hemiIcosahedron.vertices, ProjectivePolyhedron::hemiIcosahedron.segments, currentIcosahedronIsometry());

	case HEMI_DODECAHEDRAL:
		return snapCursorToShape(ProjectivePolyhedron::hemiDodecahedron.vertices, ProjectivePolyhedron::hemiDodecahedron.segments, currentDodecahedronIsometry());
	}

	return SnapCursorResult{ .cursorPos = cursorPos, .snapped = false };
}

void EditorGridTool::render(GameRenderer& renderer) {
	if (!showGrid) {
		return;
	}

	const auto gridColor = Color3::WHITE / 7.0f;

	auto drawShapeGrid = [&renderer, &gridColor](const std::vector<Vec3>& originalVertices,
		const std::vector<ProjectivePolyhedron::Segment>& originalSegments,
		Quat isometry) {

		std::vector<Vec3> vertices;
		std::vector<ProjectivePolyhedron::Segment> segments;
		transformShape(originalVertices, originalSegments, vertices, segments, isometry);
		for (const auto& segment : segments) {
			renderEllipticSegment(renderer, segment, vertices, gridColor);
		}
	};

	switch (gridType) {
		using enum GridType;

	case POLAR: {
		if (showGrid) {
			for (i32 i = 1; i < polarGridCircleCount; i++) {
				const auto r = f32(i) / f32(polarGridCircleCount);
				renderer.gfx.circle(Vec2(0.0f), r, 0.01f, gridColor);
			}

			for (i32 i = 0; i < polarGridLineCount; i++) {
				const auto t = f32(i) / f32(polarGridLineCount);
				const auto a = t * TAU<f32>;
				const auto v = Vec2::oriented(a);
				renderer.gfx.line(Vec2(0.0f), v, 0.01f, gridColor);
			}
		}
		renderer.gfx.drawLines();
		renderer.gfx.drawCircles();
		break;
	}

	case HEMI_ICOSAHEDRAL: {
		drawShapeGrid(ProjectivePolyhedron::hemiIcosahedron.vertices, ProjectivePolyhedron::hemiIcosahedron.segments, currentIcosahedronIsometry());
		break;
	}

	case HEMI_DODECAHEDRAL: {
		drawShapeGrid(ProjectivePolyhedron::hemiDodecahedron.vertices, ProjectivePolyhedron::hemiDodecahedron.segments, currentDodecahedronIsometry());
		break;
	}

	}
}

static void ratioGui(const char* label, i32& numerator, i32& denominator) {
	ImGui::PushID(label);
	ImGui::Text(label);
	ImGui::InputInt("##numerator", &numerator);
	ImGui::InputInt("##denominator", &denominator);
	ImGui::PopID();
}

static void ratioGui(const char* label, EditorGridTool::Ratio& ratio) {
	ratioGui(label, ratio.numerator, ratio.denominator);
};

void EditorGridTool::gui() {
	// made up name
	auto modularRatioGui = [](const char* label, Ratio& ratio) {
		ratioGui(label, ratio);
		ratio.denominator = std::max(ratio.denominator, 1);
		ratio.numerator %= ratio.denominator;
	};

	auto ellipticIsometryGui = [&modularRatioGui](const char* label, EllipticIsometryInput& isometry) {
		if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen)) {
			modularRatioGui("a0", isometry.a[0]);
			modularRatioGui("a1", isometry.a[1]);
			modularRatioGui("a2", isometry.a[2]);
			ImGui::TreePop();
		}
	};
	ImGui::SeparatorText("grid");
	ImGui::PushID("grid");
	{
		ImGui::Checkbox("show", &showGrid);
		ImGui::Combo("type", reinterpret_cast<int*>(&gridType), "polar\0hemi-icosahedral\0hemi-dodecahedral\0");
		ImGui::TextDisabled("(?)");
		ImGui::SetItemTooltip("Hold ctrl to snap cursor to grid.");

		switch (gridType) {
			using enum GridType;

		case POLAR:
			ImGui::InputInt("lines", &polarGridLineCount);
			polarGridLineCount = std::max(polarGridLineCount, 1);

			ImGui::InputInt("circles", &polarGridCircleCount);
			polarGridCircleCount = std::max(polarGridCircleCount, 1);
			break;

		case HEMI_ICOSAHEDRAL:
			ImGui::Combo("origin", reinterpret_cast<int*>(&icosahedronCoordinateSystem), "vertex\0triangle\0edge\0");
			ellipticIsometryGui("transformation angles", hemiIcosahedronIsometry);
			break;

		case HEMI_DODECAHEDRAL:
			ImGui::Combo("origin", reinterpret_cast<int*>(&dodecahedronCoordinateSystem), "vertex\0pentagon\0edge\0");
			ellipticIsometryGui("transformation angles", hemiDodecahedronIsometry);
			break;
		}
		
	}
	ImGui::PopID();
}

static const auto snapDistance = Constants::endpointGrabPointRadius;

EditorGridTool::SnapCursorResult EditorGridTool::snapCursorToPolarGrid(Vec2 cursorPos) {
	for (i32 iLine = 0; iLine < polarGridLineCount; iLine++) {
		for (i32 iCircle = 0; iCircle < polarGridCircleCount + 1; iCircle++) {
			auto r = f32(iCircle) / f32(polarGridCircleCount);
			const auto t = f32(iLine) / f32(polarGridLineCount);
			const auto a = t * TAU<f32>;
			const auto v = Vec2::fromPolar(a, r);

			if (iCircle == polarGridCircleCount) {
				r -= 0.01f;
			}

			if (distance(v, cursorPos) < snapDistance) {
				return SnapCursorResult{ .cursorPos = v, .snapped = true };
			}
		}
	}

	struct ClosestFeature {
		Vec2 pointOnFeature;
		f32 distance;
	};
	std::optional<ClosestFeature> closestFeature;
	// This could be done without the loop.
	for (i32 iLine = 0; iLine < polarGridLineCount; iLine++) {
		const auto t = f32(iLine) / f32(polarGridLineCount);
		const auto a = t * TAU<f32>;
		const auto p = Vec2::oriented(a) * 1.1f;

		const auto closestPointOnLine = LineSegment(Vec2(0.0f), p).closestPointTo(cursorPos);
		const auto dist = distance(closestPointOnLine, cursorPos);
		if (!closestFeature.has_value() || dist < closestFeature->distance) {
			closestFeature = ClosestFeature{ .pointOnFeature = closestPointOnLine, .distance = dist };
		}
	}
	// This also could be done without the loop.
	for (i32 iCircle = 1; iCircle < polarGridCircleCount + 1; iCircle++) {
		const auto r = f32(iCircle) / f32(polarGridCircleCount);
		const auto pointOnCircle = cursorPos.normalized() * r;
		const auto dist = distance(pointOnCircle, cursorPos);
		if (!closestFeature.has_value() || dist < closestFeature->distance) {
			closestFeature = ClosestFeature{ .pointOnFeature = pointOnCircle, .distance = dist };
		}
	}

	if (closestFeature.has_value() && closestFeature->distance < snapDistance) {
		return SnapCursorResult{ .cursorPos = closestFeature->pointOnFeature, .snapped = true };
	}

	return SnapCursorResult{ .cursorPos = cursorPos, .snapped = false };
}

EditorGridTool::SnapCursorResult EditorGridTool::snapCursorToShapeGrid(Vec2 cursorPos, const std::vector<Vec3>& vertices, const std::vector<ProjectivePolyhedron::Segment>& segments) {

	for (const auto& vertex : vertices) {
		const auto v = toStereographic(vertex);
		if (cursorPos.distanceTo(v) < snapDistance) {
			return SnapCursorResult{ .cursorPos = v, .snapped = true };
		}
	}
	for (const auto& segment : segments) {
		const auto se0 = toStereographic(vertices[segment.endpoints[0]]);
		const auto se1 = toStereographic(vertices[segment.endpoints[1]]);
		if (!segment.connectedThroughHemisphere) {
			continue;
		}
		const auto stereographicLine = ::stereographicLine(se0, se1);
		const auto boundaryIntersections = stereographicLineVsCircleIntersection(stereographicLine, Constants::boundary);
		for (const auto& intersection : boundaryIntersections) {
			if (cursorPos.distanceTo(intersection) < snapDistance) {
				return SnapCursorResult{ .cursorPos = intersection, .snapped = true };
			}
		}
	}

	return SnapCursorResult{ .cursorPos = cursorPos, .snapped = false };
}

void EditorGridTool::renderEllipticSegment(GameRenderer& renderer, const ProjectivePolyhedron::Segment& segment, const std::vector<Vec3>& vertices, Vec3 color) {
	const Circle boundary(Vec2(0.0f), 1.0f);
	const auto se0 = toStereographic(vertices[segment.endpoints[0]]);
	const auto se1 = toStereographic(vertices[segment.endpoints[1]]);

	if (segment.connectedThroughHemisphere) {
		const auto stereographicLine = ::stereographicLine(se0, se1);
		const auto boundaryIntersections = stereographicLineVsCircleIntersection(stereographicLine, boundary);
		if (boundaryIntersections.size() == 2) {
			auto i0 = boundaryIntersections[0];
			auto i1 = boundaryIntersections[1];
			const auto e0ToE1Dir = se1 - se0;
			const auto i1InTheSameDirectionAsE1 = dot(i1, e0ToE1Dir) > 0.0f;
			if (!i1InTheSameDirectionAsE1) {
				std::swap(i0, i1);
			}
			renderer.stereographicSegment(i0, se0, color);
			renderer.stereographicSegment(i1, se1, color);
		}
	} else {
		renderer.stereographicSegment(se0, se1, color);
	}
}

Quat EditorGridTool::currentIcosahedronIsometry() const {
	Quat q = hemiIcosahedronIsometry.toEllipticIsometry().toQuaternion();

	switch (icosahedronCoordinateSystem) {
		using enum IcosahedronCoordinateSystem;
	case VERTEX_CENTER:
		 q = q * ProjectivePolyhedron::hemiIcosahedronVertexToCenter;
		break;
	case TRIANGLE_CENTER:
		q = q * ProjectivePolyhedron::hemiIcosahedronFaceToCenter;
		break;
	case EDGE_CENTER:
		break;
	}
	return q;
}

Quat EditorGridTool::currentDodecahedronIsometry() const {
	Quat q = hemiDodecahedronIsometry.toEllipticIsometry().toQuaternion();
	switch (dodecahedronCoordinateSystem) {
		using enum DodecahedronCoordinateSystem;
	case VERTEX_CENTER:
		q = q * ProjectivePolyhedron::hemiDodecahedronVertexToCenter;
		break;
	case PENTAGON_CENTER:
		q = q * ProjectivePolyhedron::hemiDodecahedronFaceToCenter;
		break;
	case EDGE_CENTER:
		break;
	}
	return q;
}
