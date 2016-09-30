# Android GUB Dockerfile

Dockerfile for building GUB under Android

## How to

To create new Docker image:

```
docker build . -t gub/android
```

To compile GUB for Android:

```
docker run -v ${PWD}/build:/mnt/gub -it gub/android:latest
```
where PWD is the location for build artifacts. On Windows it should point to Virtual Box shared folder.

To remove all images and containers:

```
docker rm `docker ps --no-trunc -aq`
docker rmi $(docker images -q)
```

### Prerequisities

You need to install Docker [Docker](https://www.docker.com/)

## Author

* **Wojciech Kapsa** - *Initial work* - [email](kapsa@man.poznan.pl)


