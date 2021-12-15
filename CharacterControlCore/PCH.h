#pragma once
#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <utility>
#include <memory>
#include <fstream>
#include <filesystem>
#include <concurrent_vector.h>

using json = nlohmann::json;