/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <array>
#include <egt/painter.h>
#include <egt/ui>
#include <memory>
#include <sstream>
#include <iostream>
#include <string>
#include <egt/themes/lapis.h>


//grid 20*20
#define GRID_W_H 20
#define GRID_CNT_H 26
#define GRID_CNT_V 5
#define GRID_Y_START 33
#define GRID_2_BLOCK_DIST 100
#define GRID_SINE_DIST 340

const int G_XSTART = -6;
const int G_XSTOP = GRID_W_H*GRID_CNT_H-4;
const int G_XAXIS = 80;
const int G_XAXIS2 = G_XAXIS+GRID_2_BLOCK_DIST;
const int G_XAXIS_SINE = G_XAXIS+GRID_SINE_DIST;

int g_ecgarry[] = {
    0, 0, 0, 6, 7, 6, 0, 0, -7, 35, -10, 0, 0, 0, 0, 0, 0, 5, 9, 11, 12, 11, 9, 5, 0, 0, 0, 0, 0, 0
};

int g_sinearry[] = {
    0, 2, 4, 6, 8, 12, 24, 30, 34, 36, 35, 30, 24, 12, 8, 6, 4, 2, 0
};

class MainWindow : public egt::TopWindow
{
public:

    MainWindow() :
          m_canvas(screen()->size(), egt::PixelFormat::argb8888)
    {
        // don't draw background, we'll do it in draw()
        fill_flags().clear();
        color(egt::Palette::ColorId::bg, egt::Color(0x1d2239ff));

        auto logo = std::make_shared<egt::ImageLabel>(egt::Image("icon:egt_logo_black.png;128"));
        logo->align(egt::AlignFlag::right | egt::AlignFlag::top);
        logo->margin(5);
        add(logo);
    }

    void draw(egt::Painter& painter, const egt::Rect& rect) override
    {
        painter.set(color(egt::Palette::ColorId::bg));
        painter.draw(rect);
        painter.fill();

        painter.draw(rect.point());
        painter.draw(rect, egt::Image(m_canvas.surface()));

        egt::TopWindow::draw(painter, rect);
    }

    // Defind the variable to control the ECG waveform
    int m_sample_counter = G_XSTART;
    int m_width = 3;
    int m_i = 0;
    int m_y_new_data = 0;
    int m_y_old_data = G_XAXIS;
    int m_x_old_data = G_XSTART;

    egt::Canvas m_canvas;
};

static bool is_x_in_range(egt::DefaultDim x, egt::DefaultDim x_start, egt::DefaultDim x_end)
{
    if (x >= x_start && x <= x_end)
        return true;
    else
        return false;
} 

