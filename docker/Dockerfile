FROM ubuntu:16.04
MAINTAINER Wojciech Kapsa kapsa@man.poznan.pl

#A docker with ubuntu:16.04 for Android GUB

RUN apt-get update && apt-get -y install git build-essential pkg-config wget unzip default-jre
#GUB Ubuntu dependencies
#RUN apt-get -y install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev libgl1-mesa-dev libgles2-mesa-dev

#GUB test app dependencies
#RUN apt-get -y install libsdl2-dev libglu1-mesa-dev freeglut3-dev

#get gstreamer for android
RUN wget  https://gstreamer.freedesktop.org/data/pkg/android/1.8.2/gstreamer-1.0-android-arm-1.8.2.tar.bz2 && \
mkdir -p /usr/local/gstreamer/arm && \
tar -xjf gstreamer-1.0-android-arm-1.8.2.tar.bz2 -C /usr/local/gstreamer/arm &&\
rm gstreamer-1.0-android-arm-1.8.2.tar.bz2


#Install android ndk
RUN wget  https://dl.google.com/android/repository/android-ndk-r12b-linux-x86_64.zip && \
unzip -q android-ndk-r12b-linux-x86_64.zip -d /usr/local/ &&\
rm android-ndk-r12b-linux-x86_64.zip

#Install android SDK
RUN wget  https://dl.google.com/android/android-sdk_r24.4.1-linux.tgz && \
tar -xzf android-sdk_r24.4.1-linux.tgz && \
cp -r android-sdk-linux /usr/local/ && \
rm android-sdk_r24.4.1-linux.tgz


ENV ANDROID_SDK_HOME /usr/local/android-sdk-linux
ENV PATH $PATH:$ANDROID_SDK_HOME/tools
ENV ANDROID_NDK_HOME /usr/local/android-ndk-r12b
ENV PATH $PATH:$ANDROID_NDK_HOME
ENV GSTREAMER_ROOT_ANDROID /usr/local/gstreamer/arm

ENV ANT_HOME $ANDROID_SDK_HOME/tools
ENV PATH $PATH:$ANT_HOME

RUN ( sleep 4 && while [ 1 ]; do sleep 1; echo y; done ) | android update sdk --no-ui --force -a --filter 1,2,3,android-19 && \
cp -r android-sdk-linux/tools /usr/local/android-sdk-linux/ &&\
rm -rf android-sdk-linux


#build the GUB and cp artifacts
CMD cd /home/ && git clone https://github.com/kapsa/gst-unity-bridge.git --branch feature/dvb && cd /home/gst-unity-bridge/Plugin && android update project -p . -s --target android-19 && ndk-build -k || true && cp -r libs/armeabi /mnt/gub && cp -r libs/armeabi-v7a /mnt/gub


#RUN cd /home/gst-unity-bridge/Plugin && ant release && cd bin/classes && jar cvf gub.jar org/*








