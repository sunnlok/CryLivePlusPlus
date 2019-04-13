#pragma once
#include "CryString/CryString.h"
#include <chrono>


namespace Cry
{
	namespace Lpp
	{
		struct SPDBData;

		using TTimePoint = std::chrono::system_clock::time_point;
		struct SCompiland
		{
			SCompiland(fs::path &path, TTimePoint &lastWrite)
				: path(path)
				, lastWrite(lastWrite) {}

			SCompiland(fs::path &path)
				: path(path)
				, lastWrite(fs::last_write_time(path)){}

			fs::path path;
			TTimePoint lastWrite;

			bool operator==(const SCompiland &other) { return fs::equivalent(other.path,path); }
			bool operator==(const fs::path &other) { return fs::equivalent(other, path);}
		};
		using TCompilands = std::vector<SCompiland>;

		struct SModuleData
		{
			SModuleData(const string &moduleName, fs::path modulePath, std::unique_ptr<SPDBData> pPDBData)
				: moduleName(moduleName)
				, modulePath(modulePath)
				, pPDBData(std::move(pPDBData))
				, lastCompile(fs::last_write_time(modulePath)){}

			const string   moduleName;
			const fs::path modulePath;
			fs::path projectFilePath;
			fs::path objectsPath;

			fs::path sourcePath;		

			string buildConfiguration = "Profile"; //Assuming profile by default

			TCompilands compilands;
			TTimePoint	lastCompile;
			const std::unique_ptr<SPDBData> pPDBData;
		};
	}
}