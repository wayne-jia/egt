/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lcdcoverlay.h"

void OverlayFade::request(const std::string& name)
{
    Fade_Request_t request;
    for (auto i = 0; i < m_FadeList.size(); i++)
    {
        if (egt::detail::hash(name) == egt::detail::hash(m_FadeList[i].name))
        {
            request.dataIdx = i;
            request.status = FADE_STATUS::IDLE;
            request.alpha = m_FadeList[i].startAlpha;
            m_RequestList[m_FadeList[i].winType].emplace_back(std::move(request));
    
            for (auto timer: m_FadeTimerVector)
            {
                if (timer.second == m_FadeList[i].winType)
                {
                    if (!timer.first->running())
                        timer.first->start();

                    return;
                }
            }
        }
    }
    std::cerr << "request a non-exist fade name!" << std::endl;
}

void OverlayFade::fade_handler(std::shared_ptr<egt::PeriodicTimer> handler)
{
    uint32_t ovr = 0;
    switch (egt::detail::hash(handler->name()))
    {
        case egt::detail::hash("0"):
            ovr = 0;
            break;

        case egt::detail::hash("1"):
            ovr = 1;
            break;

        case egt::detail::hash("2"):
            ovr = 2;
            break;
        
        default:
            break;
    }

    if (m_RequestList[ovr].empty())
    {
        handler->stop();
        return;
    }

    auto idx = m_RequestList[ovr][0].dataIdx;
    auto winType = m_FadeList[idx].winType;

    switch (m_RequestList[ovr][0].status)
    {
        case FADE_STATUS::IDLE:
            m_RequestList[ovr][0].status = FADE_STATUS::RUN;
            break;
        
        case FADE_STATUS::RUN:
        {
            //Fade out
            if (m_FadeList[idx].startAlpha > m_FadeList[idx].endAlpha)
            {
                if (m_RequestList[ovr][0].alpha > m_FadeList[idx].endAlpha + m_FadeList[idx].delta)
                {
                    m_RequestList[ovr][0].alpha -= m_FadeList[idx].delta;
                }
                else
                {
                    m_RequestList[ovr][0].alpha = m_FadeList[idx].endAlpha;
                    m_RequestList[ovr][0].status = FADE_STATUS::DONE;
                }
            }
            //Fade in
            else if (m_FadeList[idx].startAlpha < m_FadeList[idx].endAlpha)
            {
                if (m_RequestList[ovr][0].alpha < m_FadeList[idx].endAlpha - m_FadeList[idx].delta)
                {
                    m_RequestList[ovr][0].alpha += m_FadeList[idx].delta;
                }
                else
                {
                    m_RequestList[ovr][0].alpha = m_FadeList[idx].endAlpha;
                    m_RequestList[ovr][0].status = FADE_STATUS::DONE;
                }
            }
            break;
        }

        case FADE_STATUS::DONE:  //Finish, pop the head of queue
            m_RequestList[ovr].erase(m_RequestList[ovr].begin());
            return;

        default:
            break;
    }
    plane_set_alpha(m_OverlayWinVector[winType]->GetOverlay()->s(), m_RequestList[ovr][0].alpha);
    plane_apply(m_OverlayWinVector[winType]->GetOverlay()->s());
}

void OverlayFade::create_fade(const std::string& name, uint32_t winType, uint32_t startAlpha, uint32_t endAlpha, uint32_t delta)
{
    Fade_Data_t request_data = {
        .name = name,
        .winType = winType,
        .startAlpha = startAlpha,
        .endAlpha = endAlpha,
        .delta = delta
    };
    m_FadeList.emplace_back(std::move(request_data));

    if (m_timer_exist[winType])
        return;

    m_timer_exist[winType] = true;

    auto fadeTimer = std::make_shared<egt::PeriodicTimer>(std::chrono::milliseconds(40));
    fadeTimer->name(std::to_string(winType));
    fadeTimer->on_timeout([this, fadeTimer]() { fade_handler(fadeTimer); });
    m_FadeTimerVector.emplace_back(fadeTimer, winType);
}