/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <egt/ui>
#include <iostream>
#include <sys/time.h>
#include <queue>


#define SCREEN_480_X_480
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

enum class DragType
{
    invalid = 0,
    subslmain,
    subsltemp
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
    egt::DefaultDim pppX;
    egt::DefaultDim subswdmenuX;
    egt::DefaultDim subslmenuX;
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
    PageType pagetype = PageType::main;
    DragType dragtype = DragType::subslmain;

    //subswd handler
    std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> SubswdBase;
    std::shared_ptr<egt::experimental::GaugeLayer> subswdbkgrdPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subswdcolorbgPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subswdvaluePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subswdunitPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subswdbackPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subswddecPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subswdswcPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subswdaddPtr;

    //subsl handler
    std::shared_ptr<egt::experimental::GaugeLayer> subslgraybgPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslmvPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslbkgrdPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslupPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslpausePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subsldownPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subsltxtlPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subsltxtrPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslbtniconPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslbtnmvPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslbackPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslbtnmvupPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> subslbtnmvdownPtr;


    wgtRelativeX_t wgtRltvX = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    auto rect = std::make_shared<egt::Rect>();
    std::ostringstream str;
    egt::DefaultDim start_drag;
    bool is_sub_start_drag;
    int clmv_cnt = 0;
    bool is_sub_open = true;
    bool is_swd_deserial = false;
    int new_clmv_cnt = 0;
    int diff = 0;
    int i = 0;
    bool is_swd_open = false;
    int swd_index = 0;
    bool is_sl_deserial = false;
    int sl_y = 137;
    int sl_BTN_Y_DIFF = 0;

    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;
    window.show();
    window.color(egt::Palette::ColorId::bg, egt::Palette::black);

#ifdef SCREEN_480_X_480
    egt::Window leftwin(egt::Rect(0, 0, SCREEN_X_START, 480));
    leftwin.color(egt::Palette::ColorId::bg, egt::Palette::black);
    egt::Window rightwin(egt::Rect(640, 0, SCREEN_X_START, 480));
    rightwin.color(egt::Palette::ColorId::bg, egt::Palette::black);
    window.add(leftwin);
    window.add(rightwin);
    leftwin.show();
    rightwin.show();
#endif
    const char *svg_files_array[4] = {"/root/main.svg", "/root/cl.svg", "/root/swd.svg", "/root/sl.svg"};
    egt::experimental::SVGDeserial smthome(app, window, 4, svg_files_array);;

    auto fullimgPtr = smthome.AddWidgetByID("/root/eraw/bkgrd.eraw", true);
    fullimgPtr->x(fullimgPtr->x() + SCREEN_X_START);

    auto blstartPtr = smthome.AddWidgetByID("/root/eraw/blstart.eraw", false);
    wgtRltvX.blstartX = blstartPtr->x();
    blstartPtr->x(blstartPtr->x() + SCREEN_X_START);

    auto pppPtr = smthome.AddWidgetByID("/root/eraw/ppp.eraw", true);
    wgtRltvX.pppX = pppPtr->x();
    pppPtr->x(pppPtr->x() + SCREEN_X_START);

    auto subslmenuPtr = smthome.AddWidgetByID("/root/eraw/subslmenu.eraw", true);
    wgtRltvX.subslmenuX = subslmenuPtr->x();
    subslmenuPtr->x(subslmenuPtr->x() + SCREEN_X_START);

    auto subswdmenuPtr = smthome.AddWidgetByID("/root/eraw/subswdmenu.eraw", true);
    wgtRltvX.subswdmenuX = subswdmenuPtr->x();
    subswdmenuPtr->x(subswdmenuPtr->x() + SCREEN_X_START);

    auto slclosedarkPtr = smthome.AddWidgetByID("/root/eraw/slclosedark.eraw", true);
    wgtRltvX.slclosedarkX = slclosedarkPtr->x();
    slclosedarkPtr->x(slclosedarkPtr->x() + SCREEN_X_START);

