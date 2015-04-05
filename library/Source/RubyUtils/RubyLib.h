#ifndef SU_UTILS_RUBYLIB_H_
#define SU_UTILS_RUBYLIB_H_

#ifdef USE_WINSOCK
	#include <Winsock2.h>
#endif

#include <ruby.h>
#ifdef HAVE_RUBY_ENCODING_H
	#include <ruby/encoding.h>
#endif

#endif // SU_UTILS_RUBYLIB_H_
