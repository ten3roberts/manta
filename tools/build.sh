#!/bin/sh
cd sandbox
premake5 gmake2 && make || >&2 echo "Failed to build sandbox"

