#include "StdAfx.h"
#include "BuildSystem.h"
#include "ModuleData.h"
#include "CryString/StringUtils.h"
#include "../Live++.h"
#include "PDBAccess.h"
#include <fstream> 
#include <string>
#include "LPP_API.h"

using namespace Cry::Lpp;


CBuildSystem::CBuildSystem(CLivePlusPlus *pLivePlusPlus)
	: m_pLivePlusPlus(pLivePlusPlus)
{
	pLivePlusPlus->RegisterPatchListener(this);
}

bool CBuildSystem::Init()
{
	m_pPDBAccess = std::make_unique<CPDBAccess>();
	return m_pPDBAccess->Init();
}

CBuildSystem::~CBuildSystem()
{
	m_pLivePlusPlus->RemovePatchListener(this);
}


bool CBuildSystem::AddModuleWatch(const string &moduleName, fs::path &modulePath)
{
	fs::path path = modulePath;
	path.replace_extension(".pdb");
	if (!fs::exists(path))
	{
		CryLogAlways("[Live++] Could not find PDB file %s", path.u8string().c_str());
		return false;
	}
	auto pdbData = m_pPDBAccess->LoadPDBData(path);
	if (pdbData.get() == nullptr)
		return false;

	auto pModuleData = std::make_unique<SModuleData>(moduleName, modulePath, std::move(pdbData));
	if (!m_pPDBAccess->GetAndFillCompilandPaths(pModuleData.get()))
		return false;

	//Get the compile configuration from the obj path
	pModuleData->buildConfiguration = pModuleData->objectsPath.filename().u8string().c_str();

	//Extract the 
	if (!FindAndFillSourcePath(pModuleData.get()))
		return false;

	if (m_cmakePath.empty() || m_msbuildPath.empty())
		CryLogAlways("[Live++][Warning] CMake or MSBuild path not found yet! Compilation disabled.");

	m_moduleEntries.emplace_back(std::move(pModuleData));
	return true;
}

void CBuildSystem::RemoveModuleWatch(const string &moduleName)
{

}

CBuildSystem::SModuleEntry::SModuleEntry(std::unique_ptr<SModuleData> &&pData)
	: moduleHash(CryStringUtils::HashString(pData->moduleName.c_str()))
	, pModuleData(std::move(pData))
{}

void Cry::Lpp::CBuildSystem::ApplyModuleChanges(const string &moduleName)
{
	if (moduleName.empty())
		return;

	auto nameHash = CryStringUtils::HashString(moduleName.c_str());
	SModuleEntry* pModuleEntry = nullptr;
	for (auto &entry : m_moduleEntries)
	{
		if (entry.moduleHash == nameHash)
		{
			pModuleEntry = &entry;
		}
	}

	if (!pModuleEntry)
		return;

	auto pModuleData = pModuleEntry->pModuleData.get();

	TPathList objectPaths;
	for (auto& p : fs::directory_iterator(pModuleData->objectsPath))
	{
		auto compilandIter = std::find(pModuleData->compilands.begin(), pModuleData->compilands.end(), p.path());
		if (compilandIter != pModuleData->compilands.end())
		{
			auto latestWrite = fs::last_write_time(p.path());
			if (compilandIter->lastWrite < latestWrite || latestWrite > pModuleData->lastCompile)
				objectPaths.emplace_back(compilandIter->path);
			
			continue;
		}

		if (p.path().extension() == ".obj" || p.path().extension() == ".o")
		{
			auto lastWrite = fs::last_write_time(p.path());
			if (lastWrite > pModuleData->lastCompile)
			{
				objectPaths.emplace_back(p.path());
			}
		}
	}
	if (objectPaths.empty())
		return;

	std::vector<const wchar_t *> modulePaths;
	modulePaths.resize(objectPaths.size());
	std::fill(modulePaths.begin(), modulePaths.end(), pModuleData->modulePath.c_str());

	std::vector<const wchar_t*> objPaths;
	objPaths.reserve(objectPaths.size());
	for (auto &path : objectPaths)
		objPaths.push_back(path.c_str());

	lpp::lppBuildPatch(m_pLivePlusPlus->GetLivePP(), modulePaths.data(), objPaths.data(), objPaths.size());
}

