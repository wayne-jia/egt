/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <egt/ui>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <random>
#include <chrono>
#include <queue>
#include <poll.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/stat.h>
#include "eraw.h"


#define HIGH_Q_TIME_THRESHHOLD 30000  //30ms
#define LOW_Q_TIME_THRESHHOLD  20000  //20ms

#define ID_MAX               1327
#define ID_MIN               847
#define STEPPER              4
#define ID_MAX_FUEL          960
#define ID_MIN_FUEL          946
#define FUEL_STEPPER         2
#define ID_MAX_TEMP          983
#define ID_MIN_TEMP          971
#define ID_MAX_TEXT_LEFT     1027
#define ID_MIN_TEXT_LEFT     995
#define ID_MAX_TEXT_RIGHT    1067
#define ID_MIN_TEXT_RIGHT    1031
#define ID_MAX_TEXT_GEAR     1383  //STEPPER=4
#define ID_MIN_TEXT_GEAR     1367
#define ID_MAX_LEFT_SPEED_L  1341  //STEPPER=4
#define ID_MIN_LEFT_SPEED_L  1309
#define ID_MAX_LEFT_SPEED_R  1381  //STEPPER=4
#define ID_MIN_LEFT_SPEED_R  1345
#define ID_MAX_RIGHT_SPEED_L 1417  //STEPPER=4
#define ID_MIN_RIGHT_SPEED_L 1385
#define ID_MAX_RIGHT_SPEED_R 1457  //STEPPER=4
#define ID_MIN_RIGHT_SPEED_R 1421
#define ID_MAX_TEMP_L        1499  //STEPPER=4
#define ID_MIN_TEMP_L        1487
#define ID_MAX_TEMP_R        1539  //STEPPER=4
#define ID_MIN_TEMP_R        1503
#define MAX_TEMP_TABLE       12
#define MAX_SPEED_TABLE      9
#define MAX_DIGIT_PAIR       2
#define MAX_FUEL_REC         8
#define MAX_TEMP_REC         7
#define PLANE_WIDTH_HF       400
#define PLANE_HEIGHT_HF      240
#define MAX_NEEDLE_INDEX     120

int g_digit = ID_MIN;
bool is_increasing = true;
int g_fuel_digit = ID_MIN_FUEL;
bool is_fuel_inc = true;
int g_temp_digit = ID_MAX_TEMP;
bool is_temp_inc = true;
int g_left_txt_digit = ID_MIN_TEXT_LEFT;
int g_right_txt_digit = ID_MIN_TEXT_RIGHT;
bool is_txt_inc = true;
int g_gear_txt_digit = ID_MIN_TEXT_GEAR;
bool is_gear_inc = true;
int g_left_txt_temp = ID_MIN_TEMP_L;
bool is_left_temp_inc = true;
int g_right_txt_temp = ID_MIN_TEMP_R;
bool is_right_temp_inc = true;


enum class SvgEleType
{
    path = 0,
    text,
    bar,
    rbar
};

typedef struct
{
    int main_speed[MAX_DIGIT_PAIR];
    int left_speed[MAX_DIGIT_PAIR];
    int right_speed[MAX_DIGIT_PAIR];
    int speed_digit;
}st_speed_table_t;

typedef struct
{
    int pan_x;
    int pan_y;
    int pan_w;
    int pan_h;

    int x;
    int y;
}st_plane_attri_t;

typedef struct
{
    int y;
    int h;
}st_fuel_temp_t;

typedef unsigned char u8;

st_plane_attri_t g_fuel[MAX_FUEL_REC] = {
    /* panx, pany, panw, panh, x, y */
    {  0, 0, 23,  22, 17, 328},  //Y bottom=350
    { 23, 0, 23,  44, 17, 306},
    { 46, 0, 23,  66, 17, 284},
    { 69, 0, 23,  88, 17, 262},
    { 92, 0, 23, 110, 17, 240},
    {115, 0, 23, 132, 17, 218},
    {138, 0, 23, 154, 17, 196},
    {161, 0, 23, 175, 17, 175}
};

st_plane_attri_t g_temp[MAX_TEMP_REC] = {
    /* panx, pany, panw, panh, x, y */
    {  0, 0, 23,  24, 757, 324},  //Y bottom=348
    { 23, 0, 23,  51, 757, 297},
    { 46, 0, 23,  77, 757, 271},
    { 69, 0, 23, 103, 757, 245},
    { 92, 0, 23, 129, 757, 219},
    {115, 0, 23, 156, 757, 192},
    {138, 0, 23, 181, 757, 167}
};

