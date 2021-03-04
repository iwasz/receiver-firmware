/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "Container.h"
#include <stm32f0xx_hal.h>

static void SystemClock_Config ();

/*****************************************************************************/

int main ()
{
        __HAL_RCC_SYSCFG_CLK_ENABLE ();
        __HAL_RCC_PWR_CLK_ENABLE ();

#ifdef PLATFORM_MICRO
        // ensure the kit is correct if this does not work.
        __HAL_REMAP_PIN_ENABLE (HAL_REMAP_PA11_PA12);
#endif

        HAL_Init ();
        SystemClock_Config ();
        // re-set the sysTick priority to lower value.
        HAL_NVIC_SetPriority (SysTick_IRQn, SYS_TICK_PRIORITY, 0U);

        container::init ();

        Timer displayTimer;
        Timer menuTimer;

        int refreshRate = 9; // Something different than 10 so the screen is a little bit out of sync. This way the last digit changes.

        // Refresh stopwatch state to reflect the config.
        auto refreshSettings = [&] {
                getDisplay ().setFlip (getConfig ().isFlip ());

                auto br = getConfig ().getBrightness ();

                if (br != Brightness::levelAuto) {
                        getDisplay ().setBrightness (uint8_t (br));
                }
#ifdef WITH_SOUND
                getBuzzer ().setActive (getConfig ().isBuzzerOn ());
#endif
                getDisplay ().setResolution (getConfig ().getResolution ());
                getIrDetector ().recalculateConstants ();
                getExtDetector ().recalculateConstants ();
        };

        refreshSettings ();
        getMenu ().onEvent (menu::Event::timePassed); // Initial state.

        while (true) {
#ifdef WITH_SOUND
                getBuzzer ().run ();
#endif

#ifdef WITH_BUTTON
                getButton ().run ();
#endif

#ifdef WITH_HISTORY
                getHistory ().run ();
#endif

#ifdef WITH_POWER_MANAGER
                getPowerManager ().run ();
#endif

                // getBeam ().run ();
                getIrDetector ().run (getStopWatch ().getTime ());

                usbcli::run ();

                if (displayTimer.isExpired ()) {

                        /*
                         * Des not have to be invoked more frequently than the screen refresh rate. Nobody
                         * would notice anyway.
                         */
                        Event evt;
                        bool isEvt{};
                        __disable_irq ();
                        if (!eventQueue.empty ()) {
                                evt = eventQueue.front ();
                                eventQueue.pop ();
                                isEvt = true;
                        }
                        __enable_irq ();

                        if (isEvt) {
                                getFastStateMachine ().run (evt);
                        }

                        getFastStateMachine ().run (Event::Type::timePassed);
                        displayTimer.start (refreshRate);
                }

#ifdef WITH_BUTTON
                if (getButton ().getPressClear ()) {
                        getMenu ().onEvent (menu::Event::shortPress);
#ifdef WITH_SOUND
                        getBuzzer ().beep (20, 0, 1);
#endif
                }

                if (getButton ().getLongPressClear ()) {
                        getMenu ().onEvent (menu::Event::longPress);
#ifdef WITH_SOUND
                        getBuzzer ().beep (20, 20, 2);
#endif
                }
#endif // WITH_BUTTON

                if (cfg::changed ()) {
                        cfg::changed () = false;
                        refreshSettings ();
#ifdef WITH_FLASH
                        getConfigFlashEepromStorage ().store (reinterpret_cast<uint8_t *> (&getConfig ()), sizeof (getConfig ()), 0);
#endif
                }

                if (menuTimer.isExpired ()) {
                        menuTimer.start (250);
                        getMenu ().onEvent (menu::Event::timePassed);
                }
        }
}

/*****************************************************************************/

#if defined(PLATFORM_REGULAR) || defined(PLATFORM_HUGE)

void SystemClock_Config ()
{
        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
        RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

        /** Configure LSE Drive Capability
         */
        HAL_PWR_EnableBkUpAccess ();
        __HAL_RCC_LSEDRIVE_CONFIG (RCC_LSEDRIVE_HIGH);
        /** Initializes the CPU, AHB and APB busses clocks
         */

        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI14 | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
        RCC_OscInitStruct.HSEState = RCC_HSE_ON; // RCC_HSE_BYPASS;
        RCC_OscInitStruct.LSEState = RCC_LSE_ON;
        RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
        RCC_OscInitStruct.HSI14CalibrationValue = 16;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
#if defined(PLATFORM_REGULAR)
        RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL5;
#elif defined(PLATFORM_HUGE)
        RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
#endif
        RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;

        if (HAL_RCC_OscConfig (&RCC_OscInitStruct) != HAL_OK) {
                Error_Handler ();
        }

        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

        if (HAL_RCC_ClockConfig (&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
                Error_Handler ();
        }

        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB | RCC_PERIPHCLK_RTC;
        PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;

        if (HAL_RCCEx_PeriphCLKConfig (&PeriphClkInit) != HAL_OK) {
                Error_Handler ();
        }
}

#elif defined(PLATFORM_MICRO)

void SystemClock_Config ()
{
        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

        /** Initializes the CPU, AHB and APB busses clocks.
         */
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI14 | RCC_OSCILLATORTYPE_HSE;
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
        RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
        RCC_OscInitStruct.HSI14CalibrationValue = 16;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
        RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL5;
        RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;

        if (HAL_RCC_OscConfig (&RCC_OscInitStruct) != HAL_OK) {
                Error_Handler ();
        }
        /** Initializes the CPU, AHB and APB busses clocks.
         */
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

        if (HAL_RCC_ClockConfig (&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
                Error_Handler ();
        }
}

#endif

/*****************************************************************************/

namespace __gnu_cxx {
void __verbose_terminate_handler ()
{
        while (true) {
        }
}
} // namespace __gnu_cxx

extern "C" void __cxa_pure_virtual ()
{
        while (true) {
        }
}

// extern "C" void __cxa_guard_acquire ()
//{
//}

// extern "C" void __cxa_guard_release ()
//{
//}

namespace std {
void __throw_bad_function_call ()
{
        while (true) {
        }
}
} // namespace std

#if 0
void *operator new (size_t size) { return nullptr; }

void operator delete (void *p) {}
void operator delete (void *p, unsigned int) {}
#endif

#ifndef WITH_USB
extern "C" void usbWrite (const char * /* str */) {}
#endif

extern "C" void __gxx_personality_v0 () {}
// extern "C" void _init () {}
