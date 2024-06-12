/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ROUNDDASH_H
#define ROUNDDASH_H


#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/time.h>
#include "lcdcoverlay.h"



#define MAX_WIDTH	720
#define MAX_HEIGHT	720
#define GPS_WIDTH   340
#define GPS_HEIGHT  363
#define GPS_X       195
#define GPS_Y       265



typedef struct
{
    //leImage * imgAst;
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
    /* Application's state machine's initial state. */
    APP_STATE_INIT=0,
    APP_STATE_SHOW_BASE,
    APP_STATE_INIT_NEEDLE_SHOW,
    APP_STATE_NEEDLE_TWIRL,
    APP_STATE_FADEOUT_ICON,
    APP_STATE_SPEED_INIT1,   
    APP_STATE_SPEED_INIT2,
    APP_STATE_SPEED_INIT3,
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
    /* The application's current state */
    APP_STATES state;
    NEEDLE_STATE nstate;

} APP_DATA;

typedef struct {
    int x;
    int y;
    int pan_h;
    int pan_y;
} frame_st;

typedef struct
{
  const char *pic;
  unsigned int *len;
  frame_st frame_attr;
  int angle;
} pic_desc;



extern std::vector<std::shared_ptr<egt::ImageLabel>> GPSImgIndicators;
extern std::vector<std::shared_ptr<egt::Label>> GPSLabels;
extern APP_DATA appData;

extern void updateNeedle(egt::detail::KMSOverlay* s, int index);
extern void cp1stNeedle2Fb(char* fb);
extern void cpNeedles2Fb(char* fb);
extern void APP_ProcessNeedle(egt::detail::KMSOverlay* s);
extern void APP_ProcessMap(void);
extern void checkNeedleAnime(void);
extern void showNavImg(uint32_t idx);
extern void handle_touch(egt::Event & event);

#endif