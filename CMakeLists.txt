#CMAKE最小版本要求,可修改
#BUILD_TYPE=release,debug  UNIT_TEST=true,false LIB_TYPE=static,shared
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT("com")

ADD_DEFINITIONS(-DMODULE_IS_LIB)

IF ("${UNIT_TEST}" STREQUAL "true")
	ADD_DEFINITIONS(-DUNIT_TEST)
ENDIF()

IF ("${BUILD_TYPE}" STREQUAL "debug")
ADD_DEFINITIONS(-D__DEBUG__)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
ELSE()
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s -O2")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s -O2")
ENDIF()

#-Woverloaded-virtual
#C/C++文件编译选项,无需修改
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSQLITE_THREADSAFE=1 -DUNQLITE_ENABLE_THREADS -DHAVE_STRINGS_H -DHAVE_SIGNAL_H -DHAVE_INTTYPES_H -DHAVE_MALLOC_H -fPIC -rdynamic -Wl,--disable-new-dtags -Wall -Werror -Wno-stringop-truncation -Wno-sign-compare -Wno-error=deprecated-declarations -Wdeprecated -Wunreachable-code -std=gnu99 -Wstrict-prototypes -fwrapv -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-parameter")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSQLITE_THREADSAFE=1 -DUNQLITE_ENABLE_THREADS -DHAVE_STRINGS_H -DHAVE_SIGNAL_H -DHAVE_INTTYPES_H -DHAVE_MALLOC_H -fPIC -rdynamic -Wl,--disable-new-dtags -Wall -Werror -Wno-stringop-truncation -Wno-sign-compare -Wno-error=deprecated-declarations -Wdeprecated -Wunreachable-code -std=c++11 -fwrapv -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-parameter")

SET(CMAKE_SKIP_BUILD_RPATH FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/lib:./:./out/:./tmp/depend:../../tmp/depend")

#头文件搜索路径,可修改
INCLUDE_DIRECTORIES(
	${PROJECT_SOURCE_DIR}/tmp/depend
	${PROJECT_SOURCE_DIR}/src/export
	${PROJECT_SOURCE_DIR}/src
)

#库文件搜索路径,可修改
LINK_DIRECTORIES(
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/tmp/depend
)

#依赖库,无需修改
SET(PROJECT_LIB
	${PROJECT_LIB}
	m
    pthread
    dl
)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

#src文件夹下的所有源文件,无需修改
#非递归方式，不允许存在子文件夹
AUX_SOURCE_DIRECTORY(src PROJECT_SRC)
IF ("${UNIT_TEST}" STREQUAL "true")
AUX_SOURCE_DIRECTORY(src/test PROJECT_SRC)
ENDIF()
#递归方式(允许存在子文件夹)
#FILE(GLOB_RECURSE PROJECT_SRC ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.c)

#无需改动
IF ("${UNIT_TEST}" STREQUAL "true")
	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)
	ADD_EXECUTABLE(${PROJECT_NAME} ${PUBLIC_SRC} ${PROJECT_SRC})
ELSE()
	SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/out)
	IF ("${LIB_TYPE}" STREQUAL "static")
	    ADD_DEFINITIONS(-DLIB_TYPE_STATIC)
	    ADD_LIBRARY(${PROJECT_NAME} STATIC ${PUBLIC_SRC} ${PROJECT_SRC})
	ELSE()
	    ADD_DEFINITIONS(-DLIB_TYPE_SHARED)
	    ADD_LIBRARY(${PROJECT_NAME} SHARED ${PUBLIC_SRC} ${PROJECT_SRC})
	ENDIF()
    FILE(COPY ${PROJECT_SOURCE_DIR}/src/export/ DESTINATION ${PROJECT_SOURCE_DIR}/out/include)
ENDIF()
STRING(STRIP "${PROJECT_LIB}" PROJECT_LIB)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_LIB})