/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "Config.h"
#ifdef WITH_FLASH
#include <storage/FlashEepromStorage.h>
#endif

extern cfg::Config &getConfig ();

#ifdef WITH_FLASH
using ConfigFlashEepromStorage = FlashEepromStorage<2048, 2>;
extern ConfigFlashEepromStorage &getConfigFlashEepromStorage ();
#endif