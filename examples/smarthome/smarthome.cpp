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


#define SCREEN_480_X_480
#define DO_SVG_SERIALIZATION

#define MEDIA_FILE_PATH "/root/"
#define solve_relative_path(relative_path) (MEDIA_FILE_PATH + relative_path)

#define SCREEN_X_START         160
#define HIGH_Q_TIME_THRESHHOLD 30000  //30ms
#define LOW_Q_TIME_THRESHHOLD  20000  //20ms
#define BAR_RASTER_WIDTH 5


enum class PageType
{
    invalid = 0,
    main,
    subcl,
    subsl,
    subbl,
    subdd,
    subswd,
    subfilm,
    subother
};

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



class SmartHome : public egt::experimental::Gauge
{
public:

    SmartHome() noexcept {}

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
    bool is_point_in_range(egt::DefaultDim point, egt::DefaultDim start, egt::DefaultDim end);
    bool is_point_in_rect(egt::DisplayPoint& point, egt::Rect rect);

    void SetStartDrag(egt::DefaultDim start_drag) { m_start_drag = start_drag; }
    egt::DefaultDim GetStartDrag() { return m_start_drag; }

    void SetSubStartDrag(bool is_sub_start_drag) { m_is_sub_start_drag = is_sub_start_drag; }
    bool GetSubStartDrag() { return m_is_sub_start_drag; }

#ifdef DO_SVG_SERIALIZATION
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg);
    void SerializeSVG(const std::string& filename, egt::SvgImage& svg, const std::string& id);
    void SerializePNG(const char* png_src, const std::string& png_dst);
#endif

private:
    egt::DefaultDim m_start_drag;
    bool m_is_sub_start_drag;
};

#ifdef DO_SVG_SERIALIZATION
void SmartHome::SerializeSVG(const std::string& filename, egt::SvgImage& svg)
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

void SmartHome::SerializeSVG(const std::string& filename, egt::SvgImage& svg, const std::string& id)
{
    auto box = svg.id_box(id);
    if ("#r1" == id)
    {
        std::cout << "id: " << id
                  << " ,wid: " << box.width()
                  << " ,het: " << box.height()
                  << " ,x: " << box.x()
                  << " ,y: " << box.y() << std::endl;
    }
    auto layer = std::make_shared<egt::Image>(svg.render(id, box));

    egt::detail::ErawImage e;
    const auto data = cairo_image_surface_get_data(layer->surface().get());
    const auto width = cairo_image_surface_get_width(layer->surface().get());
    const auto height = cairo_image_surface_get_height(layer->surface().get());

    e.save(solve_relative_path(filename), data, box.x(), box.y(), width, height);
}

void SmartHome::SerializePNG(const char* png_src, const std::string& png_dst)
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

