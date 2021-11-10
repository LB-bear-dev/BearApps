#pragma once
#include <memory>
#include <string>
#include <thread>
#include <assert.h>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <ppltasks.h>
#include <ppl.h>
#include <concurrent_unordered_map.h>

#undef GetMessage()

using json = nlohmann::json;