/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <egt/ui>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <random>
#include <chrono>
#include <queue>
#include <poll.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include "../src/detail/erawimage.h"
#include "../src/detail/eraw.h"


//#define SCREEN_480_X_480
#define DO_SVG_SERIALIZATION

#define MEDIA_FILE_PATH "/root/"
#define solve_relative_path(relative_path) (MEDIA_FILE_PATH + relative_path)

#define SCREEN_X_START         160
#define HIGH_Q_TIME_THRESHHOLD 30000  //30ms
#define LOW_Q_TIME_THRESHHOLD  20000  //20ms

enum class ClickType
{
    invalid = 0,
    blstart,
    slclosedark,
    slopendark,
    blclosedark,
    blopendark,
    cldowndark,
    clupdark,
    decdark,
    adddark
};

class MotorDash : public egt::experimental::Gauge
{
public:

    MotorDash() noexcept {}

#ifdef DO_SVG_SERIALIZATION
    void serialize_all();
#endif

    std::shared_ptr<egt::TextBox> find_text(const std::string& name)
    {
        return find_child<egt::TextBox>(name);
    }

    std::shared_ptr<egt::RectangleWidget> find_rec(const std::string& name)
    {
        return find_child<egt::RectangleWidget>(name);
    }

    std::shared_ptr<egt::experimental::GaugeLayer> find_layer(const std::string& name)
    {
        return find_child<egt::experimental::GaugeLayer>(name);
    }

    void set_text(const std::string& id, const std::string& text, int font_size, const egt::Pattern& color)
    {
        auto layer = find_rec(id);
        auto ptext = std::make_shared<egt::Label>();
        ptext->text_align(egt::AlignFlag::center);
        ptext->box(egt::Rect(layer->box().x(), layer->box().y(), layer->box().width(), layer->box().height()));
        ptext->color(egt::Palette::ColorId::label_text, color);
        ptext->font(egt::Font(font_size, egt::Font::Weight::normal));
        ptext->text(text);
        add(ptext);
    }

    void hide_all()
    {
        for (auto& child : m_children)
        {
            if (child->name().rfind("#path", 0) == 0)
                child->hide();
        }
    }

    egt::shared_cairo_surface_t DeSerialize(const std::string& filename);
    egt::shared_cairo_surface_t DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect);
    void ConvertInkscapeRect2EGT(std::shared_ptr<egt::Rect>& rect);

#ifdef DO_SVG_SERIALIZATION
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg);
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg, const std::string& id);
    void SerializePNG(const char* png_src, const std::string& png_dst);
#endif

private:

};

#ifdef DO_SVG_SERIALIZATION
void MotorDash::SerializeSVG(const std::string& filename, egt::SvgImage& svg)
{
    auto layer = std::make_shared<egt::Image>(svg);

    egt::detail::ErawImage e;
    const auto data = cairo_image_surface_get_data(layer->surface().get());
    const auto width = cairo_image_surface_get_width(layer->surface().get());
    const auto height = cairo_image_surface_get_height(layer->surface().get());
    std::cout << "wid: " << width
              << " ,het: " << height << std::endl;
    e.save(solve_relative_path(filename), data, width, height);
}

void MotorDash::SerializeSVG(const std::string& filename, egt::SvgImage& svg, const std::string& id)
{
    auto box = svg.id_box(id);
    auto layer = std::make_shared<egt::Image>(svg.render(id, box));

    egt::detail::ErawImage e;
    const auto data = cairo_image_surface_get_data(layer->surface().get());
    const auto width = cairo_image_surface_get_width(layer->surface().get());
    const auto height = cairo_image_surface_get_height(layer->surface().get());
    e.save(solve_relative_path(filename), data, box.x(), box.y(), width, height);
}

