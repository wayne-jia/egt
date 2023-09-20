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
#include "text.h"
#include "shapes.h"

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
    shared_cairo_surface_t DeSerialize(const unsigned char* buf, size_t len, std::shared_ptr<egt::Rect>& rect);
    std::shared_ptr<GaugeLayer> AddWidgetByID(const std::string& id, bool show);
    std::shared_ptr<GaugeLayer> AddWidgetByBuf(const unsigned char* buf, size_t len, bool show);
    std::shared_ptr<NeedleLayer> AddRotateWidgetByID(const std::string& id, int min, int max,
        int min_angle, int max_angle, bool clockwise, Point center);
    std::shared_ptr<NeedleLayer> AddRotateWidgetByBuf(const unsigned char* buf, size_t len, int min, int max,
        int min_angle, int max_angle, bool clockwise, Point center);
    std::shared_ptr<AnimationSequence> RotateAnimation(std::shared_ptr<NeedleLayer> widget, int min, int max,
        std::chrono::seconds length, const EasingFunc& easein, const EasingFunc& easeout);
    std::shared_ptr<egt::TextBox> find_text(const std::string& name) { return find_child<egt::TextBox>(name); }
    std::shared_ptr<egt::RectangleWidget> find_rec(const std::string& name) { return find_child<egt::RectangleWidget>(name); }
    std::shared_ptr<egt::experimental::GaugeLayer> find_layer(const std::string& name) { return find_child<egt::experimental::GaugeLayer>(name); }
    void add_text_widget(const std::string& id, const std::string& txt, const egt::Rect& rect, egt::Font::Size size);
    void add_text_widget(const std::string& id, const std::string& txt, const egt::Rect& rect, egt::Font::Size size, egt::Color color);
    void hide_all();
    bool is_point_in_line(egt::DefaultDim point, egt::DefaultDim start, egt::DefaultDim end);
    bool is_point_in_rect(egt::DisplayPoint& point, egt::Rect rect);
    ~SVGDeserial() noexcept {}

private:

};

}
}
}

#endif
