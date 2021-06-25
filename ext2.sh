# This script simply puts everything inside the root/ directory into the image.
# We need a shell script because this is something I simply don't know how to
# do in Make syntax.

cd root
e2tools 2> /dev/null
if [ $? != "1" ]
then
	echo -e "You do not have e2tools installed. Please install e2tools.\n"
	echo -e "You can find the github repo here: https://github.com/e2tools/e2tools \n"
	echo "e2tools is a utitility this shell script (ext2.sh) uses to put everything in root/ into the image file."
	exit
fi

for DIR in $(find * -type d); do
	e2mkdir ../ext2.img:${DIR}
done;
for F in $(find * -type f); do
	e2cp "${F}" ../ext2.img:/${F}
done;

cd ..
