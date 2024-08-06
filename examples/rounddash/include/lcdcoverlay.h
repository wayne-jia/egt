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

/**
 * Create an overlay window, and user can assign the window layer, set the pixcel format,
 * set the frame buffer numbers, and set the layer position/size.
 *
 * @b Example
 * @code{.cpp}
 * auto vstrip_ovr = std::make_shared<OverlayWindow>(
 *                                 egt::Rect(157, 422, 188, 14065), //layer position/size
 *                                 egt::PixelFormat::argb8888,      //layer pixcel format
 *                                 egt::WindowHint::overlay,        //layer type
 *                                 1                                //frame buffer number
 *                                 );
 * ...
 * @endcode
 */
class EGT_API OverlayWindow : public egt::Window
{
public:

    /**
     * @param[in] rect Layer position/size.
     * @param[in] format_hint Layer pixcel format.
     * @param[in] hint Layer type.
     * @param[in] num_buffers Layer frame buffer number.
     */
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

    /**
     * Get a pointer to overlay.
     */
    egt::detail::KMSOverlay* GetOverlay()
    {
        return m_overlay;
    }

private:
    egt::detail::KMSOverlay* m_overlay;
};

/**
 * This is a useful API to use the HLCDC/XLCDC alpha changes on overlay functionaility.
 * User can call API to create several fade effect queue, which is managed by this API.
 *
 * @b Example
 * @code{.cpp}
 * //Create a fade effect on the layer LCDC_OVR_2,
 * //and this fade name is "ovr2_fade_in_10", alpha range is 0~255, delta is 10
 * OverlayFade fade(OverlayWinVector, "ovr2_fade_in_10", OVERLAY_TYPE::LCDC_OVR_2, 0, 255, 10);
 * ...
 * //Add another fade effect on the layer LCDC_OVR_HEO,
 * //the name is "ovrheo_fade_out_10", alpha range is 255~0, delta is 10
 * fade.add("ovrheo_fade_out_10", OVERLAY_TYPE::LCDC_OVR_HEO, 255, 0, 10);
 * ...
 * //Once the fade effect is created or add done, the effect can be run in code anytime:
 * //Request the ovr2_fade_in_10 fade effect to start, it will set layer LCDC_OVR_2 alpha 
 * //from 0 to 255 with a delta 10 in each increment. It would stop automatically when the
 * //alpha reach the destination of range which is set in construction or add().
 * fade.request("ovr2_fade_in_10");
 * @endcode
 */
class EGT_API OverlayFade
{
public:

    /**
     * @param[in] OverlayWinVector Overlay vector.
     */
    explicit OverlayFade(std::vector<std::shared_ptr<OverlayWindow>>& OverlayWinVector)
        : m_OverlayWinVector(OverlayWinVector)
    {}

    /**
     * @param[in] OverlayWinVector Overlay vector.
     * @param[in] name The name of fade effect.
     * @param[in] winType Layer type.
     * @param[in] startAlpha Start alpha value of the fade effect.
     * @param[in] endAlpha End alpha value of the fade effect.
     * @param[in] delta Delta alpha value of the fade effect.
     */
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

    /**
     * @param[in] name The name of fade effect.
     * @param[in] winType Layer type.
     * @param[in] startAlpha Start alpha value of the fade effect.
     * @param[in] endAlpha End alpha value of the fade effect.
     * @param[in] delta Delta alpha value of the fade effect.
     */
    void add(const std::string& name, OVERLAY_TYPE winType, uint32_t startAlpha, uint32_t endAlpha, uint32_t delta)
    {
        create_fade(name, static_cast<uint32_t>(winType), startAlpha, endAlpha, delta);
    }

    /**
     * @param[in] name The name of fade effect.
     */
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