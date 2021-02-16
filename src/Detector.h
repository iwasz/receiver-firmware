/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "Types.h"
#include <cstdint>
#include <etl/circular_buffer.h>

/*****************************************************************************/

// TODO refactor (config?)
const uint16_t minTreggerEventMs = 10;
// TODO rational  arithmetic ?.
const uint8_t dutyTresholdPercent = 50;

/*****************************************************************************/

enum class EdgePolarity { rising, falling };

/**
 * Signal edge with polarity and time of occurence.
 */
struct Edge {
        Result1us timePoint{};
        EdgePolarity polarity{};
};

/**
 * A slice of a rectangular signal, 3 edges. Sometimes called simply the "slice"
 */
using EdgeQueue = etl::circular_buffer<Edge, 3>;

enum class DetectorEventType { trigger, noise, noBeam };

/**
 *
 */
struct IEdgeDetectorCallback {
        virtual void report (DetectorEventType type, Result1us timePoint) = 0;
};

/**
 * Looks for correct trigger events in the signal.
 */
class EdgeDetector {
public:
        virtual void onEdge (Edge const &e)
        {
                /*
                 * This can happen when noise frequency is very high, and the µC can't keep up,
                 * and its misses an EXTI event. This way we can end up with two consecutive edges
                 * with the same polarity.
                 */
                if (!queue.empty () && queue.back ().polarity == e.polarity) {
                        // TODO This is critical situation. Rethink. Maybe report noise?
                        // queue.clear (); //?
                        return;
                        // report Noise ?

                        // TODO rethink what to do in this situation. Can this be read from some registers? That we actually missed?
                        // while (true) {
                        // }
                }

                queue.push (e);

                if (!queue.full ()) {
                        return;
                }

                if (isTriggerEvent ()) {
                        callback->report (DetectorEventType::trigger, queue[0].timePoint);
                        return;
                }

                // This is always at the end of the chain, so no next->onEdge
        }

        virtual void run (Result1us const &now) {}

        void setCallback (IEdgeDetectorCallback *cb) { callback = cb; }

protected:
        /// IRQ context
        bool isTriggerEvent () const
        {
                // Queue should be always full at this point, because there are guards for that.
                return queue.front ().polarity == EdgePolarity::rising
                        && queue[2].timePoint - queue[1].timePoint >= msToResult1 (minTreggerEventMs)
                        && queue[1].timePoint - queue[0].timePoint >= msToResult1 (minTreggerEventMs);
        }

        EdgeQueue queue;
        IEdgeDetectorCallback *callback{};
};

/*****************************************************************************/

class EdgeFilter : public EdgeDetector {
public:
        enum State { high, low };

        EdgeFilter (EdgeDetector *next, State initialState) : next{next}, state{initialState} {}

        /// IRQ context
        void onEdge (Edge const &e) override;
        /// "main" context. As frequently as possible.
        void run (Result1us const &now) override;

private:
        Result1us const &getLastStateChange () const { return std::max (highStateStart, lowStateStart); };

        /*--------------------------------------------------------------------------*/

        EdgeDetector *next{};
        State state;
        Result1us highStateStart{};
        Result1us lowStateStart{};
        // Result1us lastStateChange{};
        // Result1us lastEdge{};
};