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


#define DO_SVG_SERIALIZATION

#define MEDIA_FILE_PATH "/root/"
#define solve_relative_path(relative_path) (MEDIA_FILE_PATH + relative_path)

#define BAR_RASTER_WIDTH       18
#define HIGH_Q_TIME_THRESHHOLD 30000  //30ms
#define LOW_Q_TIME_THRESHHOLD  20000  //20ms


class MotorDash : public egt::experimental::Gauge
{
public:

    MotorDash() noexcept
    {
        set_high_q_state(true);
        set_low_q_state(true);
        set_bar_state(false);
        set_left_icon_state(false);
        set_top_icon_state(false);
        set_right_icon_state(false);
        set_top_text_state(false);
        set_middle_text_state(false);
        set_botom_text_state(false);
        set_swth_state(false);
    }

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

    void hide_lpres(std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> LpresBase)
    {
        for (int i = 0; i <= 12; i++)
        {
            LpresBase[i]->hide();
        }
    }

    void hide_rpres(std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> RpresBase)
    {
        for (int i = 0; i <= 12; i++)
        {
            RpresBase[i]->hide();
        }
    }

    bool get_high_q_state() { return m_is_high_q_quit; }
    void set_high_q_state(bool is_high_q_quit) { m_is_high_q_quit = is_high_q_quit; }

    bool get_low_q_state() { return m_is_low_q_quit; }
    void set_low_q_state(bool is_low_q_quit) { m_is_low_q_quit = is_low_q_quit; }

    bool get_bar_state() { return m_bar_finish; }
    void set_bar_state(bool bar_finish) { m_bar_finish = bar_finish; }

    bool get_left_icon_state() { return m_left_icon_finish; }
    void set_left_icon_state(bool left_icon_finish) { m_left_icon_finish = left_icon_finish; }

    bool get_top_icon_state() { return m_top_icon_finish; }
    void set_top_icon_state(bool top_icon_finish) { m_top_icon_finish = top_icon_finish; }

    bool get_right_icon_state() { return m_right_icon_finish; }
    void set_right_icon_state(bool right_icon_finish) { m_right_icon_finish = right_icon_finish; }

    bool get_top_text_state() { return m_top_text_finish; }
    void set_top_text_state(bool top_text_finish) { m_top_text_finish = top_text_finish; }

    bool get_middle_text_state() { return m_middle_text_finish; }
    void set_middle_text_state(bool middle_text_finish) { m_middle_text_finish = middle_text_finish; }

    bool get_botom_text_state() { return m_botom_text_finish; }
    void set_botom_text_state(bool botom_text_finish) { m_botom_text_finish = botom_text_finish; }

    bool get_swth_state() { return m_swth_finish; }
    void set_swth_state(bool swth_finish) { m_swth_finish = swth_finish; }

    egt::shared_cairo_surface_t DeSerialize(const std::string& filename);
    egt::shared_cairo_surface_t DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect);
    void ConvertInkscapeRect2EGT(std::shared_ptr<egt::Rect>& rect);
    bool is_point_in_range(egt::DefaultDim point, egt::DefaultDim start, egt::DefaultDim end);

#ifdef DO_SVG_SERIALIZATION
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg);
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg, const std::string& id);
    void SerializePNG(const char* png_src, const std::string& png_dst);
#endif

