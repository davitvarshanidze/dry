#!/bin/sh

packages="";

if [ -x "$(command -v apt-get)" ]
then
    dpkg --verify libx11-dev        || packages=$packages"libx11-dev "
    dpkg --verify libxrandr-dev     || packages=$packages"libxrandr-dev "
    dpkg --verify libasound2-dev    || packages=$packages"libasound2-dev "
    dpkg --verify libegl1-mesa-dev  || packages=$packages"libegl1-mesa-dev "
    dpkg --verify libwayland-dev    || packages=$packages"libwayland-dev "
    dpkg --verify wayland-protocols || packages=$packages"wayland-protocols "
    dpkg --verify git               || packages=$packages"git "
    dpkg --verify make              || packages=$packages"make "
    dpkg --verify cmake             || packages=$packages"cmake "
    dpkg --verify qtbase5-dev       || packages=$packages"qtbase5-dev "
    dpkg --verify qt5-qmake         || packages=$packages"qt5-qmake "
    dpkg --verify build-essential   || packages=$packages"build-essential "
    if [ "$packages" ]
    then sudo apt-get install $packages
    fi
elif [ -x "$(command -v dnf)" ]
then
    rpm -q libX11-devel             || packages=$packages"libX11-devel "
    rpm -q libXrandr-devel          || packages=$packages"libXrandr-devel "
    rpm -q alsa-lib-devel           || packages=$packages"alsa-lib-devel "
    rpm -q mesa-libEGL-devel        || packages=$packages"mesa-libEGL-devel "
    rpm -q wayland-devel            || packages=$packages"wayland-devel "
    rpm -q wayland-protocols-devel  || packages=$packages"wayland-protocols-devel "
    rpm -q git                      || packages=$packages"git "
    rpm -q make                     || packages=$packages"make "
    rpm -q cmake                    || packages=$packages"cmake "
    rpm -q qt5-qtbase-devel         || packages=$packages"qt5-qtbase-devel "
    rpm -q gcc                      || packages=$packages"gcc "
    rpm -q gcc-c++                  || packages=$packages"gcc-c++ "
    if [ "$packages" ]
    then sudo dnf install $packages
    fi
fi