void SmartHome::serialize_all()
{
    //std::ostringstream str;
    //std::ostringstream path;

    egt::SvgImage svg_main("file:main.svg", egt::SizeF(1440, 480));
    egt::SvgImage svg_cl("file:cl.svg", egt::SizeF(480, 480));

    //Serialize background image
    //SerializeSVG("eraw/fullimg.eraw", m_svg);

    //Serialize main.svg
    SerializeSVG("eraw/bkgrd.eraw", svg_main, "#bkgrd");

    SerializeSVG("eraw/slclosedark.eraw", svg_main, "#slclosedark");

    SerializeSVG("eraw/blclosedark.eraw", svg_main, "#blclosedark");

    SerializeSVG("eraw/blstart.eraw", svg_main, "#blstart");

    SerializeSVG("eraw/slopendark.eraw", svg_main, "#slopendark");

    SerializeSVG("eraw/blopendark.eraw", svg_main, "#blopendark");

    SerializeSVG("eraw/cldowndark.eraw", svg_main, "#cldowndark");

    SerializeSVG("eraw/clupdark.eraw", svg_main, "#clupdark");

    SerializeSVG("eraw/decdark.eraw", svg_main, "#decdark");

    SerializeSVG("eraw/adddark.eraw", svg_main, "#adddark");

    SerializeSVG("eraw/ppp.eraw", svg_main, "#ppp");

    //Serialize cl.svg
    SerializeSVG("eraw/bkk.eraw", svg_cl, "#bkk");

    SerializeSVG("eraw/bkcolor.eraw", svg_cl, "#bkcolor");

    SerializeSVG("eraw/subclright.eraw", svg_cl, "#subclright");

    SerializeSVG("eraw/subclbtn.eraw", svg_cl, "#subclbtn");

    SerializeSVG("eraw/subclleft.eraw", svg_cl, "#subclleft");

    SerializeSVG("eraw/subclclose.eraw", svg_cl, "#subclclose");

    SerializeSVG("eraw/subclpause.eraw", svg_cl, "#subclpause");

    SerializeSVG("eraw/subclopen.eraw", svg_cl, "#subclopen");

    SerializeSVG("eraw/subper.eraw", svg_cl, "#subper");

    SerializeSVG("eraw/subback.eraw", svg_cl, "#subback");

    SerializeSVG("eraw/subclbtnicon.eraw", svg_cl, "#subclbtnicon");

    SerializeSVG("eraw/subclopenicon.eraw", svg_cl, "#subclopenicon");

    SerializeSVG("eraw/subclpauseicon.eraw", svg_cl, "#subclpauseicon");

    SerializeSVG("eraw/subclcloseicon.eraw", svg_cl, "#subclcloseicon");

    //SerializeSVG("eraw/mmm.eraw", svg_cl, "#mmm");


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

egt::shared_cairo_surface_t SmartHome::DeSerialize(const std::string& filename)
{
    return egt::detail::ErawImage::load(solve_relative_path(filename));
}

egt::shared_cairo_surface_t SmartHome::DeSerialize(const std::string& filename, std::shared_ptr<egt::Rect>& rect)
{
    return egt::detail::ErawImage::load(solve_relative_path(filename), rect);
}

//When drawing in Inkscape, the y is starting from bottom but not from top,
//so we need convert this coordinate to EGT using, the y is starting from top
void SmartHome::ConvertInkscapeRect2EGT(std::shared_ptr<egt::Rect>& rect)
{
    rect->y(screen()->size().height() - rect->y() - rect->height());
}

bool SmartHome::is_point_in_range(egt::DefaultDim point, egt::DefaultDim start, egt::DefaultDim end)
{
    if (point >= start && point <= end)
        return true;
    else
        return false;
}

bool SmartHome::is_point_in_rect(egt::DisplayPoint& point, egt::Rect rect)
{
    if (point.x() >= rect.x()
        && point.x() <= (rect.x() + rect.width())
        && point.y() >= rect.y()
        && point.y() <= (rect.y() + rect.height()))
        return true;
    else
        return false;
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
    PageType pagetype;

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
    std::shared_ptr<egt::experimental::GaugeLayer> pppPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> bkkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> bkcolorPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclclosePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclopenPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclpausePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subperPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subbackPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclrightPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclbtnPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclleftPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclbtniconPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclopeniconPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclpauseiconPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subclcloseiconPtr;

    wgtRelativeX_t wgtRltvX = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    auto rect = std::make_shared<egt::Rect>();
    std::ostringstream str;
    std::ostringstream path;
    int clmv_cnt = 0;
    bool is_cl_open = true;
    //egt::DefaultDim subCLR_X = 0;
    //egt::DefaultDim subCLL_X = 0;
    //egt::DefaultDim subCLB_X = 0;
    //egt::DefaultDim subCLBI_X = 0;

    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;

    window.color(egt::Palette::ColorId::bg, egt::Palette::black);

#ifdef SCREEN_480_X_480
    egt::Window leftwin(egt::Rect(0, 0, SCREEN_X_START, 480));
    leftwin.color(egt::Palette::ColorId::bg, egt::Palette::black);
    egt::Window rightwin(egt::Rect(640, 0, SCREEN_X_START, 480));
    rightwin.color(egt::Palette::ColorId::bg, egt::Palette::black);
#endif

    SmartHome smthm;

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
            smthm.serialize_all();
            text->clear();
            text->text("Serialize successfully, welcome!");
            app.event().step();
            sleep(1);
            text->hide();
        }
    }
#endif

    window.add(smthm);
    smthm.show();

#ifdef SCREEN_480_X_480
    window.add(leftwin);
    window.add(rightwin);
    leftwin.show();
    rightwin.show();
