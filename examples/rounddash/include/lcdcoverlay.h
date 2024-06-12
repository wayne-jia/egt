/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LCDCOVERLAY_H
#define LCDCOVERLAY_H


#include <vector>
#include <iostream>
#include <egt/ui>
#include <planes/plane.h>
#include "egt/detail/screen/kmsoverlay.h"
#include "egt/detail/screen/kmsscreen.h"


enum class OVERLAY_TYPE
{
    LCDC_OVR_1,
    LCDC_OVR_2,
    LCDC_OVR_HEO,
    NUM_OVR_TYPES
};

enum class FADE_STATUS
{
    IDLE,                   //Done or idle, no active effects
    DONE,
    RUN                     //effect in progress
};

struct Fade_Data_t
{
    const std::string name; 
    uint32_t winType;
    uint32_t startAlpha;
    uint32_t endAlpha;
    uint32_t delta;
};

struct Fade_Request_t
{
    FADE_STATUS status;
    uint32_t dataIdx;
    uint32_t alpha;
};


class EGT_API OverlayWindow : public egt::Window
{
public:
    OverlayWindow(const egt::Rect& rect, 
              egt::PixelFormat format_hint = egt::PixelFormat::argb8888,
              egt::WindowHint hint = egt::WindowHint::overlay,
              uint32_t num_buffers = 3)
        : egt::Window(rect, format_hint, hint, num_buffers)
    {
        allocate_screen();
        m_overlay = reinterpret_cast<egt::detail::KMSOverlay*>(screen());
        assert(m_overlay);
        plane_set_pos(m_overlay->s(), rect.x(), rect.y());
        plane_apply(m_overlay->s());
    }

    egt::detail::KMSOverlay* GetOverlay()
    {
        return m_overlay;
    }

private:
    egt::detail::KMSOverlay* m_overlay;
};


class EGT_API OverlayFade
{
public:
    explicit OverlayFade(std::vector<std::shared_ptr<OverlayWindow>>& OverlayWinVector)
        : m_OverlayWinVector(OverlayWinVector)
    {}

    explicit OverlayFade(std::vector<std::shared_ptr<OverlayWindow>>& OverlayWinVector,
                         const std::string& name = "default",
                         OVERLAY_TYPE winType = OVERLAY_TYPE::LCDC_OVR_1,
                         uint32_t startAlpha = 0, 
                         uint32_t endAlpha = 255, 
                         uint32_t delta = 10)
        : m_OverlayWinVector(OverlayWinVector)
    {
        create_fade(name, static_cast<uint32_t>(winType), startAlpha, endAlpha, delta);
    }

    void add(const std::string& name, OVERLAY_TYPE winType, uint32_t startAlpha, uint32_t endAlpha, uint32_t delta)
    {
        create_fade(name, static_cast<uint32_t>(winType), startAlpha, endAlpha, delta);
    }

    void request(const std::string& name);

private:
    void create_fade(const std::string& name, uint32_t winType, uint32_t startAlpha, uint32_t endAlpha, uint32_t delta);
    void fade_handler(std::shared_ptr<egt::PeriodicTimer> handler);
    std::vector<std::shared_ptr<OverlayWindow>>& m_OverlayWinVector;
    std::vector<std::pair<std::shared_ptr<egt::PeriodicTimer>, uint32_t>> m_FadeTimerVector;
    std::vector<Fade_Data_t> m_FadeList;
    std::vector<Fade_Request_t> m_RequestList[3];
    bool m_timer_exist[3] = {
        false,  //LCDC_OVR_1
        false,  //LCDC_OVR_2
        false   //LCDC_OVR_HEO
    };
};


#endif