/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rounddash.h"

//#define GENERATE_NEEDLE_DESC_DATA
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
    {{116, 411, 131, 715}, 10},
    {{110, 408, 127, 846}, 12},
    {{105, 405, 122, 973}, 14},
    {{101, 401, 117, 1095}, 16},
    {{96, 398, 112, 1212}, 18},
    {{92, 394, 107, 1324}, 20},
    {{89, 390, 102, 1431}, 22},
    {{85, 387, 97, 1533}, 24},
    {{82, 383, 91, 1630}, 26},
    {{80, 379, 86, 1721}, 28},
    {{77, 376, 80, 1807}, 30},
    {{75, 372, 74, 1887}, 32},
    {{74, 368, 68, 1961}, 34},
    {{72, 364, 63, 2029}, 36},
    {{71, 360, 57, 2092}, 38},
    {{71, 357, 51, 2149}, 40},
    {{71, 353, 44, 2200}, 42},
    {{71, 349, 38, 2244}, 44},
    {{71, 344, 32, 2282}, 46},
    {{71, 334, 35, 2314}, 48},
    {{71, 324, 42, 2349}, 50},
    {{71, 314, 48, 2391}, 52},
    {{72, 304, 54, 2439}, 54},
    {{73, 295, 59, 2493}, 56},
    {{74, 285, 65, 2552}, 58},
    {{76, 275, 73, 2617}, 60},
    {{78, 266, 78, 2690}, 62},
    {{81, 256, 84, 2768}, 64},
    {{84, 247, 89, 2852}, 66},
    {{87, 238, 95, 2941}, 68},
    {{91, 229, 100, 3036}, 70},
    {{95, 220, 105, 3136}, 72},
    {{99, 211, 111, 3241}, 74},
    {{103, 203, 115, 3352}, 76},
    {{108, 195, 120, 3467}, 78},
    {{113, 186, 125, 3587}, 80},
    {{119, 179, 129, 3712}, 82},
    {{124, 171, 134, 3841}, 84},
    {{130, 163, 139, 3975}, 86},
    {{136, 156, 142, 4114}, 88},
    {{143, 149, 146, 4256}, 90},
    {{150, 143, 149, 4402}, 92},
    {{157, 136, 153, 4551}, 94},
    {{164, 130, 157, 4704}, 96},
    {{172, 124, 160, 4861}, 98},
    {{179, 118, 163, 5021}, 100},
    {{187, 113, 166, 5184}, 102},
    {{195, 108, 168, 5350}, 104},
    {{204, 103, 171, 5518}, 106},
    {{212, 99, 173, 5689}, 108},
    {{221, 95, 174, 5862}, 110},
    {{230, 91, 176, 6036}, 112},
    {{239, 87, 179, 6212}, 114},
    {{248, 84, 180, 6391}, 116},
    {{258, 81, 181, 6571}, 118},
    {{267, 79, 181, 6752}, 120},
    {{276, 77, 182, 6933}, 122},
    {{286, 75, 183, 7115}, 124},
    {{296, 74, 183, 7298}, 126},
    {{306, 72, 183, 7481}, 128},
    {{316, 72, 183, 7664}, 130},
    {{326, 71, 183, 7847}, 132},
    {{335, 71, 183, 8030}, 134},
    {{346, 71, 182, 8213}, 136},
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
    static unsigned long len = 0;
    
    egt::Rect rect = needleWidget->rot_rect(val);
    egt::Rect rect_orig = needleWidget->get_rect_orig();

    ///--- define the crop area
    int crop_x = rect.x();
    int crop_y = rect.y();
    int crop_h = std::max(rect.height(), rect_orig.height());

    //int xstride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, NEEDLE_FB_MAX_WIDTH);
    //const auto angle = needleWidget->value2angle(val);
    //int save_h = crop_h - CENTER_RADIUS * std::sin(angle);

#ifdef GENERATE_NEEDLE_DESC_DATA
    static int pan_y = 0;
    std::cout << "{{" << crop_x << ", " << crop_y << ", " << crop_h << ", " << pan_y << "}, " << val << "}," << std::endl;
    pan_y += crop_h;
#endif

    ///--- draw the rotated needles in a canvas
    auto surface = egt::shared_cairo_surface_t(
                    cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                            2*(180+CENTER_RADIUS), 2*(180+CENTER_RADIUS)),
                    cairo_surface_destroy);
    auto cr = egt::shared_cairo_t(cairo_create(surface.get()), cairo_destroy);
    egt::Painter painter(cr);
    needleWidget->drawbuf(painter);

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

    //len += (NEEDLE_FB_MAX_WIDTH * crop_h * 4);
    len += NEEDLE_FB_XSTRIDE * crop_h;
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