# Android GUB Dockerfile

Dockerfile for building GUB under Android

### Build

To create new Docker image:

```
docker build ./docker -t gub/android
```


To compile GUB for Android:

```
export ROOT_GUB_DIR=$(realpath ./../../)
docker run -v ${ROOT_GUB_DIR}/Build/Android:/mnt/gub -it gub/android
```


### Options

To specify git branch to build GUB use ``--gubBranch branch-name`` (without this option is develop)

```
docker run -v ${ROOT_GUB_DIR}/Build/Android:/mnt/gub -it gub/android --gubBranch branch-name
```


To build GUB from local repository use ``--gubLocal``:

```
export ROOT_IMMERSIATV_PLAYER_DIR=$(realpath ./../../../../../)
docker run -v ${ROOT_GUB_DIR}/Build/Android:/mnt/gub -v ${ROOT_IMMERSIATV_PLAYER_DIR}:/mnt/immersiatv-player -it gub/android --gubLocal
```


To build GUB with own prebuild libgstreamer_android.so file use ``--gubGstAnd``:

```
export GST_AND_LIB_PATH=$(realpath ./../../../../../Projects/ImmersiaTV_Client/Assets/Plugins/GStreamer/android_arm/libgstreamer_android.so)
docker run -v ${ROOT_GUB_DIR}/Build/Android:/mnt/gub -v ${GST_AND_LIB_PATH}:/mnt/libgstreamer_android.so -it gub/android --gubGstAnd
```

To see help use ``--gubHelp``

```
docker run -it gub/android --gubHelp
```


### Test

All above options can be also used to build testing application.  
To build application for testing GUB use ``--gubTest``:

```
docker run -v ${ROOT_GUB_DIR}/Build/Android:/mnt/gub -it gub/android --gubTest
adb uninstall com.testgub
adb install -r ${ROOT_GUB_DIR}/Build/Android/test/TestGUB-debug.apk
```


### Clear

To remove all images and containers:

```
docker rm $(docker ps -a -q)
docker rmi $(docker images -q)
```


### Prerequisities

You need to install [Docker](https://www.docker.com/)


## Author

* **Wojciech Kapsa** - [email](kapsa@man.poznan.pl)
* **Daniel Piesik**  - [email](dpiesik@man.poznan.pl)