#endif

    window.show();

    fullimgPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/bkgrd.eraw")));
    fullimgPtr->x(fullimgPtr->x() + SCREEN_X_START);
    fullimgPtr->show();
    smthm.add(fullimgPtr);

    blstartPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/blstart.eraw", rect)));
    blstartPtr->box(*rect);
    wgtRltvX.blstartX = blstartPtr->x();
    blstartPtr->x(blstartPtr->x() + SCREEN_X_START);
    blstartPtr->hide();
    smthm.add(blstartPtr);

    pppPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/ppp.eraw", rect)));
    pppPtr->box(*rect);
    //wgtRltvX.blrectX = pppPtr->x();
    pppPtr->x(pppPtr->x() + SCREEN_X_START);
    pppPtr->show();
    smthm.add(pppPtr);

    slclosedarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/slclosedark.eraw", rect)));
    slclosedarkPtr->box(*rect);
    wgtRltvX.slclosedarkX = slclosedarkPtr->x();
    slclosedarkPtr->x(slclosedarkPtr->x() + SCREEN_X_START);
    slclosedarkPtr->show();
    smthm.add(slclosedarkPtr);

    slopendarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/slopendark.eraw", rect)));
    slopendarkPtr->box(*rect);
    wgtRltvX.slopendarkX = slopendarkPtr->x();
    slopendarkPtr->x(slopendarkPtr->x() + SCREEN_X_START);
    slopendarkPtr->show();
    smthm.add(slopendarkPtr);

    blclosedarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/blclosedark.eraw", rect)));
    blclosedarkPtr->box(*rect);
    wgtRltvX.blclosedarkX = blclosedarkPtr->x();
    blclosedarkPtr->x(blclosedarkPtr->x() + SCREEN_X_START);
    blclosedarkPtr->show();
    smthm.add(blclosedarkPtr);

    blopendarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/blopendark.eraw", rect)));
    blopendarkPtr->box(*rect);
    wgtRltvX.blopendarkX = blopendarkPtr->x();
    blopendarkPtr->x(blopendarkPtr->x() + SCREEN_X_START);
    blopendarkPtr->show();
    smthm.add(blopendarkPtr);

    cldowndarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/cldowndark.eraw", rect)));
    cldowndarkPtr->box(*rect);
    wgtRltvX.cldowndarkX = cldowndarkPtr->x();
    cldowndarkPtr->x(cldowndarkPtr->x() + SCREEN_X_START);
    cldowndarkPtr->show();
    smthm.add(cldowndarkPtr);

    clupdarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/clupdark.eraw", rect)));
    clupdarkPtr->box(*rect);
    wgtRltvX.clupdarkX = clupdarkPtr->x();
    clupdarkPtr->x(clupdarkPtr->x() + SCREEN_X_START);
    clupdarkPtr->show();
    smthm.add(clupdarkPtr);

    decdarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/decdark.eraw", rect)));
    decdarkPtr->box(*rect);
    wgtRltvX.decdarkX = decdarkPtr->x();
    decdarkPtr->x(decdarkPtr->x() + SCREEN_X_START);
    decdarkPtr->show();
    smthm.add(decdarkPtr);

    adddarkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/adddark.eraw", rect)));
    adddarkPtr->box(*rect);
    wgtRltvX.adddarkX = adddarkPtr->x();
    adddarkPtr->x(adddarkPtr->x() + SCREEN_X_START);
    adddarkPtr->show();
    smthm.add(adddarkPtr);

    bkcolorPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/bkcolor.eraw", rect)));
    bkcolorPtr->box(*rect);
    bkcolorPtr->x(bkcolorPtr->x() + SCREEN_X_START);
    bkcolorPtr->hide();
    smthm.add(bkcolorPtr);

    subclclosePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclclose.eraw", rect)));
    subclclosePtr->box(*rect);
    subclclosePtr->x(subclclosePtr->x() + SCREEN_X_START);
    subclclosePtr->hide();
    smthm.add(subclclosePtr);

    subclopenPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclopen.eraw", rect)));
    subclopenPtr->box(*rect);
    subclopenPtr->x(subclopenPtr->x() + SCREEN_X_START);
    subclopenPtr->hide();
    smthm.add(subclopenPtr);

    subclpausePtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclpause.eraw", rect)));
    subclpausePtr->box(*rect);
    subclpausePtr->x(subclpausePtr->x() + SCREEN_X_START);
    subclpausePtr->hide();
    smthm.add(subclpausePtr);

    subperPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subper.eraw", rect)));
    subperPtr->box(*rect);
    subperPtr->x(subperPtr->x() + SCREEN_X_START);
    subperPtr->hide();
    smthm.add(subperPtr);

    subbackPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subback.eraw", rect)));
    subbackPtr->box(*rect);
    subbackPtr->x(subbackPtr->x() + SCREEN_X_START);
    subbackPtr->hide();
    smthm.add(subbackPtr);

    subclrightPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclright.eraw", rect)));
    subclrightPtr->box(*rect);
    subclrightPtr->x(subclrightPtr->x() + SCREEN_X_START);
    //subCLR_X = subclrightPtr->x();
    subclrightPtr->hide();
    smthm.add(subclrightPtr);

    subclleftPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclleft.eraw", rect)));
    subclleftPtr->box(*rect);
    subclleftPtr->x(subclleftPtr->x() + SCREEN_X_START);
    //subCLL_X = subclleftPtr->x();
    subclleftPtr->hide();
    smthm.add(subclleftPtr);

    subclbtnPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclbtn.eraw", rect)));
    subclbtnPtr->box(*rect);
    subclbtnPtr->x(subclbtnPtr->x() + SCREEN_X_START);
    //subCLB_X = subclbtnPtr->x();
    std::cout << "subclbtn rect: " << subclbtnPtr->box() << std::endl;
    subclbtnPtr->hide();
    smthm.add(subclbtnPtr);

    subclbtniconPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclbtnicon.eraw", rect)));
    subclbtniconPtr->box(*rect);
    subclbtniconPtr->x(subclbtniconPtr->x() + SCREEN_X_START);
    //subCLBI_X = subclbtniconPtr->x();
    subclbtniconPtr->hide();
    smthm.add(subclbtniconPtr);

    bkkPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/bkk.eraw", rect)));
    bkkPtr->box(*rect);
    bkkPtr->x(bkkPtr->x() + SCREEN_X_START);
    bkkPtr->hide();
    smthm.add(bkkPtr);

    subclopeniconPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclopenicon.eraw", rect)));
    subclopeniconPtr->box(*rect);
    subclopeniconPtr->x(subclopeniconPtr->x() + SCREEN_X_START);
    subclopeniconPtr->hide();
    smthm.add(subclopeniconPtr);

    subclpauseiconPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclpauseicon.eraw", rect)));
    subclpauseiconPtr->box(*rect);
    subclpauseiconPtr->x(subclpauseiconPtr->x() + SCREEN_X_START);
    subclpauseiconPtr->hide();
    smthm.add(subclpauseiconPtr);

    subclcloseiconPtr = std::make_shared<egt::experimental::GaugeLayer>(egt::Image(smthm.DeSerialize("eraw/subclcloseicon.eraw", rect)));
    subclcloseiconPtr->box(*rect);
    subclcloseiconPtr->x(subclcloseiconPtr->x() + SCREEN_X_START);
    subclcloseiconPtr->hide();
    smthm.add(subclcloseiconPtr);

    egt::Timer click_timer(std::chrono::milliseconds(300));
    egt::PeriodicTimer clmv_timer(std::chrono::milliseconds(100));

