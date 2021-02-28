/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "Regular.h"
#include "Container.h"

/****************************************************************************/

extern "C" void EXTI4_15_IRQHandler ()
{
        if (__HAL_GPIO_EXTI_GET_IT (GPIO_PIN_8) != RESET) {
                __HAL_GPIO_EXTI_CLEAR_IT (GPIO_PIN_8);
                getIrDetector ().onEdge ({getStopWatch ().getTimeFromIsr (), EdgePolarity (getIrTriggerInput ().get ())});
        }
        else if (__HAL_GPIO_EXTI_GET_IT (GPIO_PIN_15) != RESET) {
                __HAL_GPIO_EXTI_CLEAR_IT (GPIO_PIN_15);
                getButton ().onToggle ();
        }
}

/****************************************************************************/

extern "C" void EXTI2_3_IRQHandler ()
{
        if (__HAL_GPIO_EXTI_GET_IT (GPIO_PIN_3) != RESET) {
                __HAL_GPIO_EXTI_CLEAR_IT (GPIO_PIN_3);
                getExtDetector ().onEdge ({getStopWatch ().getTimeFromIsr (), EdgePolarity (getExtTriggerInput ().get ())});
        }
}
