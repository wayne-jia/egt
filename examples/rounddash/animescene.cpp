/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rounddash.h"
#include <sys/time.h>

//#define GENERATE_NEEDLE_DESC_DATA
//#define TIME_STATISTICS_ENABLE
#define MAX_DIST_STRING_LEN      6
#define MAX_DIST_TRVL_DIGITS     4
#define DIST_TRAVEL_THRSH_KM     6

const float totDist = 213;
float travlDist = 0;

const locationOnMap maplocations[] = {{20, 0, 0, DIRECTION::RIGHT}, {42, 0, 0, DIRECTION::LEFT}, {22, 0, 0, DIRECTION::LEFT},
                                      {36, 0, 0, DIRECTION::RIGHT}, {16, 0, 0, DIRECTION::RIGHT}, {32, 0, 0, DIRECTION::LEFT},
                                      {20, 0, 0, DIRECTION::RIGHT}, {25, 0, 0, DIRECTION::DESTINATION}};

locationOnMap currentLocation;

uint8_t needleIndex, animeIndex;
uint8_t mapIndex = 0;
uint8_t req_speed;
bool dImageOn = true;
uint32_t current_img = 0;

bool updateDistance = true;
DIRECTION_STATUS directionStatus;

static int lastX, lastY;
uint8_t MAP_CNTR = 0, ANIME_CNTR=0;
const float sec = 0.0166666666666667;
static unsigned long len = 0;

#ifdef TIME_STATISTICS_ENABLE
static int timediff = 0;
static struct timeval time1, time2;
#endif

struct needleAnime 
{
    /* To track required  change and acceleration/deceleration */
    uint8_t regSpeed;           
    NEEDLE_STATE aChange;       
}; 

const struct needleAnime fixedAnime[] = {{48, DRIVE_PROGRESS_ACCEL}, {72, DRIVE_PROGRESS_ACCEL}, {56, DRIVE_PROGRESS_DECEL},
                                         {64, DRIVE_PROGRESS_ACCEL}, {36, DRIVE_PROGRESS_DECEL}, {24, DRIVE_PROGRESS_DECEL},
                                         {72, DRIVE_PROGRESS_ACCEL}, {77, DRIVE_PROGRESS_ACCEL}, {60, DRIVE_PROGRESS_DECEL},
                                         {24, DRIVE_PROGRESS_DECEL}};

const pic_desc needles[] = {
    {{146, 427, 148, 0}, 0},
    {{139, 424, 148, 148}, 2},
    {{133, 421, 144, 296}, 4},
    {{127, 418, 140, 440}, 6},
    {{121, 415, 135, 580}, 8},
    {{115, 411, 131, 715}, 10},
    {{110, 408, 127, 846}, 12},
    {{105, 404, 122, 973}, 14},
    {{100, 401, 117, 1095}, 16},
    {{96, 397, 112, 1212}, 18},
    {{92, 394, 107, 1324}, 20},
    {{88, 390, 101, 1431}, 22},
    {{85, 386, 96, 1532}, 24},
    {{82, 383, 91, 1628}, 26},
    {{79, 379, 85, 1719}, 28},
    {{77, 375, 79, 1804}, 30},
    {{75, 371, 74, 1883}, 32},
    {{73, 368, 68, 1957}, 34},
    {{72, 364, 61, 2025}, 36},
    {{71, 360, 56, 2086}, 38},
    {{71, 356, 50, 2142}, 40},
    {{71, 352, 44, 2192}, 42},
    {{71, 348, 38, 2236}, 44},
    {{71, 342, 32, 2274}, 46},
    {{71, 332, 37, 2306}, 48},
    {{71, 322, 43, 2343}, 50},
    {{71, 312, 49, 2386}, 52},
    {{72, 302, 55, 2435}, 54},
    {{73, 293, 60, 2490}, 56},
    {{75, 283, 67, 2550}, 58},
    {{77, 273, 74, 2617}, 60},
    {{79, 264, 79, 2691}, 62},
    {{82, 254, 85, 2770}, 64},
    {{85, 245, 91, 2855}, 66},
    {{88, 236, 96, 2946}, 68},
    {{92, 227, 101, 3042}, 70},
    {{96, 218, 106, 3143}, 72},
    {{100, 209, 112, 3249}, 74},
    {{105, 201, 116, 3361}, 76},
    {{109, 192, 122, 3477}, 78},
    {{115, 184, 126, 3599}, 80},
    {{120, 176, 131, 3725}, 82},
    {{126, 169, 135, 3856}, 84},
    {{132, 161, 140, 3991}, 86},
    {{139, 154, 143, 4131}, 88},
    {{145, 147, 147, 4274}, 90},
    {{152, 140, 151, 4421}, 92},
    {{159, 134, 154, 4572}, 94},
    {{167, 128, 158, 4726}, 96},
    {{174, 122, 161, 4884}, 98},
    {{182, 116, 164, 5045}, 100},
    {{190, 111, 167, 5209}, 102},
    {{199, 106, 169, 5376}, 104},
    {{207, 101, 172, 5545}, 106},
    {{216, 97, 174, 5717}, 108},
    {{225, 93, 176, 5891}, 110},
    {{234, 89, 178, 6067}, 112},
    {{243, 86, 179, 6245}, 114},
    {{252, 83, 180, 6424}, 116},
    {{262, 80, 181, 6604}, 118},
    {{271, 78, 182, 6785}, 120},
    {{281, 76, 182, 6967}, 122},
    {{291, 74, 183, 7149}, 124},
    {{300, 73, 183, 7332}, 126},
    {{310, 72, 183, 7515}, 128},
    {{320, 71, 183, 7698}, 130},
    {{330, 71, 183, 7881}, 132},
    {{340, 71, 182, 8064}, 134},
    {{347, 71, 182, 8246}, 136},
};

