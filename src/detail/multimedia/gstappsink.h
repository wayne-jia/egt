/*
 * Copyright (C) 2024 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef EGT_SRC_DETAIL_MULTIMEDIA_GSTAPPSINK_H
#define EGT_SRC_DETAIL_MULTIMEDIA_GSTAPPSINK_H

#include <memory>
#include <string>

#include <gst/gst.h>

#include "detail/multimedia/gstsink.h"
#include "egt/detail/meta.h"

namespace egt
{
inline namespace v1
{
namespace detail
{

class GstDecoderImpl;

class GstAppSink : public GstSink
{
public:
    Signal<const unsigned char*, const unsigned int> on_new_frames;

    GstAppSink(GstDecoderImpl& gst_decoder, const Size& size, Window& window, PixelFormat format);

    EGT_NODISCARD std::string description() override;

    void draw(Painter& painter, const Rect& rect) override;

    bool post_initialize() override;

    bool has_new_frame_signal() const override { return true; }

    Signal<const unsigned char*, const unsigned int>& get_new_frame_signal() override {
        return on_new_frames;
    }

private:

    static GstFlowReturn on_new_buffer(GstElement* elt, gpointer data);

    GstSample* m_videosample{nullptr};

    Window& m_window;

    cairo_format_t m_gst_format;
};

} // end of namespace detail

} // end of namespace v1

} // end of namespace egt

#endif
