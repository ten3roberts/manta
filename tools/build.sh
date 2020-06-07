#!/bin/sh

# This script will build the engine and the included tools

# Move to manta folder to be agnostic of where the script is run
cd $(dirname $0)
cd ..
DIR=`pwd`

# Check if premake is installed, if not, use provided binary in vendor/premake
command -v premake5 && echo "Using system `command -v premake5`" && premake="premake5" || echo "Using included premake5 binary" && premake="$DIR/vendor/premake/premake5"
# Move into sandbox and attempt build

cd sandbox
echo "--------Entering sandbox--------"
$premake gmake2 && make || >&2 echo "Failed to build sandbox"
cd $DIR
