#include "StdAfx.h"
#include "CVars.h"
#include "CrySystem/IConsole.h"
#include "Live++.h"

using namespace Cry::Lpp;


static void RecompileCMD(IConsoleCmdArgs* pArgs)
{
	if (auto pLPP = CLivePlusPlus::Get())
		pLPP->TriggerRecompile();
}

static void ChangeModulesCMD(IConsoleCmdArgs* pArgs, bool bEnable = true)
{
	if (auto pLPP = CLivePlusPlus::Get())
	{
		for (int i = 1; i < pArgs->GetArgCount(); ++i)
			pLPP->EnableModule(pArgs->GetArg(i), bEnable);
	}
}

static void EnableModulesCMD(IConsoleCmdArgs* pArgs)
{
	ChangeModulesCMD(pArgs);
}

static void DisableModulesCMD(IConsoleCmdArgs* pArgs)
{
	ChangeModulesCMD(pArgs, false);
}

static void OnEnableForExectuableChanged(ICVar* pVar)
{
	if (!pVar)
		return;

	if (auto pLPP = CLivePlusPlus::Get())
		pLPP->EnableExecutable(pVar->GetIVal());
}

static void OnEnableForAllPluginsChanged(ICVar* pVar)
{
	if (!pVar)
		return;

	if (auto pLPP = CLivePlusPlus::Get())
		pLPP->EnableAllPlugins(pVar->GetIVal());
}

static void OnSyncPointModeChanged(ICVar* pVar)
{
	if (!pVar)
		return;

	if (auto pLPP = CLivePlusPlus::Get())
		pLPP->SetSyncPointMode(pVar->GetIVal());
}

SLPPVariables::~SLPPVariables()
{

}

void SLPPVariables::RegisterVariables()
{
	ConsoleRegistrationHelper::AddCommand("lpp_DoRecompile", RecompileCMD, 0,
		"Manually triggers a Live++ recompile"
	);

	ConsoleRegistrationHelper::AddCommand("lpp_EnableModules", EnableModulesCMD, 0,
		"Enables the specified modules for live++ usage. If no extension is specified, .dll will be assumed."
	);
	ConsoleRegistrationHelper::AddCommand("lpp_DisableModules", DisableModulesCMD, 0,
		"Disables the specified modules for live++ usage. If no extension is specified, .dll will be assumed."
	);

	ConsoleRegistrationHelper::Register("lpp_enableForExecutable", &enableForExecutable, 0, 0
		,"Enables the current executable to be used with Live++"
		,OnEnableForExectuableChanged
	);

	ConsoleRegistrationHelper::Register("lpp_enableForAllPlugins", &enableForAllPlugins, 0, 0
		, "Enables Live++ for all loaded engine plugins"
		, OnEnableForAllPluginsChanged
	);	

	ConsoleRegistrationHelper::Register("lpp_useExceptionHandler", &useExceptionHandler, 0, 0
		, "Enables the Live++ exception handler instead of the internal one"
	);

	ConsoleRegistrationHelper::Register("lpp_useExternalBuildSystem", &useExternalBuildSystem, 0, 0
		, "Use an external build system for Live++. NOT IMPLEMENTED"
	);

	ConsoleRegistrationHelper::Register("lpp_syncPointMode", &syncPointMode, 1, 0
		, "Stage at which a Live++ Sync Point should be triggered. \n 0: No automatic sync point \n 1: pre-update \n 2: post-update"
		, OnSyncPointModeChanged
	);

	ConsoleRegistrationHelper::Register("lpp_allowExternalSyncPoints", &allowExternalSyncPoints, 1, 0
		, "Allow the use of external manual sync point calls."
	);

	ConsoleRegistrationHelper::Register("lpp_enableModulesAsync", &enableModulesAsync, 0, 0
		, "Sets Live++ to load modules asynchronously"
	);
	
	pEnabledModules = ConsoleRegistrationHelper::RegisterString("lpp_enabledModules", "", 0
		, "List of modules that are enabled for Live++. If read from config, these modules will be loaded on startup. If no extension is specified, .dll will be assumed."
	);

	pGroupNameOverride = ConsoleRegistrationHelper::RegisterString("lpp_groupNameOverride", "", 0
		, "Override name for the Live++ process group. Project name + version by default"
	);
}

