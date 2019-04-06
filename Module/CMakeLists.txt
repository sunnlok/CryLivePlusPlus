if ((WIN32) AND EXISTS "${SDK_DIR}/Live++")
	option(PLUGIN_LIVE++ "Enable Live++ support" ON)
else()
	option(PLUGIN_LIVE++ "Enable Live++ support" OFF)
endif()
	
if (NOT PLUGIN_LIVE++)
	return()
endif()

start_sources()
sources_platform(ALL)
add_sources("Live++_root.cpp"
	SOURCE_GROUP "Root"
		"resource.h"
		"StdAfx.cpp"
		"StdAfx.h"
)

add_sources("Live++_src.cpp"
	SOURCE_GROUP "Root"
		"CVars.cpp"
		"CVars.h"
        "Live++.cpp"
        "Live++.h"
)

add_sources("Live++_Interface.cpp"
	SOURCE_GROUP "Interface"
		"../Interface/ILive++.h"
)

add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"StdAfx.cpp"
)

end_sources()

CryEngineModule(Live++ PCH "StdAfx.cpp" SOLUTION_FOLDER "CryPlugins")

target_include_directories(${THIS_PROJECT}
PRIVATE 
	"${CRYENGINE_DIR}/Code/CryEngine/CryCommon"
	"${SDK_DIR}/Live++/API"
PUBLIC
    ../Interface
)
if(WIN64)
deploy_runtime_files("${SDK_DIR}/LIVE++/x64/*" "Live++")
elseif(WIN32)
deploy_runtime_files("${SDK_DIR}/LIVE++/x86/*" "Live++")
endif()

include(../Live++.cmake)
LIVEPP_ENABLE_FOR_TARGET(${THIS_PROJECT})