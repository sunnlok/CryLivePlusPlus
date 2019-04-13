if ((WIN32) AND EXISTS "${SDK_DIR}/Live++")
	option(PLUGIN_LIVE++ "Enable Live++ support" ON)
else()
	option(PLUGIN_LIVE++ "Enable Live++ support" OFF)
endif()
	
if (NOT PLUGIN_LIVE++)
	return()
endif()



if(EXISTS ${CMAKE_GENERATOR_INSTANCE})
	set(DIA_SDK_INCLUDE "${CMAKE_GENERATOR_INSTANCE}/DIA SDK/Include")
	if(WIN64)
		set(DIA_SDK_LIB "${CMAKE_GENERATOR_INSTANCE}/DIA SDK/lib/amd64")
	else()
		set(DIA_SDK_LIB "${CMAKE_GENERATOR_INSTANCE}/DIA SDK/lib/amd64")
    endif()
	
	
	if(EXISTS ${DIA_SDK_INCLUDE} AND EXISTS ${DIA_SDK_LIB})
		set(LPP_BUILD_SYSYEM TRUE)
	endif()
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
		"ProgressMeter.cpp"
		"ProgressMeter.h"
)

add_sources("Live++_Interface.cpp"
	SOURCE_GROUP "Interface"
		"../Interface/ILive++.h"
)

if(${LPP_BUILD_SYSYEM})
add_sources("Live++_BuildSystem.cpp"
	SOURCE_GROUP "Build System"
		"BuildSystem/BuildSystem.h"
		"BuildSystem/BuildSystem.cpp"
		"BuildSystem/PDBAccess.h"
		"BuildSystem/PDBAccess.cpp"
		"BuildSystem/ModuleData.h"
		"BuildSystem/PDBSymbolMappings.h"
)
endif()

add_sources("NoUberFile"
	SOURCE_GROUP "Root"
		"StdAfx.cpp"
)

end_sources()

CryEngineModule(Live++ PCH "StdAfx.cpp" SOLUTION_FOLDER "CryPlugins")

if(${LPP_BUILD_SYSYEM})
	target_compile_definitions(${THIS_PROJECT} PRIVATE LPP_BUILD_SYSTEM=1)

	target_include_directories(${THIS_PROJECT}
		PRIVATE 
		"${CRYENGINE_DIR}/Code/CryEngine/CryCommon"
		"${SDK_DIR}/Live++/API"
		"${DIA_SDK_INCLUDE}"
		PUBLIC
		../Interface
	)
	target_link_directories(${THIS_PROJECT} PRIVATE "${DIA_SDK_LIB}")
	target_link_libraries(${THIS_PROJECT} PRIVATE diaguids)

else()
	target_include_directories(${THIS_PROJECT}
		PRIVATE 
		"${CRYENGINE_DIR}/Code/CryEngine/CryCommon"
		"${SDK_DIR}/Live++/API"
		PUBLIC
		../Interface
	)
endif()




#set_target_properties(${THIS_PROJECT} PROPERTIES
#			CXX_STANDARD 17
 #           )
set_property(TARGET ${THIS_PROJECT} APPEND_STRING PROPERTY COMPILE_FLAGS_PROFILE "/std:c++17")

if(WIN64)
deploy_runtime_files("${SDK_DIR}/LIVE++/x64/*" "Live++")
elseif(WIN32)
deploy_runtime_files("${SDK_DIR}/LIVE++/x86/*" "Live++")
endif()

include(../Live++.cmake)
LIVEPP_ENABLE_FOR_TARGET(${THIS_PROJECT})