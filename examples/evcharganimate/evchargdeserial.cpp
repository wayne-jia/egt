/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <egt/ui>
#include <egt/themes/lapis.h>
#include <iostream>
#include <fstream>
#include "eraw.h"


#define IMAGE_SN_START 0
#define IMAGE_SN_END 3


class MainWindow : public egt::TopWindow
{
public:

    MainWindow() : m_imgObj(*this)
    {
        color(egt::Palette::ColorId::bg, egt::Palette::black);
        egt::global_theme(std::make_unique<egt::LapisTheme>());
        load();

        m_animation.starting(0);
        m_animation.ending(1);
        m_animation.duration(std::chrono::seconds(100000));
        m_animation.easing_func(egt::easing_linear);
        m_animation.on_change([this](egt::PropertyAnimator::Value value){
            egt::detail::ignoreparam(value);
            if (this->m_current_img >= (int)this->m_imagesequence.size())
            {
                this->m_last_img = this->m_imagesequence.size()-1;
                this->m_current_img = 0;
            }
            if (this->m_last_img >= (int)this->m_imagesequence.size())
                this->m_last_img = 0;
                
            this->m_imagesequence[this->m_last_img++]->hide();
            this->m_imagesequence[this->m_current_img++]->show();
        });
        m_seq.add(m_animation);
        m_seq.start();    
    }

private:
    size_t getFileSize(const char *fileName) 
    {
        if (fileName == NULL)
            return 0;
	    
        struct stat statbuf;
        stat(fileName, &statbuf);
        size_t filesize = statbuf.st_size;

        return filesize;
    }

    void load()
    {
        // Read eraw.bin to buffer
        const char* ERAW_NAME = "eraw.bin";
        std::string erawname;
        size_t buff_size = getFileSize(ERAW_NAME);
        void* buff_ptr = NULL;
        if (buff_size) {
            buff_ptr = malloc(buff_size);
        } else {
            std::cerr << "eraw.bin is blank" << std::endl;
            return;
        }

        std::ifstream f(ERAW_NAME, std::ios::binary);
        if(!f)
        {
            std::cerr << "read eraw.bin failed" << std::endl;
            free(buff_ptr);
            return;
        }
        f.read((char*)buff_ptr, buff_size);

        for (auto x = 0; x <= IMAGE_SN_END-IMAGE_SN_START; x++)
        {
            auto imgObj = m_imgObj.AddWidgetByBuf((const unsigned char*)buff_ptr+offset_table[x].offset, offset_table[x].len, false);
            m_imagesequence.push_back(imgObj);
        }
        free(buff_ptr);
    }

    int m_current_img{1};
    int m_last_img{0};
    std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> m_imagesequence;
    egt::PropertyAnimator m_animation;
    egt::AnimationSequence m_seq{true};
    egt::experimental::SVGDeserial m_imgObj;
};


int main(int argc, char** argv)
{
    egt::Application app(argc, argv);

    MainWindow window;

    egt::Label label1("CPU: 0000%", egt::AlignFlag::left | egt::AlignFlag::bottom);
    label1.color(egt::Palette::ColorId::label_text, egt::Palette::white);
    label1.color(egt::Palette::ColorId::label_bg, egt::Palette::transparent);
    label1.x(egt::SideBoard::HANDLE_WIDTH + 10);
    label1.no_layout(true);
    window.add(bottom(label1));
    label1.zorder_down();

    egt::experimental::CPUMonitorUsage tools;
    egt::PeriodicTimer cputimer(std::chrono::seconds(1));
    cputimer.on_timeout([&label1, &tools]()
    {
        tools.update();
        std::ostringstream ss;
        ss << "CPU: " << static_cast<int>(tools.usage()) << "%";
        label1.text(ss.str());
    });
    cputimer.start();

    window.show();

    return app.run();
}
