#include <game/Serialization.hpp>

Json::Value::ArrayType& makeArrayAt(Json::Value& v, std::string_view at) {
	return (v[std::string(at)] = Json::Value::emptyArray()).array();
}

std::optional<const Json::Value::ArrayType&> tryArrayAt(const Json::Value& v, std::string_view name) {
	const auto nameStr = std::string(name);
	if (!v.contains(nameStr)) {
		return std::nullopt;
	}
	const auto& a = v.at(nameStr);
	if (!a.isArray()) {
		return std::nullopt;
	}
	return a.array();
}