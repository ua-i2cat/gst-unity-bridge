FROM ubuntu:16.04
MAINTAINER Wojciech Kapsa kapsa@man.poznan.pl

#A docker with ubuntu:16.04 for Android GUB

RUN apt-get update && apt-get -y install git build-essential pkg-config wget unzip default-jre openjdk-8-jdk

#=====================optional linux build====================
#GUB Ubuntu dependencies
#RUN apt-get -y install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev libgl1-mesa-dev libgles2-mesa-dev
#GUB test app dependencies
#RUN apt-get -y install libsdl2-dev libglu1-mesa-dev freeglut3-dev
#=============================================================

#Install android ndk
RUN wget  https://dl.google.com/android/repository/android-ndk-r12b-linux-x86_64.zip && \
unzip -q android-ndk-r12b-linux-x86_64.zip -d /usr/local/ &&\
rm android-ndk-r12b-linux-x86_64.zip
ENV ANDROID_NDK_HOME /usr/local/android-ndk-r12b
ENV PATH $PATH:$ANDROID_NDK_HOME

ENV ANT_VERSION 1.9.4 
RUN wget http://archive.apache.org/dist/ant/binaries/apache-ant-${ANT_VERSION}-bin.tar.gz && \
tar xzf apache-ant-${ANT_VERSION}-bin.tar.gz && \
mv apache-ant-${ANT_VERSION} /opt/ant && \
rm apache-ant-${ANT_VERSION}-bin.tar.gz 
ENV ANT_HOME /opt/ant 
ENV PATH ${PATH}:/opt/ant/bin

ENV NDK_TARGET_LEVEL android-23
#Install android SDK
RUN wget  https://dl.google.com/android/android-sdk_r24.4.1-linux.tgz && \
tar -xzf android-sdk_r24.4.1-linux.tgz -C /usr/local && \
rm  android-sdk_r24.4.1-linux.tgz && \
cd /usr/local/android-sdk-linux/tools && ( sleep 4 && while [ 1 ]; do sleep 1; echo y; done ) | ./android update sdk --no-ui -a --filter 1,2,3,4,${NDK_TARGET_LEVEL}
ENV ANDROID_SDK_HOME /usr/local/android-sdk-linux
ENV PATH $PATH:$ANDROID_SDK_HOME/tools

ENV GST_ANDROID_ARCH armv7
ENV GST_ANDROID_VERISON 1.8.2
#get gstreamer for android
RUN wget  https://gstreamer.freedesktop.org/data/pkg/android/${GST_ANDROID_VERISON}/gstreamer-1.0-android-${GST_ANDROID_ARCH}-${GST_ANDROID_VERISON}.tar.bz2 && \
mkdir -p /usr/local/gstreamer/${GST_ANDROID_ARCH} && \
tar -xjf gstreamer-1.0-android-${GST_ANDROID_ARCH}-${GST_ANDROID_VERISON}.tar.bz2 -C /usr/local/gstreamer/${GST_ANDROID_ARCH} &&\
rm gstreamer-1.0-android-${GST_ANDROID_ARCH}-${GST_ANDROID_VERISON}.tar.bz2
ENV GSTREAMER_ROOT_ANDROID /usr/local/gstreamer/${GST_ANDROID_ARCH}

RUN sed -i -e 's/#define GST_GL_HAVE_GLSYNC 0/#define GST_GL_HAVE_GLSYNC 1/g' $GSTREAMER_ROOT_ANDROID/lib/gstreamer-1.0/include/gst/gl/gstglconfig.h && \
sed -i -e 's/#define GST_GL_HAVE_GLUINT64 0/#define GST_GL_HAVE_GLUINT64 1/g' $GSTREAMER_ROOT_ANDROID/lib/gstreamer-1.0/include/gst/gl/gstglconfig.h && \
sed -i -e 's/#define GST_GL_HAVE_GLINT64 0/#define GST_GL_HAVE_GLINT64 1/g' $GSTREAMER_ROOT_ANDROID/lib/gstreamer-1.0/include/gst/gl/gstglconfig.h

ENV NDK_APP_ABI armeabi-v7a
ENV ARTIFACTS_PATH /mnt/gub
#build the GUB and cp artifacts
CMD cd /home/ && git clone https://github.com/kapsa/gst-unity-bridge.git --branch feature/dvb && cd /home/gst-unity-bridge/Plugin && android update project -p . -s --target ${NDK_TARGET_LEVEL} && ndk-build APP_ABI=${NDK_APP_ABI} && cp -r libs/${NDK_APP_ABI} ${ARTIFACTS_PATH} && ant release && cd bin/classes && jar cvf gub.jar org/* && cp gub.jar ${ARTIFACTS_PATH}


