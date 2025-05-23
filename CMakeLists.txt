#CMAKE最小版本要求,可修改
#BUILD_TYPE=Release,Debug  UNIT_TEST=true,false
CMAKE_MINIMUM_REQUIRED(VERSION 2.9)

PROJECT("com")
SET(COM_MAJOR_VERSION 2)
SET(COM_MINOR_VERSION 5)
SET(COM_PATCH_VERSION 3)

ADD_DEFINITIONS("-DMODULE_IS_LIB")
ADD_DEFINITIONS("-DSQLITE_THREADSAFE=1")
ADD_DEFINITIONS("-DHAVE_SIGNAL_H")
ADD_DEFINITIONS("-DHAVE_INTTYPES_H")
ADD_DEFINITIONS("-DHAVE_MALLOC_H")
ADD_DEFINITIONS("-D_GNU_SOURCE")
ADD_DEFINITIONS("-DSPDLOG_PREVENT_CHILD_FD")

IF (MSVC)
  ADD_DEFINITIONS("-DWIN32_LEAN_AND_MEAN")
  ADD_DEFINITIONS("-DHAVE__SNPRINTF_S")
  ADD_DEFINITIONS("-DHAVE__VSNPRINTF_S")
  ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS")
  ADD_DEFINITIONS("-D_WIN32_WINNT=${CMAKE_WIN32_WINNT}")
ELSEIF (UNIX OR APPLE)
  ADD_DEFINITIONS("-DHAVE_STRINGS_H")
ENDIF()

IF ("${UNIT_TEST}" STREQUAL "true")
  ADD_DEFINITIONS("-DUNIT_TEST")
ENDIF()

IF ("${BUILD_TYPE}" STREQUAL "Debug")
  ADD_DEFINITIONS("-D__DEBUG__")
  IF (MSVC)
    ADD_COMPILE_OPTIONS( /MDd)
  ELSE()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  ENDIF()
ELSE()
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
  IF (MSVC)
    ADD_COMPILE_OPTIONS( /MD)
  ENDIF()
ENDIF()

#-Woverloaded-virtual
#C/C++文件编译选项,无需修改
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -fwrapv")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

IF (UNIX OR APPLE)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wno-unused-result")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11 -Wno-unused-result")
ENDIF()

#头文件搜索路径,可修改
INCLUDE_DIRECTORIES(
	${PROJECT_SOURCE_DIR}/src/export
	${PROJECT_SOURCE_DIR}/src
)

#库文件搜索路径,可修改
LINK_DIRECTORIES(
	${PROJECT_SOURCE_DIR}
)

#依赖库,无需修改
IF (UNIX OR APPLE)
  SET(PROJECT_LIB
  	  ${PROJECT_LIB}
      pthread
      dl
      m
)
ELSE()
  ADD_COMPILE_OPTIONS( /W3 /wd4819 /wd4251 /wd4200)
  SET(PROJECT_LIB
      ${PROJECT_LIB}
      msvcrt.lib
  )
ENDIF()

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

AUX_SOURCE_DIRECTORY(src PROJECT_SRC)
IF ("${UNIT_TEST}" STREQUAL "true")
 AUX_SOURCE_DIRECTORY(src/test PROJECT_SRC)
ENDIF()


SET(CMAKE_SKIP_BUILD_RPATH TRUE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

SET(CPACK_GENERATOR TGZ)
SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/out/export)
SET(CMAKE_INSTALL_CONFIG_NAME ${CMAKE_BUILD_TYPE})
SET(CPACK_PACKAGE_VERSION ${COM_MAJOR_VERSION}.${COM_MINOR_VERSION}.${COM_PATCH_VERSION})
SET(CPACK_PACKAGE_VERSION_MAJOR ${COM_MAJOR_VERSION})
SET(CPACK_PACKAGE_VERSION_MINOR ${COM_MINOR_VERSION})
SET(CPACK_PACKAGE_VERSION_PATCH ${COM_PATCH_VERSION})
SET(CPACK_PACKAGE_FILE_NAME ${CMAKE_PROJECT_NAME}_${CPACK_PACKAGE_VERSION}_${OS_TYPE}_${BUILD_ARCH}_${BUILD_TYPE})
SET(CPACK_PACKAGE_DIRECTORY ${PROJECT_SOURCE_DIR}/out)
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
SET(CPACK_ARCHIVE_THREADS 0)
INCLUDE(CPack)

#无需改动
IF (MSVC)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/out)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/out)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/out)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/out)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/out)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/out)
ENDIF()

IF ("${UNIT_TEST}" STREQUAL "true")
  IF (UNIX OR APPLE)
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)
  ENDIF()
  ADD_EXECUTABLE(${PROJECT_NAME} ${PROJECT_SRC})
  INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin COMPONENT dev)
ELSE()
  IF (UNIX OR APPLE)
    SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)
  ENDIF()
  
  ADD_LIBRARY(${PROJECT_NAME}_static STATIC ${PROJECT_SRC})
  IF(MSVC)
    SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME}_static)
  ELSE()
    SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
  ENDIF()
  SET_TARGET_PROPERTIES(${PROJECT_NAME}_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  
  ADD_LIBRARY(${PROJECT_NAME} SHARED ${PROJECT_SRC})
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
  SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT dev)
  INSTALL(TARGETS ${PROJECT_NAME}_static DESTINATION lib COMPONENT dev)
  INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/export/ DESTINATION include COMPONENT dev)
ENDIF()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES VERSION ${COM_MAJOR_VERSION}.${COM_MINOR_VERSION}.${COM_PATCH_VERSION})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_LIB})