void APP_InitMap(void);
void updateMapLocation(void);

static inline int getNeedleAngle(int index)
{
    if (index < 69)
        return needles[index].angle;
    else
        return index * 2;
}

void updateNeedle(egt::detail::KMSOverlay* s, int index)
{
    if (index > 68)
        index = 68 * 2 - index;
    plane_set_pos(s->s(), needles[index].frame_attr.x, needles[index].frame_attr.y);
    plane_set_pan_pos(s->s(), 0, needles[index].frame_attr.pan_y);
    plane_set_pan_size(s->s(), NEEDLE_FB_MAX_WIDTH, needles[index].frame_attr.pan_h);
    plane_apply(s->s());
    s->schedule_flip();
}

void renderNeedles(float val, 
                   std::shared_ptr<egt::experimental::NeedleLayer> needleWidget, 
                   std::shared_ptr<OverlayWindow> needleLayer)
{
#ifdef TIME_STATISTICS_ENABLE
    gettimeofday(&time1, NULL);
#endif

    egt::Rect rect = needleWidget->rot_rect(val);
    egt::Rect rect_orig = needleWidget->get_rect_orig();

    ///--- define the crop area
    int crop_x = rect.x();
    int crop_y = rect.y();
    int crop_h = std::max(rect.height(), rect_orig.height());

#ifdef GENERATE_NEEDLE_DESC_DATA
    static int pan_y = 0;
    std::cout << "{{" << crop_x << ", " << crop_y << ", " << crop_h << ", " << pan_y << "}, " << val << "}," << std::endl;
    pan_y += crop_h;
#endif

    ///--- draw the rotated needles in a canvas
    auto surface = egt::shared_cairo_surface_t(
                    cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                            2*(INIT_NEEDLE_WIDTH+CENTER_RADIUS), 2*(INIT_NEEDLE_WIDTH+CENTER_RADIUS)),
                    cairo_surface_destroy);
    auto cr = egt::shared_cairo_t(cairo_create(surface.get()), cairo_destroy);
    egt::Painter painter(cr);
    needleWidget->drawbuf(painter, CAIRO_ANTIALIAS_GOOD);

    ///--- crop the needle to overlay FB
    auto overlay_surface = egt::shared_cairo_surface_t(
                                cairo_image_surface_create_for_data(static_cast<unsigned char*>(static_cast<unsigned char*>(needleLayer->GetOverlay()->raw()) + len),
                                CAIRO_FORMAT_ARGB32,
                                NEEDLE_FB_MAX_WIDTH, crop_h,
                                NEEDLE_FB_XSTRIDE),
                                cairo_surface_destroy);
    auto overlay_cr = egt::shared_cairo_t(cairo_create(overlay_surface.get()), cairo_destroy);
    cairo_set_source_surface(overlay_cr.get(), surface.get(), -crop_x, -crop_y);
    cairo_paint(overlay_cr.get());

    len += NEEDLE_FB_XSTRIDE * crop_h;

