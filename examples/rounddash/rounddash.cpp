/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rounddash.h"
#include "erawparse.h"
#include "stage1_eraw.h"
#include "stage2_eraw.h"
#include "stage3_eraw.h"
#include "ble_eraw.h"

std::vector<std::shared_ptr<egt::Label>> GPSLabels;
std::vector<std::shared_ptr<egt::ImageLabel>> GPSImgIndicators;

APP_DATA appData;
static uint32_t prev_tick = 0, tick = 0;
static uint32_t prev_sec_tick = 0, sec_tick = 0;
static bool tick_start = false;
static uint32_t sec2 = 0;
static bool needles_stage2_cp_done = false;
static bool needles_stage3_cp_done = false;
static bool gpswgt_init_done = false;
static bool blur_alpha_high = true;
static bool udev_init_done = false;
static APP_STATES app_last_state = APP_STATE_IDLE;

#define ENABLE_UART

#ifdef ENABLE_UART
#include "uartFunc.h"

#define REFRESH_PERIOD		            500
#define STR_BLE_NOTY_INCOMING_CALL		"You have an incoming call."
#define STR_BLE_NOTY_REMOVE_CALL 		"An incoming call has been removed."
#define STR_BLE_NOTY_RETRIVE_DETAIL		"BLE_ANCS_EVT_NTFY_ATTR_IND."
#define STR_BLE_NOTY_ANSWER_CALL		"Answer the incoming call:"
#define STR_BLE_NOTY_RETRIEVE_SOCIAL	"want to retrieve a social message"
#define STR_BLE_NOTY_MISSED_CALL		"You have a missed call"

typedef enum __BLE_STATE_MACHONE__
{
	BLE_NOTY_NONE = 0,
	BLE_NOTY_INCOMING_CALL,		// new call coming
	BLE_NOTY_REMOVE_CALL,		// call was cancelled by caller
	BLE_NOTY_RETRIEVE_DETAIL,	// retrieve caller detail
	BLE_NOTY_ANSWER_CALL,		// answer call
	BLE_NOTY_SOCIAL_MEDIA,		// retrieve social media
	BLE_NOTY_MISSED_CALL,		// missed call
} BLE_NOTIFICATION;

typedef struct __BLE_CALLER_INFO__
{
	char caller_number[32];
	char msg[512];
	char date[32];
} BLE_CALLER_INFO, *pBLE_CALLER_INFO;

#define RECEIVE_BUFFER_SIZE	1024

void debug_buffer(char *buf, int len)
{
	for(int i=0; i<len; i++)
	{
		printf("%02X ", buf[i]);
	}
	
	printf("\n\r");
	
	for(int i=0; i<len; i++)
	{
		printf("%c", buf[i]);
	}
	
	printf("\n\r");

}

int ble_get_caller_detail(char *buf, int length, pBLE_CALLER_INFO caller_info)
{
	int i;
	int last_line_pos = -1;
	int index = 0;

	for(i=0; i<length; i++)
	{
		if( buf[i] == 0x0A )
		{
			switch( index )
			{
				case 0:	// BLE_ANCS_EVT_NTFY_ATTR_IND
					break;
				
				case 1:	// title
					memcpy(caller_info->caller_number, buf+(last_line_pos+1+9), i-last_line_pos-1-9);
					caller_info->caller_number[i-last_line_pos-1-9] = 0;
					break;
					
				case 2:	// sybtitle
					break;
					
				case 3:	// msg
					memcpy(caller_info->msg, buf+(last_line_pos+1+7), i-last_line_pos-1-7);
					caller_info->msg[i-last_line_pos-1-7] = 0;
					break;
					
				case 4:	// date
					memcpy(caller_info->date, buf+(last_line_pos+1+8), i-last_line_pos-1-8);
					caller_info->date[i-last_line_pos-1-8] = 0;
					break;
					
				default:
					break;
			}
		
			last_line_pos = i;	
			index++;
		}
	}
	
	return 0;
}

