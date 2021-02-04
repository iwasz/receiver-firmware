/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "Hal.h"
#include "Rtc.h"
#include "Types.h"
#include "usbd_cdc.h"
#include <gsl/gsl>

inline void print (gsl::czstring<> s) { usbWrite (s); }
void print (int i);
void print (unsigned int i);
void printResult (Result time, ResultDisplayStyle ra = ResultDisplayStyle::SECOND);
void printDate (RTC_DateTypeDef const &date, Time const &time);
