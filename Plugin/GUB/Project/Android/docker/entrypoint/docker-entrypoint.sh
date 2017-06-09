#!/bin/bash
. $(dirname $0)/commands.sh

usage="
$(basename "$0") [-t] [-b branch-name] [-l] [-g] [-h] -- program to build GUB (GStreamer Unity Bridge)

where:
    -t | --gubTest    build android application for testing GUB
    -b | --gubBranch  download repository and set given branch
    -l | --gubLocal   copy local repository
    -g | --gubGstAnd  use builded libgstreamer_android.so
    -h | --gubHelp    show this help text
"

BUILD_TEST=false
GIT_BRANCH="develop"
COPY_LOCAL_REPO=false
COPY_GST_AND_LIB=false

show_help=false
while [[ $# -gt 0 ]]
do
  key="$1"
  case $key in

    -t|--gubTest)   BUILD_TEST=true
                    ;;

    -b|--gubBranch) GIT_BRANCH="$2"
                    set_branch=true
                    if [ "$COPY_LOCAL_REPO" = true ]; then
                      echo "Invalid arguments. Can not use -b and -l together."
                      show_help=true
                    fi
                    shift
                    ;;

    -l|--gubLocal)  COPY_LOCAL_REPO=true
                    if [ "$set_branch" = true ]; then
                    echo "Invalid arguments. Can not use -b and -l together."
                    show_help=true
                    fi
                    ;;

    -g|--gubGstAnd) COPY_GST_AND_LIB=true
                    ;;

    -h|--gubHelp)   show_help=true
                    ;;

    *)              echo "Unknown argument : " $key
                    show_help=true
                    ;;
  esac
  shift
done

if [ "$show_help" = true ]; then
  echo "$usage"
  exit
fi

if [ "$COPY_LOCAL_REPO" = true ]; then
  copyLocalRepo
else
  downloadRepo $GIT_BRANCH
fi

if [ "$BUILD_TEST" = true ]; then
  buildTest $COPY_GST_AND_LIB
else
  buildGUB $COPY_GST_AND_LIB
fi
