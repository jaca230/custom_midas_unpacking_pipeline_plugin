cmake_minimum_required(VERSION 3.14)

project(custom_midas_unpacking_pipeline_plugin VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Require MIDASSYS environment
if(NOT DEFINED ENV{MIDASSYS})
  message(FATAL_ERROR
    "Environment variable MIDASSYS not set.\n"
    "Please source your MIDAS environment or run the detect_environment.sh script."
  )
endif()

set(MIDASSYS_INCLUDE_DIRS
  $ENV{MIDASSYS}/include
  $ENV{MIDASSYS}/midasio
)
set(MIDASSYS_LIB_DIR $ENV{MIDASSYS}/lib)

# Dependencies
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree Hist)
include(${ROOT_USE_FILE})
find_package(AnalysisPipelineStagesCore REQUIRED)
find_package(default_midas_unpacking_pipeline_plugin REQUIRED)
find_package(unpacker REQUIRED)

# Sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE ALL_STAGE_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/stages/*.h)
list(FILTER ALL_STAGE_HEADERS EXCLUDE REGEX "LinkDef\\.h$")

# Create shared library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Add local and install includes
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Add MIDAS includes
foreach(dir IN LISTS MIDASSYS_INCLUDE_DIRS)
  target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${dir}>
    $<INSTALL_INTERFACE:include/midas>
  )
endforeach()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        AnalysisPipelineStagesCore::analysis_pipeline_stages_core
        default_midas_unpacking_pipeline_plugin::default_midas_unpacking_pipeline_plugin
        unpacker::common_data_products
        unpacker::common_unpacking
        unpacker::nalu_data_products
        unpacker::nalu_unpacking
        ROOT::Core ROOT::RIO ROOT::Tree ROOT::Hist
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        ${MIDASSYS_LIB_DIR}/libmfe.a
        ${MIDASSYS_LIB_DIR}/libmidas.a
)

# Gather include directories for ROOT dictionary
set(ROOT_LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/include/LinkDef.h)
set(ROOT_DICTIONARY_INCLUDE_OPTIONS "-I${CMAKE_CURRENT_SOURCE_DIR}/include")

foreach(dir IN LISTS MIDASSYS_INCLUDE_DIRS)
  list(APPEND ROOT_DICTIONARY_INCLUDE_OPTIONS "-I${dir}")
endforeach()

# Helper macro for pulling include paths from a target
function(append_target_includes_to_root_dict target_name)
  get_target_property(INCLUDE_DIRS ${target_name} INTERFACE_INCLUDE_DIRECTORIES)
  if(INCLUDE_DIRS AND NOT INCLUDE_DIRS STREQUAL "${target_name}_NOTFOUND")
    foreach(dir IN LISTS INCLUDE_DIRS)
      if(dir MATCHES "^\\$<BUILD_INTERFACE:(.+)>$")
        set(actual "${CMAKE_MATCH_1}")
        if(EXISTS "${actual}")
          list(APPEND ROOT_DICTIONARY_INCLUDE_OPTIONS "-I${actual}")
        endif()
      elseif(NOT dir MATCHES "^\\$")
        if(EXISTS "${dir}")
          list(APPEND ROOT_DICTIONARY_INCLUDE_OPTIONS "-I${dir}")
        endif()
      endif()
    endforeach()
  endif()
  set(ROOT_DICTIONARY_INCLUDE_OPTIONS ${ROOT_DICTIONARY_INCLUDE_OPTIONS} PARENT_SCOPE)
endfunction()

append_target_includes_to_root_dict(AnalysisPipelineStagesCore::analysis_pipeline_stages_core)
append_target_includes_to_root_dict(default_midas_unpacking_pipeline_plugin::default_midas_unpacking_pipeline_plugin)
append_target_includes_to_root_dict(unpacker::common_data_products)
append_target_includes_to_root_dict(unpacker::common_unpacking)
append_target_includes_to_root_dict(unpacker::nalu_data_products)
append_target_includes_to_root_dict(unpacker::nalu_unpacking)


# Generate ROOT dictionary
ROOT_GENERATE_DICTIONARY(G__${PROJECT_NAME}
  ${ALL_STAGE_HEADERS}
  LINKDEF ${ROOT_LINKDEF}
  OPTIONS ${ROOT_DICTIONARY_INCLUDE_OPTIONS} -Wno-inconsistent-missing-override
)

target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/G__${PROJECT_NAME}.cxx
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

# Alias
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Install logic (only if standalone)
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  install(TARGETS ${PROJECT_NAME}
      EXPORT ${PROJECT_NAME}Targets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.rootmap
    ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_rdict.pcm
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  )

  install(EXPORT ${PROJECT_NAME}Targets
      FILE ${PROJECT_NAME}Targets.cmake
      NAMESPACE ${PROJECT_NAME}::
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  )

  install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  )
endif()