void MotorDash::SerializePNG(const char* png_src, const std::string& png_dst)
{
    egt::shared_cairo_surface_t surface;
    egt::detail::ErawImage e;
    surface =
            egt::shared_cairo_surface_t(cairo_image_surface_create_from_png(png_src),
                                        cairo_surface_destroy);
    const auto data = cairo_image_surface_get_data(surface.get());
    const auto width = cairo_image_surface_get_width(surface.get());
    const auto height = cairo_image_surface_get_height(surface.get());
    e.save(solve_relative_path(png_dst), data, 0, 0, width, height);
}

void MotorDash::serialize_all()
{
    //std::ostringstream str;
    //std::ostringstream path;

    egt::SvgImage svg_main("file:cheyi.svg", egt::SizeF(800, 480));

    //Serialize background image
    //SerializeSVG("eraw/fullimg.eraw", m_svg);

    //Serialize main.svg
    SerializeSVG("eraw/bkgrd.eraw", svg_main, "#bkgrd");
#if 0
    SerializeSVG("eraw/blrect.eraw", svg_main, "#blrect");

    SerializeSVG("eraw/slclosedark.eraw", svg_main, "#slclosedark");

    SerializeSVG("eraw/blclosedark.eraw", svg_main, "#blclosedark");

    SerializeSVG("eraw/blstart.eraw", svg_main, "#blstart");

    SerializeSVG("eraw/slopendark.eraw", svg_main, "#slopendark");

    SerializeSVG("eraw/blopendark.eraw", svg_main, "#blopendark");

    SerializeSVG("eraw/cldowndark.eraw", svg_main, "#cldowndark");

    SerializeSVG("eraw/clupdark.eraw", svg_main, "#clupdark");

    SerializeSVG("eraw/decdark.eraw", svg_main, "#decdark");

    SerializeSVG("eraw/adddark.eraw", svg_main, "#adddark");

    //Serialize cl.svg
    SerializeSVG("eraw/bk.eraw", svg_cl, "#bk");

    SerializeSVG("eraw/r1.eraw", svg_cl, "#r1");
#endif
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
#endif

egt::shared_cairo_surface_t MotorDash::DeSerialize(const std::string& filename)
{
    return egt::detail::ErawImage::load(solve_relative_path(filename));
}

egt::shared_cairo_surface_t MotorDash::DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect)
{
    return egt::detail::ErawImage::load(solve_relative_path(filename), rect);
}

//When drawing in Inkscape, the y is starting from bottom but not from top,
//so we need convert this coordinate to EGT using, the y is starting from top
void MotorDash::ConvertInkscapeRect2EGT(std::shared_ptr<egt::Rect>& rect)
{
    rect->y(screen()->size().height() - rect->y() - rect->height());
}

typedef struct
{
    egt::DefaultDim blstartX;
    egt::DefaultDim blrectX;
    egt::DefaultDim slclosedarkX;
    egt::DefaultDim blclosedarkX;
    egt::DefaultDim slopendarkX;
    egt::DefaultDim blopendarkX;
    egt::DefaultDim cldowndarkX;
    egt::DefaultDim clupdarkX;
    egt::DefaultDim decdarkX;
    egt::DefaultDim adddarkX;
} wgtRelativeX_t;

//using QueueCallback = std::function<void ()>;
using QueueCallback = std::function<std::string ()>;