static int run(int argc, char** argv)
{
    egt::Application app(argc, argv, "whiteboard");

    MainWindow win;

    egt::global_theme(std::make_unique<egt::LapisTheme>());

    egt::Label label0("LAN");
    label0.color(egt::Palette::ColorId::label_text, egt::Palette::white);
    label0.padding(3);
    label0.font(egt::Font(22));
    label0.move(egt::Point(25, 1));
    win.add(label0);

    egt::Label label1("80");
    label1.color(egt::Palette::ColorId::label_text, egt::Color(0x51e25fff));
    label1.padding(3);
    label1.font(egt::Font(80));
    label1.move(egt::Point(650, 60));
    win.add(label1);

    egt::Label label2("19");
    label2.color(egt::Palette::ColorId::label_text, egt::Color(0xddddddff));
    label2.padding(3);
    label2.font(egt::Font(70));
    label2.move(egt::Point(540, 390));
    win.add(label2);

    egt::Label labelred0("0");
    labelred0.color(egt::Palette::ColorId::label_text, egt::Palette::black);
    labelred0.color(egt::Palette::ColorId::label_bg, egt::Palette::red);
    labelred0.fill_flags(egt::Theme::FillFlag::blend);
    labelred0.padding(3);
    labelred0.box(egt::Rect(610, 60, 180, 60));
    labelred0.font(egt::Font(80));
    labelred0.hide();
    win.add(labelred0);


    egt::Painter painter(win.m_canvas.context());
    auto cr = painter.context();
    cairo_set_antialias(painter.context().get(), CAIRO_ANTIALIAS_NONE);
    cairo_set_line_cap(cr.get(), CAIRO_LINE_CAP_ROUND);
    
    // Draw the 1 grey vertical line
    egt::Line liney(egt::Point(GRID_W_H*GRID_CNT_H+1, 0), 
                    egt::Point(GRID_W_H*GRID_CNT_H+1, 480));
    painter.line_width(2);
    painter.set(egt::Palette::grey);
    painter.draw(liney.start(), liney.end());
    painter.stroke();

    // Draw the grid horizon lines
    for (auto i=0; i<11; i++)
    {
        egt::Line linex(egt::Point(0, GRID_Y_START+(i*GRID_W_H)), 
                        egt::Point(GRID_W_H*GRID_CNT_H, GRID_Y_START+(i*GRID_W_H)));
        painter.line_width(1);
        painter.set(egt::Palette::grey);
        painter.draw(linex.start(), linex.end());
        painter.stroke();
    }
    // Draw the grid vertical lines
    for (auto i=0; i<27; i++)
    {
        egt::Line linex(egt::Point(1+(i*GRID_W_H), GRID_Y_START), 
                        egt::Point(1+(i*GRID_W_H), GRID_Y_START+GRID_W_H*GRID_CNT_V*2));
        painter.line_width(1);
        painter.set(egt::Palette::grey);
        painter.draw(linex.start(), linex.end());
        painter.stroke();
    }

    // Draw the 4 grey horizon lines on the right area
    for (auto i=0; i<4; i++)
    {
        egt::Line linex(egt::Point(GRID_W_H*GRID_CNT_H+1, GRID_Y_START+(i*110)+18), 
                        egt::Point(800, GRID_Y_START+(i*110))+18);
        painter.line_width(2);
        painter.set(egt::Palette::grey);
        painter.draw(linex.start(), linex.end());
        painter.stroke();
    }

    // Draw the 2 white horizon lines
    for (auto i=0; i<2; i++)
    {
        egt::Line linex(egt::Point(0, GRID_Y_START+(i*GRID_W_H*GRID_CNT_V)), 
                        egt::Point(GRID_W_H*GRID_CNT_H, GRID_Y_START+(i*GRID_W_H*GRID_CNT_V)));
        painter.line_width(3);
        painter.set(egt::Palette::white);
        painter.draw(linex.start(), linex.end());
        painter.stroke();
    }

    // Draw the 2 white vertical lines
    // for (auto i=0; i<2; i++)
    // {
    //     egt::Line linex(egt::Point(3+(i*GRID_W_H*GRID_CNT_H-2), GRID_Y_START-1), 
    //                     egt::Point(3+(i*GRID_W_H*GRID_CNT_H-2), GRID_Y_START+GRID_W_H*GRID_CNT_V));
    //     painter.line_width(3);
    //     painter.set(egt::Palette::white);
    //     painter.draw(linex.start(), linex.end());
    //     painter.stroke();
    // }
    egt::Line linex(egt::Point(3+(GRID_W_H*GRID_CNT_H-2), GRID_Y_START-1), 
                    egt::Point(3+(GRID_W_H*GRID_CNT_H-2), GRID_Y_START+GRID_W_H*GRID_CNT_V));
    painter.line_width(3);
    painter.set(egt::Palette::white);
    painter.draw(linex.start(), linex.end());
    painter.stroke();

    // Draw the default lines in the right cubes[2]
    for (auto i=0; i<3; i++)
    {
        egt::Line linex(egt::Point(600+(i*50), 220), 
                        egt::Point(630+(i*50), 220));
        painter.line_width(10);
        painter.set(egt::Palette::cyan);
        painter.draw(linex.start(), linex.end());
        painter.stroke();
    }

    const int LINE_GAP = 20;
    // Draw the default lines in the right cubes[3]
    for (auto i=0; i<3; i++)
    {
        egt::Line linex(egt::Point(50+GRID_W_H*GRID_CNT_H+(i*30), 330), 
                        egt::Point(50+LINE_GAP+GRID_W_H*GRID_CNT_H+(i*30), 330));
        painter.line_width(6);
        painter.set(egt::Color(0xddddddff));
        painter.draw(linex.start(), linex.end());
        painter.stroke();
    }

    // Draw the / symbol
    egt::Line slash(egt::Point(135+LINE_GAP+GRID_W_H*GRID_CNT_H, 310), 
                    egt::Point(135+GRID_W_H*GRID_CNT_H, 350));
    painter.line_width(5);
    painter.set(egt::Color(0xddddddff));
    painter.draw(slash.start(), slash.end());
    painter.stroke();

    // Draw the default lines in the right cubes[3]
    for (auto i=0; i<3; i++)
    {
        egt::Line linex(egt::Point(160+GRID_W_H*GRID_CNT_H+(i*30), 330), 
                        egt::Point(160+LINE_GAP+GRID_W_H*GRID_CNT_H+(i*30), 330));
        painter.line_width(6);
        painter.set(egt::Color(0xddddddff));
        painter.draw(linex.start(), linex.end());
        painter.stroke();
    }


    auto cursor = std::make_shared<egt::LineWidget>(egt::Size(3, 65));
    cursor->move(egt::Point(G_XSTART+3, GRID_Y_START+4));
    cursor->border(2);
    cursor->fill_flags(egt::Theme::FillFlag::blend);
    cursor->horizontal(false);
    cursor->color(egt::Palette::ColorId::border, egt::Color(0x1d2239ff));
    win.add(cursor);

    bool is_2_grid_draw = false;
    egt::PeriodicTimer timer(std::chrono::milliseconds(30));
    timer.on_timeout([&]()
    {
        if (win.m_i >= sizeof(g_ecgarry)/sizeof(int))
        {
            win.m_i = 0;
        }
        else
        {
            if (is_2_grid_draw)
                win.m_y_new_data = G_XAXIS2 - g_ecgarry[win.m_i];
            else
                win.m_y_new_data = G_XAXIS - g_ecgarry[win.m_i];
        }

        if (G_XSTOP < win.m_sample_counter && !is_2_grid_draw)
        {
            is_2_grid_draw = true;
            win.m_x_old_data = G_XSTART;
            win.m_y_old_data = G_XAXIS2;
            win.m_sample_counter = G_XSTART;
            win.m_y_new_data = G_XAXIS2 - g_ecgarry[win.m_i];
            cursor->y(GRID_Y_START+4+GRID_2_BLOCK_DIST);
        }
        else if (G_XSTOP < win.m_sample_counter && is_2_grid_draw)
        {
            is_2_grid_draw = false;
            win.m_x_old_data = G_XSTART;
            win.m_y_old_data = G_XAXIS;
            win.m_sample_counter = G_XSTART;
            win.m_y_new_data = G_XAXIS - g_ecgarry[win.m_i];
            cursor->y(GRID_Y_START+4);
        }

        // Draw grid of 3 horizon lines up
        if (is_2_grid_draw)
        {
            for (auto i=1; i<4; i++)
            {
                egt::Line linex(egt::Point(win.m_x_old_data, GRID_2_BLOCK_DIST+GRID_Y_START+(i*GRID_W_H)), 
                                egt::Point(win.m_x_old_data+5, GRID_2_BLOCK_DIST+GRID_Y_START+(i*GRID_W_H)));
                painter.line_width(1);
                painter.set(egt::Palette::grey);
                painter.draw(linex.start(), linex.end());
                painter.stroke();
            }
        }
        else
        {
            for (auto i=1; i<4; i++)
            {
                egt::Line linex(egt::Point(win.m_x_old_data, GRID_Y_START+(i*GRID_W_H)), 
                                egt::Point(win.m_x_old_data+5, GRID_Y_START+(i*GRID_W_H)));
                painter.line_width(1);
                painter.set(egt::Palette::grey);
                painter.draw(linex.start(), linex.end());
                painter.stroke();
            }
        }
        
  
        // Draw grid of 1 vertical line up
        if (is_2_grid_draw)
        {
            for (auto i=0; i<25; i++)
            {
                if (is_x_in_range(1+GRID_W_H+(i*GRID_W_H), win.m_x_old_data, win.m_sample_counter))
                {
                    egt::Line linex(egt::Point(1+GRID_W_H+(i*GRID_W_H), GRID_2_BLOCK_DIST+GRID_Y_START+3), 
                                egt::Point(1+GRID_W_H+(i*GRID_W_H), GRID_2_BLOCK_DIST+GRID_W_H*6));
                    painter.line_width(1);
                    painter.set(egt::Palette::grey);
                    painter.draw(linex.start(), linex.end());
                    painter.stroke();
                    break;
                }
            }
        }
        else
        {
            for (auto i=0; i<25; i++)
            {
                if (is_x_in_range(1+GRID_W_H+(i*GRID_W_H), win.m_x_old_data, win.m_sample_counter))
                {
                    egt::Line linex(egt::Point(1+GRID_W_H+(i*GRID_W_H), GRID_Y_START+3), 
                                egt::Point(1+GRID_W_H+(i*GRID_W_H), GRID_W_H*6));
                    painter.line_width(1);
                    painter.set(egt::Palette::grey);
                    painter.draw(linex.start(), linex.end());
                    painter.stroke();
                    break;
                }
            }
        }
        

        cursor->x(win.m_sample_counter+1);
        
        // Draw the waveform
        egt::Line line(egt::Point(win.m_x_old_data, win.m_y_old_data), egt::Point(win.m_sample_counter, win.m_y_new_data));
        painter.line_width(win.m_width);
        painter.set(egt::Color(0x51e25fff));
        painter.draw(line.start(), line.end());
        painter.stroke();

        // Draw the mask to cover old data
        if (cursor->x()+7 < (GRID_W_H*GRID_CNT_H-2))
        {
            painter.set(egt::Color(0x1d2239ff));
            painter.draw(egt::Rect(egt::Point(cursor->x()+7, cursor->y()), egt::Size(2, 65)));
            painter.stroke();
        }

        // damage only the rectangle containing the new line
        auto r = line.rect();
        r += egt::Size(win.m_width * 2, win.m_width * 2);
        r -= egt::Point(win.m_width, win.m_width);
        win.damage(r);
        
        win.m_x_old_data = win.m_sample_counter;
        win.m_y_old_data = win.m_y_new_data;
        win.m_sample_counter += 3;
        win.m_i++;
    });
    timer.start();

    


    auto cursorsine = std::make_shared<egt::LineWidget>(egt::Size(5, 85));
    cursorsine->move(egt::Point(G_XSTART, GRID_SINE_DIST+4));
    cursorsine->border(3);
    cursorsine->fill_flags(egt::Theme::FillFlag::blend);
    cursorsine->horizontal(false);
    cursorsine->color(egt::Palette::ColorId::border, egt::Color(0x1d2239ff));
    win.add(cursorsine);

    // Define the variable for the sine waveform
    int idx = 0;
    int sine_sample_counter = G_XSTART;
    int sine_y_new_data = G_XAXIS_SINE;
    int sine_y_old_data = G_XAXIS_SINE;
    int sine_x_old_data = G_XSTART;

    int blink_period = 0;
    int begin_blink = 0;
    egt::PeriodicTimer timersine(std::chrono::milliseconds(100));
    timersine.on_timeout([&]()
    {
        blink_period = (blink_period > 10000) ? 0 : (blink_period + 1);
        // Control the digit blink
        if (!(blink_period%68) && (begin_blink == 0))
        {
            begin_blink = 1;
            label1.hide();
            labelred0.show();    
        }

        if (begin_blink)
        {
            labelred0.color(egt::Palette::ColorId::label_text, egt::Palette::black);
            labelred0.color(egt::Palette::ColorId::label_bg, egt::Palette::red);
            if (begin_blink%9 == 0)
            {
                labelred0.color(egt::Palette::ColorId::label_text, egt::Palette::red);
                labelred0.color(egt::Palette::ColorId::label_bg, egt::Color(0x1d2239ff));
            }
            begin_blink++;
        }

        if (begin_blink > 40)
        {
            begin_blink = 0;
            labelred0.hide();
            label1.show();
        }

        if (idx >= sizeof(g_sinearry)/sizeof(int))
        {
            idx = 0;
        }
        else
        {
            sine_y_new_data = G_XAXIS_SINE - g_sinearry[idx];
        }

        if (G_XSTOP-3 < sine_sample_counter)
        {
            sine_x_old_data = G_XSTART;
            sine_y_old_data = G_XAXIS_SINE;
            sine_sample_counter = G_XSTART;
            sine_y_new_data = G_XAXIS_SINE - g_sinearry[idx];
        }
        
        cursorsine->x(sine_sample_counter+1);
        
        // Draw the waveform
        egt::Line line(egt::Point(sine_x_old_data, sine_y_old_data), egt::Point(sine_sample_counter, sine_y_new_data));
        painter.line_width(win.m_width);
        painter.set(egt::Color(0xddddddff));
        painter.draw(line.start(), line.end());
        painter.stroke();

        // Draw the mask to cover old data
        if (cursorsine->x()+7 < (GRID_W_H*GRID_CNT_H-2))
        {
            painter.set(egt::Color(0x1d2239ff));
            painter.draw(egt::Rect(egt::Point(cursorsine->x()+7, cursorsine->y()), egt::Size(3, 85)));
            painter.stroke();
        }

        // damage only the rectangle containing the new line
        auto r = line.rect();
        r += egt::Size(win.m_width * 2, win.m_width * 2);
        r -= egt::Point(win.m_width, win.m_width);
        win.damage(r);
        
        sine_x_old_data = sine_sample_counter;
        sine_y_old_data = sine_y_new_data;
        sine_sample_counter += 3;
        idx++;
    });
    timersine.start();


    // Handle the popup event
    auto dialog_result = std::make_shared<egt::Label>();
    auto list_dialog = std::make_shared<egt::Dialog>();
    list_dialog->move(egt::Point(5, 200));
    list_dialog->resize(egt::Size(600, 300));
    //list_dialog->color(egt::Palette::ColorId::bg, egt::Palette::darkgrey);
    list_dialog->title("ECG Configuration");
    list_dialog->button(egt::Dialog::ButtonId::button1, "OK");
    list_dialog->button(egt::Dialog::ButtonId::button2, "Cancel");
    win.add(list_dialog);

    auto dlist0 = std::make_shared<egt::ListBox>();
    for (auto x = 0; x < 18; x++)
        dlist0->add_item(std::make_shared<egt::StringItem>("item " + std::to_string(x), egt::Rect(), egt::AlignFlag::left | egt::AlignFlag::center));
    list_dialog->widget(expand(dlist0));

    list_dialog->on_button1_click([list_dialog, dialog_result, dlist0]()
    {
        auto select = dynamic_cast<egt::StringItem*>(dlist0->item_at(dlist0->selected()).get())->text();
        dialog_result->text("List Dialog: " + select + " Selected");
    });

    list_dialog->on_button2_click([list_dialog, dialog_result, dlist0]()
    {
        dialog_result->text("List Dialog: Cancel button clicked");
    });

    auto list_dialog_btn = std::make_shared<egt::Button>();
    list_dialog_btn->move(egt::Point(30, 40));
    list_dialog_btn->fill_flags().clear();
    list_dialog_btn->margin(15);
    win.add(list_dialog_btn);
    list_dialog_btn->on_click([list_dialog, dialog_result](egt::Event&)
    {
        dialog_result->text("");
        list_dialog->show_modal(true);
    });
    
    win.show();

    return app.run();
}

int main(int argc, char** argv)
{
    auto res = run(argc, argv);
    return res;
}