    auto slopendarkPtr = smthome.AddWidgetByID("/root/eraw/slopendark.eraw", true);
    wgtRltvX.slopendarkX = slopendarkPtr->x();
    slopendarkPtr->x(slopendarkPtr->x() + SCREEN_X_START);

    auto blclosedarkPtr = smthome.AddWidgetByID("/root/eraw/blclosedark.eraw", true);
    wgtRltvX.blclosedarkX = blclosedarkPtr->x();
    blclosedarkPtr->x(blclosedarkPtr->x() + SCREEN_X_START);

    auto blopendarkPtr = smthome.AddWidgetByID("/root/eraw/blopendark.eraw", true);
    wgtRltvX.blopendarkX = blopendarkPtr->x();
    blopendarkPtr->x(blopendarkPtr->x() + SCREEN_X_START);

    auto cldowndarkPtr = smthome.AddWidgetByID("/root/eraw/cldowndark.eraw", true);
    wgtRltvX.cldowndarkX = cldowndarkPtr->x();
    cldowndarkPtr->x(cldowndarkPtr->x() + SCREEN_X_START);

    auto clupdarkPtr = smthome.AddWidgetByID("/root/eraw/clupdark.eraw", true);
    wgtRltvX.clupdarkX = clupdarkPtr->x();
    clupdarkPtr->x(clupdarkPtr->x() + SCREEN_X_START);

    auto decdarkPtr = smthome.AddWidgetByID("/root/eraw/decdark.eraw", true);
    wgtRltvX.decdarkX = decdarkPtr->x();
    decdarkPtr->x(decdarkPtr->x() + SCREEN_X_START);

    auto adddarkPtr = smthome.AddWidgetByID("/root/eraw/adddark.eraw", true);
    wgtRltvX.adddarkX = adddarkPtr->x();
    adddarkPtr->x(adddarkPtr->x() + SCREEN_X_START);

    auto bkcolorPtr = smthome.AddWidgetByID("/root/eraw/bkcolor.eraw", false);
    bkcolorPtr->x(bkcolorPtr->x() + SCREEN_X_START);

    auto subclclosePtr = smthome.AddWidgetByID("/root/eraw/subclclose.eraw", false);
    subclclosePtr->x(subclclosePtr->x() + SCREEN_X_START);

    auto subclopenPtr = smthome.AddWidgetByID("/root/eraw/subclopen.eraw", false);
    subclopenPtr->x(subclopenPtr->x() + SCREEN_X_START);

    auto subclpausePtr = smthome.AddWidgetByID("/root/eraw/subclpause.eraw", false);
    subclpausePtr->x(subclpausePtr->x() + SCREEN_X_START);

    auto subperPtr = smthome.AddWidgetByID("/root/eraw/subper.eraw", false);
    subperPtr->x(subperPtr->x() + SCREEN_X_START);

    auto subbackPtr = smthome.AddWidgetByID("/root/eraw/subback.eraw", false);
    subbackPtr->x(subbackPtr->x() + SCREEN_X_START);

    auto subclrightPtr = smthome.AddWidgetByID("/root/eraw/subclright.eraw", false);
    subclrightPtr->x(subclrightPtr->x() + SCREEN_X_START);

    auto subclleftPtr = smthome.AddWidgetByID("/root/eraw/subclleft.eraw", false);
    subclleftPtr->x(subclleftPtr->x() + SCREEN_X_START);

    auto subclbtnPtr = smthome.AddWidgetByID("/root/eraw/subclbtn.eraw", false);
    subclbtnPtr->x(subclbtnPtr->x() + SCREEN_X_START);

    auto subclbtniconPtr = smthome.AddWidgetByID("/root/eraw/subclbtnicon.eraw", false);
    subclbtniconPtr->x(subclbtniconPtr->x() + SCREEN_X_START);

    auto bkkPtr = smthome.AddWidgetByID("/root/eraw/bkk.eraw", false);
    bkkPtr->x(bkkPtr->x() + SCREEN_X_START);

