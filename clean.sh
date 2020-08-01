
#This shell script serves the purpose of cleaning up all the object files created
#inside the source/ folder, since there are quite a lot, and they take
#up quite some space.

. ./config.sh

for DIR in ${DIRS}; do
	cd "${SYSROOT}/${DIR}"
	./clean.sh
done;

cd "${SYSROOT}/.."

rm -rf image/
rm disk.iso

