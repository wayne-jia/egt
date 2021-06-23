/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <iostream>
#include "egt/svgdeserial.h"
#include "egt/text.h"
#include "detail/erawimage.h"
#include "detail/eraw.h"

namespace egt
{
inline namespace v1
{
namespace experimental
{

SVGDeserial::SVGDeserial(Application& app, TopWindow& parent, int svg_cnt, const char** svg_files) noexcept
{
    //Check if serialize indicator "/serialize_done" exist? If not, need serialization
    if (access("/root/serialize_done", F_OK))
    {
        if (-1 == system("rm -rf /root/eraw"))
        {
            std::cout << "rm -rf /root/eraw failed, please check permission!!!" << std::endl;
            return;
        }

        if (0 > mkdir("/root/eraw", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            std::cout << "Create serialization dir eraw failed, please check permission!!!" << std::endl;
            return;
        }
        else
        {
            //If need serialization, show indicator for user on screen
            auto text = std::make_shared<TextBox>("EGT is serializing, please wait...", Rect(70, 190, 700, 200));
            text->border(0);
            text->font(Font(50, Font::Weight::normal));
            text->color(Palette::ColorId::bg, Palette::transparent);
            text->color(Palette::ColorId::text, Palette::red);
            parent.add(text);
            app.event().step();
            std::string str;
            for (auto i = 0; i < svg_cnt; i++)
            {
                str += "/usr/bin/egt_svgconvertor " + std::string(svg_files[i]);
                std::cout << "Serialize[" << i << "]: " << str.c_str() << std::endl;
                if (-1 == system(str.c_str()))
                {
                    std::cout << str.c_str() << " abnormal!" << std::endl;
                    return;
                }
            }
            text->clear();
            text->text("Serialize successfully, welcome!");
            app.event().step();
            sleep(1);
            text->hide();
            //Create a finish indicator
            if (-1 == system("touch /root/serialize_done"))
            {
                std::cout << "touch /root/serialize_done failed, please check permission!!!" << std::endl;
                return;
            }
            if (-1 == system("sync"))
            {
                std::cout << "sync failed, please check permission!!!" << std::endl;
                return;
            }
        }
    }
    parent.add(*this);
}

SVGDeserial::SVGDeserial(TopWindow& parent) noexcept
{
    parent.add(*this);
}

shared_cairo_surface_t SVGDeserial::DeSerialize(const std::string& filename, std::shared_ptr<Rect>& rect)
{
    return detail::ErawImage::load(filename, rect);
}

std::shared_ptr<GaugeLayer> SVGDeserial::AddWidgetByID(const std::string& id, bool show)
{
    auto rect = std::make_shared<Rect>();
    std::shared_ptr<GaugeLayer> widget;
    widget = std::make_shared<GaugeLayer>(Image(DeSerialize(id, rect)));
    widget->box(*rect);
    if (show)
        widget->show();
    else
        widget->hide();
    add(widget);
    return widget;
}

std::shared_ptr<NeedleLayer> SVGDeserial::AddRotateWidgetByID(const std::string& id, int min, int max,
        int min_angle, int max_angle, bool clockwise, Point center)
{
    auto rect = std::make_shared<Rect>();
    std::shared_ptr<NeedleLayer> widget;
    widget = std::make_shared<NeedleLayer>(Image(DeSerialize(id, rect)), min, max, min_angle, max_angle, clockwise);
    widget->box(*rect);
    widget->needle_center(center - widget->box().point());
    widget->needle_point(center);
    widget->show();
    add(widget);
    return widget;
}

std::shared_ptr<AnimationSequence> SVGDeserial::RotateAnimation(std::shared_ptr<NeedleLayer> widget, int min, int max,
    std::chrono::seconds length, const EasingFunc& easein, const EasingFunc& easeout)
{
    auto animationup =
        std::make_shared<PropertyAnimator>(min, max, length, easein);
    animationup->on_change([widget](PropertyAnimator::Value v)
    {
        widget->value(v);
    });

    auto animationdown =
        std::make_shared<PropertyAnimator>(max, min, length, easeout);
    animationdown->on_change([widget](PropertyAnimator::Value v)
    {
        widget->value(v);
    });

    auto sequence = std::make_shared<AnimationSequence>(true);
    sequence->add(animationup);
    sequence->add(animationdown);
    sequence->start();
    return sequence;
}

}
}
}
