for LIB in $(find * -type d) 
do	
	. "${LIB}/build.sh";
done
