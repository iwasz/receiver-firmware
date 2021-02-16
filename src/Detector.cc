/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "Detector.h"

#ifndef UNIT_TEST
#include "Gpio.h"
Gpio senseOn{GPIOB, GPIO_PIN_4, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL};
#else
#define __disable_irq(x) x
#define __enable_irq(x) x
#endif

/*****************************************************************************/

// TOOD change bool to void
void EdgeFilter::onEdge (Edge const &e)
{
        /*
         * This can happen when noise frequency is very high, and the µC can't keep up,
         * and its misses an EXTI event. This way we can end up with two consecutive edges
         * with the same polarity.
         */
        if (!queue.empty () && queue.back ().polarity == e.polarity) {
                // Reset queue so it's still full, but pulses are 0 width.
                queue.push (e);
                queue.push ({e.timePoint, (e.polarity == EdgePolarity::rising) ? (EdgePolarity::falling) : (EdgePolarity::rising)});

                // TODO Increase noise counter
                // TODO When noise counter is high, tirn of the EXTI, so the rest of the code has a chance to run.
        }

        queue.push (e);

        /*--------------------------------------------------------------------------*/

        // Calculate duty cycle of present slice of the signal
        Result1us cycleTreshold = (queue.back ().timePoint - queue.front ().timePoint)
                * dutyTresholdPercent;                                  // slice length times treshold. See equation in the docs.
        Result1us hiDuration = queue[1].timePoint - queue[0].timePoint; // We assume for now, that queue[0] is rising
        Result1us lowDuration = queue[2].timePoint - queue[1].timePoint;

        /*--------------------------------------------------------------------------*/

        // Find out which edge in the queue is rising, which falling.
        auto *firstRising = &queue[0]; // Pointers are smaller than Edge object
        auto *firstFalling = &queue[1];

        bool longHighEdge{};
        bool longLowEdge{};

        // 50% chance that above are correct. If our assumption wasn't right, we swap.
        if (firstRising->polarity != EdgePolarity::rising) {
                std::swap (firstRising, firstFalling);
                std::swap (hiDuration, lowDuration);
                longHighEdge = hiDuration >= msToResult1 (minTreggerEventMs);
        }
        else {
                longLowEdge = lowDuration >= msToResult1 (minTreggerEventMs);
        }

        /*--------------------------------------------------------------------------*/

        // Tu może nie złapać kiedy długo było low, a potem high przez 11ms. Duty będzie na low, a był event.
        if (hiDuration * 100 > cycleTreshold || // PWM of the slice is high
            longHighEdge) {                     // Or the high level was long itself
                if (state != State::high) {
                        state = State::high;
                        highStateStart = firstRising->timePoint;
                }
        }
        else if (lowDuration * 100 >= cycleTreshold || // PWM of the slice is low
                 longLowEdge) {                        // Or the low level was long enogh itself
                if (state != State::low) {
                        state = State::low;
                        lowStateStart = firstFalling->timePoint;
                }
        }

        if (highStateStart < lowStateStart) {
                bool longHighState = (lowStateStart - highStateStart) >= msToResult1 (minTreggerEventMs);
                bool longLowState = (e.timePoint - lowStateStart) >= msToResult1 (minTreggerEventMs);

                if (longHighState && longLowState) {
                        callback->report (DetectorEventType::trigger, highStateStart);
                        highStateStart = lowStateStart; // To prevent reporting twice
                }
        }
}

/****************************************************************************/

void EdgeFilter::run (Result1us const &now)
{
        // TODO critical section
        if (queue.empty ()) {
                return;
        }

        // TODO critical section
        auto &back = queue.back ();

        // if (now - getLastStateChange () /* lastStateChange */ >= msToResult1 (minTreggerEventMs)) {
        // if (state == State::high && queue.back().polarity == EdgePolarity::falling &&  now - getLastStateChange () /* lastStateChange */ >=
        // msToResult1 (minTreggerEventMs)) {

        // If there's no noise at all, and the line stays silent, we force the check every minTreggerEventMs
        if (now - back.timePoint >= msToResult1 (minTreggerEventMs)) {
                __disable_irq ();

                if (back.polarity == EdgePolarity::falling) {
                        onEdge ({now, EdgePolarity::rising});
                        onEdge ({now, EdgePolarity::falling});
                }

                // if (state == State::high) {
                //         // if (back.polarity == EdgePolarity::rising) {
                //         onEdge ({now, EdgePolarity::falling});
                //         onEdge ({now, EdgePolarity::rising});
                // }
                // else {
                //         onEdge ({now, EdgePolarity::rising});
                //         onEdge ({now, EdgePolarity::falling});
                // }
                __enable_irq ();
        }

        // if (highStateStart < lowStateStart && lowStateStart - highStateStart >= msToResult1 (minTreggerEventMs)
        //     && now - lowStateStart >= msToResult1 (minTreggerEventMs)) {
        //         callback->report (DetectorEventType::trigger, queue[0].timePoint);
        // }
}
