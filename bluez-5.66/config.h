/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Directory for the Android daemon storage files */
#define ANDROID_STORAGEDIR "/var/lib/bluetooth/android"

/* Directory for the configuration files */
#define CONFIGDIR "/etc/bluetooth"

/* Define to 1 if you have the backtrace support. */
/* #undef HAVE_BACKTRACE_SUPPORT */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the `explicit_bzero' function. */
#define HAVE_EXPLICIT_BZERO 1

/* Define to 1 if you have the `getrandom' function. */
#define HAVE_GETRANDOM 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `asan' library (-lasan). */
/* #undef HAVE_LIBASAN */

/* Define to 1 if you have the `lsan' library (-llsan). */
/* #undef HAVE_LIBLSAN */

/* Define to 1 if you have the `ubsan' library (-lubsan). */
/* #undef HAVE_LIBUBSAN */

/* Define to 1 if you have the <linux/if_alg.h> header file. */
#define HAVE_LINUX_IF_ALG_H 1

/* Define to 1 if you have the <linux/types.h> header file. */
#define HAVE_LINUX_TYPES_H 1

/* Define to 1 if you have the <linux/uhid.h> header file. */
#define HAVE_LINUX_UHID_H 1

/* Define to 1 if you have the <linux/uinput.h> header file. */
#define HAVE_LINUX_UINPUT_H 1

/* Define to 1 if you have the `rawmemchr' function. */
#define HAVE_RAWMEMCHR 1

/* Define to 1 if you have the <readline/readline.h> header file. */
#define HAVE_READLINE_READLINE_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/random.h> header file. */
#define HAVE_SYS_RANDOM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the udev_hwdb_new() function. */
/* #undef HAVE_UDEV_HWDB_NEW */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <valgrind/memcheck.h> header file. */
/* #undef HAVE_VALGRIND_MEMCHECK_H */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Directory for the mesh daemon storage files */
#define MESH_STORAGEDIR "/var/lib/bluetooth/mesh"

/* Define if threading support is required */
/* #undef NEED_THREADS */

/* Name of package */
#define PACKAGE "bluez"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "bluez"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "bluez 5.66"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "bluez"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "5.66"

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Directory for the storage files */
#define STORAGEDIR "/var/lib/bluetooth"

/* Version number of package */
#define VERSION "5.66"

/* Define to the equivalent of the C99 'restrict' keyword, or to
   nothing if this is not supported.  Do not define if restrict is
   supported only directly.  */
#define restrict __restrict__
/* Work around a bug in older versions of Sun C++, which did not
   #define __restrict__ or support _Restrict or __restrict__
   even though the corresponding Sun C compiler ended up with
   "#define restrict _Restrict" or "#define restrict __restrict__"
   in the previous line.  This workaround can be removed once
   we assume Oracle Developer Studio 12.5 (2016) or later.  */
#if defined __SUNPRO_CC && !defined __RESTRICT && !defined __restrict__
# define _Restrict
# define __restrict__
#endif
