# Cleans and Builds the engine and tools

# Move to cuttle folder to be agnostic of where the script is run
cd $(dirname $0)
DIR=`pwd`

./clean.sh
./build.sh