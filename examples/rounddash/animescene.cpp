/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rounddash.h"


#define MAX_DIST_STRING_LEN      6
#define MAX_DIST_TRVL_DIGITS     4
#define DIST_TRAVEL_THRSH_KM     6



const float totDist = 213;
float travlDist = 0;

locationOnMap maplocations[8]={{20, 0, 0, DIRECTION::RIGHT},{42, 0, 0, DIRECTION::LEFT},{22, 0, 0, DIRECTION::LEFT},
                               {36, 0, 0, DIRECTION::RIGHT},{16, 0, 0, DIRECTION::RIGHT},{32, 0, 0, DIRECTION::LEFT},
                               {20, 0, 0, DIRECTION::RIGHT},{25, 0, 0, DIRECTION::DESTINATION}};

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

struct needleAnime fixedAnime[10]= {{48, DRIVE_PROGRESS_ACCEL}, {72, DRIVE_PROGRESS_ACCEL},
{56, DRIVE_PROGRESS_DECEL}, {64,DRIVE_PROGRESS_ACCEL}, {36, DRIVE_PROGRESS_DECEL},
{24, DRIVE_PROGRESS_DECEL}, {72,DRIVE_PROGRESS_ACCEL}, {77,DRIVE_PROGRESS_ACCEL}, 
{60,DRIVE_PROGRESS_DECEL}, {24,DRIVE_PROGRESS_DECEL}};


pic_desc needles[] = {
    {{157, 422, 137, 0}, 0},
    {{151, 420, 137, 137}, 2},
    {{145, 419, 133, 274}, 4},
    {{133, 412, 129, 407}, 6},
    {{132, 410, 124, 536}, 8},
    {{126, 406, 120, 660}, 10},
    {{111, 407, 115, 780}, 12},
    {{110, 394, 109, 895}, 14},
    {{103, 374, 104, 1004}, 16},
    {{102, 393, 99, 1108}, 18},
    {{92, 382, 94, 1207}, 20},
    {{92, 378, 88, 1301}, 22},
    {{89, 379, 82, 1389}, 24},
    {{85, 368, 77, 1471}, 26},
    {{81, 367, 71, 1548}, 28},
    {{80, 362, 65, 1619}, 30},
    {{80, 363, 59, 1684}, 32},
    {{77, 357, 52, 1743}, 34},
    {{79, 347, 46, 1795}, 36},
    {{74, 348, 40, 1841}, 38},
    {{74, 350, 34, 1881}, 40},
    {{74, 341, 33, 1915}, 42},
    {{74, 335, 39, 1948}, 44},
    {{76, 323, 45, 1987}, 46},
    {{74, 316, 51, 2032}, 48},
    {{75, 296, 58, 2083}, 50},
    {{77, 286, 64, 2141}, 52},
    {{83, 281, 70, 2205}, 54},
    {{86, 271, 76, 2275}, 56},
    {{87, 258, 81, 2351}, 58},
    {{95, 249, 87, 2432}, 60},
    {{102, 238, 92, 2519}, 62},
    {{105, 227, 98, 2611}, 64},
    {{109, 220, 103, 2709}, 66},
    {{114, 211, 108, 2812}, 68},
    {{120, 201, 114, 2920}, 70},
    {{127, 190, 118, 3034}, 72},
    {{132, 186, 123, 3152}, 74},
    {{138, 178, 128, 3275}, 76},
    {{144, 170, 133, 3403}, 78},
    {{154, 160, 136, 3536}, 80},
    {{164, 149, 141, 3672}, 82},
    {{169, 142, 145, 3813}, 84},
    {{180, 138, 148, 3958}, 86},
    {{180, 123, 152, 4106}, 88},
    {{197, 124, 155, 4258}, 90},
    {{203, 118, 158, 4413}, 92},
    {{215, 112, 162, 4571}, 94},
    {{224, 102, 164, 4733}, 96},
    {{231, 103, 167, 4897}, 98},
    {{247, 95, 170, 5064}, 100},
    {{256, 95, 172, 5234}, 102},
    {{266, 92, 174, 5406}, 104},
    {{275, 89, 175, 5580}, 106},
    {{276, 79, 177, 5755}, 108},
    {{298, 79, 178, 5932}, 110},
    {{312, 75, 180, 6110}, 112},
    {{321, 74, 180, 6290}, 114},
    {{329, 76, 181, 6470}, 116},
    {{338, 74, 181, 6651}, 118},
    {{346, 71, 182, 6832}, 120},
    {{351, 73, 182, 7014}, 122},
    {{356, 72, 182, 7196}, 124},
    {{359, 74, 182, 7378}, 126},
    {{354, 73, 181, 7560}, 128},
    {{369, 76, 181, 7741}, 130},
    {{369, 76, 181, 7922}, 132},
    {{371, 76, 181, 8103}, 134},
    {{372, 78, 182, 8284}, 136},
    {{378, 85, 182, 8466}, 138},
    {{388, 89, 182, 8648}, 140},
    {{393, 96, 182, 8830}, 142},
    {{392, 96, 182, 9012}, 144},
    {{400, 103, 182, 9194}, 146},
    {{404, 110, 180, 9376}, 148},
    {{411, 119, 180, 9556}, 150},
    {{412, 121, 178, 9736}, 152},
    {{414, 126, 177, 9914}, 154},
    {{417, 138, 176, 10091}, 156},
    {{423, 146, 173, 10267}, 158},
    {{425, 154, 172, 10440}, 160},
    {{430, 162, 169, 10612}, 162},
    {{434, 173, 167, 10781}, 164},
    {{436, 179, 165, 10948}, 166},
    {{439, 188, 161, 11113}, 168},
    {{443, 197, 159, 11274}, 170},
    {{445, 206, 155, 11433}, 172},
    {{449, 215, 152, 11588}, 174},
    {{450, 220, 149, 11740}, 176},
    {{455, 235, 145, 11889}, 178},
    {{457, 245, 140, 12034}, 180},
    {{457, 254, 136, 12174}, 182},
    {{460, 265, 132, 12310}, 184},
    {{462, 275, 128, 12442}, 186},
    {{464, 284, 123, 12570}, 188},
    {{465, 296, 119, 12693}, 190},
    {{465, 304, 114, 12812}, 192},
    {{468, 318, 108, 12926}, 194},
    {{468, 326, 103, 13034}, 196},
    {{468, 337, 98, 13137}, 198},
    {{469, 350, 93, 13235}, 200},
    {{469, 352, 87, 13328}, 202},
    {{468, 358, 81, 13415}, 204},
    {{466, 363, 76, 13496}, 206},
    {{465, 367, 70, 13572}, 208},
    {{464, 373, 64, 13642}, 210},
    {{463, 377, 58, 13706}, 212},
    {{462, 380, 51, 13764}, 214},
    {{457, 386, 45, 13815}, 216},
    {{457, 388, 39, 13860}, 218},
    {{453, 389, 33, 13899}, 220},
    {{454, 394, 33, 13932}, 222},
    {{449, 401, 39, 13965}, 224},
    {{447, 406, 45, 14004}, 226},
    {{446, 408, 51, 14049}, 228},
    {{443, 411, 58, 14100}, 230},
    {{441, 415, 64, 14158}, 232},
    {{438, 417, 70, 14222}, 234},
    {{438, 417, 76, 14292}, 236},
    {{432, 424, 81, 14368}, 238},
    {{428, 428, 87, 14449}, 240},
};