    auto subclopeniconPtr = smthome.AddWidgetByID("/root/eraw/subclopenicon.eraw", false);
    subclopeniconPtr->x(subclopeniconPtr->x() + SCREEN_X_START);

    auto subclpauseiconPtr = smthome.AddWidgetByID("/root/eraw/subclpauseicon.eraw", false);
    subclpauseiconPtr->x(subclpauseiconPtr->x() + SCREEN_X_START);

    auto subclcloseiconPtr = smthome.AddWidgetByID("/root/eraw/subclcloseicon.eraw", false);
    subclcloseiconPtr->x(subclcloseiconPtr->x() + SCREEN_X_START);

    smthome.add_text_widget("#subper", "", egt::Rect(subperPtr->x(), subperPtr->y()-15, subperPtr->width(), subperPtr->height()), 20);

    auto deserialSwd = [&]()
    {
        subswdbkgrdPtr = smthome.AddWidgetByID("/root/eraw/subswdbkgrd.eraw", false);
        subswdbkgrdPtr->x(subswdbkgrdPtr->x() + SCREEN_X_START);

        subswdcolorbgPtr = smthome.AddWidgetByID("/root/eraw/subswdcolorbg.eraw", false);
        subswdcolorbgPtr->x(subswdcolorbgPtr->x() + SCREEN_X_START);

        subswdvaluePtr = smthome.AddWidgetByID("/root/eraw/subswdvalue.eraw", false);
        subswdvaluePtr->x(subswdvaluePtr->x() + SCREEN_X_START);

        subswdunitPtr = smthome.AddWidgetByID("/root/eraw/subswdunit.eraw", false);
        subswdunitPtr->x(subswdunitPtr->x() + SCREEN_X_START);

        subswdbackPtr = smthome.AddWidgetByID("/root/eraw/subswdback.eraw", false);
        subswdbackPtr->x(subswdbackPtr->x() + SCREEN_X_START);

        subswddecPtr = smthome.AddWidgetByID("/root/eraw/subswddec.eraw", false);
        subswddecPtr->x(subswddecPtr->x() + SCREEN_X_START);

        subswdswcPtr = smthome.AddWidgetByID("/root/eraw/subswdswc.eraw", false);
        subswdswcPtr->x(subswdswcPtr->x() + SCREEN_X_START);

        subswdaddPtr = smthome.AddWidgetByID("/root/eraw/subswdadd.eraw", false);
        subswdaddPtr->x(subswdaddPtr->x() + SCREEN_X_START);

        for (i = 0; i < 25; i++)
        {
            str.str("");
            str << "/root/eraw/p" << std::to_string(i+1) << ".eraw";
            SubswdBase.push_back(smthome.AddWidgetByID(str.str(), false));
            SubswdBase[i]->x(SubswdBase[i]->x() + SCREEN_X_START);
        }
        smthome.add_text_widget("#subswdvalue", "2700", egt::Rect(subswdvaluePtr->x(), subswdvaluePtr->y()-30, subswdvaluePtr->width(), subswdvaluePtr->height()), 60);
        is_swd_deserial = true;
    };

    auto hideSwd = [&]()
    {
        for (i = 0; i < 25; i++)
        {
            SubswdBase[i]->hide();
        }
    };

