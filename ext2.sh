# This script simply puts everything inside the root/ directory into the image.
# We need a shell script because this is something I simply don't know how to
# do in Make syntax.

cd root
for DIR in $(find * -type d); do
	e2mkdir ../temp.img:${DIR}
done;
for F in $(find * -type f); do
	e2cp "${F}" ../temp.img:/${F}
done;

cd ..
