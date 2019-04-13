#include "StdAfx.h"
#include "PDBAccess.h"
#include "CrySystem/ISystem.h"
#include <unknwn.h>
#include <atlconv.h>
#include <cvconst.h>
#include "ModuleData.h"
#include "CrySystem/File/ICryPak.h"


using namespace Cry::Lpp;

HMODULE g_diaLib = NULL;

CPDBAccess::~CPDBAccess()
{
	CryFreeLibrary(g_diaLib);
}


static HRESULT CoCreateDiaFactory(CComPtr<IClassFactory>& pClassFactory)
{
	//Kill me...
	g_diaLib = CryLoadLibrary("msdia140.dll");
	if (!g_diaLib)
		return HRESULT_FROM_WIN32(GetLastError());

	BOOL(WINAPI*DllGetClassObject)(REFCLSID, REFIID, LPVOID*) =
		(BOOL(WINAPI*)(REFCLSID, REFIID, LPVOID*))GetProcAddress(g_diaLib, "DllGetClassObject");

	if (!DllGetClassObject)
		return HRESULT_FROM_WIN32(GetLastError());


	HRESULT hr = DllGetClassObject(CLSID_DiaSource, IID_IClassFactory, (void **)&pClassFactory);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

bool Cry::Lpp::CPDBAccess::Init()
{
	
	auto hr = CoCreateDiaFactory(m_pClassFactory);
	if (FAILED(hr))
	{
		CryLogAlways("[Live++] Could not create class factory for DIA COM objects.");
		return false;
	}
	return true;
}

bool Cry::Lpp::CPDBAccess::GetAndFillCompilandPaths(SModuleData* pModuleData) const
{
	if (!pModuleData)
		return false;

	IDiaDataSource* pDataSource = pModuleData->pPDBData->pDataSource;

	if (!pModuleData->pPDBData->pDataSource)
		return false;

	CComPtr<IDiaSession> psession;
	HRESULT hr = pDataSource->openSession(&psession);

	if (FAILED(hr))
	{
		CryLogAlways("[Live++][PDB ERROR] Failed to open session for %s", pModuleData->moduleName.c_str());
		return false;
	}

	CComPtr<IDiaSymbol> globalSymbol;
	hr = psession->get_globalScope(&globalSymbol);
	if (FAILED(hr))
	{
		CryLogAlways("[Live++][PDB ERROR] Failed to get global scope for %s", pModuleData->moduleName.c_str());
		return false;
	}

	//Find Compilands.
	CComPtr<IDiaEnumSymbols> compilandSymbols;
	hr = globalSymbol->findChildren(SymTagCompiland, NULL, nsNone, &compilandSymbols.p);

	if (FAILED(hr))
	{
		CryLogAlways("[Live++][PDB ERROR] Failed to find compilands for %s", pModuleData->moduleName.c_str());
		return false;
	}

	LONG lCount = 0;
	hr = compilandSymbols->get_Count(&lCount);
	if (FAILED(hr))
	{
		CryLogAlways("[Live++][PDB ERROR] Failed to enumerate compilands for %s", pModuleData->moduleName.c_str());
		return false;
	}

	IDiaSymbol* itemSymbol;
	for (DWORD dwPos = 0; dwPos < (DWORD)lCount; ++dwPos)
	{
		hr = compilandSymbols->Item(dwPos, &itemSymbol);
		if (FAILED(hr))
			continue;

		//Extract path names
		BSTR compilandName = NULL;
		if (FAILED(itemSymbol->get_name(&compilandName)))
		{
			CryLogAlways("[Live++][PDB ERROR] Failed to get name for compiland in %s", pModuleData->moduleName.c_str());
			itemSymbol->Release();
			continue;
		}
		fs::path compilandPath(compilandName);
		SysFreeString(compilandName);

		//Make sure the compiland is an obj
		if (!compilandPath.has_extension()
			|| !(compilandPath.extension() == L".obj" || compilandPath.extension() == L".o")
			)
		{
			itemSymbol->Release();
			continue;
		}	

		BSTR libraryName = NULL;
		if (FAILED(itemSymbol->get_name(&libraryName)))
		{
			CryLogAlways("[Live++][PDB ERROR] Failed to get library name for compiland %ws in %s", compilandPath.filename().wstring(), pModuleData->moduleName.c_str());
			itemSymbol->Release();
			continue;
		}
		fs::path libraryPath(libraryName);
		SysFreeString(libraryName);
		itemSymbol->Release();

		if (compilandPath != libraryPath)
		{
			CryLogAlways("[Live++][PDB ERROR] %ws and %ws not identical", compilandPath.filename().wstring(), libraryPath.filename().wstring());		
			continue;
		}

		if (pModuleData->objectsPath.empty())
		{
			//Only valid check i can think of for now that isn't a massive headache
			//Check that this compiland is in the cryengine cmake build directory
			fs::path projectCheck = compilandPath;

			//Travel upwards to find the project file
			while (!projectCheck.empty())
			{
				projectCheck = projectCheck.parent_path();
				projectCheck /= pModuleData->moduleName.c_str();
				projectCheck.replace_extension("vcxproj");

				if (fs::exists(projectCheck))
				{
					projectCheck = projectCheck.parent_path();
					break;
				}
				projectCheck = projectCheck.parent_path();
			}

			//Found the vs project file and obj location. Fill in data and return
			if (!projectCheck.empty())
			{
				pModuleData->objectsPath = compilandPath.parent_path();
				CryLogAlways("[Live++] Objects path for %s found: %ws", pModuleData->moduleName.c_str(), pModuleData->objectsPath.c_str());
				pModuleData->projectFilePath = projectCheck;
				CryLogAlways("[Live++] Project path for %s found: %ws", pModuleData->moduleName.c_str(), pModuleData->projectFilePath.c_str());
			}	
		}

		//Sort out anything not in the build system
		if (compilandPath.parent_path() != pModuleData->objectsPath)
			continue;

		//Add the compiland, if it doesn't exist yet, to the list of compilands and get its last write time.
		auto compilandIter = std::find(pModuleData->compilands.begin(), pModuleData->compilands.end(), compilandPath);
		if (compilandIter != pModuleData->compilands.end())
			continue;

		pModuleData->compilands.emplace_back(compilandPath);
	}

	if (pModuleData->projectFilePath.empty() || pModuleData->objectsPath.empty())
		return false;

	return true;
}

TPDBDataPtr CPDBAccess::LoadPDBData(fs::path pdbPath) const
{
	auto pdbData = std::make_unique<SPDBData>();

	auto hr = m_pClassFactory->CreateInstance(NULL, IID_IDiaDataSource, (void**)&pdbData->pDataSource);
	if (FAILED(hr))
	{
		CryLogAlways("[Live++] Could not CoCreate DiaDataSource for %s", pdbPath.u8string().c_str());
		return nullptr;
	}
	hr = pdbData->pDataSource->loadDataFromPdb(pdbPath.c_str());
	if (FAILED(hr))
	{
		CryLogAlways("[Live++] Could not load pdb data for %s", pdbPath.u8string().c_str());
		return nullptr;
	}

	return std::move(pdbData);
}
