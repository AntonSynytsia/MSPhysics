// Copied and extended from Thomthom's TT_lib2.

#ifndef RUBY_UTIL_H
#define RUBY_UTIL_H

// Visual Studio CRT Config

// Must define these macros for VS2013 runtime.
#define HAVE_ACOSH 1
#define HAVE_CBRT 1
#define HAVE_ERF 1
#define HAVE_ROUND 1
#define HAVE_TGAMMA 1

// Ruby Headers
#ifdef USE_WINSOCK
	#include <Winsock2.h>
#endif

#include <ruby.h>
#ifdef HAVE_RUBY_ENCODING_H
	#include <ruby/encoding.h>
#endif


// Compatibility Macros

/* The structures changes between Ruby 1.8 and 2.0 so the access to the
 * properties are different. There are new macros in Ruby 2.0 that should be
 * used instead. In order to make the code compile for both we need to add
 * matching macros for 1.8.
 */

#ifndef RARRAY_LEN
	#define RARRAY_LEN(x) (RARRAY(x)->len)
#endif

#ifndef DBL2NUM
	#define DBL2NUM(dbl)  rb_float_new(dbl)
#endif

#ifndef NUM2SIZET
	#if defined(HAVE_LONG_LONG) && SIZEOF_SIZE_T > SIZEOF_LONG
		#define NUM2SIZET(x) ((size_t)NUM2ULL(x))
	#else
		#define NUM2SIZET(x) NUM2ULONG(x)
	#endif
#endif


/*
 * Need to be very careful about how these macros are defined, especially
 * when compiling C++ code or C code with an ANSI C compiler.
 *
 * VALUEFUNC(f) is a macro used to typecast a C function that implements
 * a Ruby method so that it can be passed as an argument to API functions
 * like rb_define_method() and rb_define_singleton_method().
 *
 * VOIDFUNC(f) is a macro used to typecast a C function that implements
 * either the "mark" or "free" stuff for a Ruby Data object, so that it
 * can be passed as an argument to API functions like Data_Wrap_Struct()
 * and Data_Make_Struct().
 */

#define VALUEFUNC(f) ((VALUE (*)(ANYARGS)) f)
#define VOIDFUNC(f)	 ((RUBY_DATA_FUNC) f)


// Ruby 1.8 headers conflict with ostream because it defined a lot of macros
// that completely mess up the environment.

// win32.h
#ifdef getc
	#undef getc
#endif

#ifdef putc
	#undef putc
#endif

#ifdef fgetc
	#undef fgetc
#endif

#ifdef fputc
	#undef fputc
#endif

#ifdef getchar
	#undef getchar
#endif

#ifdef putchar
	#undef putchar
#endif

#ifdef fgetchar
	#undef fgetchar
#endif

#ifdef fputchar
	#undef fputchar
#endif

#ifdef utime
	#undef utime
#endif


#ifdef close
	#undef close
#endif

#ifdef fclose
	#undef fclose
#endif

#ifdef read
	#undef read
#endif

#ifdef write
	#undef write
#endif

#ifdef getpid
	#undef getpid
#endif

#ifdef sleep
	#undef sleep
#endif

#ifdef connect
	#undef connect
#endif

#ifdef mode_t
	#undef mode_t
#endif

// config.h
#ifdef inline
	#undef inline
#endif

#endif /* RUBY_UTIL_H */
