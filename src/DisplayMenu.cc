/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "DisplayMenu.h"

/*****************************************************************************/

void DisplayMenu::onShortPress ()
{
        // Cycle the menu.
        option = Option ((int (option) + 1) % int (Option::last_option));
        prepareMenuForOption (option);
}

/*****************************************************************************/

void DisplayMenu::onLongPress ()
{
        switch (option) {
        case Option::stop_watch:
                break;

        case Option::flip:
                config.orientationFlip = !config.orientationFlip;
                break;

        case Option::ir_on:
                config.irSensorOn = !config.irSensorOn;
                break;

        case Option::buzzer_on:
                config.buzzerOn = !config.buzzerOn;
                break;

        case Option::resolution:
                config.resolution = cfg::Resolution ((int (config.resolution) - 1) % cfg::RESOLUTION_NUMBER_OF_OPTIONS);
                break;

        default:
                break;
        }

        prepareMenuForOption (option);
        cfg::changed () = true;
}

/*****************************************************************************/

void DisplayMenu::prepareMenuForOption (Option o)
{
        display.clear ();

        switch (o) {
        case Option::stop_watch:
                machine.resume ();
                break;

        case Option::flip:
                machine.pause ();
                display.setText ("1.FLIP");
                break;

        case Option::ir_on:
                if (config.irSensorOn) {
                        display.setText ("2.I.r.on");
                }
                else {
                        display.setText ("2.I.r.off");
                }

                break;

        case Option::buzzer_on:
                if (config.buzzerOn) {
                        display.setText ("3.Sn.on");
                }
                else {
                        display.setText ("3.Sn.off");
                }
                break;

        case Option::resolution:
                switch (config.resolution) {
                case cfg::Resolution::ms_10:
                        display.setText ("4.ms.10");
                        break;

                case cfg::Resolution::ms_1:
                        display.setText ("4.ms.1");
                        break;

                case cfg::Resolution::us_100:
                        display.setText ("4.us.100");
                        break;

                case cfg::Resolution::us_10:
                        display.setText ("4.us.10");
                        break;

                default:
                        break;
                }

                break;

        default:
                break;
        }
}
