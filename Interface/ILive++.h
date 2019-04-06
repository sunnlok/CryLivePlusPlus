#pragma once

#include <CrySystem/ICryPlugin.h>

namespace Cry
{
	namespace LPP
	{
		//Implement and register this interface to receive patch events.
		struct IPatchListener
		{
			//Triggered pre module patch
			virtual void OnPrePatch() {};
			//Triggered post module patch
			virtual void OnPostPatch() {};
		};

		//Implement and register this interface to receive compile events.
		struct ICompileListener
		{
			//Triggered on compilation start
			virtual void OnCompileStarted() {};
			//Triggered when the compilation was successful
			virtual void OnCompileSuccess() {};
			//Triggered if an error occured during compilation
			virtual void OnCompileError() {};
		};

		//Query this plugin to use Live++ functionality
		class ILivePlusPlus : public IEnginePlugin
		{
			CRYINTERFACE_DECLARE_GUID(ILivePlusPlus, "{D4FAA642-02EE-4C1A-A5B9-2320474D4247}"_cry_guid);
		public:
			virtual ~ILivePlusPlus() {};

			//Enables or disables a module for Live++ usage. Assumes Dll if no exenstion is specified
			virtual bool EnableModule(const char* module, bool bEnable = true) = 0;

			//Sets the stage at which a sync point will be triggered.
			//0: No automatic sync point
			//1: Pre system update
			//2: Post render submit
			virtual void SetSyncPointMode(int mode) = 0;
			//Enables or disables Live++ for all loaded engine plugins
			virtual void EnableAllPlugins(bool bEnable) = 0;
			//Enable Live++ for the running exectuable (Launcher/Sandbox)
			virtual void EnableExecutable(bool bEnable) = 0;

			//Manually triggers a live++ sync point
			virtual void TriggerSyncPoint() const = 0;;

			//Triggers a Live++ recompile
			virtual void TriggerRecompile() const = 0;;

			//Returns the list of enabled modules
			virtual const std::vector<string>& GetEnabledModules() const = 0;

			//Registers a listener for Live++ patch events
			virtual bool RegisterPatchListener(IPatchListener* pListener) = 0;
			virtual void RemovePatchListener(IPatchListener* pListener) = 0;
			//Registers a listener for Live++ compilation events
			virtual bool RegisterCompileListener(ICompileListener* pListener) = 0;
			virtual void RemoveCompileListener(ICompileListener* pListener) = 0;
		};
	}
}