#ifdef TIME_STATISTICS_ENABLE
	int crop_w = std::max(rect.width(), rect_orig.width());
    gettimeofday(&time2, NULL);
    timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
    std::cout << "angle: " << val << ", size: " << crop_w << "x" << crop_h << ", antialias-best time cost: " << timediff << " us" << std::endl;
#endif
}

void APP_InitMap(void)
{
    currentLocation = maplocations[0];
    mapIndex = 0;
}

void showNavImg(uint32_t idx)
{
    for (auto img: GPSImgIndicators)
        img->hide();

    if (idx > 3)
        return;

    GPSImgIndicators[idx]->show();
    current_img = idx;
}

void APP_ProcessNeedle(egt::detail::KMSOverlay* s)
{
    switch (appData.nstate)
    {
        case TWIRL_ACCELERATE_START:
        {
            directionStatus = DIRECTION_STATUS::STRAIGHT;
            needleIndex = 0;
            req_speed = 77;
            appData.nstate = TWIRL_ACCELERATE_PROGRESS;
            break;
        }
        case TWIRL_ACCELERATE_PROGRESS:
        {
            if (getNeedleAngle(needleIndex) < req_speed)
            {
                needleIndex++;
                updateNeedle(s, needleIndex);
            }
            else
            {
                req_speed = 0;
                appData.nstate = TWIRL_DECELERATE_PROGRESS;
            }
            break;
        }
        case TWIRL_DECELERATE_PROGRESS:
        {
            if (getNeedleAngle(needleIndex) > req_speed)
            {                   
                needleIndex--;
                updateNeedle(s, needleIndex);
            }
            else
            {
                needleIndex = 0;
                appData.state = APP_STATE_FADEOUT_ICON;
            }
            break;
        }
        case DRIVE_START:
        {
            travlDist = 0;
            animeIndex = 0;
            needleIndex = 0;
            req_speed = 44;
            APP_InitMap();
            appData.nstate = DRIVE_PROGRESS_ACCEL;
            
            GPSLabels[4]->text("Time Left: 240 mins");
            GPSLabels[4]->show();
            showNavImg(current_img);
            GPSLabels[3]->show();
            GPSLabels[0]->text("Go Straight");
            GPSLabels[0]->show();
            break;
        }
        case DRIVE_PROGRESS_ACCEL:
        {
            if (getNeedleAngle(needleIndex) < req_speed)
            {
                needleIndex++;
                updateNeedle(s, needleIndex);
                GPSLabels[2]->text(std::to_string(getNeedleAngle(needleIndex)));
            }
            break;
        }
        case DRIVE_PROGRESS_DECEL:
        {
            if (getNeedleAngle(needleIndex) > req_speed)
            {          
                needleIndex--;
                updateNeedle(s, needleIndex);
                GPSLabels[2]->text(std::to_string(getNeedleAngle(needleIndex)));
            }
            else
            {
                if (directionStatus == DIRECTION_STATUS::REACHED)
                    appData.state = APP_STATE_PAUSE;
            }
            break;
        }
    }
}

void updateMapLocation(void)
{
    /* D = S x T */
    currentLocation.prev_dist = currentLocation.current_dist;
    currentLocation.current_dist = currentLocation.prev_dist+ (float)(getNeedleAngle(needleIndex) * sec);    
    /* Every 3 s update the distance label */
    MAP_CNTR++;
    if (MAP_CNTR > 2)
    {
        MAP_CNTR = 0;
        updateDistance = true;
    }

    if ((currentLocation.dist - currentLocation.current_dist) < DIST_TRAVEL_THRSH_KM)
    {
        directionStatus = DIRECTION_STATUS::NEXT;
        updateDistance = true;
    }

    if ((currentLocation.dist - currentLocation.current_dist) <= 0)
    {
        /* Go to next path in map unless you have reached destination */
        if (currentLocation.nextDirection != DIRECTION::DESTINATION)
        {
            travlDist = travlDist + currentLocation.dist;
            currentLocation = maplocations[++mapIndex];
            directionStatus = DIRECTION_STATUS::STRAIGHT;
            updateDistance = true;
        }
        else
        {
            directionStatus = DIRECTION_STATUS::REACHED;
            GPSLabels[0]->text("ARRIVED!");
            GPSLabels[3]->hide();
            GPSLabels[4]->hide();
            showNavImg(3);
            req_speed = 0;
            appData.nstate = DRIVE_PROGRESS_DECEL;
        }
    }
}

