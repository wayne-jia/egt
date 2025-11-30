/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <chrono>
#include <cxxopts.hpp>
#include <egt/detail/string.h>
#include <egt/ui>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstring>


#pragma pack(push, 1)  // 1 byte alignment

// BITMAPFILEHEADER - 14Bytes
struct BMPFileHeader {
    uint16_t signature;      // "BM" - 0x4D42
    uint32_t file_size;      // Size
    uint16_t reserved1;      // 0
    uint16_t reserved2;      // 0
    uint32_t data_offset;    // pixcel offset
};

// BITMAPV5HEADER - 124Bytes
struct BMPV5InfoHeader {
    uint32_t header_size;        //  (124)
    int32_t  width;
    int32_t  height;
    uint16_t planes;            //  (1)
    uint16_t bit_count;         //  (24 for RGB888)
    uint32_t compression;       //  (0 = BI_RGB)
    uint32_t image_size;
    int32_t  x_pels_per_meter;
    int32_t  y_pels_per_meter;
    uint32_t colors_used;       // color used (0 = all)
    uint32_t colors_important;  // (0 = all important)

    // V5 special
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t alpha_mask;
    uint32_t cs_type;           // color space type
    int32_t  endpoints[9];      // CIEXYZTRIPLE
    uint32_t gamma_red;
    uint32_t gamma_green;
    uint32_t gamma_blue;
    uint32_t intent;            // for render
    uint32_t profile_data;      // configure data offset
    uint32_t profile_size;      // configure data size
    uint32_t reserved;
};

#pragma pack(pop)

bool save_bmp_v5(const std::vector<uint8_t>& rgb_data, int width, int height, const char* filename);
bool ConvertBGRxToRGB888(const uint8_t* bgrx_data, int width, int height, uint8_t* rgb_data);

bool save_bmp_v5(const std::vector<uint8_t>& rgb_data, int width, int height, const char* filename) {
    // Calculate bytes（BMP must 4Bytes alignment）
    int stride = ((width * 3 + 3) & ~3);  // Bytes each line，4Bytes alignment
    int image_size = stride * height;
    int file_size = 138 + image_size;  // 14 + 124 + image_size

    // Header
    BMPFileHeader file_header;
    file_header.signature = 0x4D42;  // "BM"
    file_header.file_size = file_size;
    file_header.reserved1 = 0;
    file_header.reserved2 = 0;
    file_header.data_offset = 138;   // 14 + 124 = 138Bytes

    // V5 header
    BMPV5InfoHeader info_header;
    info_header.header_size = 124;    // BITMAPV5HEADER
    info_header.width = width;
    info_header.height = height;      // Positve means saving from bottome to top
    info_header.planes = 1;
    info_header.bit_count = 24;       // 24bit RGB
    info_header.compression = 0;      // BI_RGB - non compression
    info_header.image_size = image_size;
    info_header.x_pels_per_meter = 0; // default resolution
    info_header.y_pels_per_meter = 0;
    info_header.colors_used = 0;      // 0 all colors
    info_header.colors_important = 0;

    // V5 special
    info_header.red_mask   = 0x00FF0000;  // RGB
    info_header.green_mask = 0x0000FF00;
    info_header.blue_mask  = 0x000000FF;
    info_header.alpha_mask = 0x00000000;  // No Alpha

    // Color space - sRGB
    info_header.cs_type = 0x73524742;     // 'sRGB' in little-endian

    // Init color space (CIEXYZTRIPLE)
    memset(info_header.endpoints, 0, sizeof(info_header.endpoints));

    // Gamma (2.2 non-float)
    info_header.gamma_red   = 0x00000000;  // Default Gamma
    info_header.gamma_green = 0x00000000;
    info_header.gamma_blue  = 0x00000000;

    info_header.intent = 4;                // LCS_GM_IMAGES
    info_header.profile_data = 0;          // ICC
    info_header.profile_size = 0;
    info_header.reserved = 0;

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Couldn't create: " << filename << std::endl;
        return false;
    }

    // Header (14Bytes)
    file.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));

    // V5 Header (124Bytes)
    file.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    // Write pixels（BMP saved from bottom to top）
    for (int y = height - 1; y >= 0; --y) {
        const uint8_t* row_data = rgb_data.data() + y * width * 3;

        for (int x = 0; x < width; ++x) {
            const uint8_t* pixel = row_data + x * 3;
            // RGB888 -> BGR888
            file.put(pixel[2]);  // B
            file.put(pixel[1]);  // G
            file.put(pixel[0]);  // R
        }

        int padding = stride - width * 3;
        if (padding > 0) {
            for (int i = 0; i < padding; ++i) {
                file.put(0);
            }
        }
    }

    file.close();
    std::cout << "Saved V5 BMP: " << filename
              << " (" << width << "x" << height << ")" << std::endl;
    return true;
}

