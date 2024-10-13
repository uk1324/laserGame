#include <game/EditorEntities.hpp>

EditorWall EditorWall::DefaultInitialize::operator()() {
	return EditorWall{
		.endpoints = { Vec2(0.0f), Vec2(0.0f) }
	};
}

EditorLaser EditorLaser::DefaultInitialize::operator()() {
	return EditorLaser{
		.position = Vec2(0.0f),
		.angle = 0.0f
	};
}

EditorMirror EditorMirror::DefaultInitialize::operator()() {
	return EditorMirror{
		.center = Vec2(0.0f),
		.normalAngle = 0.0f
	};
}

EditorEntityId::EditorEntityId(const EditorWallId& id) 
	: type(EditorEntityType::WALL) 
	, index(id.index())
	, version(id.version()) {}

EditorEntityId::EditorEntityId(const EditorLaserId& id)
	: type(EditorEntityType::LASER)
	, index(id.index())
	, version(id.version()) {}

EditorEntityId::EditorEntityId(const EditorMirrorId& id)
	: type(EditorEntityType::MIRROR)
	, index(id.index())
	, version(id.version()) {}

EditorWallId EditorEntityId::wall() const {
	ASSERT(type == EditorEntityType::WALL);
	return EditorWallId(index, version);
}

EditorLaserId EditorEntityId::laser() const {
	ASSERT(type == EditorEntityType::LASER);
	return EditorLaserId(index, version);
}

EditorMirrorId EditorEntityId::mirror() const {
	ASSERT(type == EditorEntityType::MIRROR);
	return EditorMirrorId(index, version);
}