void Cry::Lpp::CBuildSystem::OnPrePatch()
{
	
}

void Cry::Lpp::CBuildSystem::OnPostPatch()
{
	//Test 
	for (auto &entry : m_moduleEntries)
	{
		m_pPDBAccess->GetAndFillCompilandPaths(entry.pModuleData.get());
	}
}

bool Cry::Lpp::CBuildSystem::FindAndFillSourcePath(SModuleData* pModuleData)
{
	if (!pModuleData || pModuleData->projectFilePath.empty())
		return false;


	fs::path searchPath = pModuleData->projectFilePath.parent_path();
	while (!searchPath.empty())
	{
		searchPath = searchPath.parent_path();
		searchPath /= L"CMakeCache.txt";
		if (fs::exists(searchPath))
			break;

		searchPath = searchPath.parent_path();
	}
	if (searchPath.empty() || searchPath.filename() != L"CMakeCache.txt")
		return false;

	std::string searchTerm = pModuleData->modulePath.stem().u8string();

	std::ifstream fileStream(searchPath, std::ifstream::in);
	std::string line;
	while (std::getline(fileStream, line))
	{
		if (!searchTerm.empty())
		{
			fs::path sourceDir = ExtractCMakeVariable(line, ECMakeVariables::ModuleSourcePath, searchTerm.c_str());
			if (!sourceDir.empty() && fs::exists(sourceDir))
			{
				pModuleData->sourcePath = sourceDir;
				CryLogAlways("[Live++] Source path for %s found: %ws", pModuleData->moduleName.c_str(), sourceDir.c_str());
				searchTerm = "";
				continue;
			}
		}
				
		FindAndSetCmakePath(line);
		FindAndSetMSbuildPath(line);
	}

	if (pModuleData->sourcePath.empty())
	{
		CryLogAlways("[Live++][CMake] Unable to extract source path for %s", pModuleData->moduleName.c_str());
		return false;
	}

	return true;
}

void CBuildSystem::FindAndSetCmakePath(const std::string &line)
{
	if (line.empty() || !m_cmakePath.empty())
		return;

	fs::path toolPath = ExtractCMakeVariable(line, ECMakeVariables::CMakePath);
	if (!fs::exists(toolPath))
		return;

	m_cmakePath = toolPath;
	CryLogAlways("[Live++] Found CMake: %ws", m_cmakePath.c_str());
}

void CBuildSystem::FindAndSetMSbuildPath(const std::string &line)
{
	if (line.empty() || !m_msbuildPath.empty())
		return;

	fs::path toolPath = ExtractCMakeVariable(line, ECMakeVariables::GeneratorPath);
	if (!fs::exists(toolPath))
		return;

	toolPath /= "MSBuild";

	if (!fs::exists(toolPath))
		return;

	//check different MSBuild versions. TODO: make this a loop
	toolPath /= "15.0";
	if (!fs::exists(toolPath))
	{
		toolPath = toolPath.parent_path();
		toolPath /= "16.0";
		if (!fs::exists(toolPath))
		{
			toolPath = toolPath.parent_path();
			toolPath /= "17.0";
			if (!fs::exists(toolPath))
				return;
		}
	}

	toolPath /= "Bin/MSBuild.exe";
	if (!fs::exists(toolPath))
		return;

	m_msbuildPath = toolPath;
	CryLogAlways("[Live++] Found MSBuild: %ws", m_msbuildPath.c_str());
}


constexpr char* g_cmakeVarNames[]
{
	"CMAKE_COMMAND:INTERNAL=",
	"CMAKE_GENERATOR_INSTANCE:INTERNAL=",
	"_SOURCE_DIR:STATIC=",
};


std::string CBuildSystem::ExtractCMakeVariable(const std::string &line, ECMakeVariables variable, const char *varPrefix /*= nullptr*/)
{
	if (!line.empty() && variable < ECMakeVariables::Last)
	{
		std::string varName;
		if (varPrefix)
			varName = varPrefix;

		varName.append(g_cmakeVarNames[(size_t)variable]);

		auto findPos = line.find(varName);
		if (findPos != line.npos)
			return line.substr(findPos + varName.size());
	}
	return "";	
}

