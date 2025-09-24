/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/// @[Example]
#include <egt/ui>
#include <iostream>
#include <string>
#include <fstream>
#include <poll.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <cxxopts.hpp>
#include "serialport.h"


#define TIMER_PEORIOD         30  // ms
#define MAX_UART_MSG_LEN      1
#define UART_CMD_MSG_LEN      5
#define UART_CHUNK_MSG_LEN    32
#define UART_CHUNK_BUF_LEN    (UART_CHUNK_MSG_LEN + 4)

typedef enum
{
    FILE_COUNT = 0xFA,
    FILE_CRC32 = 0xFD,
    FILE_SIZE = 0xFE,
    FILE_END = 0xFF
} TRANSMIT_CMD;

typedef enum
{
    MSG_VER_CHECK = 0,
    MSG_SELECT_V1,
    MSG_SELECT_V2,
    MSG_DOWNLOAD,
    MSG_ACK,
    MSG_NAK,
    MSG_ERROR
} MESSAGE_TYPE;

static char file_buffer[1024*1024] = {0};
static char recv_buf[MAX_UART_MSG_LEN] = {0};
static char cmd_buf[UART_CMD_MSG_LEN] = {0};
static char chunk_buf[UART_CHUNK_BUF_LEN] = {0};
static uint32_t selected_file_id = 0;
static size_t selected_file_size = 0;
static size_t offset = 0;
static uint32_t chunkid = 0;
static uint32_t crc32_table[256];
//static uint32_t crc32 = 0;
static const char* fw_files[] = {
    "harmony-v2.bin",
    "harmony-lvgl.bin"
};

static void init_crc32_table() 
{  
    uint32_t polynomial = 0xEDB88320;  
    for (uint32_t i = 0; i < 256; i++) {  
        uint32_t crc = i;  
        for (int j = 0; j < 8; j++) {  
            crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);  
        }  
        crc32_table[i] = crc;  
    }  
}  

static uint32_t calculate_crc32(const void *data, size_t length) 
{  
    uint32_t crc = 0xFFFFFFFF;  
    const uint8_t *bytes = (const uint8_t *)data;  
    for (size_t i = 0; i < length; i++) {  
        crc = (crc >> 8) ^ crc32_table[(crc ^ bytes[i]) & 0xFF];  
    }  
    return crc ^ 0xFFFFFFFF;  
}  

static void package_cmd_data(TRANSMIT_CMD command, const int32_t data)
{
    cmd_buf[0] = command;
    cmd_buf[1] = (data >> 0) & 0xFF;
    cmd_buf[2] = (data >> 8) & 0xFF;
    cmd_buf[3] = (data >> 16) & 0xFF;
    cmd_buf[4] = (data >> 24) & 0xFF;
}

static void package_chunk_data(const int32_t crc)
{
    chunk_buf[UART_CHUNK_MSG_LEN] = (crc >> 0) & 0xFF;
    chunk_buf[UART_CHUNK_MSG_LEN+1] = (crc >> 8) & 0xFF;
    chunk_buf[UART_CHUNK_MSG_LEN+2] = (crc >> 16) & 0xFF;
    chunk_buf[UART_CHUNK_MSG_LEN+3] = (crc >> 24) & 0xFF;
}

static size_t getFileSize(const char* fileName) 
{
    if (fileName == NULL)
        return 0;

    struct stat statbuf;
    stat(fileName, &statbuf);
    return statbuf.st_size;
}

static size_t open_read_file(const char* filename)
{
    size_t file_size = getFileSize(filename);
    std::ifstream f(filename, std::ios::binary);
    if(!f)
    {
        std::cerr << "read " << filename << " failed!" << std::endl;
        return 1;
    }
    
    f.read((char*)file_buffer, file_size);

    return file_size;
}

