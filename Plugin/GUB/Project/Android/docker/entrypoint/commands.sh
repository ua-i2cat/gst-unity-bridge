#!/bin/bash

function downloadRepo
{
  BRANCH_NAME=$1
  cd /home/
  git clone https://stash.i2cat.net/scm/imtv/immersiatv-player.git --branch $BRANCH_NAME
}

function copyLocalRepo
{
  MOUNT_DIR="/mnt/immersiatv-player"
  DST_DIR="/home/immersiatv-player"

  declare -a arr=(Modules/Core/GUB/Project/Android
                  Modules/Core/GUB/Source
                  Modules/Utils/DvbCssWc/Source
                 )

  cd $MOUNT_DIR
  find -type d -links 2 -exec mkdir -p "$DST_DIR/{}" \;
  for i in "${arr[@]}"
  do
     cp -r $MOUNT_DIR/$i $DST_DIR/$(dirname $i)
  done
}

function buildTest
{
  cd /home/immersiatv-player/Modules/Core/GUB/Project/Android/noUnityTest
  buildAPK debug $1
  mkdir -p ${ARTIFACTS_PATH}/test
  cp ./bin/TestGUB-debug.apk ${ARTIFACTS_PATH}/test
}

function buildGUB
{
  cd /home/immersiatv-player/Modules/Core/GUB/Project/Android/GUB
  buildAPK release $1
  cd bin/classes
  jar cvf gub.jar org/*
  cp gub.jar ${ARTIFACTS_PATH}
  cd ../../
  cp -r libs/${NDK_APP_ABI} ${ARTIFACTS_PATH}
}

function buildAPK
{
  BUILD_TARGET=$1
  COPY_GST_AND_LIB=$2

  rm -r ${ARTIFACTS_PATH}/*
  android update project -p . -s --target ${NDK_TARGET_LEVEL}
  ndk-build APP_ABI=${NDK_APP_ABI}
  if [ "$COPY_GST_AND_LIB" = true ]; then
    cp /mnt/libgstreamer_android.so libs/${NDK_APP_ABI}/
  fi
  ant $BUILD_TARGET
}
