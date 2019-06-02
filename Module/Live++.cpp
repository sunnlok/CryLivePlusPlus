#include "StdAfx.h"
#include "Live++.h"

#include <CryCore/Platform/platform_impl.inl>
#include <CrySystem/IProjectManager.h>
#include <CrySystem/IConsole.h>

#include <LPP_API.h>
#include "CryCore/StlUtils.h"
#include "ProgressMeter.h"

#ifdef LPP_BUILD_SYSTEM
#include "BuildSystem/BuildSystem.h"
#endif

#ifdef CRY_PLATFORM_X64
#define  LPP_MODULE "LPP_x64.dll"
#else
#define  LPP_MODULE "LPP_x86.dll"
#endif 

using namespace Cry::Lpp;

CLivePlusPlus* g_Instance = nullptr;
CLivePlusPlus* CLivePlusPlus::Get()
{
	return g_Instance;
}

CLivePlusPlus::~CLivePlusPlus()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
	CryFreeLibrary(m_livePP);
}

bool CLivePlusPlus::Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
{
	FindExecutablePath();

	//Make sure the Live++ Module is loaded
	fs::path lppPath = m_executablePath;
	lppPath.replace_filename(L"Live++");
	//lppPath /= ;
	SetDllDirectoryW(lppPath.c_str());
	m_livePP = CryLoadLibrary(LPP_MODULE);
	if (m_livePP == NULL)
	{
		CryLogAlways("[Live++] Failed to load Live++ library.");
		return false;
	}

	//Check if we have the correct version.
	const char* apiVersion = LPP_VERSION;
	const char* dllVersion = lpp::lppGetVersion(m_livePP);
	CryLogAlways("[Live++] API version: %s", apiVersion);
	CryLogAlways("[Live++] DLL version: %s", dllVersion);
	if (!lpp::lppCheckVersion(m_livePP))
	{
		CryLogAlways("[Live++] ERROR: Version mismatch detected!");
		return false;
	}
	g_Instance = this;

	auto pSystem = gEnv->pSystem;

	//Initialize Console Variables
	m_variables.RegisterVariables();

	//Install the exception Handler
	if (m_variables.useExceptionHandler)
		lpp::lppInstallExceptionHandler(m_livePP);

	//Check if we should use cryengines build system
#ifdef LPP_BUILD_SYSTEM
	if (m_variables.useExternalBuildSystem)
	{
		m_pBuildSystem = std::make_unique<CBuildSystem>(this);
		if (!m_pBuildSystem->Init())
		{
			m_pBuildSystem.reset(nullptr);
			m_variables.useExternalBuildSystem = 0;
		}
		else
		{
			lpp::lppUseExternalBuildSystem(m_livePP);
		}
	}
#endif
		

	//Register the process group
	string groupName;
	groupName.Format("%s %s", pSystem->GetIProjectManager()->GetCurrentProjectName(), pSystem->GetProductVersion().ToString().c_str());

	//Check if we have an override name
	if (m_variables.pGroupNameOverride)
	{
		const char* nameOverride = m_variables.pGroupNameOverride->GetString();
		if (nameOverride && nameOverride[0])
			groupName = nameOverride;
		else
			m_variables.pGroupNameOverride->Set(groupName.c_str());
	}
	lpp::lppRegisterProcessGroup(m_livePP, groupName.c_str());


	pSystem->GetISystemEventDispatcher()->RegisterListener(this, "Live++");
	EnableUpdate(EUpdateStep::MainUpdate, true);
	m_pProgressMeter = std::make_unique<CProgressMeter>();
	return true;
}

void CLivePlusPlus::FindExecutablePath()
{
	//Extract the executable path
	wchar_t moduleName[MAX_PATH + 1];
	GetModuleFileNameW(NULL, moduleName, MAX_PATH + 1);
	m_executablePath = moduleName;
}

void CLivePlusPlus::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
	if (event == ESystemEvent::ESYSTEM_EVENT_CRYSYSTEM_INIT_DONE && m_livePP != NULL)
		OnSystemInitialized();
}