st_fuel_temp_t g_fuel_table[7] = {
    {173, 176},
    {194, 155},
    {216, 133},
    {238, 111},
    {259, 90},
    {281, 68},
    {303, 46}
};

st_fuel_temp_t g_temp_table[6] = {
    {324, 25},
    {298, 51},
    {271, 78},
    {245, 104},
    {220, 129},
    {194, 167}
};


size_t getFileSize(const char *fileName);

class MotorDash : public egt::experimental::SVGDeserial
{
public:

    explicit MotorDash(egt::TopWindow& parent)
        : egt::experimental::SVGDeserial(parent)
    {
        set_gear_deserial_state(false);
        set_fuel_deserial_state(false);
        set_temp_deserial_state(false);
        set_needle_deserial_state(false);
        set_speed_deserial_state(false);
        set_call_deserial_state(false);
        set_bar_deserial_state(false);
        set_blink_deserial_state(false);
        set_tc_deserial_state(false);
        set_vsc_deserial_state(false);
        set_bat_deserial_state(false);
        set_snow_deserial_state(false);
        set_high_q_state(true);
        set_low_q_state(true);
        set_wifi_deserial_state(false);
    }

    bool get_high_q_state() { return m_is_high_q_quit; }
    void set_high_q_state(bool is_high_q_quit) { m_is_high_q_quit = is_high_q_quit; }

    bool get_low_q_state() { return m_is_low_q_quit; }
    void set_low_q_state(bool is_low_q_quit) { m_is_low_q_quit = is_low_q_quit; }

    bool get_gear_deserial_state() { return m_gear_deserial_finish; }
    void set_gear_deserial_state(bool gear_deserial_finish) { m_gear_deserial_finish = gear_deserial_finish; }

    bool get_fuel_deserial_state() { return m_fuel_deserial_finish; }
    void set_fuel_deserial_state(bool fuel_deserial_finish) { m_fuel_deserial_finish = fuel_deserial_finish; }

    bool get_temp_deserial_state() { return m_temp_deserial_finish; }
    void set_temp_deserial_state(bool temp_deserial_finish) { m_temp_deserial_finish = temp_deserial_finish; }

    bool get_needle_deserial_state() { return m_needle_deserial_finish; }
    void set_needle_deserial_state(bool needle_deserial_finish) { m_needle_deserial_finish = needle_deserial_finish; }

    bool get_speed_deserial_state() { return m_speed_deserial_finish; }
    void set_speed_deserial_state(bool speed_deserial_finish) { m_speed_deserial_finish = speed_deserial_finish; }

    bool get_call_deserial_state() { return m_call_deserial_finish; }
    void set_call_deserial_state(bool call_deserial_finish) { m_call_deserial_finish = call_deserial_finish; }

    bool get_bar_deserial_state() { return m_bar_deserial_finish; }
    void set_bar_deserial_state(bool bar_deserial_finish) { m_bar_deserial_finish = bar_deserial_finish; }

    bool get_blink_deserial_state() { return m_blink_deserial_finish; }
    void set_blink_deserial_state(bool blink_deserial_finish) { m_blink_deserial_finish = blink_deserial_finish; }

    bool get_tc_deserial_state() { return m_tc_deserial_finish; }
    void set_tc_deserial_state(bool tc_deserial_finish) { m_tc_deserial_finish = tc_deserial_finish; }

    bool get_vsc_deserial_state() { return m_vsc_deserial_finish; }
    void set_vsc_deserial_state(bool vsc_deserial_finish) { m_vsc_deserial_finish = vsc_deserial_finish; }

    bool get_bat_deserial_state() { return m_bat_deserial_finish; }
    void set_bat_deserial_state(bool bat_deserial_finish) { m_bat_deserial_finish = bat_deserial_finish; }

    bool get_snow_deserial_state() { return m_snow_deserial_finish; }
    void set_snow_deserial_state(bool snow_deserial_finish) { m_snow_deserial_finish = snow_deserial_finish; }

    bool get_wifi_deserial_state() { return m_wifi_deserial_finish; }
    void set_wifi_deserial_state(bool wifi_deserial_finish) { m_wifi_deserial_finish = wifi_deserial_finish; }

