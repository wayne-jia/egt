/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <egt/ui>
#include <random>
#include <iostream>
#include <sys/time.h>

using namespace std;
using namespace egt;

template<typename T, typename RandomGenerator>
T random_item(T start, T end, RandomGenerator& e)
{
    std::uniform_int_distribution<> dist(0, std::distance(start, end) - 1);
    std::advance(start, dist(e));
    return start;
}

int g_arc_inc = -180;
bool b_reverse = false;
bool b_rev_point = false;

int main(int argc, char** argv)
{
    Application app(argc, argv, "squares");

#if 1
    int timediff = 0;
    struct timeval time1, time2;
    //ofstream time_sta("time_statistic.txt");
#endif

    TopWindow win;
    win.color(egt::Palette::ColorId::bg, egt::Palette::black);
    const Color FUCHSIA(Color::css("#F012BE"));

    const auto center = Point(400, 306);
    int radius = 280;
    int inc_width = 48;
    int dec_width = 49;
    int start_angle = -25;
    auto bar_color = Color::rgb(0x3f6efc);
    auto bg_color = Color::rgb(0x000000);
    Rect rect(0, 0, 800, 480);
    Painter painter(win.screen()->context());
    float angle = detail::to_radians<float>(start_angle, g_arc_inc);

    PeriodicTimer timer(std::chrono::milliseconds(16));
    timer.on_timeout([&]()
    {
        if (50 == g_arc_inc)
        {
            b_reverse = true;
            b_rev_point = true;
        }

        if (-180 > g_arc_inc)
        {
            b_reverse = false;
        }

        if (b_reverse)
        {
            if (b_rev_point)
            {
                b_rev_point = false;
                gettimeofday(&time1, NULL);
                painter.set(bg_color);
                painter.line_width(dec_width);
                painter.draw(Arc(center, radius, angle - 0.02, angle + 0.02));
                painter.stroke();
                painter.fill();
                gettimeofday(&time2, NULL);
                if (time1.tv_sec < time2.tv_sec)
                {
                    timediff = time2.tv_usec + 1000000 - time1.tv_usec;
                }
                else
                {
                    timediff = time2.tv_usec - time1.tv_usec;
                }
                //cout << timediff << endl;
            }
            g_arc_inc -= 2;
        }
        else
        {
            g_arc_inc += 2;
        }
        //g_arc_inc = (180 < g_arc_inc) ? 0 : g_arc_inc;
        angle = detail::to_radians<float>(start_angle, g_arc_inc);

        if (b_reverse)
        {
            gettimeofday(&time1, NULL);
            painter.set(bg_color);
            painter.line_width(dec_width);
            painter.draw(Arc(center, radius, angle - 0.02, angle + 0.02));
            painter.stroke();
            painter.fill();
            gettimeofday(&time2, NULL);
            if (time1.tv_sec < time2.tv_sec)
            {
                timediff = time2.tv_usec + 1000000 - time1.tv_usec;
            }
            else
            {
                timediff = time2.tv_usec - time1.tv_usec;
            }
            //cout << timediff << endl;
        }
        else
        {
            gettimeofday(&time1, NULL);
            painter.set(bar_color);
            painter.line_width(inc_width);
            painter.draw(Arc(center, radius, angle - 0.01, angle + 0.01));
            painter.stroke();
            painter.fill();
            gettimeofday(&time2, NULL);
            if (time1.tv_sec < time2.tv_sec)
            {
                timediff = time2.tv_usec + 1000000 - time1.tv_usec;
            }
            else
            {
                timediff = time2.tv_usec - time1.tv_usec;
            }
            //cout << timediff << endl;
        }

        Screen::DamageArray damage;
        damage.push_back(rect);
        win.screen()->flip(damage);
    });
    timer.start();

    win.show();

    return app.run();
}
