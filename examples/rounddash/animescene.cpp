/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rounddash.h"
#include "needlespics.h"

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
    {eN0_157_422_188x300_data, &eN0_157_422_188x300_data_len, {157, 422, 300, 0}, 0},
    {eN2_151_420_188x146_data, &eN2_151_420_188x146_data_len, {151, 420, 146, 300}, 2},
    {eN4_145_419_188x135_data, &eN4_145_419_188x135_data_len, {145, 419, 135, 446}, 4},
    {eN6_133_412_188x130_data, &eN6_133_412_188x130_data_len, {133, 412, 130, 581}, 6},
    {eN8_132_410_188x127_data, &eN8_132_410_188x127_data_len, {132, 410, 127, 711}, 8},
    {eN10_126_406_188x119_data, &eN10_126_406_188x119_data_len, {126, 406, 119, 838}, 10},
    {eN12_111_407_188x110_data, &eN12_111_407_188x110_data_len, {111, 407, 110, 957}, 12},
    {eN14_110_394_188x120_data, &eN14_110_394_188x120_data_len, {110, 394, 120, 1067}, 14},
    {eN16_103_374_188x125_data, &eN16_103_374_188x125_data_len, {103, 374, 125, 1187}, 16},
    {eN18_102_393_188x93_data, &eN18_102_393_188x93_data_len, {102, 393, 93, 1312}, 18},
    {eN20_92_382_188x90_data, &eN20_92_382_188x90_data_len, {92, 382, 90, 1405}, 20},
    {eN22_92_378_188x90_data, &eN22_92_378_188x90_data_len, {92, 378, 90, 1495}, 22},
    {eN24_89_379_188x80_data, &eN24_89_379_188x80_data_len, {89, 379, 80, 1585}, 24},
    {eN26_85_368_188x80_data, &eN26_85_368_188x80_data_len, {85, 368, 80, 1665}, 26},
    {eN28_81_367_188x70_data, &eN28_81_367_188x70_data_len, {81, 367, 70, 1745}, 28},
    {eN30_80_362_188x70_data, &eN30_80_362_188x70_data_len, {80, 362, 70, 1815}, 30},
    {eN32_80_363_188x55_data, &eN32_80_363_188x55_data_len, {80, 363, 55, 1885}, 32},
    {eN34_77_357_188x50_data, &eN34_77_357_188x50_data_len, {77, 357, 50, 1940}, 34},
    {eN36_79_347_188x50_data, &eN36_79_347_188x50_data_len, {79, 347, 50, 1990}, 36},
    {eN38_78_340_data, &eN38_78_340_data_len, {74, 348, 40, 2040}, 38},
    {eN40_74_350_188x27_data, &eN40_74_350_188x27_data_len, {74, 350, 27, 2080}, 40},
    {eN42_74_341_188x30_data, &eN42_74_341_188x30_data_len, {74, 341, 30, 2107}, 42},
    {eN44_74_335_188x34_data, &eN44_74_335_188x34_data_len, {74, 335, 34, 2137}, 44},
    {eN46_76_323_188x47_data, &eN46_76_323_188x47_data_len, {76, 323, 47, 2171}, 46},
    {eN48_74_316_data, &eN48_74_316_data_len, {74, 316, 45, 2218}, 48},
    {eN50_75_296_data, &eN50_75_296_data_len, {75, 296, 58, 2263}, 50},
    {eN52_77_286_data, &eN52_77_286_data_len, {77, 286, 65, 2321}, 52},
    {eN54_83_281_data, &eN54_83_281_data_len, {83, 281, 65, 2386}, 54},
    {eN56_86_271_data, &eN56_86_271_data_len, {86, 271, 74, 2451}, 56},
    {eN58_87_258_data, &eN58_87_258_data_len, {87, 258, 80, 2525}, 58},
    {eN60_95_249_data, &eN60_95_249_data_len, {95, 249, 84, 2605}, 60},
    {eN62_102_238_data, &eN62_102_238_data_len, {102, 238, 90, 2689}, 62},
    {eN64_105_227_data, &eN64_105_227_data_len, {105, 227, 99, 2779}, 64},
    {eN66_109_220_data, &eN66_109_220_data_len, {109, 220, 101, 2878}, 66},
    {eN68_114_211_data, &eN68_114_211_data_len, {114, 211, 110, 2979}, 68},
    {eN70_120_201_data, &eN70_120_201_data_len, {120, 201, 112, 3089}, 70},
    {eN72_127_190_data, &eN72_127_190_data_len, {127, 190, 118, 3201}, 72},
    {eN74_132_186_data, &eN74_132_186_data_len, {132, 186, 124, 3319}, 74},
    {eN76_138_178_data, &eN76_138_178_data_len, {138, 178, 126, 3443}, 76},
    {eN78_144_170_data, &eN78_144_170_data_len, {144, 170, 134, 3569}, 78},
    {eN80_154_160_data, &eN80_154_160_data_len, {154, 160, 135, 3703}, 80},
    {eN82_164_149_data, &eN82_164_149_data_len, {164, 149, 141, 3838}, 82},
    {eN84_169_142_data, &eN84_169_142_data_len, {169, 142, 145, 3979}, 84},
    {eN86_180_138_data, &eN86_180_138_data_len, {180, 138, 147, 4124}, 86},
    {eN88_180_123_data, &eN88_180_123_data_len, {180, 123, 155, 4271}, 88},
    {eN90_197_124_data, &eN90_197_124_data_len, {197, 124, 159, 4426}, 90},
    {eN92_203_118_data, &eN92_203_118_data_len, {203, 118, 160, 4585}, 92},
    {eN94_215_112_data, &eN94_215_112_data_len, {215, 112, 167, 4745}, 94},
    {eN96_224_102_data, &eN96_224_102_data_len, {224, 102, 175, 4912}, 96},
    {eN98_231_103_data, &eN98_231_103_data_len, {231, 103, 167, 5087}, 98},
    {eN100_247_95_data, &eN100_247_95_data_len, {247, 95, 175, 5254}, 100},
    {eN102_256_95_data, &eN102_256_95_data_len, {256, 95, 167, 5429}, 102},
    {eN104_266_92_data, &eN104_266_92_data_len, {266, 92, 170, 5596}, 104},
    {eN106_275_89_data, &eN106_275_89_data_len, {275, 89, 174, 5766}, 106},
    {eN108_276_79_data, &eN108_276_79_data_len, {276, 79, 185, 5940}, 108},
    {eN110_298_79_data, &eN110_298_79_data_len, {298, 79, 179, 6125}, 110},
    {eN112_312_75_data, &eN112_312_75_data_len, {312, 75, 183, 6304}, 112},
    {eN114_321_74_data, &eN114_321_74_data_len, {321, 74, 181, 6487}, 114},
    {eN116_329_76_data, &eN116_329_76_data_len, {329, 76, 179, 6668}, 116},
    {eN118_338_74_data, &eN118_338_74_data_len, {338, 74, 179, 6847}, 118},
    {eN120_346_71_data, &eN120_346_71_data_len, {346, 71, 180, 7026}, 120},
    {eN122_351_73_data, &eN122_351_73_data_len, {351, 73, 180, 7206}, 122},
    {eN124_356_72_data, &eN124_356_72_data_len, {356, 72, 185, 7386}, 124},
    {eN126_359_74_data, &eN126_359_74_data_len, {359, 74, 183, 7571}, 126},
    {eN128_354_73_data, &eN128_354_73_data_len, {354, 73, 185, 7754}, 128},
    {eN130_369_76_data, &eN130_369_76_data_len, {369, 76, 183, 7939}, 130},
    {eN132_369_76_data, &eN132_369_76_data_len, {369, 76, 185, 8122}, 132},
    {eN134_371_76_data, &eN134_371_76_data_len, {371, 76, 185, 8307}, 134},
    {eN136_372_78_data, &eN136_372_78_data_len, {372, 78, 185, 8492}, 136},
    {eN138_378_85_data, &eN138_378_85_data_len, {378, 85, 181, 8677}, 138},
    {eN140_388_89_data, &eN140_388_89_data_len, {388, 89, 181, 8858}, 140},
    {eN142_393_96_data, &eN142_393_96_data_len, {393, 96, 176, 9039}, 142},
    {eN144_392_96_data, &eN144_392_96_data_len, {392, 96, 180, 9215}, 144},
    {eN146_400_103_data, &eN146_400_103_data_len, {400, 103, 177, 9395}, 146},
    {eN148_404_110_data, &eN148_404_110_data_len, {404, 110, 170, 9572}, 148},
    {eN150_411_119_data, &eN150_411_119_data_len, {411, 119, 165, 9742}, 150},
    {eN152_412_121_data, &eN152_412_121_data_len, {412, 121, 170, 9907}, 152},
    {eN154_414_126_data, &eN154_414_126_data_len, {414, 126, 163, 10077}, 154},
    {eN156_417_138_data, &eN156_417_138_data_len, {417, 138, 145, 10240}, 156},
    {eN158_423_146_data, &eN158_423_146_data_len, {423, 146, 150, 10385}, 158},
    {eN160_425_154_data, &eN160_425_154_data_len, {425, 154, 145, 10535}, 160},
    {eN162_430_162_data, &eN162_430_162_data_len, {430, 162, 140, 10680}, 162},
    {eN164_434_173_data, &eN164_434_173_data_len, {434, 173, 130, 10820}, 164},
    {eN166_436_179_data, &eN166_436_179_data_len, {436, 179, 125, 10950}, 166},
    {eN168_439_188_data, &eN168_439_188_data_len, {439, 188, 120, 11075}, 168},
    {eN170_443_197_data, &eN170_443_197_data_len, {443, 197, 114, 11195}, 170},
    {eN172_445_206_data, &eN172_445_206_data_len, {445, 206, 110, 11309}, 172},
    {eN174_449_215_data, &eN174_449_215_data_len, {449, 215, 106, 11419}, 174},
    {eN176_450_220_data, &eN176_450_220_data_len, {450, 220, 100, 11525}, 176},
    {eN178_455_235_data, &eN178_455_235_data_len, {455, 235, 95, 11625}, 178},
    {eN180_457_245_data, &eN180_457_245_data_len, {457, 245, 86, 11720}, 180},
    {eN182_457_254_data, &eN182_457_254_data_len, {457, 254, 83, 11806}, 182},
    {eN184_460_265_data, &eN184_460_265_data_len, {460, 265, 77, 11889}, 184},
    {eN186_462_275_data, &eN186_462_275_data_len, {462, 275, 73, 11966}, 186},
    {eN188_464_284_data, &eN188_464_284_data_len, {464, 284, 67, 12039}, 188},
    {eN190_465_296_data, &eN190_465_296_data_len, {465, 296, 61, 12106}, 190},
    {eN192_465_304_data, &eN192_465_304_data_len, {465, 304, 54, 12167}, 192},
    {eN194_468_318_data, &eN194_468_318_data_len, {468, 318, 50, 12221}, 194},
    {eN196_468_326_data, &eN196_468_326_data_len, {468, 326, 45, 12271}, 196},
    {eN198_468_337_data, &eN198_468_337_data_len, {468, 337, 38, 12316}, 198},
    {eN200_469_350_data, &eN200_469_350_data_len, {469, 350, 30, 12354}, 200},
    {eN202_469_352_data, &eN202_469_352_data_len, {469, 352, 33, 12384}, 202},
    {eN204_468_358_data, &eN204_468_358_data_len, {468, 358, 35, 12417}, 204},
    {eN206_466_363_data, &eN206_466_363_data_len, {466, 363, 38, 12452}, 206},
    {eN208_465_367_data, &eN208_465_367_data_len, {465, 367, 43, 12490}, 208},
    {eN210_464_373_data, &eN210_464_373_data_len, {464, 373, 51, 12533}, 210},
    {eN212_463_377_data, &eN212_463_377_data_len, {463, 377, 53, 12584}, 212},
    {eN214_462_380_data, &eN214_462_380_data_len, {462, 380, 60, 12637}, 214},
    {eN216_457_386_data, &eN216_457_386_data_len, {457, 386, 67, 12697}, 216},
    {eN218_457_388_data, &eN218_457_388_data_len, {457, 388, 75, 12764}, 218},
    {eN220_453_389_data, &eN220_453_389_data_len, {453, 389, 86, 12839}, 220},
    {eN222_454_394_data, &eN222_454_394_data_len, {454, 394, 86, 12925}, 222},
    {eN224_449_401_data, &eN224_449_401_data_len, {449, 401, 97, 13011}, 224},
    {eN226_447_406_data, &eN226_447_406_data_len, {447, 406, 95, 13108}, 226},
    {eN228_446_408_data, &eN228_446_408_data_len, {446, 408, 104, 13203}, 228},
    {eN230_443_411_data, &eN230_443_411_data_len, {443, 411, 110, 13307}, 230},
    {eN232_441_415_data, &eN232_441_415_data_len, {441, 415, 121, 13417}, 232},
    {eN234_438_417_data, &eN234_438_417_data_len, {438, 417, 128, 13538}, 234},
    {eN236_438_417_data, &eN236_438_417_data_len, {438, 417, 130, 13666}, 236},
    {eN238_432_424_data, &eN238_432_424_data_len, {432, 424, 132, 13796}, 238},
    {eN240_428_428_data, &eN240_428_428_data_len, {428, 428, 137, 13928}, 240},
};


