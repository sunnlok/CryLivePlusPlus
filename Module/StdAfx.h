#pragma once

#define eCryModule eCryM_EnginePlugin

#include <CryCore/Platform/platform.h>

#if _MSVC_LANG >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#else
//Use experimental for now because cryengine wont compile with cpp17
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

using TPathList = std::vector<fs::path>;