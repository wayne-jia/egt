/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef NEEDLEDASH_H
#define NEEDLEDASH_H

#include <cairo.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/time.h>
#include "lcdcoverlay.h"

/**
 * The Waveshare round panel resolution and other settings
 */
#define MAX_WIDTH	         720
#define MAX_HEIGHT	         720
#define GPS_WIDTH            340
#define GPS_HEIGHT           363
#define GPS_X                195
#define GPS_Y                265
//#define CENTER_RADIUS        109
#define NEEDLE_FB_MAX_WIDTH  208
#define NEEDLE_FB_MAX_HEIGHT 100
#define NEEDLE_FB_XSTRIDE    832
#define NEEDLE_CENTER_X      400
#define NEEDLE_CENTER_Y      308
#define NEEDLE_WIDTH         202
#define NEEDLE_HEIGHT        24

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t speed;
} needleObj;

typedef enum
{
    TWIRL_ACCELERATE_START=0,
    TWIRL_ACCELERATE_PROGRESS,
    TWIRL_DECELERATE_PROGRESS,
    DRIVE_START,
    DRIVE_PROGRESS_ACCEL,
    DRIVE_PROGRESS_DECEL,
} NEEDLE_STATE;

typedef enum
{
    APP_STATE_INIT=0,
    APP_STATE_INIT_NEEDLE_SHOW,
    APP_STATE_NEEDLE_TWIRL,
    APP_STATE_FADEOUT_ICON,
    APP_STATE_SPEED_INIT1,   
    APP_STATE_SPEED_INIT2,
    APP_STATE_SPEED_INIT3,
    APP_STATE_INIT_INPUT,
    APP_STATE_DRIVE,
    APP_STATE_PAUSE,
    APP_STATE_REACHED,
    APP_STATE_LOOPBACK,
} APP_STATES;

enum class DIRECTION
{
    RIGHT=1,
    LEFT,
    DESTINATION,
};

enum class DIRECTION_STATUS
{
    NEXT=0,
    STRAIGHT,
    NO_CHANGE,
    REACHED,
};

typedef struct
{
    float dist;
    float prev_dist;
    float current_dist;
    DIRECTION nextDirection;
} locationOnMap;

typedef struct
{
    APP_STATES state;
    NEEDLE_STATE nstate;
} APP_DATA;

typedef struct
{
    int x;
    int y;
    int pan_h;
    int pan_y;
} frame_st;

typedef struct
{
    frame_st frame_attr;
    int angle;
} pic_desc;

extern std::vector<std::shared_ptr<egt::ImageLabel>> GPSImgIndicators;
extern std::vector<std::shared_ptr<egt::Label>> GPSLabels;
extern APP_DATA appData;
extern const pic_desc needles[56];
extern void updateNeedle(egt::detail::KMSOverlay* s, int index);
extern void cp1stNeedle2Fb(char* fb);
extern void cpNeedles2Fb(char* fb);
extern void APP_ProcessNeedle(egt::detail::KMSOverlay* s);
extern void APP_ProcessMap(void);
extern void checkNeedleAnime(void);
extern void showNavImg(uint32_t idx);
extern void handle_touch(egt::Event & event);
extern void renderNeedles(float val, 
                   std::shared_ptr<egt::experimental::NeedleLayer> needleWidget, 
                   std::shared_ptr<OverlayWindow> needleLayer);
extern void drawArc(std::shared_ptr<OverlayWindow> overlay, int start, int end);

class MySpinProgress : public egt::SpinProgress
{
public:
	explicit MySpinProgress(const egt::Rect& rect,
                            int start = 0, int end = 180, int value = 0) noexcept
        : egt::SpinProgress(rect, start, end, value)
	{}
	void draw( egt::Painter& painter, const egt::Rect& rect) override
	{
		 Mydefault_draw(*this, painter, rect);
	}

private:
	void  Mydefault_draw(egt::SpinProgressType<int>& widget, egt::Painter& painter, const egt::Rect& rect)
    {
		egt::detail::ignoreparam(rect);

        auto b = widget.content_area();

        auto dim = std::min(b.width(), b.height());
        float linew = 10; dim / 5.0f;
        float radius = 200; dim / 2.0f - (linew / 2.0f);
        auto angle1 = egt::detail::to_radians<float>(180, 0);

        auto min = std::min(widget.starting(), widget.ending());
        auto max = std::max(widget.starting(), widget.ending());
        auto angle2 = egt::detail::to_radians<float>(180.0f,
                                                egt::detail::normalize_to_angle(static_cast<float>(widget.value()),
                                                static_cast<float>(min), static_cast<float>(max), 0.0f, 360.0f, true));

		painter.line_width(linew);
        std::cout << "linew: " << linew << " radius: " << radius << " angle1: " << angle1 << " angle2: " << angle2 << std::endl;
	    //painter.set(egt::Color(egt::Palette::black, 255));
        painter.set(egt::Color(egt::Palette::red, 245));
        
        painter.draw(egt::Arc(widget.center(), radius, angle1, angle2));
        //painter.paint(0);
        painter.stroke();
    }
};

#endif