    //egt::Application *app_ptr = nullptr;
    int needle_digit = ID_MIN;

private:
    bool m_is_high_q_quit;
    bool m_is_low_q_quit;
    bool m_gear_deserial_finish;
    bool m_fuel_deserial_finish;
    bool m_temp_deserial_finish;
    bool m_needle_deserial_finish;
    bool m_speed_deserial_finish;
    bool m_call_deserial_finish;
    bool m_bar_deserial_finish;
    bool m_blink_deserial_finish;
    bool m_tc_deserial_finish;
    bool m_vsc_deserial_finish;
    bool m_bat_deserial_finish;
    bool m_snow_deserial_finish;
    bool m_wifi_deserial_finish;
};


size_t getFileSize(const char *fileName) {
	if (fileName == NULL) {
		return 0;
	}
	
	struct stat statbuf;
	stat(fileName, &statbuf);
	size_t filesize = statbuf.st_size;

	return filesize;
}


//using QueueCallback = std::function<void ()>;
using QueueCallback = std::function<std::string ()>;

int main(int argc, char** argv)
{
    std::cout << "EGT start" << std::endl;

    std::queue<QueueCallback> high_pri_q;
    std::queue<QueueCallback> low_pri_q;

    //Widget handler
    std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> NeedleBase;
    std::vector<std::shared_ptr<egt::experimental::GaugeLayer>> GearBase;
    std::shared_ptr<egt::experimental::GaugeLayer> TempPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> FuelPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> TempwPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> FuelrPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> callingPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> mutePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> takeoffPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> left_blinkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> right_blinkPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> farlightPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> vscPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> wifiPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> btPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> enginePtr;
    std::shared_ptr<egt::experimental::GaugeLayer> batPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> egoilPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> hazardsPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> snowPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> absPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> mainspeedPtr;
    std::shared_ptr<egt::experimental::GaugeLayer> speed18Ptr;
    std::shared_ptr<egt::experimental::GaugeLayer> speed25Ptr;
    std::shared_ptr<egt::experimental::GaugeLayer> speed61Ptr;
    std::shared_ptr<egt::experimental::GaugeLayer> speed99Ptr;


    egt::Application app(argc, argv);  //This call will cost ~270ms on 9x60ek board
    egt::TopWindow window;
    window.color(egt::Palette::ColorId::bg, egt::Palette::black);

    MotorDash motordash(window);

    window.show();
    
    // Read eraw.bin to buffer
    const char* ERAW_NAME = "eraw.bin";
    std::string erawname;
    size_t buff_size = getFileSize(ERAW_NAME);
    void* buff_ptr = NULL;
    if (buff_size) {
        buff_ptr = malloc(buff_size);
    } else {
        std::cout << "eraw.bin is blank" << std::endl;
        return -1;
    }

    std::ifstream f(ERAW_NAME, std::ios::binary);
	if(!f)
	{
		std::cout << "read eraw.bin failed" << std::endl;
        free(buff_ptr);
		return -1;
	}
	f.read((char*)buff_ptr, buff_size);

    std::cout << "EGT show" << std::endl;

    //Lambda for de-serializing background and needles
    auto DeserialNeedles = [&]()
    {
        //Background image and needles should be de-serialized firstly before main() return
        motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[0].offset, offset_table[0].len, true);
        
        for (auto i = ID_MIN, j =0; i <= ID_MAX; i += STEPPER, j++)
        {
            for (auto k=1; k<122; k++) {
                erawname = offset_table[k].name;
                if (erawname.substr(0,4) == "path") {
                    std::string pathdigi = erawname.substr(4);
                    if (i == std::stoi(pathdigi)) {
                        NeedleBase.push_back(motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[k].offset, offset_table[k].len, false));
                    }
                }
            }
        }
        mainspeedPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[137].offset, offset_table[137].len, true);
    };

    int speed_index = 0;
    int needle_index = 0;

    bool is_needle_finish = false;
    std::string lbd_ret = "0";

    int timer_cnt;
    //int lbar = 7;
    int temp_index = 0;
    int fuel_index = 0;

