/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __accept_H__
#define __accept_H__

#include "eraw_define.h"

eraw_st accept_table[] = {
    {"accept", 0, 8100},    //0
    {"ble", 8100, 7602},    //1
    {"call", 15702, 10992},    //2
    {"reject", 26694, 5212},    //3
    {"sms", 31906, 5058},    //4
};

#endif

