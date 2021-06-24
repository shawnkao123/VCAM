#!/bin/bash

make

modprobe videobuf2_vmalloc videobuf2_v4l2
modprobe vcam
rmmod vcam
insmod vcam/vcam.ko
lsmod | grep vcam
sudo ./vcam/vcam-util -l
ls -l /dev/video0
ls -l /dev/vcamctl


sudo cp x-compressor/x test/
sudo cp x-compressor/unx test/

cd test
sudo ./test

cd ..

