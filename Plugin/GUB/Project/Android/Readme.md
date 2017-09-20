# **Android GUB Dockerfile**

Dockerfile for building GUB under Android


## **Linux**

The instruction is prepare for Ubuntu distribution and it was tested on Ubuntu 16.04.

### Prerequisites

You need to install [Docker](https://www.docker.com/) and [Docker Compose](https://docs.docker.com/compose/). To do it, you can:

* follow the instruction [here](https://docs.docker.com/compose/install), **or**
* run script below (up to date on 19.12.2016).

```
./docker/install.sh
```


### Prebuild

Create new Docker image:

```
docker-compose -f docker/docker-compose.yml build
```


### Build

Compile GUB for Android:

```
./build.sh
```


### Settings

In ``docker/docker-compose.yml`` is configuration for building and running docker image.  
If you don't want to use default settings, you can modify this file.

----------

## **Windows 10** *(Docker)*

This part is only for **Windows 10 Professional or Enterprise 64-bit**. If you have other Windows system, or you use *Doocker Toolbox*, please go to [Other Windows](#other-windows-docker-toolbox) section.


### Prerequisites

You need to install [Docker](https://www.docker.com/) and [Docker Compose](https://docs.docker.com/compose/). To do it, you can:

* download and install [this](https://download.docker.com/win/stable/InstallDocker.msi)


### Prebuild

Create new Docker image:

```
docker-compose -f docker/docker-compose.yml build
```


### Build

Compile GUB for Android:

```
docker-compose -f docker/docker-compose.yml run gub-android
```


### Settings

In ``docker/docker-compose.yml`` is configuration for building and running docker image.  
If you don't want to use default settings, you can modify this file.

----------

## **Other Windows** *(Docker Toolbox)*


### Prerequisites

You need to install [Docker Toolbox](www.docker.com/products/docker-toolbox) and [Docker Compose](https://docs.docker.com/compose/). To do it, you can:

* download and install [this](https://download.docker.com/win/stable/DockerToolbox.exe)


### Prebuild

Configure *Docker Toolbox* and *VirtualBox*:

1. Run *Docker Toolbox* and wait to end of processing
2. Run *VirtualBox*
3. Stop the image named *default*. Close -> Power off
4. On image named *default* go to Settings -> Shared Folders
5. Add your gst-unity-bridge repository folder as shared folder:  
***Folder Path*** - path to your gst-unity-bridge repository  
***Folder Name*** = gst-unity-bridge  
***Auto-mount*** - checked
6. Reopen *Docker Toolbox* and navigate to this *Readme* location

Create new Docker image:

```
docker-compose -f docker/docker-compose-toolbox.yml build
```


### Build

Compile GUB for Android:

```
docker-compose -f docker/docker-compose-toolbox.yml run gub-android
```


### Settings

In ``docker/docker-compose-toolbox.yml`` is configuration for building and running docker image.  
If you don't want to use default settings, you can modify this file.

----------

## Clear

To remove all containers and images:

```
docker rm $(docker ps -a -q)
docker rmi $(docker images -q)
```