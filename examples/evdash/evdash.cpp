/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Wayne Jia <wayne.jia@microchip.com>
 * @Microchip Inc.
 *
 */
#include <egt/ui>
#include <iostream>
#include <queue>


#define ID_MIN 310
#define ID_MAX 390
#define STEPPER 2
#define MAX_NEEDLE_INDEX 40
#define HIGH_Q_TIME_THRESHHOLD 30000  //30ms
#define LOW_Q_TIME_THRESHHOLD  20000  //20ms

using QueueCallback = std::function<std::string ()>;


int main(int argc, char** argv)
{
    // Construct egt instance
    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;
    window.show();

    // Define the local variables
    std::queue<QueueCallback> high_pri_q;
    std::queue<QueueCallback> low_pri_q;
    std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> NeedleBase;
    std::string lbd_ret = "0";
    std::ostringstream path;
    bool is_increasing = true;
    bool is_needle_finish = false;
    bool is_high_q_quit = true;
    bool is_low_q_quit = true;
    int needle_index = 0;
    int speed_index = 0;
    int timer_cnt = 0;

    // Construct the SVG handler instance
    egt::experimental::SVGDeserial evdash(window);

    // Deserial the background and some blinking widgets
    evdash.AddWidgetByID("/root/eraw/png.eraw", true);
    auto bluetooth = evdash.AddWidgetByID("/root/eraw/bluetooth.eraw", true);
    auto loading = evdash.AddWidgetByID("/root/eraw/loading.eraw", true);

    // Deserial the progress bar of battery
    for (auto i = ID_MIN, j =0; i <= ID_MAX; i += STEPPER, j++)
    {
        path.str("");
        if (316 == i)
            path << "eraw/polygon" << std::to_string(i) << ".eraw";
        else
            path << "eraw/path" << std::to_string(i) << ".eraw";

        NeedleBase.push_back(evdash.AddWidgetByID(path.str(), true));
    }

    // Handle the battery progress bar animation
    auto needle_move = [&]()
    {
        for (int i=0; i<1; i++)
        {
            if (is_increasing)
            {
                NeedleBase[needle_index]->show();
            }
            else
            {
                NeedleBase[needle_index]->hide();
            }

            if (is_increasing && MAX_NEEDLE_INDEX == needle_index)
            {
                is_increasing = false;
                is_needle_finish = true;
            }
            else if (!is_increasing && 0 == needle_index)
            {
                is_increasing = true;
                is_needle_finish = true;
            }
            else
            {
                needle_index = is_increasing ? needle_index + 1 : needle_index - 1;
            }
        }
        return "needle_move";
    };

    // Construct the text widgets
    evdash.add_text_widget("#standby", "STANDBY", egt::Rect(324, 290, 159, 63), 30, egt::Palette::white);
    evdash.add_text_widget("#battery", "80", egt::Rect(527, 172, 137, 115), 120, egt::Palette::white);
    evdash.add_text_widget("#speed", "20", egt::Rect(131, 172, 137, 115), 120, egt::Palette::gray);
    evdash.add_text_widget("#temp", "18", egt::Rect(83, 43, 21, 16), 20, egt::Palette::white);
    evdash.add_text_widget("#remain", "13", egt::Rect(593, 304, 40, 25), 25, egt::Palette::white);
    evdash.add_text_widget("#total", "0012345", egt::Rect(8, 456, 44, 16), 16, egt::Palette::white);
    evdash.add_text_widget("#today", "065", egt::Rect(716, 456, 44, 16), 16, egt::Palette::white);

    // Define periodic timer to handle animation
    egt::PeriodicTimer timer(std::chrono::milliseconds(16));
    timer.on_timeout([&]()
    {
        // Use high/low priority queue to control the task exe frequency
        if (!is_high_q_quit)
            return;
        if (!high_pri_q.empty())
        {
            is_high_q_quit = false;
            lbd_ret = high_pri_q.front()();
            high_pri_q.pop();
            is_high_q_quit = true;
        }
        else
        {
            if (!is_low_q_quit)
                return;
            if (!low_pri_q.empty())
            {
                is_low_q_quit = false;
                lbd_ret = low_pri_q.front()();
                low_pri_q.pop();
                is_low_q_quit = true;
                return;
            }
        }

        // Viariable to control the animation and frequency
        timer_cnt = (LOW_Q_TIME_THRESHHOLD <= timer_cnt) ? 0 : timer_cnt + 1;

        // Execute the low priority tasks
        if (is_needle_finish)
        {
            is_needle_finish = false;
            if (!(timer_cnt % 1))
            {
                low_pri_q.push([&]()
                {
                    speed_index = (5 <= speed_index) ? 0 : speed_index + 1;
                    evdash.find_text("#battery")->clear();
                    evdash.find_text("#speed")->clear();
                    switch (speed_index)
                    {
                        case 0:
                            evdash.find_text("#speed")->text("15");
                            evdash.find_text("#battery")->text("33");
                            break;
                        case 1:
                            evdash.find_text("#speed")->text("28");
                            evdash.find_text("#battery")->text("12");
                            break;
                        case 2:
                            evdash.find_text("#speed")->text("40");
                            evdash.find_text("#battery")->text("76");
                            break;
                        case 3:
                            evdash.find_text("#speed")->text("69");
                            evdash.find_text("#battery")->text("50");
                            break;
                        case 4:
                            evdash.find_text("#speed")->text("88");
                            evdash.find_text("#battery")->text("25");
                            break;
                        default:
                            evdash.find_text("#speed")->text("0");
                            evdash.find_text("#battery")->text("0");
                            break;
                    }

                    return "speed_change";
                });
            }

            if (!(timer_cnt % 2))
            {
                low_pri_q.push([&]()
                {
                    bluetooth->visible_toggle();
                    loading->visible_toggle();
                    return "blinking";
                });
            }
        }
        else  // This branch exec the high priority event
        {
            high_pri_q.push(needle_move);
        }
    });
    timer.start();

    return app.run();
}
