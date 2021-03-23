#ifndef ERR_H
#define ERR_H 1

#include <stdint.h>


/* This file just defines some error codes.
 *
 * How deal with errors:
 *
 * Subroutines (functions that do something instead of returning anything
 * meaningful) should return uint8_t, and return the below error codes as-is.
 *
 * Functions that return a positive integer/number/etc. should return -(ERR)
 * on error.
 *
 * Functions that must return a uint64_t or similar, should return a particular
 * out-of-range number of their choice. Make sure to limit that the function
 * can only fail in one particular way.
 *
 * Functions that return pointerss should simply return NULL.
 */

/* The function succeeded. */
#define GENERIC_SUCCESS         0

/* One of the parameters was _invalid_ */
#define ERR_INVALID_PARAM       1

/* One of the parameters was incompatible - valid, but not usable by that
 * particular function. An FS driver should return this if the file system
 * given to it is valid, but not drivable by that particular driver.
 */
#define ERR_INCOMPAT_PARAM      2

/* An object indicated by a parameter was not found. A file whith the path given
 * wasn't found, or a file whith that file descriptor doesn't exist etc.
 */
#define ERR_NOT_FOUND           3

/* The function was supposed to look for something, find them and do sth, but
 * it couldn't find them. E.g. the disk drivers couldn't find any drivable disks,
 * the fs driver couldn't find any drivable FS' etc.
 */
#define ERR_NO_RESULT           4
#define ERR_DISK                5
#define ERR_OUT_OF_BOUNDS       6
#define ERR_OUT_OF_MEM          7

int64_t print_stat(int64_t stat);


#endif /* ERR_H */