#if 0
    pppPtr->on_event([&](egt::Event&)
    {
        std::cout << "pppPtr pointer_click" << std::endl;

    }, {egt::EventId::raw_pointer_down});

    blstartPtr->on_event([&](egt::Event&)
    {
        if (blstartPtr->visible())
            blstartPtr->hide();
        else
            blstartPtr->show();
    }, {egt::EventId::raw_pointer_down});

    bkPtr->on_event([&](egt::Event&)
    {
        std::cout << "r1 + 2 before: " << r1Ptr->width() << std::endl;
        r1Ptr->width(r1Ptr->width() + 2);
        std::cout << "r1 + 2 after: " << r1Ptr->width() << std::endl;
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
        std::cout << "slclosedarkPtr pointer_click" << std::endl;
        slclosedarkPtr->hide();
        clktype = ClickType::slclosedark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    slopendarkPtr->on_event([&](egt::Event&)
    {
        std::cout << "slopendarkPtr pointer_click" << std::endl;
        slopendarkPtr->hide();
        clktype = ClickType::slopendark;
        click_timer.start();
    }, {egt::EventId::raw_pointer_down});

    cldowndarkPtr->on_event([&](egt::Event&)
    {
        std::cout << "cldowndarkPtr pointer_click" << std::endl;
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
#endif

    //Animation of main page
    /***********************************************************
    * -800    -320     160     640
    *                current
	*         left1
    * left2
    *         right1
	*                right2
    ************************************************************/
    auto wgtMove = [&](egt::DefaultDim value)
    {
        if (-800 > fullimgPtr->x())
        {
            fullimgPtr->x(-800);
            return;
        }
        else if (SCREEN_X_START < fullimgPtr->x())
        {
            fullimgPtr->x(SCREEN_X_START);
            return;
        }
        fullimgPtr->x(fullimgPtr->x() + value);
        blstartPtr->x(wgtRltvX.blstartX + fullimgPtr->x());
        //blrectPtr->x(wgtRltvX.blrectX + fullimgPtr->x());
        slclosedarkPtr->x(wgtRltvX.slclosedarkX + fullimgPtr->x());
        slopendarkPtr->x(wgtRltvX.slopendarkX + fullimgPtr->x());
        blclosedarkPtr->x(wgtRltvX.blclosedarkX + fullimgPtr->x());
        blopendarkPtr->x(wgtRltvX.blopendarkX + fullimgPtr->x());
        cldowndarkPtr->x(wgtRltvX.cldowndarkX + fullimgPtr->x());
        clupdarkPtr->x(wgtRltvX.clupdarkX + fullimgPtr->x());
        decdarkPtr->x(wgtRltvX.decdarkX + fullimgPtr->x());
        adddarkPtr->x(wgtRltvX.adddarkX + fullimgPtr->x());
    };

#if 0
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
#endif

    struct submv_t{
        int index;
        egt::DefaultDim right_x;
        egt::DefaultDim left_x;
        egt::DefaultDim btn_x;
    };

    int new_clmv_cnt = 0;
    int diff = 0;
    int i = 0;

#if 0
    std::ofstream clmvtable("clmv.txt");

    if (clmvtable.is_open())
    {
        clmvtable << clmv_cnt << "," << subclrightPtr->x() << "," << subclleftPtr->x() << "," << subclbtnPtr->x() << std::endl;
    }
#endif

    auto clclose_mv = [&]()
    {
        if (100 <= clmv_cnt)
            return;

        clmv_cnt++;
#if 0
        if (clmvtable.is_open())
        {
            clmvtable << clmv_cnt << "," << subclrightPtr->x() - 1 << "," << subclleftPtr->x() + 1 << "," << subclbtnPtr->x() - 1 << std::endl;
        }
#endif
        subclrightPtr->x(subclrightPtr->x() - 1);
        subclbtnPtr->x(subclbtnPtr->x() - 1);
        subclbtniconPtr->x(subclbtniconPtr->x() - 1);
        subclleftPtr->x(subclleftPtr->x() + 1);
        str.str("");
        str << std::to_string(clmv_cnt) << "%";
        smthm.find_text("#subper")->clear();
        smthm.find_text("#subper")->text(str.str());
    };

    auto clopen_mv = [&]()
    {
        if (0 >= clmv_cnt)
            return;

        clmv_cnt--;
        subclrightPtr->x(subclrightPtr->x() + 1);
        subclbtnPtr->x(subclbtnPtr->x() + 1);
        subclbtniconPtr->x(subclbtniconPtr->x() + 1);
        subclleftPtr->x(subclleftPtr->x() - 1);
        str.str("");
        str << std::to_string(clmv_cnt) << "%";
        smthm.find_text("#subper")->clear();
        smthm.find_text("#subper")->text(str.str());
    };

    clmv_timer.on_timeout([&]()
    {
        if (is_cl_open)
            clopen_mv();
        else
            clclose_mv();

        if (100 == clmv_cnt && !is_cl_open)
        {
            clmv_timer.stop();
            //clmvtable.close();
        }
        else if (0 == clmv_cnt && is_cl_open)
            clmv_timer.stop();

    });

    auto handle_touch = [&](egt::Event & event)
    {
        switch (event.id())
        {
            case egt::EventId::pointer_click:
            /*
                std::cout << "click point: " << event.pointer().point
                          << " ,open x: " << subclopenPtr->box().x()
                          << "-" << subclopenPtr->box().x() + subclopenPtr->box().width()
                          << " , y: " << subclopenPtr->box().y()
                          << "-" << subclopenPtr->box().y() + subclopenPtr->box().height() << std::endl;
            */
                if (smthm.is_point_in_rect(event.pointer().point, pppPtr->box()))
                {
                    pagetype = PageType::subcl;
                    bkcolorPtr->show();
                    bkkPtr->show();
                    subclrightPtr->show();
                    subclleftPtr->show();
                    subclclosePtr->show();
                    subclopenPtr->show();
                    subclpausePtr->show();
                    subbackPtr->show();
                    smthm.add_text_widget("#subper", "0", egt::Rect(subperPtr->x(), subperPtr->y()-15, subperPtr->width(), subperPtr->height()), 20);
                    subclbtnPtr->show();
                    subclbtniconPtr->show();
                }
                else if (smthm.is_point_in_rect(event.pointer().point, subclopenPtr->box()))
                {
                    subclopeniconPtr->show();
                    subclcloseiconPtr->hide();
                    subclpauseiconPtr->hide();
                    is_cl_open = true;
                    if (!clmv_timer.running())
                        clmv_timer.start();
                }
                else if (smthm.is_point_in_rect(event.pointer().point, subclclosePtr->box()))
                {
                    subclcloseiconPtr->show();
                    subclopeniconPtr->hide();
                    subclpauseiconPtr->hide();
                    is_cl_open = false;
                    if (!clmv_timer.running())
                        clmv_timer.start();
                }
                else if (smthm.is_point_in_rect(event.pointer().point, subclpausePtr->box()))
                {
                    subclpauseiconPtr->show();
                    subclcloseiconPtr->hide();
                    subclopeniconPtr->hide();
                    if (clmv_timer.running())
                        clmv_timer.stop();
                }
                break;
            case egt::EventId::pointer_drag_start:
                switch (pagetype)
                {
                    case PageType::main:
                        smthm.SetStartDrag(event.pointer().drag_start.x());
                        break;
                    case PageType::subcl:
                        if (smthm.is_point_in_rect(event.pointer().drag_start, subclbtnPtr->box()))
                        {
                            std::cout << "btn drag start" << std::endl;
                            smthm.SetSubStartDrag(true);
                            if (clmv_timer.running())
                                clmv_timer.stop();
                        }
                        break;
                    case PageType::subsl:
                        break;
                    case PageType::subbl:
                        break;
                    case PageType::subdd:
                        break;
                    case PageType::subswd:
                        break;
                    case PageType::subfilm:
                        break;
                    case PageType::subother:
                        break;
                    default:
                        break;
                }
                break;
            case egt::EventId::pointer_drag_stop:
                switch (pagetype)
                {
                    case PageType::main:
                        break;
                    case PageType::subcl:
                        smthm.SetSubStartDrag(false);
                        break;
                    case PageType::subsl:
                        break;
                    case PageType::subbl:
                        break;
                    case PageType::subdd:
                        break;
                    case PageType::subswd:
                        break;
                    case PageType::subfilm:
                        break;
                    case PageType::subother:
                        break;
                    default:
                        break;
                }
                break;
            case egt::EventId::pointer_drag:
            {
                switch (pagetype)
                {
                    case PageType::main:
                        if (160 <= event.pointer().point.x()
                            && 640 >= event.pointer().point.x())
                        {
                            wgtMove(event.pointer().point.x() - smthm.GetStartDrag());
                        }
                        smthm.SetStartDrag(event.pointer().point.x());
                        break;
                    case PageType::subcl:
                        if (smthm.GetSubStartDrag())
                        {
                            std::cout << "btn drag: " << event.pointer().point.x() << std::endl;
                            if (501 <= event.pointer().point.x())
                                new_clmv_cnt = 0;
                            else if (401 >= event.pointer().point.x())
                                new_clmv_cnt = 100;
                            else
                                new_clmv_cnt = 501 - event.pointer().point.x();

                            if (new_clmv_cnt > clmv_cnt) //Is closing
                            {
                                std::cout << "closing clmv_cnt: " << clmv_cnt
                                          << " ,new_clmv_cnt: " << new_clmv_cnt << std::endl;
                                diff = new_clmv_cnt - clmv_cnt;
                                for (i = 0; i < diff; i++)
                                {
                                    clclose_mv();
                                }
                            }
                            else if (new_clmv_cnt < clmv_cnt) //Is opening
                            {
                                std::cout << "opening clmv_cnt: " << clmv_cnt
                                          << " ,new_clmv_cnt: " << new_clmv_cnt << std::endl;
                                diff = clmv_cnt - new_clmv_cnt;
                                for (i = 0; i < diff; i++)
                                {
                                    clopen_mv();
                                }
                            }

                        }
                        break;
                    case PageType::subsl:
                        break;
                    case PageType::subbl:
                        break;
                    case PageType::subdd:
                        break;
                    case PageType::subswd:
                        break;
                    case PageType::subfilm:
                        break;
                    case PageType::subother:
                        break;
                    default:
                        break;
                }
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
