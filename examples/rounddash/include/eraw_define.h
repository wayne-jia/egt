/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ERAW_DEFINE_H__	
#define __ERAW_DEFINE_H__	
	
#include <string>	
	
typedef struct {	
    std::string name;	
    int offset;	
    int len;	
} eraw_st;	

#endif	