BLE_NOTIFICATION ble_identity_notification(char *buf, int length)
{
	int index;
	
	if( strstr(buf, STR_BLE_NOTY_INCOMING_CALL) != NULL )
	{
		return BLE_NOTY_INCOMING_CALL;
	}
	else if( strstr(buf, STR_BLE_NOTY_REMOVE_CALL) != NULL )
	{
		return BLE_NOTY_REMOVE_CALL;
	}
	else if( strstr(buf, STR_BLE_NOTY_RETRIVE_DETAIL) != NULL )
	{
		return BLE_NOTY_RETRIEVE_DETAIL;
	}
	else if( strstr(buf, STR_BLE_NOTY_ANSWER_CALL) != NULL )
	{
		return BLE_NOTY_ANSWER_CALL;
	}
	else if( strstr(buf, STR_BLE_NOTY_RETRIEVE_SOCIAL) != NULL )
	{
		return BLE_NOTY_SOCIAL_MEDIA;
	}
	else if( strstr(buf, STR_BLE_NOTY_MISSED_CALL) != NULL )
	{
		return BLE_NOTY_MISSED_CALL;
	}
	
	return BLE_NOTY_NONE;
}

char recBuff[RECEIVE_BUFFER_SIZE];
char cmdBuff[RECEIVE_BUFFER_SIZE];	

#endif // end of ENABLE_UART


