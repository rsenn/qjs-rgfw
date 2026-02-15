project(qjs-rgfw C)

cmake_minimum_required(VERSION 3.5)

message("Building project [1;31m${PROJECT_NAME}[0m")

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)

include(FindOpenGL)
if(NOT PKG_CONFIG_FOUND)
  include(FindPkgConfig)
endif(NOT PKG_CONFIG_FOUND)
include(CheckCCompilerFlag)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindQuickJS.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/UseMultiArch.cmake)

check_c_compiler_flag("-fvisibility=hidden" VISIBILITY_HIDDEN)
if(VISIBILITY_HIDDEN)
  set(MODULE_COMPILE_FLAGS ${MODULE_COMPILE_FLAGS} "-fvisibility=hidden")
endif(VISIBILITY_HIDDEN)

find_quickjs()
configure_quickjs()

add_library(
  qjs-rgfw SHARED
   quickjs-rgfw.c)

set_target_properties(
  qjs-rgfw PROPERTIES OUTPUT_NAME rgfw PREFIX "" COMPILE_FLAGS
                                                     "${MODULE_COMPILE_FLAGS} -DJS_SHARED_LIBRARY")

target_include_directories(qjs-rgfw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/rgfw/src
                                              ${QUICKJS_INCLUDE_DIR})
target_link_libraries(qjs-rgfw PRIVATE #${GLFW_LIBRARY}
                                         ${GLEW_LIBRARY} OpenGL::GL ${QUICKJS_LIBRARY})
target_compile_definitions(qjs-rgfw PRIVATE JS_SHARED_LIBRARY=1 )

if(NOT CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT CMAKE_INSTALL_LIBDIR)
if(NOT QUICKJS_C_MODULE_DIR)
  set(QUICKJS_C_MODULE_DIR "${CMAKE_INSTALL_LIBDIR}/quickjs")
endif(NOT QUICKJS_C_MODULE_DIR)

install(TARGETS qjs-rgfw DESTINATION "${QUICKJS_C_MODULE_DIR}"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
                    WORLD_EXECUTE)