#if 1 //use timer or screen click to debug? 1:use timer; 0:use click
    egt::PeriodicTimer timer(std::chrono::milliseconds(10));
    timer.on_timeout([&]()
    {
        //motordash.get_high_q_state/get_low_q_state are used to protect high q not be interrupted by other event re-enter
        //if the state is false, it means this queue has not execute finished yet.
        if (!motordash.get_high_q_state())
            return;
        if (!high_pri_q.empty())
        {
            //gettimeofday(&time1, NULL);
            motordash.set_high_q_state(false);
            lbd_ret = high_pri_q.front()();
            high_pri_q.pop();
            motordash.set_high_q_state(true);
            //gettimeofday(&time2, NULL);
            //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
            //if (HIGH_Q_TIME_THRESHHOLD <= timediff)
                //std::cout << "warning!!! high pri q: " << lbd_ret <<"() exec too longer: " << timediff << "us" << std::endl;

        }
        else
        {
            if (!motordash.get_low_q_state())
                return;
            if (!low_pri_q.empty())
            {
                //gettimeofday(&time1, NULL);
                motordash.set_low_q_state(false);
                lbd_ret = low_pri_q.front()();
                low_pri_q.pop();
                motordash.set_low_q_state(true);
                //gettimeofday(&time2, NULL);
                //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                //if (LOW_Q_TIME_THRESHHOLD <= timediff)
                    //std::cout << "warning!!! low pri q: " << lbd_ret <<"() exec too longer: " << timediff << "us" << std::endl;
                return;
            }
        }

#ifdef EGT_STATISTICS_ENABLE

#else
        //viariable to control the animation and frequency
        timer_cnt = (LOW_Q_TIME_THRESHHOLD <= timer_cnt) ? 0 : timer_cnt + 1;

        //needle move function implemented by lambda
        auto needle_move = [&]()
        {
            //needles
            for (int i=0; i<8; i++)
            {
                if (is_increasing)
                {
                    NeedleBase[needle_index]->show();
                }
                else
                {
                    NeedleBase[needle_index]->hide();
                }

                if (is_increasing && MAX_NEEDLE_INDEX == needle_index)
                {
                    is_increasing = false;
                    is_needle_finish = true;
                    return "needle_move";
                }
                else if (!is_increasing && 0 == needle_index)
                {
                    is_increasing = true;
                    is_needle_finish = true;
                    return "needle_move";
                }
                else
                {
                    needle_index = is_increasing ? needle_index + 1 : needle_index - 1;
                }
            }
            return "needle_move";
        };

#if 1
        //make sure the needles run a circle finish, then can enter to low priority procedure
        if (is_needle_finish)
        {
            is_needle_finish = false;

            //deserialize speed
            if (!motordash.get_speed_deserial_state())
            {

                low_pri_q.push([&]()
                {
                    speed18Ptr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[140].offset, offset_table[140].len, false);
                    speed25Ptr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[179].offset, offset_table[179].len, false);
                    speed61Ptr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[184].offset, offset_table[184].len, false);
                    speed99Ptr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[190].offset, offset_table[190].len, false);
                    return "lspeed_text";
                });
                low_pri_q.push([&]()
                {
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[151].offset, offset_table[151].len, true);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[154].offset, offset_table[154].len, true);
                    return "rspeed_text";
                });
                motordash.set_speed_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 3))
                {
                    low_pri_q.push([&]()
                    {
                        if (4 < speed_index)
                            speed_index = 0;
                        switch (speed_index)
                        {
                            case 0:
                                mainspeedPtr->show();
                                speed18Ptr->hide();
                                speed25Ptr->hide();
                                speed61Ptr->hide();
                                speed99Ptr->hide();
                                break;
                            case 1:
                                mainspeedPtr->hide();
                                speed18Ptr->show();
                                speed25Ptr->hide();
                                speed61Ptr->hide();
                                speed99Ptr->hide();
                                break;
                            case 2:
                                mainspeedPtr->hide();
                                speed18Ptr->hide();
                                speed25Ptr->show();
                                speed61Ptr->hide();
                                speed99Ptr->hide();
                                break;
                            case 3:
                                mainspeedPtr->hide();
                                speed18Ptr->hide();
                                speed25Ptr->hide();
                                speed61Ptr->show();
                                speed99Ptr->hide();
                                break;
                            case 4:
                                mainspeedPtr->hide();
                                speed18Ptr->hide();
                                speed25Ptr->hide();
                                speed61Ptr->hide();
                                speed99Ptr->show();
                                break;
                            default:
                                std::cout << "spd full!" << std::endl;
                                break;
                        }
                        speed_index++;
                        return "temp";
                    });
                }
            }

            //this branch exec the gear de-serialize only execute once
            if (!motordash.get_gear_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[144].offset, offset_table[144].len, true);

                    return "gear_deserial";
                });
                motordash.set_gear_deserial_state(true);
                return;
            }
            else
            {

            }

            //deserialize temp
            if (!motordash.get_temp_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    TempwPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[135].offset, offset_table[135].len, true);
                    TempPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[136].offset, offset_table[136].len, true);

                    return "temp_deserial6";
                });
                motordash.set_temp_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 5))
                {
                    low_pri_q.push([&]()
                    {
                        if (7 <= temp_index)
                            temp_index = 0;
                        switch (temp_index)
                        {
                            case 0:
                                TempPtr->hide();
                                TempwPtr->show();
                                TempwPtr->y(g_temp_table[0].y);
                                TempwPtr->height(g_temp_table[0].h);
                                break;
                            case 1:
                                TempPtr->hide();
                                TempwPtr->show();
                                TempwPtr->y(g_temp_table[1].y);
                                TempwPtr->height(g_temp_table[1].h);
                                break;
                            case 2:
                                TempPtr->hide();
                                TempwPtr->show();
                                TempwPtr->y(g_temp_table[2].y);
                                TempwPtr->height(g_temp_table[2].h);
                                break;
                            case 3:
                                TempPtr->hide();
                                TempwPtr->show();
                                TempwPtr->y(g_temp_table[3].y);
                                TempwPtr->height(g_temp_table[3].h);
                                break;
                            case 4:
                                TempPtr->hide();
                                TempwPtr->show();
                                TempwPtr->y(g_temp_table[4].y);
                                TempwPtr->height(g_temp_table[4].h);
                                break;
                            case 5:
                                TempPtr->hide();
                                TempwPtr->show();
                                TempwPtr->y(g_temp_table[5].y);
                                TempwPtr->height(g_temp_table[5].h);
                                break;
                            case 6:
                                TempwPtr->hide();
                                TempPtr->show();
                                break;
                            default:
                                std::cout << "temp full!" << std::endl;
                                break;
                        }
                        temp_index++;
                        return "temp";
                    });
                }
            }

            //deserialize fuel
            if (!motordash.get_fuel_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    FuelrPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[122].offset, offset_table[122].len, true);
                    FuelPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[129].offset, offset_table[129].len, true);

                    return "fuel_deserial";
                });
                motordash.set_fuel_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 3))
                {
                    low_pri_q.push([&]()
                    {
                        if (8 <= fuel_index)
                            fuel_index = 0;
                        switch (fuel_index)
                        {
                            case 0:
                                FuelrPtr->hide();
                                FuelPtr->show();
                                FuelPtr->y(g_fuel_table[0].y);
                                FuelPtr->height(g_fuel_table[0].h);
                                break;
                            case 1:
                                FuelrPtr->hide();
                                FuelPtr->show();
                                FuelPtr->y(g_fuel_table[1].y);
                                FuelPtr->height(g_fuel_table[1].h);
                                break;
                            case 2:
                                FuelrPtr->hide();
                                FuelPtr->show();
                                FuelPtr->y(g_fuel_table[2].y);
                                FuelPtr->height(g_fuel_table[2].h);
                                break;
                            case 3:
                                FuelrPtr->hide();
                                FuelPtr->show();
                                FuelPtr->y(g_fuel_table[3].y);
                                FuelPtr->height(g_fuel_table[3].h);
                                break;
                            case 4:
                                FuelrPtr->hide();
                                FuelPtr->show();
                                FuelPtr->y(g_fuel_table[4].y);
                                FuelPtr->height(g_fuel_table[4].h);
                                break;
                            case 5:
                                FuelrPtr->hide();
                                FuelPtr->show();
                                FuelPtr->y(g_fuel_table[5].y);
                                FuelPtr->height(g_fuel_table[5].h);
                                break;
                            case 6:
                                FuelrPtr->hide();
                                FuelPtr->show();
                                FuelPtr->y(g_fuel_table[6].y);
                                FuelPtr->height(g_fuel_table[6].h);
                                break;
                            case 7:
                                FuelPtr->hide();
                                FuelrPtr->show();
                                break;
                            default:
                                std::cout << "fuel full!" << std::endl;
                                break;
                        }
                        fuel_index++;
                        return "fuel";
                    });
                }
            }



            //deserialize call
            if (!motordash.get_call_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[145].offset, offset_table[145].len, true);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[148].offset, offset_table[148].len, true);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[175].offset, offset_table[175].len, true);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[176].offset, offset_table[176].len, true);
                    return "callname_text";
                });
                low_pri_q.push([&]()
                {
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[157].offset, offset_table[157].len, true);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[178].offset, offset_table[178].len, true);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[177].offset, offset_table[177].len, true);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[125].offset, offset_table[125].len, true);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[133].offset, offset_table[133].len, true);
                    return "phone_text";
                });
                low_pri_q.push([&]()
                {
                    //gettimeofday(&time1, NULL);
                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[165].offset, offset_table[165].len, true);
                    //gettimeofday(&time2, NULL);
                    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                    //std::cout << "Create text call time: " << timediff << " us" << std::endl;
                    return "call_text";
                });
                motordash.set_call_deserial_state(true);
                return;
            }
            else
            {

            }

            //deserialize eng5 & tc
            if (!motordash.get_tc_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    //gettimeofday(&time1, NULL);
                    //De-serialize eng5 & tc
                    low_pri_q.push([&]()
                    {
                        //motordash.add_text_widget("#engine5", "5", egt::Rect(106, 14, 25, 40), 40);
                        return "callname_text";
                    });
                    low_pri_q.push([&]()
                    {
                        //motordash.add_text_widget("#tc", "3", egt::Rect(80, 76, 22, 35), 30);
                        return "phone_text";
                    });

                    motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[160].offset, offset_table[160].len, true);
                    //gettimeofday(&time2, NULL);
                    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                    //std::cout << "Create text eng5 & tc time: " << timediff << " us" << std::endl;
                    return "eng5tc_text";
                });
                motordash.set_tc_deserial_state(true);
                return;
            }
            else
            {

            }

            //deserialize bar
            if (!motordash.get_bar_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    //gettimeofday(&time1, NULL);
                    //De-serialize tmp
                    //motordash.add_text_widget("#lbar", "0.5", egt::Rect(310, 550, 42, 23), 25, egt::Palette::red);
                    //motordash.add_text_widget("#rbar", "0.8", egt::Rect(639, 550, 42, 23), 25, egt::Palette::red);
                    //gettimeofday(&time2, NULL);
                    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                    //std::cout << "Create text bar time: " << timediff << " us" << std::endl;
                    return "bar_text";
                });
                motordash.set_bar_deserial_state(true);
                return;
            }
            else
            {
                // if (!(timer_cnt % 11))
                // {
                //     low_pri_q.push([&]()
                //     {
                //         //bar
                //         motordash.find_text("#lbar")->clear();
                //         motordash.find_text("#rbar")->clear();
                //         if (7 == lbar)
                //         {
                //             motordash.find_text("#lbar")->text("0.3");
                //             motordash.find_text("#rbar")->text("0.6");
                //             lbar = 3;
                //         }
                //         else
                //         {
                //             motordash.find_text("#lbar")->text("0.7");
                //             motordash.find_text("#rbar")->text("0.9");
                //             lbar = 7;
                //         }
                //         return "bar_change";
                //     });
                // }
            }

            //deserialize blink
            if (!motordash.get_blink_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    //gettimeofday(&time1, NULL);

                    left_blinkPtr =  motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[161].offset, offset_table[161].len, true);
                    right_blinkPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[173].offset, offset_table[173].len, true);
                    //gettimeofday(&time2, NULL);
                    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                    //std::cout << "Desrial blink time: " << timediff << " us" << std::endl;
                    return "blink_deserial";
                });
                motordash.set_blink_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 3))
                {
                    low_pri_q.push([&]()
                    {
                        //blink
                        if (left_blinkPtr->visible())
                            left_blinkPtr->hide();
                        else
                            left_blinkPtr->show();

                        if (right_blinkPtr->visible())
                            right_blinkPtr->hide();
                        else
                            right_blinkPtr->show();

                        return "left_right_blink";
                    });
                }
            }

            //deserialize vsc & farlight & takeoff
            if (!motordash.get_vsc_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    //gettimeofday(&time1, NULL);
                    vscPtr =  motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[164].offset, offset_table[164].len, true);
                    farlightPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[163].offset, offset_table[163].len, true);
                    takeoffPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[162].offset, offset_table[162].len, true);
                    //gettimeofday(&time2, NULL);
                    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                    //std::cout << "Desrial vsc time: " << timediff << " us" << std::endl;
                    return "vsc_deserial";
                });
                motordash.set_vsc_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 5))
                {
                    low_pri_q.push([&]()
                    {
                        //vsc
                        if (vscPtr->visible())
                            vscPtr->hide();
                        else
                            vscPtr->show();

                        if (farlightPtr->visible())
                            farlightPtr->hide();
                        else
                            farlightPtr->show();

                        if (takeoffPtr->visible())
                            takeoffPtr->hide();
                        else
                            takeoffPtr->show();

                        return "vsc_farlight_takeoff";
                    });
                }
            }

            //deserialize bat & egoil & hazards
            if (!motordash.get_bat_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    //gettimeofday(&time1, NULL);
                    batPtr =  motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[166].offset, offset_table[166].len, true);
                    egoilPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[167].offset, offset_table[167].len, true);
                    hazardsPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[168].offset, offset_table[168].len, true);
                    //gettimeofday(&time2, NULL);
                    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                    //std::cout << "Desrial bat time: " << timediff << " us" << std::endl;
                    return "bat_deserial";
                });
                motordash.set_bat_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 9))
                {
                    low_pri_q.push([&]()
                    {
                        //vsc
                        if (batPtr->visible())
                            batPtr->hide();
                        else
                            batPtr->show();

                        if (egoilPtr->visible())
                            egoilPtr->hide();
                        else
                            egoilPtr->show();

                        if (hazardsPtr->visible())
                            hazardsPtr->hide();
                        else
                            hazardsPtr->show();

                        return "bat_egoil_hazards";
                    });
                }
            }

            //deserialize snow & abs & engine
            if (!motordash.get_snow_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    //gettimeofday(&time1, NULL);
                    snowPtr =  motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[169].offset, offset_table[169].len, true);
                    absPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[170].offset, offset_table[170].len, true);
                    enginePtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[174].offset, offset_table[174].len, true);
                    //gettimeofday(&time2, NULL);
                    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                    //std::cout << "Desrial snow time: " << timediff << " us" << std::endl;
                    return "snow_deserial";
                });
                motordash.set_snow_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 7))
                {
                    low_pri_q.push([&]()
                    {
                        //vsc
                        if (snowPtr->visible())
                            snowPtr->hide();
                        else
                            snowPtr->show();

                        if (absPtr->visible())
                            absPtr->hide();
                        else
                            absPtr->show();

                        if (enginePtr->visible())
                            enginePtr->hide();
                        else
                            enginePtr->show();

                        return "snow_abs_engin";
                    });
                }
            }

            //deserialize wifi & bt
            if (!motordash.get_wifi_deserial_state())
            {
                low_pri_q.push([&]()
                {
                    //gettimeofday(&time1, NULL);

                    wifiPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[171].offset, offset_table[171].len, true);
                    btPtr = motordash.AddWidgetByBuf((const u8*)buff_ptr+offset_table[172].offset, offset_table[172].len, true);
                    //gettimeofday(&time2, NULL);
                    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
                    //std::cout << "Desrial wifi time: " << timediff << " us" << std::endl;
                    free(buff_ptr);
                    return "wifi_deserial";
                });
                motordash.set_wifi_deserial_state(true);
                return;
            }
            else
            {
                if (!(timer_cnt % 9))
                {
                    low_pri_q.push([&]()
                    {
                        //vsc
                        if (wifiPtr->visible())
                            wifiPtr->hide();
                        else
                            wifiPtr->show();

                        if (btPtr->visible())
                            btPtr->hide();
                        else
                            btPtr->show();

                        return "wifi_bt";
                    });
                }
            }
        }
        else  //this branch exec the high priority event
        {
            high_pri_q.push(needle_move);
        }
#endif

#endif
    });

    DeserialNeedles();
    timer.start();

#endif
    //gettimeofday(&time2, NULL);
    //timediff = (time1.tv_sec < time2.tv_sec) ? (time2.tv_usec + 1000000 - time1.tv_usec) : (time2.tv_usec - time1.tv_usec);
    //std::cout << "app start-up time: " << timediff << " us" << std::endl;
    //gettimeofday(&time1, NULL);
    return app.run();
}