void updateNeedle(egt::detail::KMSOverlay* s, int index)
{
    plane_set_pos(s->s(), needles[index].frame_attr.x, needles[index].frame_attr.y);
    plane_set_pan_pos(s->s(), 0, needles[index].frame_attr.pan_y);
    plane_set_pan_size(s->s(), 180, needles[index].frame_attr.pan_h);
    plane_apply(s->s());
    s->schedule_flip();
}

void cp1stNeedle2Fb(char* fb)
{
    static bool called = false;
    if (called)
        return;

    char* ptr = fb;
    memcpy(ptr, needles[0].pic, *needles[0].len);
    called = true;
}

void cpNeedles2Fb(char* fb)
{
    static bool called = false;
    if (called)
        return;

    // struct timeval time1, time2;
    // uint32_t timediff = 0;

    char* ptr = fb + *needles[0].len;

    //gettimeofday(&time1, NULL);
    for (size_t i = 1; i < sizeof(needles) / sizeof(pic_desc); i++) 
    {
        memcpy(ptr, needles[i].pic, *needles[i].len);
        ptr += *needles[i].len;
    }
    // gettimeofday(&time2, NULL);
    // if (time1.tv_sec < time2.tv_sec)
    // {
    //     timediff = time2.tv_usec + 1000000 - time1.tv_usec;
    // }
    // else if (time1.tv_sec > time2.tv_sec)
    // {
    //     std::cerr << "abnormal!!!time1.sec:" << time1.tv_sec << "time1.usec:" << time1.tv_usec << std::endl;
    // }
    // else
    // {
    //     timediff = time2.tv_usec - time1.tv_usec;
    // }
    //std::cout << "before cp sec:" << time1.tv_sec << " usec:" << time1.tv_usec << std::endl;
    //std::cout << "after cp sec:" << time2.tv_sec << " usec:" << time2.tv_usec << std::endl;
    //std::cout << "needles buffer copy time cost: " << timediff << "us" << std::endl;
    called = true;
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