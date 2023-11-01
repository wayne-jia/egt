/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <chrono>
#include <cxxopts.hpp>
#include <egt/detail/string.h>
#include <egt/ui>
#include <fstream>
#include <iostream>
#include <sstream>
#include <egt/themes/lapis.h>


int main(int argc, char** argv)
{
    egt::Size size(800, 480);
    auto format = egt::PixelFormat::yuv420;

    egt::Application app(argc, argv);
    egt::TopWindow win;
    win.color(egt::Palette::ColorId::bg, egt::Palette::black);
    egt::global_theme(std::make_unique<egt::LapisTheme>());

    egt::Label errlabel;
    errlabel.color(egt::Palette::ColorId::label_text, egt::Palette::white);
    errlabel.align(egt::AlignFlag::expand);
    errlabel.text_align(egt::AlignFlag::top);
    win.add(errlabel);

    // player after label to handle drag
    egt::VideoWindow player(size, format, egt::WindowHint::overlay);
    player.move_to_center(win.center());
    player.volume(5);
    player.color(egt::Palette::ColorId::bg, egt::Palette::transparent);
    player.fill_flags().clear();
    win.add(player);

    std::vector<std::shared_ptr<egt::ImageLabel>> imagesequence;
    for (auto x = 1; x < 6; x++)
    {
        auto image = egt::Image("file:800x400_" + std::to_string(x) + ".png");
        auto imagelbl = std::make_shared<egt::ImageLabel>(image);
        imagesequence.push_back(imagelbl);
        imagelbl->hide();
        win.add(imagelbl);
    }

    egt::Window ctrlwindow(egt::Size(win.width(), 72), egt::PixelFormat::argb8888);
    ctrlwindow.align(egt::AlignFlag::center);
    ctrlwindow.align(egt::AlignFlag::top);
    ctrlwindow.color(egt::Palette::ColorId::bg, egt::Palette::transparent);
    if (!ctrlwindow.plane_window())
        ctrlwindow.fill_flags(egt::Theme::FillFlag::blend);
    win.add(ctrlwindow);

    int idx = 1;
    int lastidx = 0;
    egt::PeriodicTimer timer(std::chrono::seconds(2));
    timer.on_timeout([&win, &idx, &lastidx, &imagesequence]()
    {
        if (idx > 4)
        {
            lastidx = 4;
            idx = 0;
        }
        if (lastidx > 4)
            lastidx = 0;
            
        imagesequence[lastidx]->hide();
        imagesequence[idx]->show();
        idx++;
        lastidx++;
    });
    
    auto FuncStaticPlayloop = [&player, &timer]()
    {
        if (player.playing())
            player.pause();

        player.hide();
        timer.start(); 
    };

    auto FuncStaticDynamicPlayloop = [&player, &timer]()
    {
        timer.stop(); 
        player.show();
        if (player.playing())
            player.pause();

        player.media("file:800x400_4_video.mp4");
        player.play();
    };

    auto FuncDynamicPlayloop = [&player, &timer]()
    {
        timer.stop();
        player.show();
        if (player.playing())
            player.pause();

        player.media("file:800x400_all.mp4");
        player.play();
        player.loopback(true);
    };

    const char* play_items[] =
    {
        "Static playloop",
        "Static/Dynamic playloop",
        "Dynamic playloop"
    };

    auto demooption = std::make_shared<egt::ComboBox>();
    for (auto& i : play_items)
        demooption->add_item(std::make_shared<egt::StringItem>(i));
    demooption->align(egt::AlignFlag::center);
    demooption->margin(5);
    ctrlwindow.add(demooption);

    demooption->on_selected_changed([&]()
    {
        switch (demooption->selected())
        {
            case 0:
                FuncStaticPlayloop();
                break;

            case 1:
                FuncStaticDynamicPlayloop();
                break;

            case 2:
                FuncDynamicPlayloop();
                break;

            default:
                FuncStaticPlayloop();
                break;
        }
    });
    demooption->selected(0);

    ctrlwindow.show();
    player.show();
    win.show();

    return app.run();
}