void APP_InitMap(void);
void updateMapLocation(void);


void updateNeedle(egt::detail::KMSOverlay* s, int index)
{
    plane_set_pos(s->s(), needles[index].frame_attr.x, needles[index].frame_attr.y);
    plane_set_pan_pos(s->s(), 0, needles[index].frame_attr.pan_y);
    plane_set_pan_size(s->s(), 188, needles[index].frame_attr.pan_h);
    plane_apply(s->s());
    s->schedule_flip();
}

void renderNeedles(float val, 
                   std::shared_ptr<egt::experimental::NeedleLayer> needleWidget, 
                   std::shared_ptr<OverlayWindow> needleLayer)
{
    static unsigned long len = 0;
    //static int last_h = 0;
    egt::Rect rect = needleWidget->rot_rect(val);
    egt::Rect rect_orig = needleWidget->get_rect_orig();

    if (!val)
    {
        rect = egt::Rect(33, 170, 156, 137);
    }

    ///--- define the crop area
    int crop_x = rect.x();
    int crop_y = rect.y();
    int crop_w = MAX(rect.width(), rect_orig.width());
    int crop_h = MAX(rect.height(), rect_orig.height());

    ///--- draw the rotated needles in a canvas
    auto surface = egt::shared_cairo_surface_t(
                    cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                            360, 360),
                    cairo_surface_destroy);
    auto cr = egt::shared_cairo_t(cairo_create(surface.get()), cairo_destroy);
    egt::Painter painter(cr);
    needleWidget->drawbuf(painter);
    //std::cout << crop_h << ", " << last_h << std::endl;

    ///--- crop the needle to overlay FB
    auto overlay_surface = egt::shared_cairo_surface_t(
                                cairo_image_surface_create_for_data(static_cast<unsigned char*>(needleLayer->GetOverlay()->raw() + len),
                                CAIRO_FORMAT_ARGB32,
                                188, crop_h,
                                cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, 188)),
                                cairo_surface_destroy);
    auto overlay_cr = egt::shared_cairo_t(cairo_create(overlay_surface.get()), cairo_destroy);
    cairo_set_source_surface(overlay_cr.get(), surface.get(), -crop_x, -crop_y);
    cairo_paint(overlay_cr.get());
    //last_h += crop_h;
    len += (188 * crop_h * 4);  
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
            if(needles[needleIndex].angle < req_speed)
            {
                needleIndex++;
                updateNeedle(s, needleIndex);
            }else
            {
                req_speed = 0;
                appData.nstate = TWIRL_DECELERATE_PROGRESS;
            }
            break;
        }
        case TWIRL_DECELERATE_PROGRESS:
        {
            if(needles[needleIndex].angle > req_speed)
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
            if(needles[needleIndex].angle < req_speed)
            {
                needleIndex++;
                updateNeedle(s, needleIndex);
                GPSLabels[2]->text(std::to_string(needles[needleIndex].angle));
            }
            break;
        }
        case DRIVE_PROGRESS_DECEL:
        {
            if(needles[needleIndex].angle > req_speed)
            {          
                needleIndex--;
                updateNeedle(s, needleIndex);
                GPSLabels[2]->text(std::to_string(needles[needleIndex].angle));
            }
            else
            {
                if(directionStatus == DIRECTION_STATUS::REACHED)
                {
                    appData.state = APP_STATE_PAUSE;
                    
                }
            }
            break;
        }
    }
    
}

