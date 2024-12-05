/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "needledash.h"

//#define GENERATE_NEEDLE_DESC_DATA
#define MAX_DIST_STRING_LEN      6
#define MAX_DIST_TRVL_DIGITS     4
#define DIST_TRAVEL_THRSH_KM     6
#define GET_MIRROR_INDEX(index) ((index) - 2 * ((index) - 55))


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
    {{217, 293, 92, 0}, 0},
    {{215, 293, 92, 92}, 2},
    {{214, 294, 86, 184}, 4},
    {{212, 294, 78, 270}, 6},
    {{211, 294, 72, 348}, 8},
    {{210, 295, 66, 420}, 10},
    {{210, 295, 58, 486}, 12},
    {{210, 295, 52, 544}, 14},
    {{210, 296, 45, 596}, 16},
    {{210, 296, 38, 641}, 18},
    {{210, 296, 31, 679}, 20},
    {{210, 290, 31, 710}, 22},
    {{210, 283, 38, 741}, 24},
    {{210, 277, 45, 779}, 26},
    {{210, 270, 52, 824}, 28},
    {{210, 264, 58, 876}, 30},
    {{211, 257, 66, 934}, 32},
    {{212, 251, 72, 1000}, 34},
    {{214, 245, 78, 1072}, 36},
    {{215, 238, 86, 1150}, 38},
    {{217, 232, 92, 1236}, 40},
    {{219, 226, 98, 1328}, 42},
    {{221, 220, 104, 1426}, 44},
    {{223, 214, 111, 1530}, 46},
    {{226, 209, 116, 1641}, 48},
    {{229, 203, 122, 1757}, 50},
    {{232, 198, 127, 1879}, 52},
    {{235, 192, 133, 2006}, 54},
    {{239, 187, 138, 2139}, 56},
    {{242, 182, 143, 2277}, 58},
    {{246, 177, 148, 2420}, 60},
    {{250, 172, 153, 2568}, 62},
    {{254, 168, 157, 2721}, 64},
    {{259, 163, 162, 2878}, 66},
    {{263, 159, 166, 3040}, 68},
    {{268, 155, 170, 3206}, 70},
    {{273, 151, 174, 3376}, 72},
    {{278, 148, 177, 3550}, 74},
    {{283, 144, 181, 3727}, 76},
    {{289, 141, 184, 3908}, 78},
    {{294, 138, 187, 4092}, 80},
    {{300, 135, 190, 4279}, 82},
    {{305, 132, 193, 4469}, 84},
    {{311, 130, 194, 4662}, 86},
    {{317, 128, 196, 4856}, 88},
    {{323, 126, 198, 5052}, 90},
    {{329, 124, 200, 5250}, 92},
    {{336, 123, 200, 5450}, 94},
    {{342, 121, 202, 5650}, 96},
    {{348, 120, 203, 5852}, 98},
    {{355, 119, 203, 6055}, 100},
    {{361, 119, 203, 6258}, 102},
    {{368, 118, 204, 6461}, 104},
    {{374, 118, 204, 6665}, 106},
    {{381, 118, 203, 6869}, 108},
    {{388, 118, 203, 7072}, 110},  //index:55, 90°
};

void APP_InitMap(void);
void updateMapLocation(void);

static inline int getNeedleAngle(int index)
{
    if (index < 56)
        return needles[index].angle;
    else
        return 110 + 2 * (index - 55);   // mirror index
}

void updateNeedle(egt::detail::KMSOverlay* s, int index)
{
    int x = needles[index].frame_attr.x;
    if (index > 55)
    {
        index = GET_MIRROR_INDEX(index);
        x = 592 - x; // x` = Cx - x - pan_w + Cx; Cx: the center x=400, pan_w=NEEDLE_FB_MAX_WIDTH
        plane_set_rotate(s->s(), 360);  // 360: X-Mirror; 450: Y-Mirror
    }

    plane_set_pos(s->s(), x, needles[index].frame_attr.y);
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
    //std::cout << "xstride: " << xstride << std::endl;
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
                            NEEDLE_WIDTH*2+12, NEEDLE_WIDTH*2+12),
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

    //len += (NEEDLE_FB_MAX_WIDTH * crop_h * 4);
    len += NEEDLE_FB_XSTRIDE * crop_h;
}

#if 1
void  drawArc(std::shared_ptr<OverlayWindow> overlay, int start, int end)
{
    auto overlay_surface = egt::shared_cairo_surface_t(
                                cairo_image_surface_create_for_data(static_cast<unsigned char*>(overlay->GetOverlay()->raw()),
                                CAIRO_FORMAT_ARGB32,
                                480, 480,
                                1920),
                                cairo_surface_destroy);
    auto overlay_cr = egt::shared_cairo_t(cairo_create(overlay_surface.get()), cairo_destroy);
    //egt::Painter painter(overlay_cr);

// int xstride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, 480);
// std::cout << "xstride: " << xstride << std::endl;

    float linew = 80;
    float radius = 200;
    auto angle1 = egt::detail::to_radians<float>(0.0f, start);

    auto angle2 = egt::detail::to_radians<float>(0.0f,
                                            egt::detail::normalize_to_angle(static_cast<float>(end),
                                            0.0f, 360.0f, 0.0f, 360.0f, true));

    cairo_set_operator(overlay_cr.get(), CAIRO_OPERATOR_SOURCE);
    //painter.line_width(linew);
    cairo_set_line_width(overlay_cr.get(), linew);
    //painter.set(egt::Color(egt::Palette::black, 255));
    cairo_set_source_rgba(overlay_cr.get(), 0.0f, 0.0f, 0.0f, 0.0f);
    //painter.draw(egt::Arc(egt::Point(240, 240), radius, angle1, angle2));
    cairo_arc(overlay_cr.get(), 240, 240, radius, angle1, angle2);
    //painter.paint(0);
    //painter.stroke();
    //cairo_stroke(overlay_cr.get());
    cairo_stroke_preserve(overlay_cr.get());
    //cairo_fill(overlay_cr.get());
    //cairo_paint(overlay_cr.get());
    //cairo_paint_with_alpha(overlay_cr.get(), 0.5f);
    //cairo_surface_flush(overlay_surface.get());
    overlay->GetOverlay()->schedule_flip();
}
#endif

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