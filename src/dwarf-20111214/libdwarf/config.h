/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if <alloca.h> works. */
#define HAVE_ALLOCA_H 1

/* Define 1 if want to allow producer to build with 32/64bit section offsets
   per dwarf3 */
#define HAVE_DWARF2_99_EXTENSION 1

/* Define to 1 if the elf64_getehdr function is in libelf.a. */
/* #undef HAVE_ELF64_GETEHDR */

/* Define to 1 if the elf64_getshdr function is in libelf.a. */
/* #undef HAVE_ELF64_GETSHDR */

/* Define 1 if Elf64_Rela defined. */
/* #undef HAVE_ELF64_RELA */

/* Define 1 if Elf64_Sym defined. */
/* #undef HAVE_ELF64_SYM */

/* Define to 1 if you have the <elfaccess.h> header file. */
/* #undef HAVE_ELFACCESS_H */

/* Define to 1 if you have the <elf.h> header file. */
/* #undef HAVE_ELF_H */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <libelf.h> header file. */
/* #undef HAVE_LIBELF_H */

/* Define to 1 if you have the <libelf/libelf.h> header file. */
/* #undef HAVE_LIBELF_LIBELF_H */

/* Define 1 if off64 is defined via libelf with GNU_SOURCE. */
/* #undef HAVE_LIBELF_OFF64_OK */

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the <malloc/malloc.h> header file. */
#define HAVE_MALLOC_MALLOC_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define 1 if need nonstandard printf format for 64bit */
/* #undef HAVE_NONSTANDARD_PRINTF_64_FORMAT */

/* Define 1 to default to old DW_FRAME_CFA_COL */
/* #undef HAVE_OLD_FRAME_CFA_COL */

/* Define 1 if plain libelf builds. */
/* #undef HAVE_RAW_LIBELF_OK */

/* Define 1 if R_IA_64_DIR32LSB is defined (might be enum value). */
/* #undef HAVE_R_IA_64_DIR32LSB */

/* Define 1 if want producer to build with IRIX offset sizes */
/* #undef HAVE_SGI_IRIX_OFFSETS */

/* Define 1 if we have the Windows specific header stdafx.h */
/* #undef HAVE_STDAFX_H */

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define 1 if want producer to build with only 32bit section offsets */
/* #undef HAVE_STRICT_DWARF2_32BIT_OFFSET */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if you have the <sys/ia64/elf.h> header file. */
/* #undef HAVE_SYS_IA64_ELF_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define 1 if want to allow Windows full path detection */
/* #undef HAVE_WINDOWS_PATH */

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* See if __uint32_t is predefined in the compiler. */
/* #undef HAVE___UINT32_T */

/* Define 1 if __uint32_t is in sgidefs.h. */
/* #undef HAVE___UINT32_T_IN_SGIDEFS_H */

/* Define 1 if sys/types.h defines __uint32_t. */
#define HAVE___UINT32_T_IN_SYS_TYPES_H 1

/* See if __uint64_t is predefined in the compiler. */
/* #undef HAVE___UINT64_T */

/* Define 1 if is in sgidefs.h. */
/* #undef HAVE___UINT64_T_IN_SGIDEFS_H */

/* Define 1 if sys/types.h defines __uint64_t. */
#define HAVE___UINT64_T_IN_SYS_TYPES_H 1

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "davea42@earthlink.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libdwarf"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libdwarf 2011.12.14"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libdwarf"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2011.12.14"

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