void CLivePlusPlus::OnSystemInitialized()
{
	SetSyncPointMode(m_variables.syncPointMode);

	if (m_variables.pEnabledModules)
	{
		auto pModules = m_variables.pEnabledModules->GetString();
		if (!pModules || !pModules[0])
			return;

		string modules(pModules);

		int tokenPos = 0;
		string module;
		while (tokenPos >= 0)
		{
			module = modules.Tokenize(" ", tokenPos);
			if (!module.empty())
				EnableModule(module.c_str(), true);
		}
	}

	//enable modules that should be loaded
	if (m_variables.enableForExecutable)
		EnableExecutable(true);

	if (m_variables.enableForAllPlugins)
		EnableAllPlugins(true);
}

void CLivePlusPlus::SetSyncPointMode(int mode)
{
	bool bBeforeSystem = false;
	bool bAfterRender = false;
	switch (mode)
	{
	case 0:
		break;
	case 2:
		bAfterRender = true;
		break;
	case 1:
	default:
		bBeforeSystem = true;
		break;
	}

	EnableUpdate(EUpdateStep::BeforeSystem, bBeforeSystem);
	EnableUpdate(EUpdateStep::AfterRenderSubmit, bAfterRender);
	m_variables.syncPointMode = mode;
}

void CLivePlusPlus::EnableAllPlugins(bool bEnable)
{
	auto pProjectManager = gEnv->pSystem->GetIProjectManager();
	int numPlugins = pProjectManager->GetPluginCount();

	Cry::IPluginManager::EType type;
	string pluginPath;
	DynArray<EPlatform> platforms;
	for (int i = 0; i < numPlugins; ++numPlugins)
	{
		pProjectManager->GetPluginInfo(i, type, pluginPath, platforms);
		if (!pluginPath.empty() && EnableModule(pluginPath.c_str(), bEnable))
			m_variables.enableForAllPlugins = bEnable;		
	}
}

void CLivePlusPlus::EnableExecutable(bool bEnable)
{
	if (!m_executablePath.empty() && m_executablePath.has_filename())
	{
		if (!m_executablePath.has_extension())
			m_executablePath.replace_extension(".exe");

		if (EnableModule(m_executablePath.filename().u8string().c_str(), bEnable))
			m_variables.enableForExecutable = bEnable;
	}
		
}

void CLivePlusPlus::TriggerSyncPoint() const
{
	if (m_variables.allowExternalSyncPoints)
		DoSync();
}


void CLivePlusPlus::DoSync() const
{
	if (m_livePP)
		lpp::lppSyncPoint(m_livePP);
}

void CLivePlusPlus::TriggerRecompile() const
{
	if (!m_livePP)
		return;

	if (m_pBuildSystem)
	{
		for (auto &module : m_enabledModules)
		{
			m_pBuildSystem->ApplyModuleChanges(module);
		}

	}

	lpp::lppTriggerRecompile(m_livePP);
}

bool CLivePlusPlus::EnableModule(const char* module, bool bEnable /*= true*/)
{
	if (bEnable)
		CryLogAlways("[Live++] Trying to enable module %s", module);
	else
		CryLogAlways("[Live++] Trying to disable module %s", module);

	//Module needs to be loaded
	HMODULE moduleHandle = NULL;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &moduleHandle))
		CryLogAlways("[Live++] Failed to get handle for module %s", module);

	wchar_t modulePath[MAX_PATH + 1];
	if (!GetModuleFileNameW(moduleHandle, modulePath, MAX_PATH + 1))
		CryLogAlways("[Live++] Failed to get path for module %s", module);

	fs::path path(modulePath);

	//sanitize for .dll or .exe... just in case
	if (!path.has_extension())
		path.replace_extension(".dll");
			
	
	//check if the module is already loaded
	string moduleName(path.filename().u8string().c_str());
	auto findIter = std::find(m_enabledModules.begin(), m_enabledModules.end(), moduleName);

	if (findIter != m_enabledModules.end() && bEnable)
	{
		CryLogAlways("[Live++] Module already enabled.");
		return true;
	}	
	else if (findIter == m_enabledModules.end() && !bEnable)
	{
		CryLogAlways("[Live++] Module not enabled. No need to disable");
		return false;
	}
		
	if (!fs::exists(path))
	{
		CryLogAlways("[Live++] Failed to load module at %s", path.u8string().c_str());
		return false;
	}

	//Load the module
	void* waitToken = nullptr;
	if (bEnable)
	{
		bool lppEnable = true;
#ifdef LPP_BUILD_SYSTEM
		if (m_pBuildSystem.get())
			lppEnable = m_pBuildSystem->AddModuleWatch(moduleName, path);
		else
			lppEnable = !m_variables.useExternalBuildSystem;
#endif 
		if(lppEnable)
		{
			waitToken = lpp::lppEnableModuleAsync(m_livePP, path.c_str());
		}
	}
	else
	{
#ifdef LPP_BUILD_SYSTEM
		if (m_pBuildSystem.get())
			m_pBuildSystem->RemoveModuleWatch(moduleName);
#endif 
		waitToken = lpp::lppDisableModuleAsync(m_livePP, path.c_str());
	}
		
	
	if (!waitToken)
	{
		CryLogAlways("[Live++] Failed to load module at %s", path.u8string().c_str());
		return false;
	}
	lpp::lppWaitForToken(m_livePP, waitToken);

	//Triger a recompile in case the module wasnt recompiled after the last session
	if (bEnable)
	{
		lpp::lppTriggerRecompile(m_livePP);
	}

	//Update the list
	if (!bEnable)
		m_enabledModules.erase(findIter);
	else
		m_enabledModules.emplace_back(std::move(moduleName));

	if (auto pModuleList = m_variables.pEnabledModules)
	{
		string newModuleList;
		for (auto &enabledModule : m_enabledModules)
		{
			newModuleList.append(enabledModule);
			newModuleList.append(" ");
		}


		pModuleList->Set(newModuleList.c_str());
	}

	return true;
}

