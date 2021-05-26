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

//#define HAVE_LIBPLANES

#ifdef HAVE_LIBPLANES
#include <planes/plane.h>
#include "egt/detail/screen/kmsoverlay.h"
#endif
#include "../src/detail/erawimage.h"
#include "../src/detail/eraw.h"


#define DO_SVG_SERIALIZATION
//#define CAN_DEBUG

#define MEDIA_FILE_PATH "~/src/"
#define solve_relative_path(relative_path) (MEDIA_FILE_PATH + relative_path)

#ifdef HAVE_LIBPLANES
#define SCALE_TO_MAX_AND_STAY    //If this macro defined, scale animation will scale from min to max and stay, then enter to demo.
                                 //Else if not defined, scale animation will scale from min to max, and then scale to min, after that enter to demo.
#endif

#define HIGH_Q_TIME_THRESHHOLD 30000  //30ms
#define LOW_Q_TIME_THRESHHOLD  20000  //20ms
//#define EGT_STATISTICS_ENABLE

enum class SvgEleType
{
    path = 0,
    text,
    bar,
    rbar
};

class MotorDash : public egt::experimental::Gauge
{
public:

#ifdef DO_SVG_SERIALIZATION
    MotorDash(egt::SvgImage& svg) noexcept: m_svg(svg)
#else
    MotorDash() noexcept
#endif
    {
        set_gear_deserial_state(false);
        set_fuel_deserial_state(false);
        set_temp_deserial_state(false);
        set_needle_deserial_state(false);
        set_speed_deserial_state(false);
        set_call_deserial_state(false);
        set_bar_deserial_state(false);
        set_blink_deserial_state(false);
        set_tc_deserial_state(false);
        set_vsc_deserial_state(false);
        set_bat_deserial_state(false);
        set_snow_deserial_state(false);
        set_high_q_state(true);
        set_low_q_state(true);
        set_wifi_deserial_state(false);
    }

#ifdef DO_SVG_SERIALIZATION
    void serialize_all();
#endif

    void test()
    {
        auto text = std::make_shared<egt::TextBox>("5", egt::Rect(egt::Point(82, 1), egt::Size(20, 2)));
        text->border(0);
        text->font(egt::Font(40, egt::Font::Weight::normal));
        text->color(egt::Palette::ColorId::bg, egt::Palette::transparent);
        text->color(egt::Palette::ColorId::text, egt::Palette::white);
        add(text);
    }

    void add_text_widget(const std::string& id, const std::string& txt, const egt::Rect& rect, egt::Font::Size size)
    {
        auto text = std::make_shared<egt::TextBox>(txt, rect, egt::AlignFlag::center);
        text->border(0);
        text->font(egt::Font(size, egt::Font::Weight::normal));
        text->color(egt::Palette::ColorId::bg, egt::Palette::transparent);
        text->color(egt::Palette::ColorId::text, egt::Palette::white);
        add(text);
        text->name(id);
    }

    void add_text_widget(const std::string& id, const std::string& txt, const egt::Rect& rect, egt::Font::Size size, egt::Color color)
    {
        auto text = std::make_shared<egt::TextBox>(txt, rect);
        text->border(0);
        text->font(egt::Font(size, egt::Font::Weight::normal));
        text->color(egt::Palette::ColorId::bg, egt::Palette::transparent);
        text->color(egt::Palette::ColorId::text, color);
        add(text);
        text->name(id);
    }

#ifdef DO_SVG_SERIALIZATION
    void add_rec_widget(const std::string& id, bool is_hiding)
    {
        if (!m_svg.id_exists(id))
            return;

        auto box = m_svg.id_box(id);
        auto rec = std::make_shared<egt::RectangleWidget>(egt::Rect(std::floor(box.x()),
                                                                    std::floor(box.y()),
                                                                    std::ceil(box.width()),
                                                                    std::ceil(box.height())));
        rec->color(egt::Palette::ColorId::button_bg, egt::Palette::black);
        if (is_hiding)
            rec->hide();
        add(rec);
        rec->name(id);
    }
#endif

    void add_rec_widget_with_rec(const std::string& id, const egt::Rect& rect, bool is_hiding)
    {
        auto rec = std::make_shared<egt::RectangleWidget>(rect);
        rec->color(egt::Palette::ColorId::button_bg, egt::Palette::black);
        if (is_hiding)
            rec->hide();
        add(rec);
        rec->name(id);
    }

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


