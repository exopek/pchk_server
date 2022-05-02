/**
 * @file	include/lcnrtl/timerhelper.h
 * Timer-Funktionen und Datentypen.
 * @author	Lothar May
 * @since	2006-03-13
 * $Id: timerhelper.h 1032 2006-03-10 12:37:58Z lm $
 */

#ifndef INC_LCNRTL_TIMERHELPER_H_
#define INC_LCNRTL_TIMERHELPER_H_

#include "../include/defs.h"

#include <string>
#include <ctime>


#if !defined(_WIN32) || defined(__MINGW32__)
	typedef long long TimerInt64;
#else
	typedef __int64 TimerInt64;
#endif

#ifdef _MSC_VER
#define TIME32_T __time32_t
#else
#define TIME32_T std::time_t
#endif

LCNRTL_COMP_EXTERN TimerInt64 Timer_GetTimeStamp();  // Liefert einen Zeitstempel zum Vergleich von Zeitunterschieden mit einer Genauigkeit von 1 ms
LCNRTL_COMP_EXTERN int LOCALTIME_S(struct std::tm* _tm, const std::time_t *time);
LCNRTL_COMP_EXTERN struct std::tm GetCurrentLocalTime();
LCNRTL_COMP_EXTERN std::string XmlDateTimeToString(const std::time_t& t, bool add_dash = true, bool add_colon = true);
LCNRTL_COMP_EXTERN std::string XmlDateToString(const std::time_t& t, bool add_dash = true);
LCNRTL_COMP_EXTERN bool XmlStringToDate(const char* str, std::time_t* t);
LCNRTL_COMP_EXTERN bool XmlStringToDateTime(const char* str, std::time_t* t);

// Wrappers to force 32-bit resolution
LCNRTL_COMP_EXTERN std::string XmlDateTimeToString32(const TIME32_T& t, bool add_dash = true, bool add_colon = true);
LCNRTL_COMP_EXTERN std::string XmlDateToString32(const TIME32_T& t, bool add_dash = true);
LCNRTL_COMP_EXTERN bool XmlStringToDate32(const char* str, TIME32_T* t);
LCNRTL_COMP_EXTERN bool XmlStringToDateTime32(const char* str, TIME32_T* t);

LCNRTL_COMP_EXTERN std::wstring TimeToString(const struct std::tm* tm);

#ifdef _MSC_VER
    #define TIME32(t) _time32(t)
    #define TIME64(t) _time64(t)
#else
    #define TIME32(t) std::time(t)
#endif


#endif