bool CLivePlusPlus::RegisterPatchListener(IPatchListener* pListener)
{
	return stl::push_back_unique(m_patchListeners, pListener);
}

void CLivePlusPlus::RemovePatchListener(IPatchListener* pListener)
{
	stl::find_and_erase(m_patchListeners, pListener);
}

bool CLivePlusPlus::RegisterCompileListener(ICompileListener* pListener)
{
	return stl::push_back_unique(m_compileListeners, pListener);
}

void CLivePlusPlus::RemoveCompileListener(ICompileListener* pListener)
{
	stl::find_and_erase(m_compileListeners, pListener);
}

void CLivePlusPlus::TriggerEvent(ELivePlusPlusEvent event)
{
	if (event == ELivePlusPlusEvent::PrePatch || event == ELivePlusPlusEvent::PostPatch)
	{
		for (auto pListener : m_patchListeners)
		{
			if (event == ELivePlusPlusEvent::PrePatch)
				pListener->OnPrePatch();		
			else if (event == ELivePlusPlusEvent::PostPatch)
				pListener->OnPostPatch();
		}
	}
	else
	{
		for (auto pListener : m_compileListeners)
		{
			if (event == ELivePlusPlusEvent::CompileStarted)
				pListener->OnCompileStarted();
			else if (event == ELivePlusPlusEvent::CompileSuccess)
				pListener->OnCompileSuccess();
			else if (event == ELivePlusPlusEvent::CompileError)
				pListener->OnCompileError();
		}
	}
}

void CLivePlusPlus::MainUpdate(float frameTime)
{
	if (m_pProgressMeter.get())
		m_pProgressMeter->Update();
}

static void OnPrePatch()
{
	if (g_Instance)
		g_Instance->TriggerEvent(CLivePlusPlus::ELivePlusPlusEvent::PrePatch);
}
LPP_PREPATCH_HOOK(OnPrePatch);

static void OnPostPatch()
{
	if (g_Instance)
		g_Instance->TriggerEvent(CLivePlusPlus::ELivePlusPlusEvent::PostPatch);
}
LPP_POSTPATCH_HOOK(OnPostPatch);

static void OnCompileStarted()
{
	if (g_Instance)
		g_Instance->TriggerEvent(CLivePlusPlus::ELivePlusPlusEvent::CompileStarted);
}
LPP_COMPILE_START_HOOK(OnCompileStarted);

static void OnCompileSuccess()
{
	if (g_Instance)
		g_Instance->TriggerEvent(CLivePlusPlus::ELivePlusPlusEvent::CompileSuccess);
}
LPP_COMPILE_SUCCESS_HOOK(OnCompileSuccess);

static void OnCompileError()
{
	if (g_Instance)
		g_Instance->TriggerEvent(CLivePlusPlus::ELivePlusPlusEvent::CompileError);
}
LPP_COMPILE_ERROR_HOOK(OnCompileError);

CRYREGISTER_SINGLETON_CLASS(CLivePlusPlus)