int main(int argc, char** argv)
{
    std::cout << std::endl << "EGT start" << std::endl; 


#ifdef ENABLE_UART
    bool isCalling = false;
    bool isAnswered = false;
    bool isSMS = false;
    bool isReject = false;
    int refresh_count = 0;

	struct pollfd pollUartfds;
	int nread;
	
	BLE_NOTIFICATION ble_no;
	BLE_CALLER_INFO caller_info;
	
	if( argc != 2 )
	{
		std::cout << "USAGE: uart_transmit UART_PORT" << std::endl;
		return -1;
	}
	pollUartfds.events = POLLRDNORM;
	
#endif	// end of ENABLE_UART


    std::vector<std::shared_ptr<OverlayWindow>> OverlayWinVector;
    std::vector<std::shared_ptr<egt::ImageLabel>> ImgNeedlesVector;

    egt::Application app(argc, argv);
    egt::TopWindow window;

    ImageParse imgs("stage1_eraw.bin", Speedo_table, sizeof(Speedo_table)/sizeof(eraw_st));

    window.background(egt::Image(imgs.GetImageObj(0)));
    window.on_show([]()    
    {        
        std::cout << std::endl << "EGT show" << std::endl;    
    });

    ///============ Needle layer =============
    OverlayWinVector.emplace_back(std::make_shared<OverlayWindow>(egt::Rect(157, 422, 188, 14065),
                                                               egt::PixelFormat::argb8888,
                                                               egt::WindowHint::overlay,
                                                               1));
    OverlayWinVector[0]->fill_flags().clear();
    //OverlayWinVector[0]->disable();

    auto imgN0 = std::make_shared<egt::ImageLabel>(*OverlayWinVector[0], egt::Image(imgs.GetImageObj(9)));
    imgN0->image_align(egt::AlignFlag::center);
    imgN0->move(egt::Point(0, 0));
    window.add(OverlayWinVector[0]);
    ///============ Needle  layer end =============

    ///============ GPS layer =============
    OverlayWinVector.emplace_back(std::make_shared<OverlayWindow>(egt::Rect(GPS_X, GPS_Y, GPS_WIDTH, GPS_HEIGHT)));
    OverlayWinVector[1]->fill_flags().clear();
    //window.add(OverlayWinVector[1]);
    //OverlayWinVector[1]->disable();

    auto imgLblLogobg = std::make_shared<egt::ImageLabel>(*OverlayWinVector[1], egt::Image(imgs.GetImageObj(1)));
    imgLblLogobg->image_align(egt::AlignFlag::center);
    imgLblLogobg->move(egt::Point(76, 48));

    auto imgLblLogo = std::make_shared<egt::ImageLabel>(*OverlayWinVector[1], egt::Image(imgs.GetImageObj(2)));
    imgLblLogo->image_align(egt::AlignFlag::center);
    imgLblLogo->move(egt::Point(78, 49));

    auto imgLblInfobg = std::make_shared<egt::ImageLabel>(*OverlayWinVector[1], egt::Image(imgs.GetImageObj(3)));
    imgLblInfobg->image_align(egt::AlignFlag::center);
    imgLblInfobg->move(egt::Point(53, 237));

    auto lblInstru = std::make_shared<egt::Label>(*OverlayWinVector[1], "DRIVE SAFE!");
    lblInstru->color(egt::Palette::ColorId::label_text, egt::Palette::white);
    lblInstru->font(egt::Font("Noto Sans", 21, egt::Font::Weight::normal));
    lblInstru->move(egt::Point(103, 278));
    GPSLabels.emplace_back(lblInstru);  //[0]
    ///============ GPS  layer end =============

    ///============ Blue layer =============
    OverlayWinVector.emplace_back(std::make_shared<OverlayWindow>(egt::Rect(0, 0, MAX_WIDTH, MAX_HEIGHT),
                                                               egt::PixelFormat::argb8888,
                                                               egt::WindowHint::overlay,
                                                               1));
    window.add(OverlayWinVector[2]);
    //OverlayWinVector[2]->disable();
    OverlayWinVector[2]->fill_flags().clear();
    auto imgLblBlur = std::make_shared<egt::ImageLabel>(*OverlayWinVector[2], egt::Image(imgs.GetImageObj(4)));
    imgLblBlur->fill_flags().clear();
    imgLblBlur->image_align(egt::AlignFlag::center);
    ///============ Blue  layer end =============

#ifdef ENABLE_UART
    ImageParse bleimgs("ble_eraw.bin", accept_table, sizeof(accept_table)/sizeof(eraw_st));
    auto imgBtnaccept = std::make_shared<egt::ImageButton>(window, egt::Image(bleimgs.GetImageObj(0)));
    imgBtnaccept->image_align(egt::AlignFlag::center);
    imgBtnaccept->move(egt::Point(GPS_X+56, GPS_Y+249));
    imgBtnaccept->hide();
    imgBtnaccept->on_click([&](egt::Event&)
    {
        if( isCalling )
		{
			std::cout << "Accpet..." << std::endl;		
			strcpy(cmdBuff, "Y\n\r");
			write(pollUartfds.fd, cmdBuff, 3);
			
			isAnswered = true;
			isCalling = false;
			appData.blestate = BLE_CALL_ANSWERED;
		}
    });

    auto imgLblble = std::make_shared<egt::ImageLabel>(window, egt::Image(bleimgs.GetImageObj(1)));
    imgLblble->image_align(egt::AlignFlag::center);
    imgLblble->move(egt::Point(MAX_WIDTH / 2 - 32, 184));

    auto imgLblcall = std::make_shared<egt::ImageLabel>(window, egt::Image(bleimgs.GetImageObj(2)));
    imgLblcall->image_align(egt::AlignFlag::center);
    imgLblcall->move_to_center();
    imgLblcall->hide();

    auto imgBtnreject = std::make_shared<egt::ImageButton>(window, egt::Image(bleimgs.GetImageObj(3)));
    imgBtnreject->image_align(egt::AlignFlag::center);
    imgBtnreject->move(egt::Point(GPS_X+56+127, GPS_Y+249));
    imgBtnreject->hide();
    imgBtnreject->on_click([&](egt::Event&)
    {
        if( isCalling || isAnswered)
		{
			std::cout << "Reject..." << std::endl;		
			strcpy(cmdBuff, "N\n\r");
			write(pollUartfds.fd, cmdBuff, 3);
			
			isReject = true;
            appData.blestate = BLE_QUIT_CALL_SMS;		
		}
    });

    auto imgLblsms = std::make_shared<egt::ImageLabel>(window, egt::Image(bleimgs.GetImageObj(4)));
    imgLblsms->image_align(egt::AlignFlag::center);
    imgLblsms->move_to_center();
    imgLblsms->hide();

    auto lblCaller = std::make_shared<egt::Label>(window, "???");
    lblCaller->color(egt::Palette::ColorId::label_text, egt::Palette::white);
    lblCaller->font(egt::Font("Noto Sans TC", 23, egt::Font::Weight::bold));
    lblCaller->x(MAX_WIDTH/2 - lblCaller->width()/2);
    lblCaller->y(GPS_Y+199);
    lblCaller->hide();

    //auto panSMS = std::make_shared<egt::Label>(window, "");
    auto lblSMS = std::make_shared<egt::Label>(window, "The Microchip Graphics Suite Linux, \n"
                                                       "formely known as The Ensemble Graphics Toolkit (EGT),\n"
                                                       "is a free and open-source C++ GUI widget toolkit for\n"
                                                       "Microchip AT91/SAMA5 microprocessors. It is used to\n"
                                                       "develop graphical embedded Linux applications.");
    lblSMS->text_align(egt::AlignFlag::center);
    lblSMS->resize(egt::Size(460, 310));
    lblSMS->move_to_center();
    lblSMS->color(egt::Palette::ColorId::label_text, egt::Palette::white);
    lblSMS->color(egt::Palette::ColorId::label_bg, egt::Palette::grey);
    lblSMS->font(egt::Font("Noto Sans TC", 28, egt::Font::Weight::bold));
    lblSMS->fill_flags(egt::Theme::FillFlag::blend);
    lblSMS->hide();
#endif

    // Create fade effect for OVR2 and HEO
    OverlayFade fade(OverlayWinVector, "ovr2_fade_in_10", OVERLAY_TYPE::LCDC_OVR_2, 0, 255, 10);
    fade.add("ovr2_fade_in_5", OVERLAY_TYPE::LCDC_OVR_2, 0, 255, 5);
    fade.add("ovr2_fade_out_10", OVERLAY_TYPE::LCDC_OVR_2, 255, 0, 10);
    fade.add("ovr2_fade_out_50", OVERLAY_TYPE::LCDC_OVR_2, 255, 0, 50);
    fade.add("ovrheo_fade_in_10", OVERLAY_TYPE::LCDC_OVR_HEO, 0, 255, 10);
    fade.add("ovrheo_fade_out_10", OVERLAY_TYPE::LCDC_OVR_HEO, 255, 0, 10);
    fade.add("ovrheo_fade_in_lit_10", OVERLAY_TYPE::LCDC_OVR_HEO, 100, 255, 10);
    fade.add("ovrheo_fade_out_lit_10", OVERLAY_TYPE::LCDC_OVR_HEO, 255, 100, 10);
    fade.add("ovrheo_fade_in_lit_50", OVERLAY_TYPE::LCDC_OVR_HEO, 100, 255, 50);

    // Init GPS widegt function
    auto initGPSwgt = [&OverlayWinVector, &imgs]()
    {
        auto lblSpdUnit = std::make_shared<egt::Label>(*OverlayWinVector[1], "km/h");
        lblSpdUnit->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        lblSpdUnit->font(egt::Font("Noto Sans", 23, egt::Font::Weight::bold, egt::Font::Slant::italic));
        lblSpdUnit->move(egt::Point(136, 120));
        lblSpdUnit->hide();
        GPSLabels.emplace_back(lblSpdUnit);   //[1]

        auto lblSpd = std::make_shared<egt::Label>(*OverlayWinVector[1], "0");
        lblSpd->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        lblSpd->font(egt::Font("Noto Sans", 60, egt::Font::Weight::bold, egt::Font::Slant::italic));
        lblSpd->width(100);
        lblSpd->text_align(egt::AlignFlag::center);
        lblSpd->move(egt::Point(114, 49));
        lblSpd->hide();
        GPSLabels.emplace_back(lblSpd);   //[2]

        auto lblDist = std::make_shared<egt::Label>(*OverlayWinVector[1], "20 km");
        lblDist->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        lblDist->width(100);
        lblDist->text_align(egt::AlignFlag::center);
        lblDist->font(egt::Font("Noto Sans", 18, egt::Font::Weight::bold));
        lblDist->move(egt::Point(116, 248));
        lblDist->hide();
        GPSLabels.emplace_back(lblDist);   //[3]

        auto lblRTime = std::make_shared<egt::Label>(*OverlayWinVector[1], "Time Left:         mins");
        lblRTime->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        lblRTime->font(egt::Font("Noto Sans", 18, egt::Font::Weight::normal));
        lblRTime->move(egt::Point(81, 313));
        lblRTime->hide();
        GPSLabels.emplace_back(lblRTime);   //[4]

        for (auto i=5; i<9; i++)
        {
            auto navImg = std::make_shared<egt::ImageLabel>(*OverlayWinVector[1], egt::Image(imgs.GetImageObj(i)));
            navImg->fill_flags().clear();
            navImg->image_align(egt::AlignFlag::center);
            navImg->move(egt::Point(56, 264));
            navImg->hide();
            GPSImgIndicators.emplace_back(navImg);
        }
    };

    // The pulse blue blur effect function
    auto pulseBlur = [&fade]()
    {
        if (blur_alpha_high == true)
        {
            blur_alpha_high = false;
            fade.request("ovrheo_fade_out_lit_10");
        }
        else
        {
            blur_alpha_high = true;
            fade.request("ovrheo_fade_in_lit_10");
        }
    };

    auto initStage2Needles = [&OverlayWinVector, &ImgNeedlesVector]()
    {
        auto needle_num = sizeof(N002_151_420_188x146_table)/sizeof(eraw_st);
        auto imgs = std::make_shared<ImageParse>("stage2_eraw.bin", N002_151_420_188x146_table, needle_num);
        for (uint32_t i=0; i<needle_num; i++)
        {
            auto imgNeedle = std::make_shared<egt::ImageLabel>(*OverlayWinVector[0], egt::Image(imgs->GetImageObj(i)));
            imgNeedle->image_align(egt::AlignFlag::center);
            imgNeedle->move(egt::Point(0, needles[i+1].frame_attr.pan_y));
            ImgNeedlesVector.emplace_back(imgNeedle);
        }
    };

    auto initStage3Needles = [&OverlayWinVector, &ImgNeedlesVector]()
    {
        auto needle_num = sizeof(N078_144_170_table)/sizeof(eraw_st);
        auto imgs = std::make_shared<ImageParse>("stage3_eraw.bin", N078_144_170_table, needle_num);
        for (uint32_t i=0; i<needle_num; i++)
        {
            auto imgNeedle = std::make_shared<egt::ImageLabel>(*OverlayWinVector[0], egt::Image(imgs->GetImageObj(i)));
            imgNeedle->image_align(egt::AlignFlag::center);
            imgNeedle->move(egt::Point(0, needles[i+39].frame_attr.pan_y));
            ImgNeedlesVector.emplace_back(imgNeedle);
        }
    };

    auto initLibInput = [&app
#ifdef ENABLE_UART
        ,&pollUartfds, &argv
#endif
    ]()
    {
        if (!udev_init_done)
            udev_init_done = true;
        else
            return;

        std::cout << std::endl << "Enable libinput in app" << std::endl;
        app.setup_inputs();

#ifdef ENABLE_UART
        pollUartfds.fd = uartOpen(argv[1]);
        uartSetSpeed(pollUartfds.fd, 115200);

        if (uartSetParity(pollUartfds.fd,8,1,'N') == -1)
        {
            printf("Set Parity Error\n");
            return;
        }
        else
        {
            printf("%s connected\r\n", argv[1]);
        }
#endif
    };

    // One second periodic timer
    auto sec_timer = std::make_shared<egt::PeriodicTimer>(std::chrono::milliseconds(1000));
    sec_timer->on_timeout([](){ sec_tick++; });

    ///============ Main timer for state machine =============
    egt::PeriodicTimer main_timer(std::chrono::milliseconds(1));
    main_timer.on_timeout([&]() 
    {
#ifdef ENABLE_UART    
		if( 0 < poll(&pollUartfds, 1, 0) )
		{
			// check if any data came from UART
			if( (nread = read(pollUartfds.fd, recBuff, 512)) >0)
			{
                //std::cout << "read byte: " << nread << std::endl;
				debug_buffer(recBuff, nread);
				
				ble_no = ble_identity_notification(recBuff, nread);
				switch( ble_no )
				{
					case BLE_NOTY_INCOMING_CALL:
						strcpy(cmdBuff, "Y\n\r");
						write(pollUartfds.fd, cmdBuff, 3);
                        isCalling = true;
						break;
						
					case BLE_NOTY_REMOVE_CALL:
                        isCalling = false;
                        appData.blestate = BLE_QUIT_CALL_SMS;
						break;

                    case BLE_NOTY_MISSED_CALL:
                        //isCalling = false;
                        //appData.blestate = BLE_QUIT_CALL_SMS;
						break;
					
					case BLE_NOTY_RETRIEVE_DETAIL:
                        //std::cout << "get detail" << std::endl;
						ble_get_caller_detail(recBuff, nread, &caller_info);
						printf("Caller: %s\r\n", caller_info.caller_number);
						printf("MSG: %s\r\n", caller_info.msg);
						printf("Date: %s\r\n", caller_info.date);
						
						strcpy(cmdBuff, "Y\n\r");
						write(pollUartfds.fd, cmdBuff, 3);
                        if (isCalling)
                            appData.blestate = BLE_CALL_IN;
                        // else if (isSMS)
                        //     appData.blestate = BLE_SMS_IN;
						break;
						
					case BLE_NOTY_ANSWER_CALL:
                        
						break;
						
					case BLE_NOTY_SOCIAL_MEDIA:
                        if( !isReject )
                        {
                            printf("BLE_NOTY_SOCIAL_MEDIA\n\r");
                            strcpy(cmdBuff, "Y\n\r");
                            write(pollUartfds.fd, cmdBuff, 3);
                            isSMS = true;
                        }
                        else
                        {
                            isReject = false;
                        }
						break;
						
					default:
						break;
				}
			
				memset(recBuff, 0, RECEIVE_BUFFER_SIZE);
			}
    	}

        switch (appData.blestate)
        {
            case BLE_QUIT_CALL_SMS:
            {
                imgBtnaccept->hide();
                imgLblcall->hide();
                imgBtnreject->hide();
                imgLblsms->hide();
                lblCaller->hide();
                refresh_count = 0;
                OverlayWinVector[1]->show();
                appData.blestate = BLE_NONE;
                //appData.state = app_last_state;
                break;
            }
            case BLE_CALL_IN:
            {
                std::cout << "go to BLE_CALL_IN" << std::endl;
                OverlayWinVector[1]->hide();
                imgBtnaccept->show();
                imgBtnreject->show();
                lblCaller->text(caller_info.caller_number);
                lblCaller->show();
                appData.blestate = BLE_CALL_BLINKING;
                break;
            }
            case BLE_CALL_BLINKING:
            {
                if( REFRESH_PERIOD <= refresh_count )
                {
                    imgLblcall->visible_toggle();
                    refresh_count = 0;
                }
                
                refresh_count++;
                break;
            }
            case BLE_CALL_ANSWERED:
            {
                imgBtnaccept->hide();
                imgLblcall->show();
                appData.blestate = BLE_NONE;
                //app_last_state = appData.state;
                //appData.state = APP_STATE_IDLE;
                break;
            }
            case BLE_SMS_IN:
            {
                OverlayWinVector[1]->hide();
                lblCaller->text(caller_info.caller_number);
                lblCaller->show();
                appData.blestate = BLE_SMS_BLINKING;
                break;
            }
            case BLE_SMS_BLINKING:
            {
                if( REFRESH_PERIOD <= refresh_count )
                    imgLblsms->visible_toggle();
                
                refresh_count++;

                if( refresh_count >= REFRESH_PERIOD * 12 )
                {
                    isSMS = false;
                    refresh_count = 0;
                    appData.blestate = BLE_SMS_SHOW;
                }
                break;
            }
            case BLE_SMS_SHOW:
            {
                OverlayWinVector[1]->hide();
                lblSMS->show();
                refresh_count++;

                if( refresh_count >= REFRESH_PERIOD * 32 )
                    appData.blestate = BLE_QUIT_CALL_SMS;

                break;
            }
            case BLE_NONE:
            {
                break;
            }
            default:
            {
                break;
            }
        }
#endif // end of ENABLE_UART   


        if (tick_start)
            tick++;

        switch (appData.state)
        {
            case APP_STATE_INIT:
            {
                sec_tick = 0;
                appData.state = APP_STATE_INIT_NEEDLE_SHOW;
                break;
            }
            case APP_STATE_INIT_NEEDLE_SHOW:
            {
                for (auto i=0; i<3; i++)
                    OverlayWinVector[i]->show();
                tick_start = true;
                appData.state = APP_STATE_NEEDLE_TWIRL;   
                appData.nstate = TWIRL_ACCELERATE_START;
                break;
            }
            case APP_STATE_NEEDLE_TWIRL:
            {
                if (!needles_stage2_cp_done)
                {
                    initStage2Needles();
                    needles_stage2_cp_done = true;
                }
                
                if (tick != prev_tick)
                {               
                    prev_tick = tick; 
                    APP_ProcessNeedle(OverlayWinVector[0]->GetOverlay());
                }
                break;
            }  
            case APP_STATE_FADEOUT_ICON:
            {
                tick_start = false;
                fade.request("ovr2_fade_out_50");
                imgLblLogo->hide();
                GPSLabels[0]->hide();
                sec_timer->start();
                appData.state = APP_STATE_SPEED_INIT1;
                break;
            }
            case APP_STATE_SPEED_INIT1:
            {
                if (!gpswgt_init_done)
                {
                    initGPSwgt();
                    gpswgt_init_done = true;
                }

                if (sec_tick > 0)
                {
                    sec_tick = 0;
                    fade.request("ovr2_fade_in_10");
                    tick = 0;
                    tick_start = true;
                    appData.state = APP_STATE_SPEED_INIT2;
                }
                break;
            }
            case APP_STATE_SPEED_INIT2:
            {
                if (tick > 45)
                {
                    for (auto i=1; i<3; i++)
                        GPSLabels[i]->show();
                    appData.state = APP_STATE_SPEED_INIT3;
                }  
                break;
            }
            case APP_STATE_SPEED_INIT3:
            {
                if (!needles_stage3_cp_done)
                {
                    initStage3Needles();
                    needles_stage3_cp_done = true;
                }

                if (sec_tick > 1)
                {
                    appData.nstate = DRIVE_START;
                    appData.state = APP_STATE_INIT_INPUT;
                }
                break;
            }
            case APP_STATE_INIT_INPUT:
            {
                initLibInput();
                appData.state = APP_STATE_DRIVE;
                break;
            }
            case APP_STATE_DRIVE:
            {
                APP_ProcessNeedle(OverlayWinVector[0]->GetOverlay());
                if (sec_tick != prev_sec_tick)
                {   
                    sec2++;            
                    prev_sec_tick = sec_tick; 
                    APP_ProcessMap();
                    checkNeedleAnime();
                    if (sec2 > 1)
                    {
                        sec2 = 0;
                        pulseBlur();
                    }
                }
                break;
            }
            case APP_STATE_PAUSE:
            {
                sec_tick = 0;
                appData.state = APP_STATE_REACHED;
                if (blur_alpha_high==false)
                {
                    blur_alpha_high = true;
                    fade.request("ovrheo_fade_in_lit_50");
                }
                break;
            }
            case APP_STATE_REACHED:
            {
                if (sec_tick > 2)
                {
                    /* Reset the demo so we can restart everything */
                    sec_tick = 0;
                    fade.request("ovr2_fade_out_10");
                    fade.request("ovrheo_fade_out_10");
                    appData.state = APP_STATE_LOOPBACK;
                    showNavImg(0xFF);
                    OverlayWinVector[0]->hide();
                }
                break;
            }
            case APP_STATE_LOOPBACK:
            {
                if (sec_tick > 2)
                {
                    GPSLabels[0]->text("DRIVE SAFE!");
                    imgLblLogo->show();
                    GPSLabels[2]->hide();
                    GPSLabels[1]->hide();
                    appData.state = APP_STATE_INIT_NEEDLE_SHOW;
                    sec_timer->stop();
                    tick_start = false;
                }
                break;
            }
            case APP_STATE_IDLE:
                break;
            default:
                break;
        }
    });
    main_timer.start();
    ///============ Main timer for state machine end =============

    // Touch event handler
    window.on_event(handle_touch);

    window.show();

    auto ret = app.run();

    // Destruction for application if needed

    return ret;
}