    void apply(int digit, bool is_inc, SvgEleType type)
    {
        //std::cout << "apply: " << digit << "  type:" << static_cast<int>(type) << "  inc:" << is_inc << std::endl;
        std::ostringstream ss;
        switch (type)
        {
            case SvgEleType::path:
                ss << "#path" << std::to_string(digit);
                break;
            case SvgEleType::text:
                ss << "#text" << std::to_string(digit);
                find_layer(ss.str())->show();
                return;
            case SvgEleType::bar:
                ss << "#bar" << std::to_string(digit);
                find_layer(ss.str())->show();
                return;
            case SvgEleType::rbar:
                ss << "#rbar" << std::to_string(digit);
                find_layer(ss.str())->show();
                return;
            default:
                break;
        }

        if (is_inc)
        {
            //std::cout << "show: " << ss.str() << std::endl;
            find_layer(ss.str())->show();
        }
        else
        {
            //std::cout << "hide: " << ss.str() << std::endl;
            find_layer(ss.str())->hide();
        }
    }

    void hide_all()
    {
        for (auto& child : m_children)
        {
            if (child->name().rfind("#path", 0) == 0)
                child->hide();
        }
    }

    bool get_high_q_state() { return m_is_high_q_quit; }
    void set_high_q_state(bool is_high_q_quit) { m_is_high_q_quit = is_high_q_quit; }

    bool get_low_q_state() { return m_is_low_q_quit; }
    void set_low_q_state(bool is_low_q_quit) { m_is_low_q_quit = is_low_q_quit; }

    bool get_gear_deserial_state() { return m_gear_deserial_finish; }
    void set_gear_deserial_state(bool gear_deserial_finish) { m_gear_deserial_finish = gear_deserial_finish; }

    bool get_fuel_deserial_state() { return m_fuel_deserial_finish; }
    void set_fuel_deserial_state(bool fuel_deserial_finish) { m_fuel_deserial_finish = fuel_deserial_finish; }

    bool get_temp_deserial_state() { return m_temp_deserial_finish; }
    void set_temp_deserial_state(bool temp_deserial_finish) { m_temp_deserial_finish = temp_deserial_finish; }

    bool get_needle_deserial_state() { return m_needle_deserial_finish; }
    void set_needle_deserial_state(bool needle_deserial_finish) { m_needle_deserial_finish = needle_deserial_finish; }

    bool get_speed_deserial_state() { return m_speed_deserial_finish; }
    void set_speed_deserial_state(bool speed_deserial_finish) { m_speed_deserial_finish = speed_deserial_finish; }

    bool get_call_deserial_state() { return m_call_deserial_finish; }
    void set_call_deserial_state(bool call_deserial_finish) { m_call_deserial_finish = call_deserial_finish; }

    bool get_bar_deserial_state() { return m_bar_deserial_finish; }
    void set_bar_deserial_state(bool bar_deserial_finish) { m_bar_deserial_finish = bar_deserial_finish; }

    bool get_blink_deserial_state() { return m_blink_deserial_finish; }
    void set_blink_deserial_state(bool blink_deserial_finish) { m_blink_deserial_finish = blink_deserial_finish; }

    bool get_tc_deserial_state() { return m_tc_deserial_finish; }
    void set_tc_deserial_state(bool tc_deserial_finish) { m_tc_deserial_finish = tc_deserial_finish; }

    bool get_vsc_deserial_state() { return m_vsc_deserial_finish; }
    void set_vsc_deserial_state(bool vsc_deserial_finish) { m_vsc_deserial_finish = vsc_deserial_finish; }

    bool get_bat_deserial_state() { return m_bat_deserial_finish; }
    void set_bat_deserial_state(bool bat_deserial_finish) { m_bat_deserial_finish = bat_deserial_finish; }

    bool get_snow_deserial_state() { return m_snow_deserial_finish; }
    void set_snow_deserial_state(bool snow_deserial_finish) { m_snow_deserial_finish = snow_deserial_finish; }

    bool get_wifi_deserial_state() { return m_wifi_deserial_finish; }
    void set_wifi_deserial_state(bool wifi_deserial_finish) { m_wifi_deserial_finish = wifi_deserial_finish; }

    egt::shared_cairo_surface_t DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect);
    void ConvertInkscapeRect2EGT(std::shared_ptr<egt::Rect>& rect);

#ifdef DO_SVG_SERIALIZATION
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg);
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg, const std::string& id);
    void SerializePNG(const char* png_src, const std::string& png_dst);
    std::shared_ptr<egt::ImageButton> add_svg_layer(const std::string& ss, bool is_hiding)
    {
        if (nullptr != find_layer(ss))
            return nullptr;
        auto layer = std::make_shared<egt::ImageButton>(m_svg.render(ss));
        if (is_hiding)
            layer->hide();
        add(layer);
        return layer;
        //std::cout << "render widget:" << ss << std::endl;
    }
