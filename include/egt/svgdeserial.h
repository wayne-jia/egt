/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef EGT_SVGDESERIAL_H
#define EGT_SVGDESERIAL_H


#include "app.h"
#include "window.h"
#include "gauge.h"
#include "animation.h"

namespace egt
{
inline namespace v1
{
namespace experimental
{

class EGT_API SVGDeserial : public Gauge
{
public:
    SVGDeserial(Application& app, TopWindow& parent, int svg_cnt, const char** svg_files) noexcept;
    SVGDeserial(TopWindow& parent) noexcept;
    shared_cairo_surface_t DeSerialize(const std::string& filename, std::shared_ptr<Rect>& rect);
    std::shared_ptr<GaugeLayer> AddWidgetByID(const std::string& id, bool show);
    std::shared_ptr<NeedleLayer> AddRotateWidgetByID(const std::string& id, int min, int max,
        int min_angle, int max_angle, bool clockwise, Point center);
    std::shared_ptr<AnimationSequence> RotateAnimation(std::shared_ptr<NeedleLayer> widget, int min, int max,
        std::chrono::seconds length, const EasingFunc& easein, const EasingFunc& easeout);
    ~SVGDeserial() noexcept {}
private:

};

}
}
}

#endif