private:
    bool m_is_high_q_quit;
    bool m_is_low_q_quit;
    bool m_bar_finish;
    bool m_left_icon_finish;
    bool m_top_icon_finish;
    bool m_right_icon_finish;
    bool m_top_text_finish;
    bool m_middle_text_finish;
    bool m_botom_text_finish;
    bool m_swth_finish;
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
    int i;
    std::ostringstream str;
    std::ostringstream path;

    egt::SvgImage svg_main("file:cheyi.svg", egt::SizeF(800, 480));

    for (i = 0; i <= 12; i++)
    {
        str.str("");
        path.str("");
        str << "#lpres" << std::to_string(i);
        path << "eraw/lpres" << std::to_string(i) << ".eraw";
        SerializeSVG(path.str(), svg_main, str.str());
    }

    for (i = 0; i <= 12; i++)
    {
        str.str("");
        path.str("");
        str << "#rpres" << std::to_string(i);
        path << "eraw/rpres" << std::to_string(i) << ".eraw";
        SerializeSVG(path.str(), svg_main, str.str());
    }

    //Serialize main.svg
    SerializeSVG("eraw/bkgrd.eraw", svg_main, "#bkgrd");

    SerializeSVG("eraw/fuel.eraw", svg_main, "#fuel");

    SerializeSVG("eraw/tempbar.eraw", svg_main, "#tempbar");

    SerializeSVG("eraw/mainspd.eraw", svg_main, "#mainspd");

    SerializeSVG("eraw/time.eraw", svg_main, "#time");

    SerializeSVG("eraw/bardigit.eraw", svg_main, "#bardigit");

    SerializeSVG("eraw/gear.eraw", svg_main, "#gear");

    SerializeSVG("eraw/voltage.eraw", svg_main, "#voltage");

    SerializeSVG("eraw/date.eraw", svg_main, "#date");

    SerializeSVG("eraw/lpresdigit.eraw", svg_main, "#lpresdigit");

    SerializeSVG("eraw/rpresdigit.eraw", svg_main, "#rpresdigit");

    SerializeSVG("eraw/odo.eraw", svg_main, "#odo");

    SerializeSVG("eraw/trip.eraw", svg_main, "#trip");

    SerializeSVG("eraw/adblueper.eraw", svg_main, "#adblueper");

    SerializeSVG("eraw/hazard.eraw", svg_main, "#hazard");

    SerializeSVG("eraw/ebs.eraw", svg_main, "#ebs");

    SerializeSVG("eraw/ecas.eraw", svg_main, "#ecas");

    SerializeSVG("eraw/retarder.eraw", svg_main, "#retarder");

    SerializeSVG("eraw/temp.eraw", svg_main, "#temp");

    SerializeSVG("eraw/p4wd.eraw", svg_main, "#4wd");

    SerializeSVG("eraw/ldws.eraw", svg_main, "#ldws");

    SerializeSVG("eraw/stop.eraw", svg_main, "#stop");

    SerializeSVG("eraw/ldwshand.eraw", svg_main, "#ldwshand");

    SerializeSVG("eraw/dpf.eraw", svg_main, "#dpf");

    SerializeSVG("eraw/difflock.eraw", svg_main, "#difflock");

    SerializeSVG("eraw/filament.eraw", svg_main, "#filament");

    SerializeSVG("eraw/adblue.eraw", svg_main, "#adblue");

    SerializeSVG("eraw/airfilter.eraw", svg_main, "#airfilter");

    SerializeSVG("eraw/fuelrast.eraw", svg_main, "#fuelrast");

    //Left icons
    SerializeSVG("eraw/cargoload.eraw", svg_main, "#cargoload");

    SerializeSVG("eraw/drivelock.eraw", svg_main, "#drivelock");

    SerializeSVG("eraw/gargolift.eraw", svg_main, "#gargolift");

    SerializeSVG("eraw/whistle.eraw", svg_main, "#whistle");

    SerializeSVG("eraw/lowspd.eraw", svg_main, "#lowspd");

    SerializeSVG("eraw/highspd.eraw", svg_main, "#highspd");

    //Right icons
    SerializeSVG("eraw/aebson.eraw", svg_main, "#aebson");

    SerializeSVG("eraw/liftbrg.eraw", svg_main, "#liftbrg");

    SerializeSVG("eraw/aebsoff.eraw", svg_main, "#aebsoff");

    SerializeSVG("eraw/sideslip.eraw", svg_main, "#sideslip");

    SerializeSVG("eraw/wheeldiff.eraw", svg_main, "#wheeldiff");

    SerializeSVG("eraw/axialdiff.eraw", svg_main, "#axialdiff");

    //Switch handle/auto
    SerializeSVG("eraw/swth.eraw", svg_main, "#swth");

    SerializeSVG("eraw/thandle.eraw", svg_main, "#thandle");

    SerializeSVG("eraw/tauto.eraw", svg_main, "#tauto");

    //drag
	SerializeSVG("eraw/lpresobj.eraw", svg_main, "#lpresobj");

	SerializeSVG("eraw/rpresobj.eraw", svg_main, "#rpresobj");

	SerializeSVG("eraw/fuelobj.eraw", svg_main, "#fuelobj");

	SerializeSVG("eraw/tempobj.eraw", svg_main, "#tempobj");

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

bool MotorDash::is_point_in_range(egt::DefaultDim point, egt::DefaultDim start, egt::DefaultDim end)
{
    if (point >= start && point <= end)
        return true;
    else
        return false;
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

    //Widget handler
    std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> LpresBase;
    std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> RpresBase;
    std::shared_ptr<egt::experimental::GaugeLayer> bkgrdPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> fuelPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> tempbarPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> mainspdPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> timePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> bardigitPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> gearPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> voltagePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> datePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> lpresdigitPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> rpresdigitPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> odoPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> tripPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> adblueperPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> hazardPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> ebsPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> ecasPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> retarderPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> tempPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> p4wdPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> ldwsPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> stopPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> ldwshandPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> dpfPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> difflockPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> filamentPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> adbluePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> airfilterPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> fuelrastPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> cargoloadPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> drivelockPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> gargoliftPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> whistlePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> lowspdPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> highspdPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> aebsonPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> liftbrgPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> aebsoffPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> sideslipPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> wheeldiffPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> axialdiffPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> swthPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> thandlePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> tautoPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> lpresobjPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> rpresobjPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> fuelobjPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> tempobjPtr;

    auto rect = std::make_shared<egt::Rect>();
    std::ostringstream str;
    std::ostringstream path;
    std::string lbd_ret = "0";

    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;

    window.color(egt::Palette::ColorId::bg, egt::Palette::black);

    MotorDash motordash;
    auto swthanimatedown = std::make_shared<egt::PropertyAnimator>(-81, 0, std::chrono::milliseconds(1000), egt::easing_quintic_easeinout);
    auto swthanimateup = std::make_shared<egt::PropertyAnimator>(0, -81, std::chrono::milliseconds(1000), egt::easing_quintic_easeinout);
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
    }