#endif

private:
#ifdef DO_SVG_SERIALIZATION
    egt::SvgImage& m_svg;
#endif
    //egt::ImageButton m_blclose;
    bool m_is_high_q_quit;
    bool m_is_low_q_quit;
    bool m_gear_deserial_finish;
    bool m_fuel_deserial_finish;
    bool m_temp_deserial_finish;
    bool m_needle_deserial_finish;
    bool m_speed_deserial_finish;
    bool m_call_deserial_finish;
    bool m_bar_deserial_finish;
    bool m_blink_deserial_finish;
    bool m_tc_deserial_finish;
    bool m_vsc_deserial_finish;
    bool m_bat_deserial_finish;
    bool m_snow_deserial_finish;
    bool m_wifi_deserial_finish;
};


class DeserializeDash
{
public:
    DeserializeDash() noexcept {}
    egt::shared_cairo_surface_t DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect);
    ~DeserializeDash() {}
};

#ifdef DO_SVG_SERIALIZATION
void MotorDash::SerializeSVG(const std::string& filename, egt::SvgImage& svg)
{
    auto layer = std::make_shared<egt::Image>(svg);

    egt::detail::ErawImage e;
    const auto data = cairo_image_surface_get_data(layer->surface().get());
    const auto width = cairo_image_surface_get_width(layer->surface().get());
    const auto height = cairo_image_surface_get_height(layer->surface().get());
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

    //Serialize background image
    //SerializeSVG("eraw/fullimg.eraw", m_svg);

    SerializeSVG("eraw/blclose.eraw", m_svg, "#blclose");

    SerializeSVG("eraw/blclosedark.eraw", m_svg, "#blclosedark");

    SerializeSVG("eraw/blpause.eraw", m_svg, "#blpause");

    SerializeSVG("eraw/blopen.eraw", m_svg, "#blopen");

    SerializeSVG("eraw/blopendark.eraw", m_svg, "#blopendark");


    //Create a finish indicator
    if (-1 == system("touch ~/src/serialize_done"))
    {
        std::cout << "touch ~/src/serialize_done failed, please check permission!!!" << std::endl;
        return;
    }
    if (-1 == system("sync"))
    {
        std::cout << "sync failed, please check permission!!!" << std::endl;
        return;
    }
}
#endif

egt::shared_cairo_surface_t DeserializeDash::DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect)
{
    return egt::detail::ErawImage::load(filename, rect);
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

#ifdef CAN_DEBUG
    //Communication by CAN bus
    struct sockaddr_can addr;
    int sockfd;
    int family = PF_CAN, type = SOCK_RAW, proto = CAN_RAW;
    struct pollfd pollfds;
    ssize_t n;
    struct can_frame frame;
#endif

    //Widget handler
    std::shared_ptr<egt::experimental::GaugeLayer> fullimgPtr;
    std::shared_ptr<egt::ImageButton> blclosePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> blpausePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> blopenPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> blclosedarkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> blopendarkPtr;

    auto rect = std::make_shared<egt::Rect>();
    std::ostringstream str;
    std::ostringstream path;

#ifdef DO_SVG_SERIALIZATION
    bool need_serialization = false;
#endif

#ifdef HAVE_LIBPLANES
    //Scale effect variables
    float scale_factor = 0.01;
    bool is_scale_rev = false;
    bool is_scale_2_max = false;
    bool is_scale_finish = false;
#endif

    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;

    window.color(egt::Palette::ColorId::bg, egt::Palette::black);
    egt::Window leftwin(egt::Rect(0, 0, 160, 480));
    leftwin.color(egt::Palette::ColorId::bg, egt::Palette::black);
    egt::Window rightwin(egt::Rect(640, 0, 160, 480));
    rightwin.color(egt::Palette::ColorId::bg, egt::Palette::black);
#ifdef EXAMPLEDATA
    egt::add_search_path(EXAMPLEDATA);
#endif

#ifdef DO_SVG_SERIALIZATION
    //Check if serialize indicator "/serialize_done" exist? If not, need serialization
    if (access("~/src/serialize_done", F_OK))
    {
        /*
        if (-1 == system("rm -rf ~/src/eraw"))
        {
            std::cout << "rm -rf ~/src/eraw failed, please check permission!!!" << std::endl;
            return -1;
        }
        if (0 > mkdir("~/src/eraw", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
        {
            std::cout << "Create serialization dir eraw failed, please check permission!!!" << std::endl;
            return -1;
        }
        else
        */
            need_serialization = true;
    }

    //If need serialization, use serialize.svg to parse SVG parameters, else use a blank one
    std::string svgpath = need_serialization ? "file:shm.svg" : "file:deserialize.svg";
    auto svg = std::make_unique<egt::SvgImage>(svgpath, egt::SizeF(window.content_area().width(), 0));
    MotorDash motordash(*svg);
#else
    MotorDash motordash;
#endif

    fullimgPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::SvgImage("file:shm.svg", egt::SizeF(1440, 480)));
    fullimgPtr->move(egt::Point(160, 0));
    fullimgPtr->show();
    window.add(fullimgPtr);

