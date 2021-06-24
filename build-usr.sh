# The makefile calls this,  this calls the actual build script.
# Look, I know this is ugly, but I couldn't make it work ay other way
# I'm planning to change the entire build system, so this is not
# a permanent solution anyway, ok?

cd src/usr/
. ./build.sh
cd ../../
