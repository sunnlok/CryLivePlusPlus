#pragma once

#include <CrySystem/ISystem.h>
#include "../interface/ILive++.h"
#include "CVars.h"



namespace Cry
{
	namespace Lpp
	{
		class CLivePlusPlus final
			: public Cry::Lpp::ILivePlusPlus
			, public ISystemEventListener
		{
			CRYINTERFACE_BEGIN()
				CRYINTERFACE_ADD(Cry::Lpp::ILivePlusPlus)
				CRYINTERFACE_ADD(Cry::IEnginePlugin)
			CRYINTERFACE_END()

			CRYGENERATE_SINGLETONCLASS_GUID(CLivePlusPlus, "Live++", "{F2B10846-E489-4C00-B3AC-165DFED0847D}"_cry_guid)

				CLivePlusPlus() = default;
			virtual ~CLivePlusPlus();

			// Cry::IEnginePlugin
			virtual const char* GetCategory() const override { return "Utility"; }
			virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams) override;
			virtual const char* GetName() const override { return "Live++"; }
			virtual void UpdateBeforeSystem() override { DoSync(); }
			virtual void UpdateAfterRenderSubmit() override { DoSync(); }
			// ~Cry::IEnginePlugin

			// ISystemEventListener
			virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;
			// ~ISystemEventListener
		public:
			
			//Enables or disables a module for Live++ usage. Assumes Dll if no exenstion is specified
			bool EnableModule(const char* module, bool bEnable = true);

			//Sets the stage at which a sync point will be triggered.
			//0: No automatic sync point
			//1: Pre system update
			//2: Post render submit
			void SetSyncPointMode(int mode);
			//Enables or disables Live++ for all loaded engine plugins
			void EnableAllPlugins(bool bEnable);
			//Enable Live++ for the running exectuable (Launcher/Sandbox)
			void EnableExecutable(bool bEnable);

			//Manually triggers a live++ sync point
			void TriggerSyncPoint() const;

			//Triggers a Live++ recompile
			void TriggerRecompile() const;

			//Returns the list of enabled modules
			const std::vector<string>& GetEnabledModules() const { return m_enabledModules; }

			//Registers a listener for Live++ patch events
			virtual bool RegisterPatchListener(IPatchListener* pListener) override;
			virtual void RemovePatchListener(IPatchListener* pListener) override;

			//Registers a listener for Live++ compilation events
			virtual bool RegisterCompileListener(ICompileListener* pListener) override;
			virtual void RemoveCompileListener(ICompileListener* pListener) override;
		private:
			//Trigger Live++ Sync point
			void DoSync() const;

			//Reference to the live++ module;
			HMODULE m_livePP = NULL;
			
			//Store the Path and executable name in UTF-8 because we need it for enabling/disabling modules 
			string m_executablePath;
			string m_executableName;

			//List of all module names for which Live++ should be enabled
			std::vector<string> m_enabledModules;

			//Console variable storage
			SLPPVariables m_variables;

			//Live++ event listeners
			std::vector<ICompileListener*> m_compileListeners;
			std::vector<IPatchListener*> m_patchListeners;
		public:
			enum class ELivePlusPlusEvent
			{
				PrePatch,
				PostPatch,
				CompileStarted,
				CompileSuccess,
				CompileError
			};
			void  TriggerEvent(ELivePlusPlusEvent event);

			static CLivePlusPlus* Get();			
		};
	}
}