float a, b, t;
void APP_ProcessMap(void)
{
    switch (directionStatus)
    {
        case DIRECTION_STATUS::REACHED:
            showNavImg(current_img);
            return;

        case DIRECTION_STATUS::STRAIGHT:
            GPSLabels[0]->text("Go Straight");
            showNavImg(0);
            directionStatus = DIRECTION_STATUS::NO_CHANGE;
            break;

        case DIRECTION_STATUS::NEXT:
        {
            if (currentLocation.nextDirection == DIRECTION::RIGHT)
            {
                GPSLabels[0]->text("Turn Right");
                showNavImg(1);
                directionStatus = DIRECTION_STATUS::NO_CHANGE;
            }
            else if (currentLocation.nextDirection == DIRECTION::LEFT)
            {
                GPSLabels[0]->text("Turn Left");
                showNavImg(2);
                directionStatus = DIRECTION_STATUS::NO_CHANGE;
            }
            break;
        }    

        default:
            break;
    }

    if (updateDistance)
    {
        updateDistance = false;
        std::string diststr = std::to_string(static_cast<uint32_t>(currentLocation.dist - currentLocation.current_dist)) + " km";
        GPSLabels[3]->text(diststr);
    
        a = travlDist + currentLocation.current_dist;
        b = totDist - a;
        t = b;
        std::string distTravstr = "Time Left: " + std::to_string((int)t) + " mins";
        GPSLabels[4]->clear();
        GPSLabels[4]->text(distTravstr); 
    }

    updateMapLocation();

    if (dImageOn)
    {
        GPSImgIndicators[current_img]->hide();
        dImageOn = false;
    }
    else
    {
        GPSImgIndicators[current_img]->show();
        dImageOn = true;
    }
}

void checkNeedleAnime(void)
{
    if (directionStatus != DIRECTION_STATUS::REACHED)
    {
        ANIME_CNTR++;
        if (ANIME_CNTR > 2)
        {
            ANIME_CNTR=0;
            req_speed = fixedAnime[animeIndex].regSpeed;
            appData.nstate = fixedAnime[animeIndex].aChange;
            if (++animeIndex > 8)
                animeIndex = 0;
        }
    }  
}

void handle_touch(egt::Event & event) 
{
    switch (event.id())
    {
        case egt::EventId::raw_pointer_down:
        {
            if (appData.state == APP_STATE_DRIVE)
            {
                lastX = event.pointer().point.x();
                lastY = event.pointer().point.y();
                ANIME_CNTR =0;
            }
            break;
        }
        case egt::EventId::raw_pointer_up:
        {
            if (appData.state == APP_STATE_DRIVE)
            {
                lastX = event.pointer().point.x();
                lastY = event.pointer().point.y();
                ANIME_CNTR = 0;
            }  
            break;
        }
        case egt::EventId::raw_pointer_move:
        {
            ANIME_CNTR = 0;
            //we don't want to increase speed if destination has already been reached
            if ((appData.state == APP_STATE_DRIVE)  && (directionStatus != DIRECTION_STATUS::REACHED))
            {
                int diffX = abs(event.pointer().point.x() - lastX);
                int diffY = abs(lastY - event.pointer().point.y());
                
                if ((diffY > diffX && event.pointer().point.y() < lastY) || (diffX > diffY && event.pointer().point.x() > lastX))
                {
                    if (needleIndex < 120)           
                    {
                        int newIndx = needleIndex + (diffX + diffY)/3;
                        if (newIndx > 120)
                            newIndx = 120;
                        req_speed = getNeedleAngle(newIndx);
                        appData.nstate = DRIVE_PROGRESS_ACCEL;
                    }
                
                }
                else
                {
                    if (needleIndex > 0)
                    {
                        int newIndx = needleIndex - (diffX + diffY)/3;
                        if (newIndx < 0)
                            newIndx = 0;
                        req_speed = getNeedleAngle(newIndx);
                        appData.nstate = DRIVE_PROGRESS_DECEL;
                    }
                }
                lastX = event.pointer().point.x();
                lastY = event.pointer().point.y();
            }
            break;
        }
        default:
            break;
    }
};