bool ConvertBGRxToRGB888(const uint8_t* bgrx_data,
                         int width,
                         int height,
                         uint8_t* rgb_data)
{
    if (!bgrx_data || !rgb_data || width <= 0 || height <= 0)
        return false;

    const size_t total_pixels = static_cast<size_t>(width) * height;
    const uint8_t* src = bgrx_data;
    uint8_t* dst = rgb_data;

    for (size_t i = 0; i < total_pixels; ++i) {
        // BGRx: [B, G, R, X]
        uint8_t b = src[0];
        uint8_t g = src[1];
        uint8_t r = src[2];
        // X = src[3]

        dst[0] = r;
        dst[1] = g;
        dst[2] = b;

        src += 4;
        dst += 3;
    }

    return true;
}

// warning: not UTF-8 safe
static std::string line_break(const std::string& in, size_t width = 50)
{
    std::string out;
    std::string tmp;
    char last = '\0';
    size_t i = 0;

    for (auto& cur : in)
    {
        if (++i == width)
        {
            tmp = egt::detail::ltrim(tmp);
            out += "\n" + tmp;
            i = tmp.length();
            tmp.clear();
        }
        else if (isspace(cur) && !isspace(last))
        {
            out += tmp;
            tmp.clear();
        }
        tmp += cur;
        last = cur;
    }
    return out + tmp;
}

template<class T>
static inline T ns2ms(T n)
{
    return n / 1000000UL;
}

static bool is_target_sama5d4()
{
    std::ifstream infile("/proc/device-tree/model");
    if (infile.is_open())
    {
        std::string line;
        while (getline(infile, line))
        {
            if (line.find("SAMA5D4") != std::string::npos)
            {
                infile.close();
                return true;
            }
        }
        infile.close();
    }
    return false;
}

