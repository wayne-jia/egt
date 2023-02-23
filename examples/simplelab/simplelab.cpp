/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <egt/ui>


#define ERAW_GENERATED

int main(int argc, char** argv)
{
    std::vector<std::shared_ptr<egt::AnimationSequence>> animations;
    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;
    window.show();

#ifndef ERAW_GENERATED
    const char *svg_files_arry[1] = {"/root/clock.svg"};
    egt::experimental::SVGDeserial smplab(app, window, 1, svg_files_arry);
#else
    egt::experimental::SVGDeserial smplab(window);
#endif

    smplab.AddWidgetByID("/root/eraw/watch.eraw", true);
    smplab.AddWidgetByID("/root/eraw/point.eraw", true);
    auto circlePtr = smplab.AddWidgetByID("/root/eraw/circle.eraw", true);
    auto needle_point = circlePtr->box().center();
    animations.push_back(smplab.RotateAnimation(smplab.AddRotateWidgetByID("/root/eraw/hour.eraw", 0, 100, 0, 360, true, needle_point),
                         0, 100, std::chrono::seconds(3), egt::easing_circular_easein, egt::easing_circular_easeout));
    animations.push_back(smplab.RotateAnimation(smplab.AddRotateWidgetByID("/root/eraw/minute.eraw", 0, 100, 0, 360, true, needle_point),
                         0, 200, std::chrono::seconds(7), egt::easing_sine_easein, egt::easing_sine_easeout));

    return app.run();
}
