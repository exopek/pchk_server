/**
 * @file	include/lcnrtl/internal/defs.h
 * Definitionen.
 * @author	Lothar May
 * @since	2003-10-10
 * @version	$Id: defs.h 1879 2015-09-08 12:06:39Z ak $
 */

#ifndef INC_LCNRTL_DEFS_H_
#define INC_LCNRTL_DEFS_H_

#if defined _WIN32
	#define LCNRTL_COMP_EXPORT __declspec (dllexport)
	#define LCNRTL_COMP_IMPORT __declspec (dllimport)
	#define LCNRTL_COMP_EXTERN extern
#else
	#define LCNRTL_COMP_EXPORT
	#define LCNRTL_COMP_IMPORT
	#define LCNRTL_COMP_EXTERN
#endif

#if defined LCNRTL_BUILD_DLL
	#define LCNRTL_IMPEXP LCNRTL_COMP_EXPORT
	#define LCNRTL_EXT_STL
#elif defined LCNRTL_USE_DLL
	#define LCNRTL_IMPEXP LCNRTL_COMP_IMPORT
	#define LCNRTL_EXT_STL LCNRTL_COMP_EXTERN
#else
	#define LCNRTL_IMPEXP
#endif

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#define DEPRECATED(func) ((func))
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
	/* TODO: Not fully implemented for non-MSVC platforms */
	/* #define _MUTEXDEADLOCKDETECT */
#endif

#endif