int main(int argc, char** argv)
{
    std::cout << "EGT start" << std::endl;
    //int timediff = 0;
    //struct timeval time1, time2;
    //gettimeofday(&time1, NULL);

    std::queue<QueueCallback> high_pri_q;
    std::queue<QueueCallback> low_pri_q;
    ClickType clktype;

    //Widget handler
    std::shared_ptr<egt::experimental::GaugeLayer> fullimgPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> blstartPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> blrectPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> blclosedarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> blopendarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> slclosedarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> slopendarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> cldowndarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> clupdarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> decdarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> adddarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> bkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> r1Ptr;

    wgtRelativeX_t wgtRltvX = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    auto rect = std::make_shared<egt::Rect>();
    std::ostringstream str;
    std::ostringstream path;

    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;

    window.color(egt::Palette::ColorId::bg, egt::Palette::black);

#ifdef SCREEN_480_X_480
    egt::Window leftwin(egt::Rect(0, 0, SCREEN_X_START, 480));
    leftwin.color(egt::Palette::ColorId::bg, egt::Palette::black);
    egt::Window rightwin(egt::Rect(640, 0, SCREEN_X_START, 480));
    rightwin.color(egt::Palette::ColorId::bg, egt::Palette::black);
#endif

    MotorDash motordash;

#ifdef EXAMPLEDATA
    egt::add_search_path(EXAMPLEDATA);
#endif

#ifdef DO_SVG_SERIALIZATION
    //Check if serialize indicator "/serialize_done" exist? If not, need serialization
    if (access("/root/serialize_done", F_OK))
    {
        if (-1 == system("rm -rf /root/eraw"))
        {
            std::cout << "rm -rf /root/eraw failed, please check permission!!!" << std::endl;
            return -1;
        }

        if (0 > mkdir("/root/eraw", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            std::cout << "Create serialization dir eraw failed, please check permission!!!" << std::endl;
            return -1;
        }
        else
        {
            //If need serialization, show indicator for user on screen
            auto text = std::make_shared<egt::TextBox>("EGT is serializing, please wait...", egt::Rect(160, 190, 480, 200));
            text->border(0);
            text->font(egt::Font(50, egt::Font::Weight::normal));
            text->color(egt::Palette::ColorId::bg, egt::Palette::transparent);
            text->color(egt::Palette::ColorId::text, egt::Palette::red);
            window.add(text);
            app.event().step();
            motordash.serialize_all();
            text->clear();
            text->text("Serialize successfully, welcome!");
            app.event().step();
            sleep(1);
            text->hide();
        }
    }
#endif

    window.add(motordash);
    motordash.show();

#ifdef SCREEN_480_X_480
    window.add(leftwin);
    window.add(rightwin);
    leftwin.show();
    rightwin.show();
#endif

    window.show();

    fullimgPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/bkgrd.eraw")));
    fullimgPtr->x(fullimgPtr->x() + SCREEN_X_START);
    fullimgPtr->show();
    motordash.add(fullimgPtr);

    blstartPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blstart.eraw", rect)));
    blstartPtr->box(*rect);
    wgtRltvX.blstartX = blstartPtr->x();
    blstartPtr->x(blstartPtr->x() + SCREEN_X_START);
    blstartPtr->hide();
    motordash.add(blstartPtr);

    blrectPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blrect.eraw", rect)));
    blrectPtr->box(*rect);
    wgtRltvX.blrectX = blrectPtr->x();
    blrectPtr->x(blrectPtr->x() + SCREEN_X_START);
    blrectPtr->hide();
    motordash.add(blrectPtr);

    slclosedarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/slclosedark.eraw", rect)));
    slclosedarkPtr->box(*rect);
    wgtRltvX.slclosedarkX = slclosedarkPtr->x();
    slclosedarkPtr->x(slclosedarkPtr->x() + SCREEN_X_START);
    slclosedarkPtr->show();
    motordash.add(slclosedarkPtr);

    slopendarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/slopendark.eraw", rect)));
    slopendarkPtr->box(*rect);
    wgtRltvX.slopendarkX = slopendarkPtr->x();
    slopendarkPtr->x(slopendarkPtr->x() + SCREEN_X_START);
    slopendarkPtr->show();
    motordash.add(slopendarkPtr);

    blclosedarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blclosedark.eraw", rect)));
    blclosedarkPtr->box(*rect);
    wgtRltvX.blclosedarkX = blclosedarkPtr->x();
    blclosedarkPtr->x(blclosedarkPtr->x() + SCREEN_X_START);
    blclosedarkPtr->show();
    motordash.add(blclosedarkPtr);

    blopendarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blopendark.eraw", rect)));
    blopendarkPtr->box(*rect);
    wgtRltvX.blopendarkX = blopendarkPtr->x();
    blopendarkPtr->x(blopendarkPtr->x() + SCREEN_X_START);
    blopendarkPtr->show();
    motordash.add(blopendarkPtr);

    cldowndarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/cldowndark.eraw", rect)));
    cldowndarkPtr->box(*rect);
    wgtRltvX.cldowndarkX = cldowndarkPtr->x();
    cldowndarkPtr->x(cldowndarkPtr->x() + SCREEN_X_START);
    cldowndarkPtr->show();
    motordash.add(cldowndarkPtr);

    clupdarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/clupdark.eraw", rect)));
    clupdarkPtr->box(*rect);
    wgtRltvX.clupdarkX = clupdarkPtr->x();
    clupdarkPtr->x(clupdarkPtr->x() + SCREEN_X_START);
    clupdarkPtr->show();
    motordash.add(clupdarkPtr);

    decdarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/decdark.eraw", rect)));
    decdarkPtr->box(*rect);
    wgtRltvX.decdarkX = decdarkPtr->x();
    decdarkPtr->x(decdarkPtr->x() + SCREEN_X_START);
    decdarkPtr->show();
    motordash.add(decdarkPtr);

    adddarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/adddark.eraw", rect)));
    adddarkPtr->box(*rect);
    wgtRltvX.adddarkX = adddarkPtr->x();
    adddarkPtr->x(adddarkPtr->x() + SCREEN_X_START);
    adddarkPtr->show();
    motordash.add(adddarkPtr);

    bkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/bk.eraw", rect)));
    bkPtr->box(*rect);
    bkPtr->hide();
    motordash.add(bkPtr);

    r1Ptr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/r1.eraw", rect)));
    r1Ptr->box(*rect);
    r1Ptr->hide();
    motordash.add(r1Ptr);

    egt::Timer click_timer(std::chrono::milliseconds(300));

    blstartPtr->on_event([&](egt::Event&)
    {
        if (blstartPtr->visible())
            blstartPtr->hide();
        else
            blstartPtr->show();
    }, {egt::EventId::raw_pointer_down});

    blrectPtr->on_event([&](egt::Event&)
    {
        bkPtr->show();
        r1Ptr->show();
    }, {egt::EventId::raw_pointer_down});

    bkPtr->on_event([&](egt::Event&)
    {
        r1Ptr->width(r1Ptr->width() + 2);
    }, {egt::EventId::raw_pointer_down});

    blclosedarkPtr->on_event([&](egt::Event&)
    {
        blclosedarkPtr->hide();
        clktype = ClickType::blclosedark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    blopendarkPtr->on_event([&](egt::Event&)
    {
        blopendarkPtr->hide();
        clktype = ClickType::blopendark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    slclosedarkPtr->on_event([&](egt::Event&)
    {
        slclosedarkPtr->hide();
        clktype = ClickType::slclosedark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    slopendarkPtr->on_event([&](egt::Event&)
    {
        slopendarkPtr->hide();
        clktype = ClickType::slopendark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    cldowndarkPtr->on_event([&](egt::Event&)
    {
        cldowndarkPtr->hide();
        clktype = ClickType::cldowndark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    clupdarkPtr->on_event([&](egt::Event&)
    {
        clupdarkPtr->hide();
        clktype = ClickType::clupdark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    decdarkPtr->on_event([&](egt::Event&)
    {
        decdarkPtr->hide();
        clktype = ClickType::decdark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    adddarkPtr->on_event([&](egt::Event&)
    {
        adddarkPtr->hide();
        clktype = ClickType::adddark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    //Animation of main page
    /***********************************************************
    * -800    -320     160     640
    *                current
	*         left1
    * left2
    *         right1
	*                right2
    ************************************************************/
    auto wgtMove = [&](egt::PropertyAnimator::Value value)
    {
        fullimgPtr->x(value);
        blstartPtr->x(wgtRltvX.blstartX + fullimgPtr->x());
        blrectPtr->x(wgtRltvX.blrectX + fullimgPtr->x());
        slclosedarkPtr->x(wgtRltvX.slclosedarkX + fullimgPtr->x());
        slopendarkPtr->x(wgtRltvX.slopendarkX + fullimgPtr->x());
        blclosedarkPtr->x(wgtRltvX.blclosedarkX + fullimgPtr->x());
        blopendarkPtr->x(wgtRltvX.blopendarkX + fullimgPtr->x());
        cldowndarkPtr->x(wgtRltvX.cldowndarkX + fullimgPtr->x());
        clupdarkPtr->x(wgtRltvX.clupdarkX + fullimgPtr->x());
        decdarkPtr->x(wgtRltvX.decdarkX + fullimgPtr->x());
        adddarkPtr->x(wgtRltvX.adddarkX + fullimgPtr->x());
    };

    egt::EasingFunc ease = egt::easing_quintic_easeinout;
    std::chrono::milliseconds duration = std::chrono::milliseconds(500);
    auto animationleft1 = std::make_shared<egt::PropertyAnimator>(SCREEN_X_START, -320, duration, ease);
    animationleft1->on_change([&](egt::PropertyAnimator::Value value) { wgtMove(value); });

    auto animationleft2 = std::make_shared<egt::PropertyAnimator>(-320, -800, duration, ease);
    animationleft2->on_change([&](egt::PropertyAnimator::Value value) { wgtMove(value); });

    auto animationright1 = std::make_shared<egt::PropertyAnimator>(-800, -320, duration, ease);
    animationright1->on_change([&](egt::PropertyAnimator::Value value) { wgtMove(value); });

    auto animationright2 = std::make_shared<egt::PropertyAnimator>(-320, SCREEN_X_START, duration, ease);
    animationright2->on_change([&](egt::PropertyAnimator::Value value) { wgtMove(value); });

    auto handle_touch = [&](egt::Event & event)
    {
        switch (event.id())
        {
            case egt::EventId::pointer_click:
                break;
            case egt::EventId::keyboard_up:
                break;
            case egt::EventId::pointer_drag_start:
                if (event.pointer().point.x() < event.pointer().drag_start.x()) //left move
                {
                    if (SCREEN_X_START == fullimgPtr->x())
                        animationleft1->start();
                    else if (-320 == fullimgPtr->x())
                        animationleft2->start();
                }
                else if (event.pointer().point.x() > event.pointer().drag_start.x()) //right move
                {
                    if (-800 == fullimgPtr->x())
                        animationright1->start();
                    else if (-320 == fullimgPtr->x())
                        animationright2->start();
                }
                break;
            case egt::EventId::pointer_drag_stop:
                break;
            break;
            case egt::EventId::pointer_drag:
            {
                break;
            }
            default:
                break;
        }
    };
    window.on_event(handle_touch);

    click_timer.on_timeout([&]()
    {
        switch (clktype)
        {
            case ClickType::blclosedark:
                blclosedarkPtr->show();
                break;
            case ClickType::blopendark:
                blopendarkPtr->show();
                break;
            case ClickType::slclosedark:
                slclosedarkPtr->show();
                break;
            case ClickType::slopendark:
                slopendarkPtr->show();
                break;
            case ClickType::cldowndark:
                cldowndarkPtr->show();
                break;
            case ClickType::clupdark:
                clupdarkPtr->show();
                break;
            case ClickType::decdark:
                decdarkPtr->show();
                break;
            case ClickType::adddark:
                adddarkPtr->show();
                break;
            default:
               break;
        }
        clktype = ClickType::invalid;
        click_timer.stop();
    });

    return app.run();
}
