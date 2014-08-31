AC_PREREQ([2.65])
AC_INIT([bingram], [0.1])
AM_INIT_AUTOMAKE([1.11 no-define foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

# bingram sub dependencies
CPPFLAGS="-I$PWD/json/include/json-c/"
LDFLAGS="-L$PWD/json/lib"

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([stdlib.h unistd.h json.h])

# Checks for library functions.
AC_CHECK_LIB([json-c], [json_object_new_object], [],[
         echo "JSON-C library is required for this program"
         exit -1])
	 
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
