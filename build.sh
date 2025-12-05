#!/bin/bash

show_usage()
{
	echo "Usage:  -d build with debug option"
	echo "        -u build as unit test app"
	echo "        -s build static library"
	echo "        -c remove all build file"
	echo "        -W build for win32"
	echo ""
	exit -1
}

show_message()
{
    echo "[-------------"$1"-----------------]"
}

BUILD_TYPE="Release"
UNIT_TEST="false"
PROJECT_CLEAN= "false"
OS_TYPE=`uname`
BUILD_ARCH="x64"
if [ x"${OS_TYPE}" == x"Darwin" ];then
    DIR_ROOT=`pwd`
else
    DIR_ROOT=`realpath $(dirname "$0")`
fi

if [ -n "$1" ];then
    if [ x"${1:0:1}" != "x-" ];then
      show_usage
    fi
fi

while getopts 'cduj:W' OPT; do
	case $OPT in
	c)
		PROJECT_CLEAN="true";;
	d)
		BUILD_TYPE="Debug";;
	u)
		UNIT_TEST="true";;
    j)
		JOBS=$OPTARG;;
    W)
		BUILD_ARCH="Win32";;
	?)
		show_usage
	esac
done
shift $(($OPTIND - 1))

if [ x"${OS_TYPE}" == x"Darwin" ];then
    export KERNEL_BITS=64
elif [ x"${OS_TYPE}" == x"Linux" ];then
    OS_TYPE="Linux"
else
    OS_TYPE="Windows"
fi

#清理环境
if [ x"${PROJECT_CLEAN}" == x"true" ];then
	rm -rf ${DIR_ROOT}/tmp/ 2>&1 > /dev/null
	show_message "clean done"
	echo ""
	exit 0
fi

#环境准备
mkdir -pv ${DIR_ROOT}/tmp/ > /dev/null 2>&1

#编译
if [ x"${JOBS}" == x"" ];then
    JOBS=`grep -c ^processor /proc/cpuinfo`
fi

if [ -z ${JOBS} ];then
    JOBS=1
fi

cd ${DIR_ROOT}/tmp
if [ -d ${DIR_ROOT} ];then
  if [ x"$OS_TYPE" == x"Windows" ];then
      cmake -A ${BUILD_ARCH} -DBUILD_ARCH=${BUILD_ARCH} -DBUILD_TYPE=${BUILD_TYPE} -DUNIT_TEST=${UNIT_TEST} -DOS_TYPE=${OS_TYPE} ${DIR_ROOT} -DCMAKE_WIN32_WINNT=0x0600 && cmake --build . --target install package --config ${BUILD_TYPE}
  else
      cmake -DBUILD_ARCH=${BUILD_ARCH} -DBUILD_TYPE=${BUILD_TYPE} -DUNIT_TEST=${UNIT_TEST} -DOS_TYPE=${OS_TYPE} ${DIR_ROOT} && make -j${JOBS} && make install && make package
  fi
  if [ $? != 0 ]; then
    show_message "failed to make project"
    exit -1
  fi
fi

show_message "succeed"
