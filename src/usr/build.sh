# The order is important! mostly, lib cannot be built after bin.
export DIRS="lib bin"

# Better build scripts would put these in a seperate configure script.
export ROOT="../../../root"

# each thing has its own build script.
for DIR in $DIRS
do	
	cd ${DIR}
	. "build.sh"
	cd ..
done
