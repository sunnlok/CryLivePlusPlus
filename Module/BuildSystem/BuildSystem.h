#pragma once
#include "ILive++.h"


namespace Cry
{
	namespace Lpp
	{

		struct SModuleData;
		class  CLivePlusPlus;
		class CPDBAccess;

		enum class ECMakeVariables : size_t
		{
			CMakePath,
			GeneratorPath,
			ModuleSourcePath,
			Last
		};

		class CBuildSystem : public IPatchListener
		{
		public:
			CBuildSystem(CLivePlusPlus* pLpp);
			~CBuildSystem();
			bool Init();


			//Starts watching the specified module for changes
			bool AddModuleWatch(const string &moduleName, fs::path &modulePath);

			void RemoveModuleWatch(const string &moduleName);

			void ApplyModuleChanges(const string &moduleName);

			virtual void OnPrePatch() override;


			virtual void OnPostPatch() override;

		private:

			//Tries to extract the actual source path from the CMake cache.
			bool FindAndFillSourcePath(SModuleData* pModuleData);
			std::string ExtractCMakeVariable(const std::string &line, ECMakeVariables variable, const char *varPrefix = nullptr);

			void		FindAndSetCmakePath(const std::string &line);
			void		FindAndSetMSbuildPath(const std::string &line);
		private:
			CLivePlusPlus*					m_pLivePlusPlus;
			std::unique_ptr<CPDBAccess>	    m_pPDBAccess;

			fs::path						m_cmakePath;
			fs::path						m_msbuildPath;


		private:
			struct SModuleEntry
			{
				SModuleEntry(std::unique_ptr<SModuleData> &&moduleData);

				bool   isDirty = false;

				uint32 moduleHash = 0;
				std::unique_ptr<SModuleData> pModuleData;
			};

			std::vector<SModuleEntry> m_moduleEntries;
		};
	}
}