int main(int argc, char** argv)
{
    cxxopts::Options options(argv[0], "play video file");
    options.add_options()
    ("h,help", "Show help")
    ("i,input", "URI to video file. If there is a '&' in the URI, escape it with \\ or surround the URI with double quotes.", cxxopts::value<std::string>())
    ("width", "Width of the stream", cxxopts::value<int>()->default_value("320"))
    ("height", "Height of the stream", cxxopts::value<int>()->default_value("192"))
    ("f,format", "Pixel format", cxxopts::value<std::string>()->default_value(is_target_sama5d4() ? "xrgb8888" : "yuv420"), "[egt::PixelFormat]")
    ("pipeline", "Custom pipeline that is given to GStreamer without any changes. It discards previous options. It must contains a capsfilter for video named vcaps and an appsink named appsink, for instance: capsfilter caps=video/x-raw,format=RGB16,width=320,height=192 name=vcaps ! appsink name=appsink", cxxopts::value<std::string>());
    auto args = options.parse(argc, argv);

    if (args.count("help") ||
        (!args.count("input") && !args.count("pipeline")))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    egt::Size size(args["width"].as<int>(), args["height"].as<int>());
    auto format = egt::detail::enum_from_string<egt::PixelFormat>(args["format"].as<std::string>());
    const auto input = args.count("pipeline") ? args["pipeline"].as<std::string>() : args["input"].as<std::string>();
    const auto is_pipeline = args.count("pipeline") ? true : false;

    egt::Application app(argc, argv);
#ifdef EXAMPLEDATA
    egt::add_search_path(EXAMPLEDATA);
#endif

    egt::TopWindow win;
    win.color(egt::Palette::ColorId::bg, egt::Palette::black);

    egt::Label errlabel;
    errlabel.color(egt::Palette::ColorId::label_text, egt::Palette::white);
    errlabel.align(egt::AlignFlag::expand);
    errlabel.text_align(egt::AlignFlag::center_horizontal | egt::AlignFlag::top);
    win.add(errlabel);

    // player after label to handle drag
    egt::PixelFormat gst_format = egt::PixelFormat::xrgb8888;
    egt::VideoWindow player(size, gst_format, format, egt::WindowHint::software);
    player.move_to_center(win.center());
    player.volume(5);
    win.add(player);
    std::cout << "gst_format: " << gst_format << std::endl;

    egt::Window ctrlwindow(egt::Size(win.width(), 72), egt::PixelFormat::argb8888);
    ctrlwindow.align(egt::AlignFlag::bottom | egt::AlignFlag::center_horizontal);
    ctrlwindow.color(egt::Palette::ColorId::bg, egt::Palette::transparent);
    if (!ctrlwindow.plane_window())
        ctrlwindow.fill_flags(egt::Theme::FillFlag::blend);
    win.add(ctrlwindow);

    egt::HorizontalBoxSizer hpos;
    hpos.resize(ctrlwindow.size());
    ctrlwindow.add(hpos);

    auto logo = std::make_shared<egt::ImageLabel>(egt::Image("icon:mgs_logo_icon.png;32"));
    logo->margin(10);
    // The size needs to be set again since the margin has been modified.
    const auto m = logo->moat();
    logo->resize(logo->image().size_orig() + egt::Size(2 * m, 2 * m));
    hpos.add(logo);

    egt::ImageButton playbtn(egt::Image("res:pause_png"));
    playbtn.fill_flags().clear();
    hpos.add(playbtn);

    playbtn.on_click([&playbtn, &player](egt::Event&)
    {
        if (player.playing())
        {
            if (player.pause())
                playbtn.image(egt::Image("res:play_png"));
        }
        else
        {
            if (player.play())
                playbtn.image(egt::Image("res:pause_png"));
        }
    });

    egt::Slider position(0, 100, 0, egt::Orientation::horizontal);
    position.width(ctrlwindow.width() * 0.20);
    position.align(egt::AlignFlag::expand_vertical);
    position.slider_flags().set({egt::Slider::SliderFlag::round_handle});
    hpos.add(position);

    position.on_value_changed([&position, &player]()
    {
        auto state = player.playing();
        if (state)
            player.pause();

        player.seek((player.duration() * position.value()) / position.ending());

        if (state)
            player.play();

    });

    egt::ImageButton volumei(egt::Image("res:volumeup_png"));
    volumei.fill_flags().clear();
    hpos.add(volumei);

    egt::Slider volume(egt::Size(ctrlwindow.width() * 0.10,
                                 ctrlwindow.height()),
                       0, 10, 0, egt::Orientation::horizontal);
    hpos.add(volume);
    volume.slider_flags().set({egt::Slider::SliderFlag::round_handle});
    volume.value(5);
    player.volume(5.0);
    volume.on_value_changed([&volume, &player]()
    {
        auto val = static_cast<double>(volume.value());
        player.volume(val);
    });
    volume.value(5);

    egt::ImageButton fullscreenbtn(egt::Image("res:fullscreen_png"));
    fullscreenbtn.fill_flags().clear();
    hpos.add(fullscreenbtn);

    const auto ssize = egt::Application::instance().screen()->size();

    fullscreenbtn.on_click([&fullscreenbtn, &player, &ssize, &win](egt::Event&)
    {
        static bool scaled = true;
        if (scaled)
        {
            auto wscale = static_cast<float>(ssize.width()) / player.width();
            auto hscale = static_cast<float>(ssize.height()) / player.height();
            player.move(egt::Point(0, 0));
            player.scale(wscale, hscale);
            fullscreenbtn.image(egt::Image("res:fullscreen_exit_png"));
            scaled = false;
        }
        else
        {
            player.scale(1.0, 1.0);
            player.move_to_center(win.center());
            fullscreenbtn.image(egt::Image("res:fullscreen_png"));
            scaled = true;
        }
    });

    egt::ImageButton loopbackbtn(egt::Image("res:repeat_one_png"));
    loopbackbtn.fill_flags().clear();
    hpos.add(loopbackbtn);

    loopbackbtn.on_click([&loopbackbtn, &player](egt::Event&)
    {
        if (player.loopback())
        {
            loopbackbtn.image(egt::Image("res:repeat_one_png"));
            player.loopback(false);
        }
        else
        {
            loopbackbtn.image(egt::Image("res:repeat_png"));
            player.loopback(true);
        }
    });

    egt::Label cpulabel("CPU: 0%");
    cpulabel.color(egt::Palette::ColorId::label_text, egt::Palette::white);
    cpulabel.margin(5);
    hpos.add(cpulabel);

    egt::experimental::CPUMonitorUsage tools;
    egt::PeriodicTimer cputimer(std::chrono::seconds(1));
    cputimer.on_timeout([&cpulabel, &tools]()
    {
        tools.update();
        std::ostringstream ss;
        ss << "CPU: " << static_cast<int>(tools.usage()) << "%";
        cpulabel.text(ss.str());
    });
    cputimer.start();

    // wait to start playing the video until the window is shown
    win.on_show([&player, input, is_pipeline, &position, &ctrlwindow, &volume, &volumei, &hpos]()
    {
        if (is_pipeline)
            player.gst_custom_pipeline(input);
        else
        {
            player.media(input);
        }

        if (!player.has_audio())
        {
            position.width(ctrlwindow.width() * 0.45);
            hpos.remove(&volume);
            hpos.remove(&volumei);
        }

        player.play();
    });

    player.on_position_changed([&player, &position](int64_t pos)
    {
        if (player.playing())
        {
            position.on_value_changed.disable();
            position.value((ns2ms<double>(pos) /
                            ns2ms<double>(player.duration())) * 100.);
            position.on_value_changed.enable();
        }
    });

    bool frame_got = false;
    player.on_new_frame([&frame_got, size](const unsigned char* buf, const unsigned int buf_size)
    {
        if (!frame_got) {
            frame_got = true;
            int width = static_cast<int>(size.width());
            int height = static_cast<int>(size.height());
            std::vector<uint8_t> rgb_data(width * height * 3, 0);

            std::cout << "Snapshot one frame, size: " << buf_size << std::endl;
            ConvertBGRxToRGB888(buf, width, height, rgb_data.data());
            save_bmp_v5(rgb_data, width, height, "genv5.bmp");
        }
    });

    player.on_error([&errlabel](const std::string & err)
    {
        errlabel.text(line_break(err));
    });

    player.user_drag(true);
    player.user_track_drag(true);
    player.on_event([&player, &win, &ssize](egt::Event & event)
    {
        static egt::Point drag_start_point;
        switch (event.id())
        {
        case egt::EventId::pointer_drag_start:
        {
            drag_start_point = player.box().point();
            break;
        }
        case egt::EventId::pointer_drag:
        {
            auto wscale = static_cast<float>(ssize.width()) / player.width();
            if (!(egt::detail::float_equal(player.hscale(), wscale)))
            {
                auto diff = event.pointer().drag_start - event.pointer().point;
                auto p = drag_start_point - egt::Point(diff.x(), diff.y());
                auto max_x = win.width() - player.width();
                auto max_y = win.height() - player.height();
                if (p.x() >= max_x)
                    p.x(max_x);
                if (p.x() < 0)
                    p.x(0);
                if (p.y() >= max_y)
                    p.y(max_y);
                if (p.y() < 0)
                    p.y(0);
                player.move(p);
            }
            break;
        }
        default:
            break;
        }
    });

    win.show();
    win.layout();
    player.show();
    ctrlwindow.show();

    return app.run();
}