    auto deserialSl = [&]()
    {
        subslgraybgPtr = smthome.AddWidgetByID("/root/eraw/subslgraybg.eraw", false);
        subslgraybgPtr->x(subslgraybgPtr->x() + SCREEN_X_START);

        subslmvPtr = smthome.AddWidgetByID("/root/eraw/subslmv.eraw", false);
        subslmvPtr->x(subslmvPtr->x() + SCREEN_X_START);

        subslbkgrdPtr = smthome.AddWidgetByID("/root/eraw/subslbkgrd.eraw", false);
        subslbkgrdPtr->x(subslbkgrdPtr->x() + SCREEN_X_START);

        subslupPtr = smthome.AddWidgetByID("/root/eraw/subslup.eraw", false);
        subslupPtr->x(subslupPtr->x() + SCREEN_X_START);

        subslpausePtr = smthome.AddWidgetByID("/root/eraw/subslpause.eraw", false);
        subslpausePtr->x(subslpausePtr->x() + SCREEN_X_START);

        subsldownPtr = smthome.AddWidgetByID("/root/eraw/subsldown.eraw", false);
        subsldownPtr->x(subsldownPtr->x() + SCREEN_X_START);

        subsltxtlPtr = smthome.AddWidgetByID("/root/eraw/subsltxtl.eraw", false);
        subsltxtlPtr->x(subsltxtlPtr->x() + SCREEN_X_START);

        subsltxtrPtr = smthome.AddWidgetByID("/root/eraw/subsltxtr.eraw", false);
        subsltxtrPtr->x(subsltxtrPtr->x() + SCREEN_X_START);

        subslbtniconPtr = smthome.AddWidgetByID("/root/eraw/subslbtnicon.eraw", false);
        subslbtniconPtr->x(subslbtniconPtr->x() + SCREEN_X_START);
        sl_BTN_Y_DIFF = subslbtniconPtr->y() - subslmvPtr->y();

        subslbtnmvPtr = smthome.AddWidgetByID("/root/eraw/subslbtnmv.eraw", false);
        subslbtnmvPtr->x(subslbtnmvPtr->x() + SCREEN_X_START);

        subslbackPtr = smthome.AddWidgetByID("/root/eraw/subslback.eraw", false);
        subslbackPtr->x(subslbackPtr->x() + SCREEN_X_START);

        subslbtnmvupPtr = smthome.AddWidgetByID("/root/eraw/subslbtnmvup.eraw", false);
        subslbtnmvupPtr->x(subslbtnmvupPtr->x() + SCREEN_X_START);

        subslbtnmvdownPtr = smthome.AddWidgetByID("/root/eraw/subslbtnmvdown.eraw", false);
        subslbtnmvdownPtr->x(subslbtnmvdownPtr->x() + SCREEN_X_START);

        smthome.add_text_widget("#subsltxtl", "100%", egt::Rect(subsltxtlPtr->x()-10, subsltxtlPtr->y()-15, subsltxtlPtr->width(), subsltxtlPtr->height()), 20);
        smthome.add_text_widget("#subsltxtr", "121°", egt::Rect(subsltxtrPtr->x(), subsltxtrPtr->y()-15, subsltxtrPtr->width(), subsltxtrPtr->height()), 20);
        is_sl_deserial = true;
    };

