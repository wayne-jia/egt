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
    {{157, 422, 300, 0}, 0},
    {{151, 420, 146, 300}, 2},
    {{145, 419, 135, 446}, 4},
    {{133, 412, 130, 581}, 6},
    {{132, 410, 127, 711}, 8},
    {{126, 406, 119, 838}, 10},
    {{111, 407, 110, 957}, 12},
    {{110, 394, 120, 1067}, 14},
    {{103, 374, 125, 1187}, 16},
    {{102, 393, 93, 1312}, 18},
    {{92, 382, 90, 1405}, 20},
    {{92, 378, 90, 1495}, 22},
    {{89, 379, 80, 1585}, 24},
    {{85, 368, 80, 1665}, 26},
    {{81, 367, 70, 1745}, 28},
    {{80, 362, 70, 1815}, 30},
    {{80, 363, 55, 1885}, 32},
    {{77, 357, 50, 1940}, 34},
    {{79, 347, 50, 1990}, 36},
    {{74, 348, 40, 2040}, 38},
    {{74, 350, 27, 2080}, 40},
    {{74, 341, 30, 2107}, 42},
    {{74, 335, 34, 2137}, 44},
    {{76, 323, 47, 2171}, 46},
    {{74, 316, 45, 2218}, 48},
    {{75, 296, 58, 2263}, 50},
    {{77, 286, 65, 2321}, 52},
    {{83, 281, 65, 2386}, 54},
    {{86, 271, 74, 2451}, 56},
    {{87, 258, 80, 2525}, 58},
    {{95, 249, 84, 2605}, 60},
    {{102, 238, 90, 2689}, 62},
    {{105, 227, 99, 2779}, 64},
    {{109, 220, 101, 2878}, 66},
    {{114, 211, 110, 2979}, 68},
    {{120, 201, 112, 3089}, 70},
    {{127, 190, 118, 3201}, 72},
    {{132, 186, 124, 3319}, 74},
    {{138, 178, 126, 3443}, 76},
    {{144, 170, 134, 3569}, 78},
    {{154, 160, 135, 3703}, 80},
    {{164, 149, 141, 3838}, 82},
    {{169, 142, 145, 3979}, 84},
    {{180, 138, 147, 4124}, 86},
    {{180, 123, 155, 4271}, 88},
    {{197, 124, 159, 4426}, 90},
    {{203, 118, 160, 4585}, 92},
    {{215, 112, 167, 4745}, 94},
    {{224, 102, 175, 4912}, 96},
    {{231, 103, 167, 5087}, 98},
    {{247, 95, 175, 5254}, 100},
    {{256, 95, 167, 5429}, 102},
    {{266, 92, 170, 5596}, 104},
    {{275, 89, 174, 5766}, 106},
    {{276, 79, 185, 5940}, 108},
    {{298, 79, 179, 6125}, 110},
    {{312, 75, 183, 6304}, 112},
    {{321, 74, 181, 6487}, 114},
    {{329, 76, 179, 6668}, 116},
    {{338, 74, 179, 6847}, 118},
    {{346, 71, 180, 7026}, 120},
    {{351, 73, 180, 7206}, 122},
    {{356, 72, 185, 7386}, 124},
    {{359, 74, 183, 7571}, 126},
    {{354, 73, 185, 7754}, 128},
    {{369, 76, 183, 7939}, 130},
    {{369, 76, 185, 8122}, 132},
    {{371, 76, 185, 8307}, 134},
    {{372, 78, 185, 8492}, 136},
    {{378, 85, 181, 8677}, 138},
    {{388, 89, 181, 8858}, 140},
    {{393, 96, 176, 9039}, 142},
    {{392, 96, 180, 9215}, 144},
    {{400, 103, 177, 9395}, 146},
    {{404, 110, 170, 9572}, 148},
    {{411, 119, 165, 9742}, 150},
    {{412, 121, 170, 9907}, 152},
    {{414, 126, 163, 10077}, 154},
    {{417, 138, 145, 10240}, 156},
    {{423, 146, 150, 10385}, 158},
    {{425, 154, 145, 10535}, 160},
    {{430, 162, 140, 10680}, 162},
    {{434, 173, 130, 10820}, 164},
    {{436, 179, 125, 10950}, 166},
    {{439, 188, 120, 11075}, 168},
    {{443, 197, 114, 11195}, 170},
    {{445, 206, 110, 11309}, 172},
    {{449, 215, 106, 11419}, 174},
    {{450, 220, 100, 11525}, 176},
    {{455, 235, 95, 11625}, 178},
    {{457, 245, 86, 11720}, 180},
    {{457, 254, 83, 11806}, 182},
    {{460, 265, 77, 11889}, 184},
    {{462, 275, 73, 11966}, 186},
    {{464, 284, 67, 12039}, 188},
    {{465, 296, 61, 12106}, 190},
    {{465, 304, 54, 12167}, 192},
    {{468, 318, 50, 12221}, 194},
    {{468, 326, 45, 12271}, 196},
    {{468, 337, 38, 12316}, 198},
    {{469, 350, 30, 12354}, 200},
    {{469, 352, 33, 12384}, 202},
    {{468, 358, 35, 12417}, 204},
    {{466, 363, 38, 12452}, 206},
    {{465, 367, 43, 12490}, 208},
    {{464, 373, 51, 12533}, 210},
    {{463, 377, 53, 12584}, 212},
    {{462, 380, 60, 12637}, 214},
    {{457, 386, 67, 12697}, 216},
    {{457, 388, 75, 12764}, 218},
    {{453, 389, 86, 12839}, 220},
    {{454, 394, 86, 12925}, 222},
    {{449, 401, 97, 13011}, 224},
    {{447, 406, 95, 13108}, 226},
    {{446, 408, 104, 13203}, 228},
    {{443, 411, 110, 13307}, 230},
    {{441, 415, 121, 13417}, 232},
    {{438, 417, 128, 13538}, 234},
    {{438, 417, 130, 13666}, 236},
    {{432, 424, 132, 13796}, 238},
    {{428, 428, 137, 13928}, 240},
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
            if (needles[needleIndex].angle < req_speed)
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
            if (needles[needleIndex].angle > req_speed)
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
            if (needles[needleIndex].angle < req_speed)
            {
                needleIndex++;
                updateNeedle(s, needleIndex);
                GPSLabels[2]->text(std::to_string(needles[needleIndex].angle));
            }
            break;
        }
        case DRIVE_PROGRESS_DECEL:
        {
            if (needles[needleIndex].angle > req_speed)
            {          
                needleIndex--;
                updateNeedle(s, needleIndex);
                GPSLabels[2]->text(std::to_string(needles[needleIndex].angle));
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
    currentLocation.current_dist = currentLocation.prev_dist+ (float)(needles[needleIndex].angle*sec);    
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
                        req_speed = needles[newIndx].angle;
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