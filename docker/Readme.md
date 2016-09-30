# Android GUB Dockerfile

Dockerfile for building GUB under Android

### Build

To create new Docker image:

```
docker build . -t gub/android
```
Once created image can be reused for a multiple building proces from the next step.


To compile GUB for Android:

```
docker run -v ${PWD}/build:/mnt/gub -it gub/android:latest
```
Where PWD is the location for build artifacts. On Windows it should point to the Virtual Box shared folder.


To remove all images and containers:

```
docker rm `docker ps --no-trunc -aq`
docker rmi $(docker images -q)
```

### Prerequisities

You need to install [Docker](https://www.docker.com/)

### ToDo

Dockerfile ENV (NDK_TARGET_LEVEL, GST_ANDROID_ARCH, GST_ANDROID_VERISON, NDK_APP_ABI) vars should be easier to change and tune.

## Author

* **Wojciech Kapsa** - [email](kapsa@man.poznan.pl)


