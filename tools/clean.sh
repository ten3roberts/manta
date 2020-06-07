#!/bin/sh
#This will clean the engine and included tools binary and intermediate files

cd $(dirname $0)
cd ..
DIR=`pwd`

echo "--------Cleaning manta--------"

# manta
rm -rv bin
rm -rv obj

# GLFW
rm -rv vendor/glfw/bin
rm -rv vendor/glfw/bin-int

# compiled shaders
rm -rv assets/shaders/*.spv