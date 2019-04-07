#pragma once

struct ICVar;

namespace Cry
{
	namespace Lpp
	{
		struct SLPPVariables final
		{
			SLPPVariables() = default;
			~SLPPVariables();

			void RegisterVariables();

			int enableForExecutable		= 0;	//lpp_enableForExecutable
			int enableForAllPlugins		= 0;	//lpp_enableForAllPlugins
			int useExceptionHandler		= 0;	//lpp_useExceptionHandler
			int useExternalBuildSystem	= 0;	//Not sure how to implement this yet
			int syncPointMode			= 1;	//lpp_syncPointMode
			int allowExternalSyncPoints = 1;	//lpp_allowExternalSyncPoints
			int enableModulesAsync		= 0;		//lpp_enableModulesAsync

			ICVar*	pEnabledModules		= nullptr; // List of modules to load and enable for Live++
			ICVar*  pGroupNameOverride	= nullptr;
		};
	};
};