/*
    egt::SideBoard board(egt::SideBoard::PositionFlag::right);
    board.color(egt::Palette::ColorId::bg, egt::Palette::transparent);
    board.add(fullimgPtr);
    window.add(board);
    board.show();
*/
    //window.add(motordash);
    //motordash.show();
    window.add(leftwin);
    window.add(rightwin);
    leftwin.show();
    rightwin.show();
    window.show();

    /***********************************************************
    * -800    -320     160     640
    *                current
	*         left1
    * left2
    *         right1
	*                right2
    ************************************************************/
    egt::EasingFunc ease = egt::easing_quintic_easeinout;
    std::chrono::milliseconds duration = std::chrono::seconds(1);
    auto animationleft1 = std::make_shared<egt::PropertyAnimator>(160, -320, duration, ease);
    animationleft1->on_change([&](egt::PropertyAnimator::Value value) { fullimgPtr->move(egt::Point(value, 0)); });

    auto animationleft2 = std::make_shared<egt::PropertyAnimator>(-320, -800, duration, ease);
    animationleft2->on_change([&](egt::PropertyAnimator::Value value) { fullimgPtr->move(egt::Point(value, 0)); });

    auto animationright1 = std::make_shared<egt::PropertyAnimator>(-800, -320, duration, ease);
    animationright1->on_change([&](egt::PropertyAnimator::Value value) { fullimgPtr->move(egt::Point(value, 0)); });

    auto animationright2 = std::make_shared<egt::PropertyAnimator>(-320, 160, duration, ease);
    animationright2->on_change([&](egt::PropertyAnimator::Value value) { fullimgPtr->move(egt::Point(value, 0)); });

#ifdef KDJFA //DO_SVG_SERIALIZATION
    if (need_serialization)
    {
        //If need serialization, show indicator for user on screen
        auto text = std::make_shared<egt::TextBox>("EGT is serializing, please wait...", egt::Rect(70, 190, 700, 200));
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
#endif
/*
    fullimgPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/fullimg.eraw", rect)));
    fullimgPtr->box(*rect);
    fullimgPtr->show();
    motordash.add(fullimgPtr);

    blclosePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blclose.eraw", rect)));
    blclosePtr->box(*rect);
    blclosePtr->show();
    motordash.add(blclosePtr);

    blpausePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blpause.eraw", rect)));
    blpausePtr->box(*rect);
    blpausePtr->show();
    motordash.add(blpausePtr);

    blopenPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blopen.eraw", rect)));
    blopenPtr->box(*rect);
    blopenPtr->show();
    motordash.add(blopenPtr);

    blclosedarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blclosedark.eraw", rect)));
    blclosedarkPtr->box(*rect);
    blclosedarkPtr->show();
    motordash.add(blclosedarkPtr);

    blopendarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/blopendark.eraw", rect)));
    blopendarkPtr->box(*rect);
    blopendarkPtr->show();
    motordash.add(blopendarkPtr);
*/

    blclosePtr = motordash.add_svg_layer("#blclose", true);

    blclosePtr->on_click([&](egt::Event&)
    {
        std::cout << "user click" << std::endl;
        if (blclosePtr->visible())
        {
            std::cout << "now is visible" << std::endl;
            blclosePtr->hide();
        }
        else
        {
            std::cout << "now is unvisible" << std::endl;
            blclosePtr->show();
        }
    });

    //blclosedarkPtr = motordash.add_svg_layer("#blclosedark", false);
    //blpausePtr = motordash.add_svg_layer("#blpause", false);
    //blopenPtr = motordash.add_svg_layer("#blopen", false);
    //blopendarkPtr = motordash.add_svg_layer("#blopendark", false);

    auto handle_touch = [&](egt::Event & event)
    {
        switch (event.id())
        {
            case egt::EventId::pointer_click:
                break;
            case egt::EventId::keyboard_up:
            {
                break;
            }
            case egt::EventId::pointer_drag_start:
                if (event.pointer().point.x() < event.pointer().drag_start.x()) //left move
                {
                    if (160 == fullimgPtr->x())
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

    return app.run();
}
