/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <egt/ui>
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sys/time.h>
#include <fstream>
#ifdef HAVE_LIBPLANES
#include <planes/plane.h>
#include "egt/detail/screen/kmsoverlay.h"
#endif

using namespace std;
using namespace egt;

#define FIXED_HEIGHT 480
#define FIXED_WIDTH 800
#define DEFAULT_MS_INTERVAL 35
#define FLICK_MS_INTERVAL 180
#define THRESHOLD_FLICK_MS 600

int g_current_x = 0;
int timediff_idx = 0;
bool g_is_timer_start = false;
bool g_is_plane_hide = false;
bool g_one_period_over = false;
//bool g_is_file_write = false;

#ifdef HAVE_LIBPLANES
void show_rec_overlay(egt::detail::KMSOverlay* s, int steper, int drag_step);
void flick_animation(egt::detail::KMSOverlay* s);
void flick_animation(egt::detail::KMSOverlay* s)
{
    if (!g_is_plane_hide)
    {
        plane_hide(s->s());
        g_is_plane_hide = true;
    }
    else
    {
        plane_apply(s->s());
        g_is_plane_hide = false;
    }
}

void show_rec_overlay(egt::detail::KMSOverlay* s, int steper, int drag_step)
{
    g_current_x += (steper + drag_step);
    if (FIXED_WIDTH <= g_current_x)
    {
        g_current_x = 0;
    }
    plane_set_pan_size(s->s(), g_current_x, FIXED_HEIGHT);
    plane_apply(s->s());
}
#endif

int main(int argc, char** argv)
{
    Application app(argc, argv, "tachometer");
#ifdef EXAMPLEDATA
    egt::add_search_path(EXAMPLEDATA);
#endif
    TopWindow win;

    int timediff[300] = {0};
    //int i;
    struct timeval time1, time2;
    //ofstream time_sta("time_statistic.txt");
#ifdef HAVE_LIBPLANES
    Sprite sprite2(Image("file:top.png"), Size(800, 480), 1, Point(0, 0));
    Sprite sprite1(Image("file:bottom.png"), Size(800, 480), 1, Point(0, 0));
    win.add(sprite1);
    win.add(sprite2);
    sprite1.show();
    sprite2.show();
    egt::detail::KMSOverlay* s = reinterpret_cast<egt::detail::KMSOverlay*>(sprite1.screen());
    PeriodicTimer flicktimer(std::chrono::milliseconds(FLICK_MS_INTERVAL));
    flicktimer.on_timeout([&]()
    {
        flick_animation(s);
    });

#if 1
    PeriodicTimer animatetimer(std::chrono::milliseconds(DEFAULT_MS_INTERVAL));
    animatetimer.on_timeout([&]()
    {
        if (!g_one_period_over)
        {
            gettimeofday(&time1, NULL);
        }
        show_rec_overlay(s, 4, 0);
        if (!g_one_period_over)
        {
            gettimeofday(&time2, NULL);
            if (time1.tv_sec < time2.tv_sec)
            {
                //cout << "time1.sec:" << time1.tv_sec << "time1.usec:" << time1.tv_usec << endl;
                //cout << "time2.sec:" << time2.tv_sec << "time2.usec:" << time2.tv_usec << endl;
                timediff[timediff_idx] = time2.tv_usec + 1000000 - time1.tv_usec;
            }
            else if (time1.tv_sec > time2.tv_sec)
            {
                cout << "abnormal!!!time1.sec:" << time1.tv_sec << "time1.usec:" << time1.tv_usec << endl;
                //cout << "time2.sec:" << time2.tv_sec << "time2.usec:" << time2.tv_usec << endl;
            }
            else
            {
                timediff[timediff_idx] = time2.tv_usec - time1.tv_usec;
            }
            timediff_idx = (299 <= timediff_idx) ? 0 : timediff_idx + 1;
        }
        if (THRESHOLD_FLICK_MS <=g_current_x)
        {
            if (!g_is_timer_start)
            {
                flicktimer.start();
                g_is_timer_start = true;
                //cout << "start flick timer" << endl;
            }
        }
        else
        {
            if (g_is_timer_start)
            {
                flicktimer.stop();
                plane_apply(s->s());
                g_is_plane_hide = false;
                g_is_timer_start = false;
                g_one_period_over = true;
                //cout << "stop flick timer, timediff_idx:" << timediff_idx << endl;
            }

        }
#if 0
        if (g_one_period_over && !g_is_file_write)
        {
            if (time_sta.is_open())
            {
                for (i=0; i < timediff_idx; i++)
                {
                    time_sta << timediff[i] << endl;
                }
                time_sta.close();
            }
            g_is_file_write = true;
        }
#endif
    });
    animatetimer.start();
#else

    auto handle_touch = [&](Event & event)
    {
        switch (event.id())
        {
            case eventid::pointer_click:
            {
                gettimeofday(&time, NULL);
                //cout << "call before sec:" << time.tv_sec << "usec:" << time.tv_usec << endl;
                show_rec_overlay(s, 100, 0);
                gettimeofday(&time, NULL);
                //cout << "call return sec:" << time.tv_sec << "usec:" << time.tv_usec << endl;
                //sprite3.showtoplayer();
                break;
            }
            case eventid::pointer_drag_start:
                //cout << "pointer_drag_start" << endl;

                break;
            case eventid::pointer_drag_stop:

            break;
            case eventid::pointer_drag:
            {
                //cout << "pointer_drag" << endl;
                break;
            }
            default:
                break;
        }
    };

    win.on_event(handle_touch);
#endif
    win.show();
#endif
    return app.run();
}


