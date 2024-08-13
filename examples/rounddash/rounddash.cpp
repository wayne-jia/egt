/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rounddash.h"
#include "erawparse.h"
#include "multiple_eraw.h"

//#define HAND_GENERATE_NEEDLE

std::vector<std::shared_ptr<egt::Label>> GPSLabels;
std::vector<std::shared_ptr<egt::ImageLabel>> GPSImgIndicators;

APP_DATA appData;
static uint32_t prev_tick = 0, tick = 0;
static uint32_t prev_sec_tick = 0, sec_tick = 0;
static bool tick_start = false;
static uint32_t sec2 = 0;
static bool needles_init_done = false;
static bool gpswgt_init_done = false;
static bool blur_alpha_high = true;

int main(int argc, char** argv)
{
    std::cout << std::endl << "EGT start" << std::endl; 

    std::vector<std::shared_ptr<OverlayWindow>> OverlayWinVector;
    std::shared_ptr<egt::Label> AAInidcator;

    egt::Application app(argc, argv);
    egt::TopWindow window;

    ImageParse imgs("multiple_eraw.bin", Speedo_table, sizeof(Speedo_table)/sizeof(eraw_st));

    egt::Point centerCircle = egt::Point(360, 360);
    auto needleWidget = std::make_shared<egt::experimental::NeedleLayer>(egt::Image(imgs.GetImageObj(10)), 0, 240, 40, -45, 195, true);
    needleWidget->move(egt::Point(360-INIT_NEEDLE_WIDTH-CENTER_RADIUS, 360-(INIT_NEEDLE_HEIGHT/2)));
    needleWidget->needle_center(centerCircle - needleWidget->box().point());
    needleWidget->needle_point(centerCircle);
    needleWidget->hide();
    window.add(needleWidget);

    auto img1stNeedle = std::make_shared<egt::ImageLabel>(window, egt::Image(imgs.GetImageObj(9)));
    img1stNeedle->image_align(egt::AlignFlag::center);
    img1stNeedle->move(egt::Point(157, 422));

    window.background(egt::Image(imgs.GetImageObj(0)));
    window.on_show([]()    
    {        
        std::cout << std::endl << "EGT show" << std::endl;    
    });

    ///============ Needle layer =============
    OverlayWinVector.emplace_back(std::make_shared<OverlayWindow>(egt::Rect(1000, 1000, NEEDLE_FB_MAX_WIDTH, NEEDLE_FB_MAX_HEIGHT),
                                                               egt::PixelFormat::argb8888,
                                                               egt::WindowHint::overlay,
                                                               1));
    OverlayWinVector[0]->fill_flags().clear();
    window.add(OverlayWinVector[0]);
    ///============ Needle  layer end =============

    ///============ GPS layer =============
    OverlayWinVector.emplace_back(std::make_shared<OverlayWindow>(egt::Rect(GPS_X, GPS_Y, GPS_WIDTH, GPS_HEIGHT)));
    OverlayWinVector[1]->fill_flags().clear();
    window.add(OverlayWinVector[1]);

    auto imgLblLogobg = std::make_shared<egt::ImageLabel>(*OverlayWinVector[1], egt::Image(imgs.GetImageObj(1)));
    imgLblLogobg->image_align(egt::AlignFlag::center);
    imgLblLogobg->move(egt::Point(76, 48));

    auto imgLblLogo = std::make_shared<egt::ImageLabel>(*OverlayWinVector[1], egt::Image(imgs.GetImageObj(2)));
    imgLblLogo->image_align(egt::AlignFlag::center);
    imgLblLogo->move(egt::Point(78, 49));

    auto imgLblInfobg = std::make_shared<egt::ImageLabel>(*OverlayWinVector[1], egt::Image(imgs.GetImageObj(3)));
    imgLblInfobg->image_align(egt::AlignFlag::center);
    imgLblInfobg->move(egt::Point(53, 237));

    auto lblInstru = std::make_shared<egt::Label>(*OverlayWinVector[1], "DRIVE SAFE!");
    lblInstru->color(egt::Palette::ColorId::label_text, egt::Palette::white);
    lblInstru->font(egt::Font("Noto Sans", 21, egt::Font::Weight::normal));
    lblInstru->move(egt::Point(103, 278));
    GPSLabels.emplace_back(lblInstru);  //[0]
    ///============ GPS  layer end =============

    ///============ Blue layer =============
    OverlayWinVector.emplace_back(std::make_shared<OverlayWindow>(egt::Rect(0, 0, MAX_WIDTH, MAX_HEIGHT),
                                                               egt::PixelFormat::argb8888,
                                                               egt::WindowHint::overlay,
                                                               1));
    window.add(OverlayWinVector[2]);
    OverlayWinVector[2]->fill_flags().clear();
    auto imgLblBlur = std::make_shared<egt::ImageLabel>(*OverlayWinVector[2], egt::Image(imgs.GetImageObj(4)));
    imgLblBlur->fill_flags().clear();
    imgLblBlur->image_align(egt::AlignFlag::center);
    ///============ Blue  layer end =============

    // Create fade effect for OVR2 and HEO
    OverlayFade fade(OverlayWinVector, "ovr2_fade_in_10", OVERLAY_TYPE::LCDC_OVR_2, 0, 255, 10);
    fade.add("ovr2_fade_in_5", OVERLAY_TYPE::LCDC_OVR_2, 0, 255, 5);
    fade.add("ovr2_fade_out_10", OVERLAY_TYPE::LCDC_OVR_2, 255, 0, 10);
    fade.add("ovr2_fade_out_50", OVERLAY_TYPE::LCDC_OVR_2, 255, 0, 50);
    fade.add("ovrheo_fade_in_10", OVERLAY_TYPE::LCDC_OVR_HEO, 0, 255, 10);
    fade.add("ovrheo_fade_out_10", OVERLAY_TYPE::LCDC_OVR_HEO, 255, 0, 10);
    fade.add("ovrheo_fade_in_lit_10", OVERLAY_TYPE::LCDC_OVR_HEO, 100, 255, 10);
    fade.add("ovrheo_fade_out_lit_10", OVERLAY_TYPE::LCDC_OVR_HEO, 255, 100, 10);
    fade.add("ovrheo_fade_in_lit_50", OVERLAY_TYPE::LCDC_OVR_HEO, 100, 255, 50);

    // Init GPS widegt function
    auto initGPSwgt = [&OverlayWinVector, &imgs, &AAInidcator]()
    {
        auto lblSpdUnit = std::make_shared<egt::Label>(*OverlayWinVector[1], "km/h");
        lblSpdUnit->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        lblSpdUnit->font(egt::Font("Noto Sans", 23, egt::Font::Weight::bold, egt::Font::Slant::italic));
        lblSpdUnit->move(egt::Point(136, 120));
        lblSpdUnit->hide();
        GPSLabels.emplace_back(lblSpdUnit);   //[1]

        AAInidcator = std::make_shared<egt::Label>(*OverlayWinVector[1], "AA-On");
        AAInidcator->color(egt::Palette::ColorId::label_text, egt::Palette::green);
        AAInidcator->font(egt::Font("Noto Sans", 40, egt::Font::Weight::bold));
        AAInidcator->width(100);
        AAInidcator->text_align(egt::AlignFlag::center);
        AAInidcator->move(egt::Point(100, 2));

        auto lblSpd = std::make_shared<egt::Label>(*OverlayWinVector[1], "0");
        lblSpd->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        lblSpd->font(egt::Font("Noto Sans", 60, egt::Font::Weight::bold, egt::Font::Slant::italic));
        lblSpd->width(100);
        lblSpd->text_align(egt::AlignFlag::center);
        lblSpd->move(egt::Point(114, 49));
        lblSpd->hide();
        GPSLabels.emplace_back(lblSpd);   //[2]

        auto lblDist = std::make_shared<egt::Label>(*OverlayWinVector[1], "20 km");
        lblDist->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        lblDist->width(100);
        lblDist->text_align(egt::AlignFlag::center);
        lblDist->font(egt::Font("Noto Sans", 18, egt::Font::Weight::bold));
        lblDist->move(egt::Point(116, 248));
        lblDist->hide();
        GPSLabels.emplace_back(lblDist);   //[3]

        auto lblRTime = std::make_shared<egt::Label>(*OverlayWinVector[1], "Time Left:         mins");
        lblRTime->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        lblRTime->font(egt::Font("Noto Sans", 18, egt::Font::Weight::normal));
        lblRTime->move(egt::Point(81, 313));
        lblRTime->hide();
        GPSLabels.emplace_back(lblRTime);   //[4]

        for (auto i=5; i<9; i++)
        {
            auto navImg = std::make_shared<egt::ImageLabel>(*OverlayWinVector[1], egt::Image(imgs.GetImageObj(i)));
            navImg->fill_flags().clear();
            navImg->image_align(egt::AlignFlag::center);
            navImg->move(egt::Point(56, 264));
            navImg->hide();
            GPSImgIndicators.emplace_back(navImg);
        }
    };

    // The pulse blue blur effect function
    auto pulseBlur = [&fade]()
    {
        if (blur_alpha_high == true)
        {
            blur_alpha_high = false;
            fade.request("ovrheo_fade_out_lit_10");
        }
        else
        {
            blur_alpha_high = true;
            fade.request("ovrheo_fade_in_lit_10");
        }
    };

    auto initLibInput = [&app]()
    {
        std::cout << std::endl << "Enable libinput in app" << std::endl;
        app.setup_inputs();
    };

#ifdef HAND_GENERATE_NEEDLE
    egt::Button btn("Test rotation");
    btn.move(egt::Point(500, 350));
    window.add(btn);

    int idx = 0;
    btn.on_click([&](egt::Event&)
    {
        
        if (idx > 70)
            idx = 0;

        std::cout << "idx:" << idx <<  std::endl;
        updateNeedle(OverlayWinVector[0]->GetOverlay(), idx);
        idx++;
    });

    egt::Button btn2("Test generate");
    btn2.move(egt::Point(500, 500));
    window.add(btn2);

    btn2.on_click([&](egt::Event&)
    {
        for (auto i=0; i<137; i+=2)
            renderNeedles(i, needleWidget, OverlayWinVector[0]);
    });
#else
    auto initNeedles = [&OverlayWinVector, &needleWidget]()
    {
        for (auto i=0; i<137; i+=2)
            renderNeedles(i, needleWidget, OverlayWinVector[0]);
    };
#endif

    // One second periodic timer
    auto sec_timer = std::make_shared<egt::PeriodicTimer>(std::chrono::milliseconds(1000));
    sec_timer->on_timeout([](){ sec_tick++; });

    ///============ Main timer for state machine =============
    egt::PeriodicTimer main_timer(std::chrono::milliseconds(1));
    main_timer.on_timeout([&]() 
    {
        if (tick_start)
            tick++;

        switch (appData.state)
        {
            case APP_STATE_INIT:
            {
                sec_tick = 0;
                appData.state = APP_STATE_INIT_NEEDLE_SHOW;
                break;
            }
            case APP_STATE_INIT_NEEDLE_SHOW:
            {
                for (auto i=0; i<3; i++)
                    OverlayWinVector[i]->show();
                tick_start = true;
            #ifdef HAND_GENERATE_NEEDLE
                appData.state = APP_STATE_FADEOUT_ICON;
            #else
                appData.state = APP_STATE_NEEDLE_TWIRL;
            #endif
                appData.nstate = TWIRL_ACCELERATE_START;
                break;
            }
            case APP_STATE_NEEDLE_TWIRL:
            {
                if (!needles_init_done)
                {
                    needles_init_done = true;
                #ifndef HAND_GENERATE_NEEDLE
                    std::cout << "begin rotating" << std::endl;
                    initNeedles();
                    std::cout << "end rotating" << std::endl;
                #endif
                    img1stNeedle->hide();
                }
                
                if (tick != prev_tick)
                {
                    prev_tick = tick; 
                    APP_ProcessNeedle(OverlayWinVector[0]->GetOverlay());
                }
                break;
            }  
            case APP_STATE_FADEOUT_ICON:
            {
                tick_start = false;
                fade.request("ovr2_fade_out_50");
                imgLblLogo->hide();
                GPSLabels[0]->hide();
                sec_timer->start();
                appData.state = APP_STATE_SPEED_INIT1;
                break;
            }
            case APP_STATE_SPEED_INIT1:
            {
                if (!gpswgt_init_done)
                {
                    initGPSwgt();
                    gpswgt_init_done = true;
                }

                if (sec_tick > 0)
                {
                    sec_tick = 0;
                    fade.request("ovr2_fade_in_10");
                    tick = 0;
                    tick_start = true;
                    appData.state = APP_STATE_SPEED_INIT2;
                }
                break;
            }
            case APP_STATE_SPEED_INIT2:
            {
                if (tick > 45)
                {
                    for (auto i=1; i<3; i++)
                        GPSLabels[i]->show();
                    appData.state = APP_STATE_SPEED_INIT3;
                }  
                break;
            }
            case APP_STATE_SPEED_INIT3:
            {
                if (sec_tick > 1)
                {
                    appData.nstate = DRIVE_START;
                    appData.state = APP_STATE_INIT_INPUT;
                }
                break;
            }
            case APP_STATE_INIT_INPUT:
            {
                initLibInput();
            #ifdef HAND_GENERATE_NEEDLE
                main_timer.stop();
                std::cout << "main timer stopped" << std::endl;
            #endif
                appData.state = APP_STATE_DRIVE;
                break;
            }
            case APP_STATE_DRIVE:
            {
                APP_ProcessNeedle(OverlayWinVector[0]->GetOverlay());
                if (sec_tick != prev_sec_tick)
                {   
                    sec2++;            
                    prev_sec_tick = sec_tick; 
                    APP_ProcessMap();
                    checkNeedleAnime();
                    if (sec2 > 1)
                    {
                        sec2 = 0;
                        pulseBlur();
                    }
                }
                break;
            }
            case APP_STATE_PAUSE:
            {
                sec_tick = 0;
                appData.state = APP_STATE_REACHED;
                if (blur_alpha_high==false)
                {
                    blur_alpha_high = true;
                    fade.request("ovrheo_fade_in_lit_50");
                }
                break;
            }
            case APP_STATE_REACHED:
            {
                if (sec_tick > 2)
                {
                    /* Reset the demo so we can restart everything */
                    sec_tick = 0;
                    fade.request("ovr2_fade_out_10");
                    fade.request("ovrheo_fade_out_10");
                    appData.state = APP_STATE_LOOPBACK;
                    showNavImg(0xFF);
                    OverlayWinVector[0]->hide();
                }
                break;
            }
            case APP_STATE_LOOPBACK:
            {
                if (sec_tick > 2)
                {
                    GPSLabels[0]->text("DRIVE SAFE!");
                    imgLblLogo->show();
                    GPSLabels[2]->hide();
                    GPSLabels[1]->hide();
                    appData.state = APP_STATE_INIT_NEEDLE_SHOW;
                    sec_timer->stop();
                    tick_start = false;
                }
                break;
            }
            default:
                break;
        }
    });
    main_timer.start();
    ///============ Main timer for state machine end =============

    // Touch event handler
    window.on_event(handle_touch);

    window.show();

    auto ret = app.run();

    // Destruction for application if needed

    return ret;
}
