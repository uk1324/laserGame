#pragma once

#include <Json/JsonValue.hpp>
#include <optional>
#include <RefOptional.hpp>

Json::Value::ArrayType& makeArrayAt(Json::Value& v, std::string_view at);
std::optional<const Json::Value::ArrayType&> tryArrayAt(const Json::Value& v, std::string_view name);