    egt::Timer click_timer(std::chrono::milliseconds(300));
    egt::PeriodicTimer submv_timer(std::chrono::milliseconds(100));

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
        slclosedarkPtr->x(wgtRltvX.slclosedarkX + fullimgPtr->x());
        slopendarkPtr->x(wgtRltvX.slopendarkX + fullimgPtr->x());
        blclosedarkPtr->x(wgtRltvX.blclosedarkX + fullimgPtr->x());
        blopendarkPtr->x(wgtRltvX.blopendarkX + fullimgPtr->x());
        cldowndarkPtr->x(wgtRltvX.cldowndarkX + fullimgPtr->x());
        clupdarkPtr->x(wgtRltvX.clupdarkX + fullimgPtr->x());
        decdarkPtr->x(wgtRltvX.decdarkX + fullimgPtr->x());
        adddarkPtr->x(wgtRltvX.adddarkX + fullimgPtr->x());
        pppPtr->x(wgtRltvX.pppX + fullimgPtr->x());
        subswdmenuPtr->x(wgtRltvX.subswdmenuX + fullimgPtr->x());
        subslmenuPtr->x(wgtRltvX.subslmenuX + fullimgPtr->x());
    };

    auto clclose_mv = [&]()
    {
        if (100 <= clmv_cnt)
            return;

        clmv_cnt++;
        subclrightPtr->x(subclrightPtr->x() - 1);
        subclbtnPtr->x(subclbtnPtr->x() - 1);
        subclbtniconPtr->x(subclbtniconPtr->x() - 1);
        subclleftPtr->x(subclleftPtr->x() + 1);
        str.str("");
        str << std::to_string(clmv_cnt) << "%";
        smthome.find_text("#subper")->clear();
        smthome.find_text("#subper")->text(str.str());
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
        smthome.find_text("#subper")->clear();
        smthome.find_text("#subper")->text(str.str());
    };

    auto sl_mv = [&]()
    {
        int percent = std::ceil((sl_y+subslmvPtr->height()-138)/247.0 * 100);
        if (100 < percent)
            percent = 100;
        else if(0 > percent)
            percent = 0;

        subslmvPtr->y(sl_y);
        subslbtniconPtr->y(sl_BTN_Y_DIFF + sl_y);
        str.str("");
        str << std::to_string(percent) << "%";
        smthome.find_text("#subsltxtl")->clear();
        smthome.find_text("#subsltxtl")->text(str.str());
    };

    auto sltemp_mv = [&](int pos)
    {
        if ((296 < pos)
            || (192 > pos))
            return;
        subslbtnmvPtr->y(pos);
        str.str("");
        str << std::to_string(330 - pos) << "°";
        smthome.find_text("#subsltxtr")->clear();
        smthome.find_text("#subsltxtr")->text(str.str());
    };

    submv_timer.on_timeout([&]()
    {
        switch (pagetype)
        {
            case PageType::subcl:
                if (is_sub_open)
                    clopen_mv();
                else
                    clclose_mv();

                if (100 == clmv_cnt && !is_sub_open)
                {
                    submv_timer.stop();
                    //clmvtable.close();
                }
                else if (0 == clmv_cnt && is_sub_open)
                    submv_timer.stop();
                break;
            case PageType::subsl:
                if (is_sub_open)
                    sl_y -= 2;
                else
                    sl_y += 2;
                sl_mv();
                if ((137 < sl_y && !is_sub_open)
                    || (-110 > sl_y && is_sub_open))
                    submv_timer.stop();
                break;
            default:
                break;
        }

    });

    auto subcl_state = [&](bool b_enter)
    {
        if (b_enter)
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
            subclbtnPtr->show();
            subclbtniconPtr->show();
            smthome.find_text("#subper")->show();
        }
        else
        {
            pagetype = PageType::main;
            bkcolorPtr->hide();
            bkkPtr->hide();
            subclrightPtr->hide();
            subclleftPtr->hide();
            subclclosePtr->hide();
            subclopenPtr->hide();
            subclpausePtr->hide();
            subclpauseiconPtr->hide();
            subbackPtr->hide();
            subclbtnPtr->hide();
            subclbtniconPtr->hide();
            smthome.find_text("#subper")->hide();
        }
    };

    auto subswd_switch = [&]()
    {
        if (is_swd_open)
        {
            SubswdBase[swd_index]->show();
            subswdcolorbgPtr->show();
        }
        else
        {
            SubswdBase[swd_index]->hide();
            subswdcolorbgPtr->hide();
        }
    };

    auto subswd_state = [&](bool b_enter)
    {
        if (b_enter)
        {
            if (!is_swd_deserial)
                deserialSwd();
            pagetype = PageType::subswd;
            subswdbkgrdPtr->show();
            subswdunitPtr->show();
            subswdbackPtr->show();
            subswddecPtr->show();
            subswdswcPtr->show();
            subswdaddPtr->show();
            smthome.find_text("#subswdvalue")->show();
            subswd_switch();
        }
        else
        {
            pagetype = PageType::main;
            subswdbkgrdPtr->hide();
            subswdunitPtr->hide();
            subswdbackPtr->hide();
            subswddecPtr->hide();
            subswdswcPtr->hide();
            subswdaddPtr->hide();
            SubswdBase[swd_index]->hide();
            subswdcolorbgPtr->hide();
            smthome.find_text("#subswdvalue")->hide();
        }
    };

    auto subsl_state = [&](bool b_enter)
    {
        if (b_enter)
        {
            if (!is_sl_deserial)
                deserialSl();
            pagetype = PageType::subsl;
            subslgraybgPtr->show();
            subslmvPtr->show();
            subslbkgrdPtr->show();
            //subslupPtr->show();
            //subslpausePtr->show();
            //subsldownPtr->show();
            subslbtniconPtr->show();
            subslbtnmvPtr->show();
            subslbackPtr->show();
            smthome.find_text("#subsltxtl")->show();
            smthome.find_text("#subsltxtr")->show();
        }
        else
        {
            pagetype = PageType::main;
            subslgraybgPtr->hide();
            subslmvPtr->hide();
            subslbkgrdPtr->hide();
            subslupPtr->hide();
            subslpausePtr->hide();
            subsldownPtr->hide();
            subslbtniconPtr->hide();
            subslbtnmvPtr->hide();
            subslbackPtr->hide();
            smthome.find_text("#subsltxtl")->hide();
            smthome.find_text("#subsltxtr")->hide();
        }
    };

    auto subswd_move = [&]()
    {
        hideSwd();
        SubswdBase[swd_index]->show();
        str.str("");
        str << std::to_string(swd_index*2+27) << "00";
        smthome.find_text("#subswdvalue")->clear();
        smthome.find_text("#subswdvalue")->text(str.str());
    };



    auto handle_touch = [&](egt::Event & event)
    {
        switch (event.id())
        {
            case egt::EventId::pointer_click:
                //std::cout << "click.point: " << event.pointer().point << std::endl;
                switch (pagetype)
                {
                    case PageType::main:
                        if (smthome.is_point_in_rect(event.pointer().point, pppPtr->box()))
                            subcl_state(true);
                        else if (smthome.is_point_in_rect(event.pointer().point, subswdmenuPtr->box()))
                            subswd_state(true);
                        else if (smthome.is_point_in_rect(event.pointer().point, subslmenuPtr->box()))
                            subsl_state(true);
                        break;
                    case PageType::subcl:
                        if (smthome.is_point_in_rect(event.pointer().point, subclopenPtr->box()))
                        {
                            subclopeniconPtr->show();
                            subclcloseiconPtr->hide();
                            subclpauseiconPtr->hide();
                            is_sub_open = true;
                            if (!submv_timer.running())
                                submv_timer.start();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subclclosePtr->box()))
                        {
                            subclcloseiconPtr->show();
                            subclopeniconPtr->hide();
                            subclpauseiconPtr->hide();
                            is_sub_open = false;
                            if (!submv_timer.running())
                                submv_timer.start();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subclpausePtr->box()))
                        {
                            subclpauseiconPtr->show();
                            subclcloseiconPtr->hide();
                            subclopeniconPtr->hide();
                            if (submv_timer.running())
                                submv_timer.stop();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subbackPtr->box()))
                            subcl_state(false);
                        break;
                    case PageType::subsl:
                        if (smthome.is_point_in_rect(event.pointer().point, subslupPtr->box()))
                        {
                            subslupPtr->show();
                            subsldownPtr->hide();
                            subslpausePtr->hide();
                            is_sub_open = true;
                            if (!submv_timer.running())
                                submv_timer.start();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subsldownPtr->box()))
                        {
                            subsldownPtr->show();
                            subslupPtr->hide();
                            subslpausePtr->hide();
                            is_sub_open = false;
                            if (!submv_timer.running())
                                submv_timer.start();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subslpausePtr->box()))
                        {
                            subslpausePtr->show();
                            subsldownPtr->hide();
                            subslupPtr->hide();
                            if (submv_timer.running())
                                submv_timer.stop();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subslbtnmvupPtr->box()))
                        {
                            is_sub_open = true;
                            sltemp_mv(subslbtnmvPtr->y() - 2);
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subslbtnmvdownPtr->box()))
                        {
                            is_sub_open = false;
                            sltemp_mv(subslbtnmvPtr->y() + 2);
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subslbackPtr->box()))
                            subsl_state(false);
                        break;
                    case PageType::subbl:
                        break;
                    case PageType::subdd:
                        break;
                    case PageType::subswd:
                        if (smthome.is_point_in_rect(event.pointer().point, subswdswcPtr->box()))
                        {
                            is_swd_open = is_swd_open ? false : true;
                            subswd_switch();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subswdaddPtr->box()))
                        {
                            swd_index++;
                            if (24 <= swd_index)
                                swd_index = 24;
                            subswd_move();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subswddecPtr->box()))
                        {
                            swd_index--;
                            if (0 >= swd_index)
                                swd_index = 0;
                            subswd_move();
                        }
                        else if (smthome.is_point_in_rect(event.pointer().point, subswdbackPtr->box()))
                        {
                            subswd_state(false);
                        }
                        break;
                    case PageType::subfilm:
                        break;
                    case PageType::subother:
                        break;
                    default:
                        break;
                }
                break;
            case egt::EventId::pointer_drag_start:
                switch (pagetype)
                {
                    case PageType::main:
                        start_drag = event.pointer().drag_start.x();
                        break;
                    case PageType::subcl:
                        if (smthome.is_point_in_rect(event.pointer().drag_start, subclbtnPtr->box()))
                        {
                            is_sub_start_drag = true;
                            if (submv_timer.running())
                                submv_timer.stop();
                        }
                        break;
                    case PageType::subsl:
                        is_sub_start_drag = true;
                        if (submv_timer.running())
                            submv_timer.stop();
                        if (smthome.is_point_in_rect(event.pointer().drag_start, subslbtniconPtr->box()))
                            dragtype = DragType::subslmain;
                        else if (smthome.is_point_in_rect(event.pointer().drag_start, subslbtnmvPtr->box()))
                            dragtype = DragType::subsltemp;
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
                    case PageType::subsl:
                        is_sub_start_drag = false;
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
                        diff = event.pointer().point.x() - start_drag;
                        if (160 <= event.pointer().point.x()
                            && 640 >= event.pointer().point.x())
                        {
                            wgtMove(diff);
                        }
                        start_drag = event.pointer().point.x();
                        break;
                    case PageType::subcl:
                        if (is_sub_start_drag)
                        {
                            if (501 <= event.pointer().point.x())
                                new_clmv_cnt = 0;
                            else if (401 >= event.pointer().point.x())
                                new_clmv_cnt = 100;
                            else
                                new_clmv_cnt = 501 - event.pointer().point.x();

                            if (new_clmv_cnt > clmv_cnt) //Is closing
                            {
                                diff = new_clmv_cnt - clmv_cnt;
                                for (i = 0; i < diff; i++)
                                {
                                    clclose_mv();
                                }
                            }
                            else if (new_clmv_cnt < clmv_cnt) //Is opening
                            {
                                diff = clmv_cnt - new_clmv_cnt;
                                for (i = 0; i < diff; i++)
                                {
                                    clopen_mv();
                                }
                            }
                        }
                        break;
                    case PageType::subsl:
                        if (is_sub_start_drag)
                        {
                            switch (dragtype)
                            {
                                case DragType::subslmain:
                                    if (385 <= event.pointer().point.y())
                                        sl_y = 385-subslmvPtr->height();
                                    else if (138 >= event.pointer().point.y())
                                        sl_y = 138-subslmvPtr->height();
                                    else
                                        sl_y = event.pointer().point.y()-subslmvPtr->height();
                                    sl_mv();
                                    break;
                                case DragType::subsltemp:
                                    sltemp_mv(event.pointer().point.y());
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    case PageType::subbl:
                        break;
                    case PageType::subdd:
                        break;
                    case PageType::subswd:
                        for (i = 0; i < 25; i++)
                        {
                            if (smthome.is_point_in_rect(event.pointer().point, SubswdBase[i]->box()))
                            {
                                swd_index = i;
                                subswd_move();
                                break;
                            }
                        }
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
