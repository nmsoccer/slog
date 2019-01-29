#!/bin/bash
HEADER_DIR=/usr/local/include/slog
LIB_DIR=/usr/local/lib/

HEADER_NAME="slog.h"
SO_NAME=libslog.so
LINK_NAME="${SO_NAME}.0.1"
SRC_FILE="slog.c"
STATIC_LIB="libslog.a"
WORK_DIR=`pwd`

#echo "usage $0 [install] to compile and install"


#if [[ -n $1 && $1 == "install" ]]
#then
function install()
{
  echo "try to install..."
  gcc -fPIC -shared ${SRC_FILE} -o ${SO_NAME}
  
  #static lib
  gcc -g -c ${SRC_FILE}
  ar rcvs ${STATIC_LIB} *.o
  if [[ $? -ne 0 ]]
  then
    echo "create ${STATIC_LIB} failed!"
    exit 1
  fi


  mkdir -p ${HEADER_DIR}
  if [[ $? -ne 0 ]]
  then
    #echo "create ${HEADER_DIR} failed!"
    exit 1 
  fi

  cp ${HEADER_NAME} ${HEADER_DIR}
  if [[ $? -ne 0 ]]
  then
    exit 1
  fi

  cp ${SO_NAME} ${LIB_DIR}
  if [[ $? -ne 0 ]]
  then
    exit 1
  fi

  cd ${LIB_DIR}
  rm ${LINK_NAME}
  ln -s ${SO_NAME} ${LINK_NAME}
  if [[ $? -ne 0 ]]
  then
    exit 1
  fi

  cd ${WORK_DIR}
  rm ${SO_NAME}

  cp ${STATIC_LIB} ${LIB_DIR}
  rm *.o
  rm ${STATIC_LIB}


  echo "install success!"
}

install 
