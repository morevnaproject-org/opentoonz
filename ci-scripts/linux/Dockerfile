FROM ubuntu:trusty
# This will prevent some errors on the console when installing packages
ARG DEBIAN_FRONTEND=noninteractive

COPY ./otx-install.sh /otx-install.sh
RUN chmod +x /otx-install.sh

RUN apt update

RUN apt install -y software-properties-common
RUN apt install -y wget unzip
RUN add-apt-repository --yes ppa:beineri/opt-qt597-trusty
RUN add-apt-repository --yes ppa:achadwick/mypaint-testing
RUN apt update
RUN apt-get install -y cmake3 liblzo2-dev liblz4-dev libfreetype6-dev libpng-dev libegl1-mesa-dev libgles2-mesa-dev libglew-dev freeglut3-dev qt59script libsuperlu3-dev qt59svg qt59tools qt59multimedia wget libusb-1.0-0-dev libboost-all-dev liblzma-dev libjson-c-dev libmypaint-dev libjpeg-turbo8-dev libglib2.0-dev qt59serialport
RUN apt-get install -y nasm yasm libgnutls-dev libass-dev libbluray-dev libmp3lame-dev libopus-dev libsnappy-dev libtheora-dev libvorbis-dev libvpx-dev libwebp-dev libxml2-dev libfontconfig1-dev libfreetype6-dev libopencore-amrnb-dev libopencore-amrwb-dev libopenjpeg-dev libspeex-dev libsoxr-dev
RUN apt-get install -y python3-pip

RUN apt install -y libfuse2

WORKDIR /root/opentoonz

CMD  ["/bin/bash"]
