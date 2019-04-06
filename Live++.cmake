function(LIVEPP_ENABLE_FOR_TARGET target)
if(WIN32)
    option(LIVE++_${target} "Enable Live++ settings for ${target}" ON)
elseif() 
    option(LIVE++_${target}"Enable Live++ support for ${target}" OFF)
endif()

if(NOT LIVE++_${target})
    return()
endif()

set(CompFlags "/Zi")

if(NOT WIN64)
    set(CompFlags "${CompFlags} /hotpatch")
endif()

if(OPTION_UNITY_BUILD)
    set(CompFlags "${CompFlags} /Gm-")
endif()

set(LinkFlags "/FUNCTIONPADMIN /OPT:NOREF /OPT:NOICF /DEBUG:FULL")

set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS_PROFILE "${CompFlags}")
set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_PROFILE " ${LinkFlags}")

set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS_DEBUG "${CompFlags}")
set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG " ${LinkFlags}")

endfunction()