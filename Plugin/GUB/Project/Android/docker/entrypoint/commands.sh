#!/bin/bash

DOCKER_GST_UNITY_BRIDGE="/home/gst-unity-bridge"

function copyRepo
{
  declare -a arr=(Plugin/GUB/Project/Android
                  Plugin/GUB/Source
				  Plugin/DvbCssWc/Project/Android
                  Plugin/DvbCssWc/Source
				  Plugin/Externals/gstreamer/Project/Android
                 )

  cd ${MOUNT_GST_UNITY_BRIDGE}
  find -type d -exec mkdir -p "$DOCKER_GST_UNITY_BRIDGE/{}" \;
  for i in "${arr[@]}"
  do
     cp -r ${MOUNT_GST_UNITY_BRIDGE}/$i $DOCKER_GST_UNITY_BRIDGE/$(dirname $i)
  done
}

function buildGUB
{
  # it HAVE TO be subdirectory of ${MOUNT_OUTPUT_PATH}
  OUTPUT_PATH=${MOUNT_OUTPUT_PATH}/gub
  cd $DOCKER_GST_UNITY_BRIDGE/Plugin/GUB/Project/Android/GUB
  buildAPK release $OUTPUT_PATH
  cd bin/classes
  jar cvf gub.jar org/*
  cp gub.jar $OUTPUT_PATH
  cd ../../
  cp -r libs/${NDK_APP_ABI} $OUTPUT_PATH
}