void updateMapLocation(void)
{
    /* D = S x T */
    currentLocation.prev_dist = currentLocation.current_dist;
    currentLocation.current_dist = currentLocation.prev_dist+ (float)(needles[needleIndex].angle*sec);    
    /* Every 3 s update the distance label */
    MAP_CNTR++;
    if(MAP_CNTR > 2)
    {
        MAP_CNTR = 0;
        updateDistance = true;
    }

    if((currentLocation.dist - currentLocation.current_dist) < DIST_TRAVEL_THRSH_KM)
    {
        directionStatus = DIRECTION_STATUS::NEXT;
        updateDistance = true;
    }
    if((currentLocation.dist - currentLocation.current_dist) <= 0)
    {
        /* Go to next path in map unless you have reached destination */
        if(currentLocation.nextDirection != DIRECTION::DESTINATION)
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
            if(currentLocation.nextDirection == DIRECTION::RIGHT)
            {
                GPSLabels[0]->text("Turn Right");
                showNavImg(1);
                directionStatus = DIRECTION_STATUS::NO_CHANGE;
            }
            else if(currentLocation.nextDirection == DIRECTION::LEFT)
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

    if(updateDistance)
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

    if(dImageOn)
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
    if(directionStatus != DIRECTION_STATUS::REACHED)
    {
        ANIME_CNTR++;
        if(ANIME_CNTR > 2)
        {
            ANIME_CNTR=0;
            req_speed = fixedAnime[animeIndex].regSpeed;
            appData.nstate = fixedAnime[animeIndex].aChange;
            if(++animeIndex > 8)
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
            if(appData.state == APP_STATE_DRIVE)
            {
                lastX = event.pointer().point.x();
                lastY = event.pointer().point.y();
                ANIME_CNTR =0;
            }
            break;
        }
        case egt::EventId::raw_pointer_up:
        {
            if(appData.state == APP_STATE_DRIVE)
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
            if((appData.state == APP_STATE_DRIVE)  && (directionStatus != DIRECTION_STATUS::REACHED))
            {
                int diffX = abs(event.pointer().point.x() - lastX);
                int diffY = abs(lastY - event.pointer().point.y());
                
                if ((diffY > diffX && event.pointer().point.y() < lastY) || (diffX > diffY && event.pointer().point.x() > lastX))
                {
                    if(needleIndex < 120)           
                    {
                        int newIndx = needleIndex + (diffX + diffY)/3;
                        if(newIndx > 120)
                            newIndx = 120;
                        req_speed = needles[newIndx].angle;
                        appData.nstate = DRIVE_PROGRESS_ACCEL;
                    }
                
                }
                else
                {
                    if( needleIndex > 0 )
                    {
                        int newIndx = needleIndex - (diffX + diffY)/3;
                        if(newIndx < 0)
                            newIndx = 0;
                        req_speed = needles[newIndx].angle;
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