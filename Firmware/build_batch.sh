#!/bin/sh
if [ ! -e hex ]; then
	mkdir hex
fi

make LIGHTPACK_HW=4
mv Lightpack_hw4.hex hex/
make clean LIGHTPACK_HW=4
make LIGHTPACK_HW=5
mv Lightpack_hw5.hex hex/
make clean LIGHTPACK_HW=5
make LIGHTPACK_HW=6
mv Lightpack_hw6.hex hex/
make clean LIGHTPACK_HW=6
make LIGHTPACK_HW=7
mv Lightpack_hw7.hex hex/
make clean LIGHTPACK_HW=7