static void prepare_chunk(void)
{
    memcpy(chunk_buf, &file_buffer[offset], UART_CHUNK_MSG_LEN);
    offset += UART_CHUNK_MSG_LEN;
    ++chunkid;
    int32_t crc32 = calculate_crc32(chunk_buf, UART_CHUNK_MSG_LEN);
    //std::cout << "calculated chunk crc32: 0x" << std::hex << crc32 << std::dec << std::endl;
    package_chunk_data(crc32);
}

// void printbuf(char *buf, int len)
// {
//     int i;
//     for (i=0; i<len; i++)
//     {
//         printf("0x%02X ", buf[i]);
//         if ((i+1)%16 == 0)
//             printf("\r\n");
//     }
//     printf("\r\n");
// }

int main(int argc, char** argv)
{
    cxxopts::Options options("uartserver", "Nand flash IAP via Serial Port");
	options.add_options()
	("h,help", "help")
	("i,input-format", "tty device name",
		cxxopts::value<std::string>()->default_value("ttyS1"))
	("positional", "DEVICE", cxxopts::value<std::vector<std::string>>())
	;
	options.positional_help("DEVICE");

	options.parse_positional({"positional"});
	auto result = options.parse(argc, argv);

	if (result.count("help"))
	{
			std::cout << options.help() << std::endl;
			return 0;
	}

	if (result.count("positional") != 1)
	{
			std::cerr << options.help() << std::endl;
			return 1;
	}

	auto& positional = result["positional"].as<std::vector<std::string>>();

	std::string in = positional[0];
	//cout << "user input: " << in << endl;

	in = "/dev/" + in;
	std::cout << "construct serialport device: " << in << std::endl;
	auto tty = std::make_shared<SerialPort>(in);
	std::cout << "is ttyS1 open: " << tty->isOpen() << std::endl;
	if (tty.get() == nullptr) 
    {
		std::cerr << "ERROR get tty ptr" << std::endl;
		return 1;
	}

    struct pollfd fds[1];
    fds[0].fd = tty->getFd();
    fds[0].events = POLLIN;
    int32_t nBytes = 0;

    init_crc32_table();
    memset(file_buffer, 0, sizeof(file_buffer));

    egt::Application app(argc, argv);

    egt::TopWindow window;
    window.color(egt::Palette::ColorId::bg, egt::Palette::grey);
    egt::PeriodicTimer timer(std::chrono::milliseconds(TIMER_PEORIOD));

    auto vsizer = std::make_shared<egt::BoxSizer>(egt::Orientation::vertical,
                       egt::Justification::justify);
    vsizer->margin(2);
    window.add(center(vsizer));

    egt::Label label("Linux UART server");
    label.align(egt::AlignFlag::center);
    label.color(egt::Palette::ColorId::label_text, egt::Palette::white);
    label.font(egt::Font("Noto Sans", 58, egt::Font::Weight::bold));
    vsizer->add(label);

    egt::Label labelupd("\nUpgradable candidates:");
    labelupd.align(egt::AlignFlag::center);
    labelupd.color(egt::Palette::ColorId::label_text, egt::Palette::white);
    labelupd.font(egt::Font(38));
    vsizer->add(labelupd);

    egt::Label labelcan("\n[0] harmony-v2.bin\n [1] harmony-lvgl.bin ");
    labelcan.align(egt::AlignFlag::center);
    labelcan.color(egt::Palette::ColorId::label_text, egt::Palette::yellow);
    labelcan.font(egt::Font(32));
    vsizer->add(labelcan);

    // egt::Button quit("Transmit \"nihao\"");
    // quit.font(egt::Font(32));
    // quit.align(egt::AlignFlag::center);
    // quit.color(egt::Palette::ColorId::button_bg, egt::Palette::red);
    // quit.color(egt::Palette::ColorId::button_fg, egt::Palette::white);
    // quit.on_click([&tty](egt::Event&)
    // {
    //     tty->write("1234567890", 10);
    //     std::cout << "Transmit \"1234567890\" to client" << std::endl;
    // });
    // vsizer->add(quit);

    auto handle_message = [&labelcan, &tty, &timer]()
    {
        timer.stop();
        uint32_t crc32 = 0;
        switch ((MESSAGE_TYPE)recv_buf[0])
        {
            case MSG_VER_CHECK:
                std::cout << "VER_CHECK" << std::endl;
                package_cmd_data(FILE_COUNT, 2);
                tty->write(cmd_buf, UART_CMD_MSG_LEN);
                break; 
            case MSG_SELECT_V1:
                std::cout << "handle SELECT_V1" << std::endl;
                selected_file_id = 0;
                selected_file_size = getFileSize(fw_files[selected_file_id]);
                labelcan.text("\n[0] harmony-v2.bin (Selected: " + std  ::to_string(selected_file_size / 1024) + "KB)\n [1] harmony-lvgl.bin");
                package_cmd_data(FILE_SIZE, selected_file_size);
                tty->write(cmd_buf, UART_CMD_MSG_LEN);
                break;
            case MSG_SELECT_V2:
                std::cout << "handle SELECT_V2" << std::endl;
                selected_file_id = 1;
                selected_file_size = getFileSize(fw_files[selected_file_id]);
                labelcan.text("\n[0] harmony-v2.bin\n [1] harmony-lvgl.bin (Selected: " + std::to_string(selected_file_size / 1024) + "KB) ");
                package_cmd_data(FILE_SIZE, selected_file_size);
                tty->write(cmd_buf, UART_CMD_MSG_LEN);
                break;
            case MSG_DOWNLOAD:
                //std::cout << "handle DOWNLOAD" << std::endl;
                selected_file_size = open_read_file(fw_files[selected_file_id]);
                crc32 = calculate_crc32(file_buffer, selected_file_size);
                std::cout << "calculated file crc32: 0x" << std::hex << crc32 << std::dec << std::endl;
                offset = 0;
                package_cmd_data(FILE_CRC32, crc32);
                tty->write(cmd_buf, UART_CMD_MSG_LEN);
                break;
            case MSG_ACK:
                if (offset >= selected_file_size)
    {
                    package_cmd_data(FILE_END, 0);
                    tty->write(cmd_buf, UART_CMD_MSG_LEN);
                    std::cout << "Transmit FILE END" << std::endl;
                }
                else
    {
                    prepare_chunk();
                    tty->write(chunk_buf, UART_CHUNK_BUF_LEN);
                    //std::cout << "Transmit CHUNK: " << chunkid << std::endl;
                }
                break;
            case MSG_NAK:
                //std::cout << "handle NAK" << std::endl;
                offset = (offset > UART_CHUNK_MSG_LEN) ? (offset - UART_CHUNK_MSG_LEN) : 0;
                prepare_chunk();
                --chunkid;
                tty->write(chunk_buf, UART_CHUNK_BUF_LEN);
                std::cout << "Re-Transmit CHUNK: " << chunkid << std::endl;
                break;
            case MSG_ERROR:
                std::cout << "error" << std::endl;
                break;
            default:
                std::cout << "unknown message" << std::endl;
                break;
        }
        //printbuf(cmd_buf, UART_CMD_MSG_LEN);
        memset(recv_buf, 0, sizeof(recv_buf));
        timer.start();
    };

    timer.on_timeout([&tty, &fds, &handle_message, &nBytes]()
    {
                //Poll UART RX
                if (0 < poll(fds, 1, 0))
                {
            if ((nBytes = tty->read(recv_buf, MAX_UART_MSG_LEN)) < 0)
                    {
                        perror("error when server read from client");
                    }
            else if (nBytes == 0)
                    {
                        perror("uart read 0");
                    }
                    else
                    {
                handle_message();
                }     
        }
    });
    timer.start();

    window.show();

    return app.run();
}
/// @[Example]