#endif

    window.add(motordash);
    motordash.show();
    window.show();

    auto bar_deserial = [&]()
    {
        motordash.add(std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/bkgrd.eraw", rect))));

        for (int i = 0; i <= 12; i++)
        {
            path.str("");
            path << "eraw/lpres" << std::to_string(i) << ".eraw";
            LpresBase.push_back(std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize(path.str(), rect))));
            LpresBase[i]->box(*rect);
            LpresBase[i]->hide();
            motordash.add(LpresBase[i]);
        }
        LpresBase[0]->show();

        for (int i = 0; i <= 12; i++)
        {
            path.str("");
            path << "eraw/rpres" << std::to_string(i) << ".eraw";
            RpresBase.push_back(std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize(path.str(), rect))));
            RpresBase[i]->box(*rect);
            RpresBase[i]->hide();
            motordash.add(RpresBase[i]);
        }
        RpresBase[0]->show();

        mainspdPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/mainspd.eraw", rect)));
        motordash.add_text_widget("#mainspd", "  ", egt::Rect(rect->x()-30, rect->y()-68, rect->width(), rect->height()+10), 130);
    };

    bar_deserial();
    bool is_increasing = true;
    bool is_spd_inc = true;
    bool is_pres_finish = false;
    int pres_index = 0;
    int mainspd_digit = 0;
    int timer_cnt = 0;
    //int speed_index = 0;
    int FUEL_WIDTH = 0;
    int TEMP_WIDTH = 0;
    bool is_fuel_inc = true;
    bool is_automatic_animate = true;

    egt::PeriodicTimer timer(std::chrono::milliseconds(50));
    timer.on_timeout([&]()
    {
        if (!motordash.get_high_q_state())
            return;
        if (!high_pri_q.empty())
        {
            //gettimeofday(&time1, NULL);
            motordash.set_high_q_state(false);
            lbd_ret = high_pri_q.front()();
            high_pri_q.pop();
            motordash.set_high_q_state(true);
            //gettimeofday(&time2, NULL);
            //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
            //if (HIGH_Q_TIME_THRESHHOLD <= timediff)
                //std::cout << "warning!!! high pri q: " << lbd_ret <<"() exec too longer: " << timediff << "us" << std::endl;

        }
        else
        {
            if (!motordash.get_low_q_state())
                return;
            if (!low_pri_q.empty())
            {
                //gettimeofday(&time1, NULL);
                motordash.set_low_q_state(false);
                lbd_ret = low_pri_q.front()();
                low_pri_q.pop();
                motordash.set_low_q_state(true);
                //gettimeofday(&time2, NULL);
                //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                //if (LOW_Q_TIME_THRESHHOLD <= timediff)
                    //std::cout << "warning!!! low pri q: " << lbd_ret <<"() exec too longer: " << timediff << "us" << std::endl;
                return;
            }
        }

        timer_cnt = (LOW_Q_TIME_THRESHHOLD <= timer_cnt) ? 0 : timer_cnt + 1;
        auto mainspd_change = [&]()
        {
            str.str("");
            str << std::to_string(mainspd_digit);
            motordash.find_text("#mainspd")->clear();
            motordash.find_text("#mainspd")->text(str.str());
            app.event().step();
            mainspd_digit = is_spd_inc ? mainspd_digit + 3 : mainspd_digit - 3;
            if (100 <= mainspd_digit)
            {
                is_spd_inc = false;
                is_pres_finish = true;
            }
            else if (0 >= mainspd_digit)
            {
                str.str("");
                str << std::to_string(mainspd_digit);
                motordash.find_text("#mainspd")->clear();
                motordash.find_text("#mainspd")->text(str.str());
                is_spd_inc = true;
                is_pres_finish = true;
            }
            return "mainspd_change";
        };

        auto pres_move = [&]()
        {
            motordash.hide_rpres(RpresBase);
            RpresBase[pres_index]->show();

            if (is_increasing)
                LpresBase[pres_index]->show();
            else
                LpresBase[pres_index]->hide();

            if (is_increasing && 12 == pres_index)
            {
                is_increasing = false;
                //is_pres_finish = true;
            }
            else if (!is_increasing && 0 == pres_index)
            {
                is_increasing = true;
                //is_pres_finish = true;
            }
            else
            {
                pres_index = is_increasing ? pres_index + 1 : pres_index - 1;
            }
            return "pres_move";
        };

        if (is_pres_finish)
        {
            is_pres_finish = false;
            low_pri_q.push(pres_move);
            //low_pri_q.push(mainspd_change);
            //deserialize speed
            if (!motordash.get_middle_text_state())
            {
                low_pri_q.push([&]()
                {
                    lpresdigitPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/lpresdigit.eraw", rect)));
                    motordash.add_text_widget("#lpres", "6.5", egt::Rect(rect->x()-10, rect->y()-10, rect->width(), rect->height()), 40);
                    return "lpres_text";
                });
                low_pri_q.push([&]()
                {
                    rpresdigitPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/rpresdigit.eraw", rect)));
                    motordash.add_text_widget("#rpres", "6.5", egt::Rect(rect->x()-10, rect->y()-10, rect->width(), rect->height()), 40);
                    return "rpres_text";
                });
                motordash.set_middle_text_state(true);
                return;
            }
            else
            {

            }

            //deserialize bar
            if (!motordash.get_bar_state())
            {
                low_pri_q.push([&]()
                {
                    fuelPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/fuel.eraw", rect)));
                    fuelPtr->box(*rect);
                    FUEL_WIDTH = fuelPtr->width();
                    fuelPtr->show();
                    motordash.add(fuelPtr);

                    tempbarPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/tempbar.eraw", rect)));
                    tempbarPtr->box(*rect);
                    TEMP_WIDTH = tempbarPtr->width();
                    tempbarPtr->show();
                    motordash.add(tempbarPtr);

                    fuelrastPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/fuelrast.eraw", rect)));
                    fuelrastPtr->box(*rect);
                    fuelrastPtr->show();
                    motordash.add(fuelrastPtr);

                    return "bar_deserial";
                });
                motordash.set_bar_state(true);
                return;
            }
            else
            {
                low_pri_q.push([&]()
                {
                    if (is_fuel_inc)
                    {
                        fuelPtr->width(fuelPtr->width() + 6);
                        tempbarPtr->width(tempbarPtr->width() + 4);
                    }
                    else
                    {
                        fuelPtr->width(fuelPtr->width() - 6);
                        tempbarPtr->width(tempbarPtr->width() - 4);
                    }

                    if (!is_fuel_inc && 0 >= fuelPtr->width())
                    {
                        is_fuel_inc = true;
                    }
                    else if (is_fuel_inc && FUEL_WIDTH <= fuelPtr->width())
                    {
                        is_fuel_inc = false;
                    }
                    return "fuel_temp_bar";
                });
            }

            if (!motordash.get_top_icon_state())
            {
                low_pri_q.push([&]()
                {
                    hazardPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/hazard.eraw", rect)));
                    hazardPtr->box(*rect);
                    hazardPtr->hide();
                    motordash.add(hazardPtr);

                    ebsPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/ebs.eraw", rect)));
                    ebsPtr->box(*rect);
                    ebsPtr->hide();
                    motordash.add(ebsPtr);

                    ecasPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/ecas.eraw", rect)));
                    ecasPtr->box(*rect);
                    ecasPtr->hide();
                    motordash.add(ecasPtr);

                    retarderPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/retarder.eraw", rect)));
                    retarderPtr->box(*rect);
                    retarderPtr->hide();
                    motordash.add(retarderPtr);
                    return "top_icon1_deserial";
                });
                low_pri_q.push([&]()
                {
                    tempPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/temp.eraw", rect)));
                    tempPtr->box(*rect);
                    tempPtr->hide();
                    motordash.add(tempPtr);

                    p4wdPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/p4wd.eraw", rect)));
                    p4wdPtr->box(*rect);
                    p4wdPtr->hide();
                    motordash.add(p4wdPtr);

                    ldwsPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/ldws.eraw", rect)));
                    ldwsPtr->box(*rect);
                    ldwsPtr->hide();
                    motordash.add(ldwsPtr);

                    stopPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/stop.eraw", rect)));
                    stopPtr->box(*rect);
                    stopPtr->hide();
                    motordash.add(stopPtr);
                    return "top_icon2_deserial";
                });
                low_pri_q.push([&]()
                {
                    ldwshandPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/ldwshand.eraw", rect)));
                    ldwshandPtr->box(*rect);
                    ldwshandPtr->hide();
                    motordash.add(ldwshandPtr);

                    dpfPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/dpf.eraw", rect)));
                    dpfPtr->box(*rect);
                    dpfPtr->hide();
                    motordash.add(dpfPtr);

                    difflockPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/difflock.eraw", rect)));
                    difflockPtr->box(*rect);
                    difflockPtr->hide();
                    motordash.add(difflockPtr);

                    filamentPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/filament.eraw", rect)));
                    filamentPtr->box(*rect);
                    filamentPtr->hide();
                    motordash.add(filamentPtr);
                    return "top_icon3_deserial";
                });
                low_pri_q.push([&]()
                {
                    adbluePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/adblue.eraw", rect)));
                    adbluePtr->box(*rect);
                    adbluePtr->hide();
                    motordash.add(adbluePtr);

                    airfilterPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/airfilter.eraw", rect)));
                    airfilterPtr->box(*rect);
                    airfilterPtr->hide();
                    motordash.add(airfilterPtr);
                    return "top_icon4_deserial";
                });
                motordash.set_top_icon_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 1))
                {
                    low_pri_q.push([&]()
                    {
                        if (hazardPtr->visible())
                            hazardPtr->hide();
                        else
                            hazardPtr->show();

                        if (retarderPtr->visible())
                            retarderPtr->hide();
                        else
                            retarderPtr->show();

                        if (ldwsPtr->visible())
                            ldwsPtr->hide();
                        else
                            ldwsPtr->show();

                        return "hazard_icons";
                    });
                    low_pri_q.push([&]()
                    {
                        if (ecasPtr->visible())
                            ecasPtr->hide();
                        else
                            ecasPtr->show();

                        if (p4wdPtr->visible())
                            p4wdPtr->hide();
                        else
                            p4wdPtr->show();

                        if (ldwshandPtr->visible())
                            ldwshandPtr->hide();
                        else
                            ldwshandPtr->show();

                        return "ecas_icons";
                    });
                    low_pri_q.push([&]()
                    {
                        if (difflockPtr->visible())
                            difflockPtr->hide();
                        else
                            difflockPtr->show();

                        if (adbluePtr->visible())
                            adbluePtr->hide();
                        else
                            adbluePtr->show();

                        return "difflock_icons";
                    });
                }
                else if (!(timer_cnt % 3))
                {
                    low_pri_q.push([&]()
                    {
                        if (ebsPtr->visible())
                            ebsPtr->hide();
                        else
                            ebsPtr->show();

                        if (tempPtr->visible())
                            tempPtr->hide();
                        else
                            tempPtr->show();

                        if (stopPtr->visible())
                            stopPtr->hide();
                        else
                            stopPtr->show();

                        return "ebs_icons";
                    });
                    low_pri_q.push([&]()
                    {
                        if (dpfPtr->visible())
                            dpfPtr->hide();
                        else
                            dpfPtr->show();

                        if (filamentPtr->visible())
                            filamentPtr->hide();
                        else
                            filamentPtr->show();

                        if (airfilterPtr->visible())
                            airfilterPtr->hide();
                        else
                            airfilterPtr->show();

                        return "dpf_icons";
                    });
                }
            }

            if (!motordash.get_left_icon_state())
            {
                low_pri_q.push([&]()
                {
                    cargoloadPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/cargoload.eraw", rect)));
                    cargoloadPtr->box(*rect);
                    cargoloadPtr->hide();
                    motordash.add(cargoloadPtr);

                    drivelockPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/drivelock.eraw", rect)));
                    drivelockPtr->box(*rect);
                    drivelockPtr->hide();
                    motordash.add(drivelockPtr);

                    whistlePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/whistle.eraw", rect)));
                    whistlePtr->box(*rect);
                    whistlePtr->hide();
                    motordash.add(whistlePtr);
                    return "left_icon1_deserial";
                });
                low_pri_q.push([&]()
                {
                    gargoliftPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/gargolift.eraw", rect)));
                    gargoliftPtr->box(*rect);
                    gargoliftPtr->hide();
                    motordash.add(gargoliftPtr);

                    lowspdPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/lowspd.eraw", rect)));
                    lowspdPtr->box(*rect);
                    lowspdPtr->hide();
                    motordash.add(lowspdPtr);

                    highspdPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/highspd.eraw", rect)));
                    highspdPtr->box(*rect);
                    highspdPtr->hide();
                    motordash.add(highspdPtr);

                    return "left_icon2_deserial";
                });
                motordash.set_left_icon_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 1))
                {
                    low_pri_q.push([&]()
                    {
                        if (cargoloadPtr->visible())
                            cargoloadPtr->hide();
                        else
                            cargoloadPtr->show();

                        if (gargoliftPtr->visible())
                            gargoliftPtr->hide();
                        else
                            gargoliftPtr->show();

                        if (highspdPtr->visible())
                            highspdPtr->hide();
                        else
                            highspdPtr->show();

                        return "cargoload_icons";
                    });
                }
                else if (!(timer_cnt % 3))
                {
                    low_pri_q.push([&]()
                    {
                        if (drivelockPtr->visible())
                            drivelockPtr->hide();
                        else
                            drivelockPtr->show();

                        if (lowspdPtr->visible())
                            lowspdPtr->hide();
                        else
                            lowspdPtr->show();

                        if (whistlePtr->visible())
                            whistlePtr->hide();
                        else
                            whistlePtr->show();

                        return "drivelock_icons";
                    });
                }
            }

            if (!motordash.get_right_icon_state())
            {
                low_pri_q.push([&]()
                {
                    aebsonPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/aebson.eraw", rect)));
                    aebsonPtr->box(*rect);
                    aebsonPtr->hide();
                    motordash.add(aebsonPtr);

                    liftbrgPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/liftbrg.eraw", rect)));
                    liftbrgPtr->box(*rect);
                    liftbrgPtr->hide();
                    motordash.add(liftbrgPtr);

                    aebsoffPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/aebsoff.eraw", rect)));
                    aebsoffPtr->box(*rect);
                    aebsoffPtr->hide();
                    motordash.add(aebsoffPtr);
                    return "right_icon1_deserial";
                });
                low_pri_q.push([&]()
                {
                    sideslipPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/sideslip.eraw", rect)));
                    sideslipPtr->box(*rect);
                    sideslipPtr->hide();
                    motordash.add(sideslipPtr);

                    wheeldiffPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/wheeldiff.eraw", rect)));
                    wheeldiffPtr->box(*rect);
                    wheeldiffPtr->hide();
                    motordash.add(wheeldiffPtr);

                    axialdiffPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/axialdiff.eraw", rect)));
                    axialdiffPtr->box(*rect);
                    axialdiffPtr->hide();
                    motordash.add(axialdiffPtr);

                    return "left_icon2_deserial";
                });
                motordash.set_right_icon_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 1))
                {
                    low_pri_q.push([&]()
                    {
                        if (aebsonPtr->visible())
                            aebsonPtr->hide();
                        else
                            aebsonPtr->show();

                        if (sideslipPtr->visible())
                            sideslipPtr->hide();
                        else
                            sideslipPtr->show();

                        if (aebsoffPtr->visible())
                            aebsoffPtr->hide();
                        else
                            aebsoffPtr->show();

                        return "aebson_icons";
                    });
                }
                else if (!(timer_cnt % 3))
                {
                    low_pri_q.push([&]()
                    {
                        if (liftbrgPtr->visible())
                            liftbrgPtr->hide();
                        else
                            liftbrgPtr->show();

                        if (wheeldiffPtr->visible())
                            wheeldiffPtr->hide();
                        else
                            wheeldiffPtr->show();

                        if (axialdiffPtr->visible())
                            axialdiffPtr->hide();
                        else
                            axialdiffPtr->show();

                        return "liftbrg_icons";
                    });
                }
            }

            if (!motordash.get_top_text_state())
            {
                low_pri_q.push([&]()
                {
                    timePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/time.eraw", rect)));
                    motordash.add_text_widget("#time", "19:32", egt::Rect(rect->x(), rect->y()-15, rect->width(), rect->height()), 40);

                    bardigitPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/bardigit.eraw", rect)));
                    motordash.add_text_widget("#bardigit", "3.5", egt::Rect(rect->x()-10, rect->y()-12, rect->width(), rect->height()), 35);

                    gearPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/gear.eraw", rect)));
                    motordash.add_text_widget("#gear", "D", egt::Rect(rect->x(), rect->y()-15, rect->width(), rect->height()), 50, egt::Palette::greenyellow);
                    motordash.add_text_widget("#gear1", "1", egt::Rect(rect->x()+37, rect->y()+14, rect->width(), rect->height()), 20, egt::Palette::greenyellow);

                    voltagePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/voltage.eraw", rect)));
                    motordash.add_text_widget("#voltage", "24.5", egt::Rect(rect->x()-15, rect->y()-15, rect->width(), rect->height()), 35);

                    datePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/date.eraw", rect)));
                    motordash.add_text_widget("#date", "2021/01/30", egt::Rect(rect->x()-20, rect->y()-10, rect->width(), rect->height()), 35);
                    return "top_txt_deserial";
                });
                motordash.set_top_text_state(true);
                return;
            }
            else
            {

            }

            if (!motordash.get_botom_text_state())
            {
                low_pri_q.push([&]()
                {
                    odoPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/odo.eraw", rect)));
                    motordash.add_text_widget("#odo", "223", egt::Rect(rect->x(), rect->y()-5, rect->width(), rect->height()), 30);

                    tripPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/trip.eraw", rect)));
                    motordash.add_text_widget("#trip", "79.9", egt::Rect(rect->x(), rect->y()-5, rect->width(), rect->height()), 30);

                    adblueperPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/adblueper.eraw", rect)));
                    motordash.add_text_widget("#adblueper", "100", egt::Rect(rect->x(), rect->y()-5, rect->width(), rect->height()), 25);
                    return "botom_txt_deserial";
                });
                motordash.set_botom_text_state(true);
                return;
            }
            else
            {

            }

            if (!motordash.get_swth_state())
            {
                low_pri_q.push([&]()
                {
                    swthPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/swth.eraw", rect)));
                    swthPtr->box(*rect);
                    swthPtr->hide();
                    motordash.add(swthPtr);

                    thandlePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/thandle.eraw", rect)));
                    thandlePtr->box(*rect);
                    thandlePtr->hide();
                    motordash.add(thandlePtr);

                    tautoPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/tauto.eraw", rect)));
                    tautoPtr->box(*rect);
                    tautoPtr->hide();
                    motordash.add(tautoPtr);

                    swthanimatedown->on_change([&](egt::PropertyAnimator::Value value)
                    {
                        if (!swthPtr->visible())
                            swthPtr->show();
                        swthPtr->y(value);
                    });
                    swthanimateup->on_change([&](egt::PropertyAnimator::Value value)
                    {
                        swthPtr->y(value);
                        if (-81 >= swthPtr->y())
                        {
                            thandlePtr->hide();
                            tautoPtr->hide();
                        }
                    });
                    swthPtr->on_event([&](egt::Event&)
                    {
                        is_automatic_animate = is_automatic_animate ? false : true;
                        if (is_automatic_animate)
                        {
                            thandlePtr->hide();
                            tautoPtr->show();
                            timer.start();
                        }
                        else
                        {
                            tautoPtr->hide();
                            thandlePtr->show();
                            timer.stop();
                        }
                    }, {egt::EventId::pointer_click});
                    return "swth_deserial";
                });
                low_pri_q.push([&]()
                {
                    lpresobjPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/lpresobj.eraw", rect)));
                    lpresobjPtr->box(*rect);
                    motordash.add(lpresobjPtr);

                    rpresobjPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/rpresobj.eraw", rect)));
                    rpresobjPtr->box(*rect);
                    motordash.add(rpresobjPtr);

                    fuelobjPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/fuelobj.eraw", rect)));
                    fuelobjPtr->box(*rect);
                    motordash.add(fuelobjPtr);

                    tempobjPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(motordash.DeSerialize("eraw/tempobj.eraw", rect)));
                    tempobjPtr->box(*rect);
                    motordash.add(tempobjPtr);

                    lpresobjPtr->on_event([&](egt::Event& event)
                    {
                        int i;
                        switch (event.id())
                        {
                            case egt::EventId::pointer_click:
                                break;
                            case egt::EventId::pointer_drag_start:
                                break;
                            case egt::EventId::pointer_drag_stop:
                                break;
                            case egt::EventId::pointer_drag:
                            {
                                motordash.hide_lpres(LpresBase);
                                //std::cout << "drag point: " << event.pointer().point.y() << std::endl;
                                if (motordash.is_point_in_range(event.pointer().point.y(), 355, 404))
                                    LpresBase[0]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 337, 355))
                                {
                                    for (i = 0; i <= 1; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 320, 337))
                                {
                                    for (i = 0; i <= 2; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 300, 320))
                                {
                                    for (i = 0; i <= 3; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 282, 300))
                                {
                                    for (i = 0; i <= 4; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 262, 282))
                                {
                                    for (i = 0; i <= 5; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 244, 262))
                                {
                                    for (i = 0; i <= 6; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 226, 244))
                                {
                                    for (i = 0; i <= 7; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 208, 226))
                                {
                                    for (i = 0; i <= 8; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 188, 208))
                                {
                                    for (i = 0; i <= 9; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 170, 188))
                                {
                                    for (i = 0; i <= 10; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 152, 170))
                                {
                                    for (i = 0; i <= 11; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 132, 152))
                                {
                                    for (i = 0; i <= 12; i++)
                                    {
                                        LpresBase[i]->show();
                                    }
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    });

                    rpresobjPtr->on_event([&](egt::Event& event)
                    {
                        switch (event.id())
                        {
                            case egt::EventId::pointer_click:
                                break;
                            case egt::EventId::pointer_drag_start:
                                break;
                            case egt::EventId::pointer_drag_stop:
                                break;
                            case egt::EventId::pointer_drag:
                            {
                                motordash.hide_rpres(RpresBase);
                                //std::cout << "drag point: " << event.pointer().point.y() << std::endl;
                                if (motordash.is_point_in_range(event.pointer().point.y(), 355, 404))
                                    RpresBase[0]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 337, 355))
                                    RpresBase[1]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 320, 337))
                                    RpresBase[2]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 300, 320))
                                    RpresBase[3]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 282, 300))
                                    RpresBase[4]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 262, 282))
                                    RpresBase[5]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 244, 262))
                                    RpresBase[6]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 226, 244))
                                    RpresBase[7]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 208, 226))
                                    RpresBase[8]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 188, 208))
                                    RpresBase[9]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 170, 188))
                                    RpresBase[10]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 152, 170))
                                    RpresBase[11]->show();
                                else if (motordash.is_point_in_range(event.pointer().point.y(), 132, 152))
                                    RpresBase[12]->show();
                                break;
                            }
                            default:
                                break;
                        }
                    });

                    fuelobjPtr->on_event([&](egt::Event& event)
                    {
                        switch (event.id())
                        {
                            case egt::EventId::pointer_click:
                                break;
                            case egt::EventId::pointer_drag_start:
                                break;
                            case egt::EventId::pointer_drag_stop:
                                break;
                            case egt::EventId::pointer_drag:
                            {
                                if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x()-BAR_RASTER_WIDTH, fuelPtr->x()))
                                    fuelPtr->width(0);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x(), fuelPtr->x()+BAR_RASTER_WIDTH))
                                    fuelPtr->width(BAR_RASTER_WIDTH);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x()+BAR_RASTER_WIDTH, fuelPtr->x()+BAR_RASTER_WIDTH*2))
                                    fuelPtr->width(BAR_RASTER_WIDTH*2);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x()+BAR_RASTER_WIDTH*2, fuelPtr->x()+BAR_RASTER_WIDTH*3))
                                    fuelPtr->width(BAR_RASTER_WIDTH*3);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x()+BAR_RASTER_WIDTH*3, fuelPtr->x()+BAR_RASTER_WIDTH*4))
                                    fuelPtr->width(BAR_RASTER_WIDTH*4);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x()+BAR_RASTER_WIDTH*4, fuelPtr->x()+BAR_RASTER_WIDTH*5))
                                    fuelPtr->width(BAR_RASTER_WIDTH*5);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x()+BAR_RASTER_WIDTH*5, fuelPtr->x()+BAR_RASTER_WIDTH*6))
                                    fuelPtr->width(BAR_RASTER_WIDTH*6);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x()+BAR_RASTER_WIDTH*6, fuelPtr->x()+BAR_RASTER_WIDTH*7))
                                    fuelPtr->width(BAR_RASTER_WIDTH*7);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), fuelPtr->x()+BAR_RASTER_WIDTH*7, fuelPtr->x()+BAR_RASTER_WIDTH*9))
                                    fuelPtr->width(BAR_RASTER_WIDTH*8);
                                break;
                            }
                            default:
                                break;
                        }
                    });

                    tempobjPtr->on_event([&](egt::Event& event)
                    {
                        switch (event.id())
                        {
                            case egt::EventId::pointer_click:
                                break;
                            case egt::EventId::pointer_drag_start:
                                break;
                            case egt::EventId::pointer_drag_stop:
                                break;
                            case egt::EventId::pointer_drag:
                            {
                                if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x()-BAR_RASTER_WIDTH, tempbarPtr->x()))
                                    tempbarPtr->width(0);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x(), tempbarPtr->x()+BAR_RASTER_WIDTH))
                                    tempbarPtr->width(BAR_RASTER_WIDTH);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x()+BAR_RASTER_WIDTH, tempbarPtr->x()+BAR_RASTER_WIDTH*2))
                                    tempbarPtr->width(BAR_RASTER_WIDTH*2);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x()+BAR_RASTER_WIDTH*2, tempbarPtr->x()+BAR_RASTER_WIDTH*3))
                                    tempbarPtr->width(BAR_RASTER_WIDTH*3);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x()+BAR_RASTER_WIDTH*3, tempbarPtr->x()+BAR_RASTER_WIDTH*4))
                                    tempbarPtr->width(BAR_RASTER_WIDTH*4);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x()+BAR_RASTER_WIDTH*4, tempbarPtr->x()+BAR_RASTER_WIDTH*5))
                                    tempbarPtr->width(BAR_RASTER_WIDTH*5);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x()+BAR_RASTER_WIDTH*5, tempbarPtr->x()+BAR_RASTER_WIDTH*6))
                                    tempbarPtr->width(BAR_RASTER_WIDTH*6);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x()+BAR_RASTER_WIDTH*6, tempbarPtr->x()+BAR_RASTER_WIDTH*7))
                                    tempbarPtr->width(BAR_RASTER_WIDTH*7);
                                else if (motordash.is_point_in_range(event.pointer().point.x(), tempbarPtr->x()+BAR_RASTER_WIDTH*7, tempbarPtr->x()+BAR_RASTER_WIDTH*9))
                                    tempbarPtr->width(BAR_RASTER_WIDTH*8);
                                break;
                            }
                            default:
                                break;
                        }
                    });
                    return "drag_deserial";
                });
                motordash.set_swth_state(true);
                return;
            }
            else
            {

            }

        }
        else  //this branch exec the high priority event
        {
            //high_pri_q.push(pres_move);
            high_pri_q.push(mainspd_change);
        }
    });
    timer.start();

    auto handle_touch = [&](egt::Event & event)
    {
        switch (event.id())
        {
            case egt::EventId::pointer_click:
                break;
            case egt::EventId::keyboard_up:
                break;
            case egt::EventId::pointer_drag_start:
                if (300 < event.pointer().point.x()
                    && 500 > event.pointer().point.x()
                    && 0 < event.pointer().point.y()
                    && 100 > event.pointer().point.y())
                {
                    if (event.pointer().point.y() > event.pointer().drag_start.y())
                        swthanimatedown->start();
                    else if (event.pointer().point.y() < event.pointer().drag_start.y())
                        swthanimateup->start();
                }
                break;
            case egt::EventId::pointer_drag_stop:
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
