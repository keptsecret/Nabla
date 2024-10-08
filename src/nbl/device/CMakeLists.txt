get_filename_component(SRC_VIDEO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../video/device_capabilities" ABSOLUTE)
get_filename_component(NBL_DEVICE_GEN_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include" ABSOLUTE) # TODO: fix a bug in checking for GENERATED property which doesnt eveluate properly generator expressions so $<CONFIG> fails

set(NBL_DEVICE_GEN_INCLUDE_DIR "${NBL_DEVICE_GEN_INCLUDE_DIR}")
set(DEVICEGEN_BUILTIN_RESOURCES_DIRECTORY_PATH "${NBL_DEVICE_GEN_INCLUDE_DIR}/nbl/video" CACHE INTERNAL "" FORCE)

macro(NBL_REGISTER_INPUT_DEVICE_FILE NBL_INPUT_PATH NBL_CMD_ARGUMENT)
	list(APPEND NBL_DEPENDS "${NBL_INPUT_PATH}") # update depends list
	list(APPEND NBL_INPUT_GEN_ARGUMENTS "${NBL_CMD_ARGUMENT}" "${NBL_INPUT_PATH}") # update input file gen arguments list for the python script
endmacro()

set(OUTPUT_DL_TEMPLATE "${DEVICEGEN_BUILTIN_RESOURCES_DIRECTORY_PATH}/SPhysicalDeviceLimits_")
set(OUTPUT_DF_TEMPLATE "${DEVICEGEN_BUILTIN_RESOURCES_DIRECTORY_PATH}/SPhysicalDeviceFeatures_")
set(OUTPUT_DT_TEMPLATE "${DEVICEGEN_BUILTIN_RESOURCES_DIRECTORY_PATH}/device_capabilities_traits_")

macro(NBL_REGISTER_OUTPUT_DEVICE_FILE NBL_TEMPLATE_QUALIFIER NBL_HEADER_POSTFIX_WTIH_EXTENSION NBL_CMD_ARGUMENT)
	set(_NBL_OUTPUT_ "${OUTPUT_${NBL_TEMPLATE_QUALIFIER}_TEMPLATE}${NBL_HEADER_POSTFIX_WTIH_EXTENSION}")
	list(APPEND NBL_OUTPUT_HEADERS "${_NBL_OUTPUT_}") # update output headers list
	list(APPEND NBL_OUTPUT_GEN_ARGUMENTS "${NBL_CMD_ARGUMENT}" "${_NBL_OUTPUT_}") # update output file gen arguments list for the python script
	unset(_NBL_OUTPUT_)
endmacro()

# prepare inputs
NBL_REGISTER_INPUT_DEVICE_FILE("${SRC_VIDEO_PATH}/device_limits.json" --limits_json_path)
NBL_REGISTER_INPUT_DEVICE_FILE("${SRC_VIDEO_PATH}/device_features.json" --features_json_path)

# prepare outputs
NBL_REGISTER_OUTPUT_DEVICE_FILE(DL members.h --limits_output_members_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DL subset.h --limits_output_subset_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DF members.h --features_output_members_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DF union.h --features_output_union_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DF intersect.h --features_output_intersect_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DT members.hlsl --traits_output_members_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DT testers.hlsl --traits_output_testers_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DT defaults.hlsl --traits_output_defaults_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DT floats.hlsl --traits_output_floats_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DT enums.hlsl --traits_output_enums_path)
NBL_REGISTER_OUTPUT_DEVICE_FILE(DT jit.h --jit_traits_output_path)

# python script paths
set(NBL_GEN_PY "${CMAKE_CURRENT_SOURCE_DIR}/gen.py")
set(NBL_DGEN_PY "${CMAKE_CURRENT_SOURCE_DIR}/DeviceGen.py")

list(APPEND NBL_DEPENDS
	"${NBL_GEN_PY}"
	"${NBL_DGEN_PY}"
)

set(NBL_COMMAND
	"${_Python3_EXECUTABLE}"
	"${NBL_GEN_PY}"
	${NBL_INPUT_GEN_ARGUMENTS}
	${NBL_OUTPUT_GEN_ARGUMENTS}
)

add_custom_command(OUTPUT ${NBL_OUTPUT_HEADERS}
    DEPENDS ${NBL_DEPENDS}
    COMMAND ${NBL_COMMAND}
    VERBATIM
)

add_custom_target(DeviceHeaders DEPENDS ${NBL_OUTPUT_HEADERS})

if(NBL_EMBED_BUILTIN_RESOURCES)
	LIST_BUILTIN_RESOURCE(NBL_DEVICE_GEN_RESOURCES_TO_EMBED "video/device_capabilities_traits_members.hlsl")
	LIST_BUILTIN_RESOURCE(NBL_DEVICE_GEN_RESOURCES_TO_EMBED "video/device_capabilities_traits_testers.hlsl")
	LIST_BUILTIN_RESOURCE(NBL_DEVICE_GEN_RESOURCES_TO_EMBED "video/device_capabilities_traits_defaults.hlsl")
	LIST_BUILTIN_RESOURCE(NBL_DEVICE_GEN_RESOURCES_TO_EMBED "video/device_capabilities_traits_floats.hlsl")
	LIST_BUILTIN_RESOURCE(NBL_DEVICE_GEN_RESOURCES_TO_EMBED "video/device_capabilities_traits_enums.hlsl")

	get_filename_component(_DEVICE_GEN_BR_OUTPUT_DIRECTORY_HEADER_ "${CMAKE_CURRENT_BINARY_DIR}/builtin/include" ABSOLUTE)
	get_filename_component(_DEVICE_GEN_BR_OUTPUT_DIRECTORY_SOURCE_ "${CMAKE_CURRENT_BINARY_DIR}/builtin/src" ABSOLUTE)

	ADD_CUSTOM_BUILTIN_RESOURCES(deviceGenBuiltinResourceData NBL_DEVICE_GEN_RESOURCES_TO_EMBED "${NBL_DEVICE_GEN_INCLUDE_DIR}" "nbl" "nbl::devicegen::builtin" "${_DEVICE_GEN_BR_OUTPUT_DIRECTORY_HEADER_}" "${_DEVICE_GEN_BR_OUTPUT_DIRECTORY_SOURCE_}" "STATIC" "INTERNAL")
	add_dependencies(deviceGenBuiltinResourceData DeviceHeaders)
endif()

set(NBL_DEVICE_GEN_INCLUDE_DIR
	"${NBL_DEVICE_GEN_INCLUDE_DIR}"
PARENT_SCOPE)