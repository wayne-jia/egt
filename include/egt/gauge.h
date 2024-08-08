/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef EGT_GAUGE_H
#define EGT_GAUGE_H

/**
 * @file
 * @brief Working with gauges.
 */

#include <egt/color.h>
#include <egt/detail/math.h>
#include <egt/detail/meta.h>
#include <egt/frame.h>
#include <egt/image.h>
#include <egt/widget.h>
#include <memory>
#include <vector>

namespace egt
{
inline namespace v1
{
namespace experimental
{
class Gauge;

/**
 * A layer of a Gauge.
 */
class EGT_API GaugeLayer : public Widget
{
public:

    /**
     * Construct a gauge layer with an image.
     *
     * @param[in] image The image to display.
     */
    explicit GaugeLayer(const Image& image = {}) noexcept;

    /**
     * Construct a gauge layer with an image and a parent gauge.
     *
     * @param gauge Parent gauge.
     * @param[in] image The image to display.
     */
    explicit GaugeLayer(Gauge& gauge, const Image& image = {}) noexcept;

    void draw(Painter& painter, const Rect& rect) override;

    /**
     * Set the image of the gauge layer.
     *
     * This will resize the layer to be the same size as the image.
     *
     * @param[in] image The image to display.
     */
    void image(const Image& image)
    {
        m_image = image;
        resize(m_image.size());
        damage();
    }

    void mask_color(const Color& color)
    {
        if (detail::change_if_diff<>(m_mask_color, color))
            damage();
    }

    /**
     * Get the gauge of the layer.
     */
    EGT_NODISCARD Gauge* gauge() const { return m_gauge; }

protected:

    /**
     * Normally this does not need to be called directly.
     *
     * When a GaugeLayer is added to a Gauge, this will automatically be called.
     */
    virtual void gauge(Gauge* gauge)
    {
        if (!m_gauge || !gauge)
        {
            if (detail::change_if_diff<>(m_gauge, gauge))
                damage();
        }
    }

    /// The Layer image.
    Image m_image;

    /// Parent gauge.
    Gauge* m_gauge{nullptr};

    /// Optional mask color
    Color m_mask_color;

    friend class Gauge;
};

/**
 * Special GaugeLayer that deals with a rotating needle.
 *
 * This works like an egt:: ValueRangeWidget. The needle is created with any
 * range you chose, and the value of the needle must fall in this range.  The
 * range corresponds to the configured display start and stop angle.
 */
class EGT_API NeedleLayer : public GaugeLayer
{
public:

    /**
     * Event signal.
     * @{
     */
    /// Invoked when the value of the layer changes.
    Signal<> on_value_changed;
    /** @} */

    using GaugeLayer::GaugeLayer;

    /**
     * @param[in] image The image for the layer.
     * @param[in] min The min value.
     * @param[in] max The max value.
     * @param[in] angle_start The starting angle of the needle.
     * @param[in] angle_stop The stop angle of the needle.
     * @param[in] clockwise Rotation is clockwise.
     */
    explicit NeedleLayer(const Image& image,
                         float min,
                         float max,
                         float angle_start = 0,
                         float angle_stop = 360,
                         bool clockwise = true) noexcept;

    /**
     * @param[in] image The image for the layer.
     * @param[in] min The min value.
     * @param[in] max The max value.
     * @param[in] init_value The initialized value.
     * @param[in] angle_start The starting angle of the needle.
     * @param[in] angle_stop The stop angle of the needle.
     * @param[in] clockwise Rotation is clockwise.
     */
    explicit NeedleLayer(const Image& image,
                         float min,
                         float max,
                         float init_value,
                         float angle_start = 0,
                         float angle_stop = 360,
                         bool clockwise = true) noexcept;

    void draw(Painter& painter, const Rect& rect) override;

    /**
     * Get the angle start value.
     */
    EGT_NODISCARD float angle_start() const { return m_angle_start; }

    /**
     * Set the start angle of the needle.
     *
     * @param[in] angle_start The starting angle of the needle.
     */
    float angle_start(float angle_start)
    {
        float orig = m_angle_start;
        if (detail::change_if_diff<float>(m_angle_start, angle_start))
            damage();
        return orig;
    }

    /**
     * Get the angle stop value.
     */
    EGT_NODISCARD float angle_stop() const { return m_angle_stop; }

    /**
     * Set the stop angle of the needle.
     *
     * @param[in] angle_stop The stop angle of the needle.
     */
    float angle_stop(float angle_stop)
    {
        float orig = m_angle_stop;
        if (detail::change_if_diff<float>(m_angle_stop, angle_stop))
            damage();
        return orig;
    }

    /// Get the min value.
    EGT_NODISCARD float min() const { return m_min; }

    /**
     * Set the min value.
     *
     * @param[in] min The min value.
     */
    float min(float min)
    {
        float orig = m_min;
        if (detail::change_if_diff<float>(m_min, min))
        {
            value(m_value);
            damage();
        }

        return orig;
    }

    /// Get the max value.
    EGT_NODISCARD float max() const { return m_max; }

    /**
     * Set the max value.
     *
     * @param[in] max The max value.
     */
    float max(float max)
    {
        float orig = m_max;
        if (detail::change_if_diff<float>(m_max, max))
        {
            value(m_value);
            damage();
        }

        return orig;
    }

    /// Get the value of the needle.
    EGT_NODISCARD float value() const { return m_value; }

    /**
     * Set the value.
     *
     * If this results in changing the value, it will damage() the widget.
     *
     * @return The old value.
     */
    float value(float value)
    {
        auto orig = m_value;

        value = std::round(detail::clamp<float>(value, m_min, m_max));

        if (!detail::float_equal(m_value, value))
        {
            damage(rectangle_of_rotated());
            m_value = value;
            on_value_changed.invoke();
            damage(rectangle_of_rotated());
        }

        return orig;
    }

    /**
     * Get the needle point.
     */
    EGT_NODISCARD PointF needle_point() const { return m_point; }

    /**
     * Set the needle point.
     *
     * This is an alternate way to position the needle instead of using the
     * normal Widget::move(), you can use the @b "needle center".
     *
     * @param point This is the rotate point of the @ref needle_center() on the
     *              gauge surface.
     */
    void needle_point(const PointF& point)
    {
        if (detail::change_if_diff<PointF>(m_point, point))
        {
            auto dim = std::max(m_image.width(), m_image.height());
            auto circle = Circle(Point(m_point.x(), m_point.y()), dim);
            auto superrect = circle.rect();

            // real widget size is the big rect, center is a common point
            resize(superrect.size());
            move_to_center(superrect.center());
        }
    }

    /**
     * Get the needle center.
     */
    EGT_NODISCARD PointF needle_center() const { return m_center; }

    /**
     * Set the needle center.
     *
     * This is the rotate point of the needle on the needle itself.  For example,
     * if a symmetrical needle rotates on its center, this would be a point on
     * the center of the needle layer itself.
     */
    void needle_center(const PointF& center)
    {
        if (detail::change_if_diff<PointF>(m_center, center))
            damage();
    }

    /**
     * Get the clockwise value.
     */
    EGT_NODISCARD bool clockwise() const { return m_clockwise; }

    /**
     * Set the clockwise value.
     *
     * @param[in] value The clockwise value.
     */
    void clockwise(bool value)
    {
        if (detail::change_if_diff<>(m_clockwise, value))
            damage();
    }

    Rect rot_rect(float value);
    Rect get_rect_orig() { return m_rect_orig; }
    void drawbuf(Painter& painter);

protected:

    Rect rectangle_of_rotated();

    Rect m_rect_orig;

    Rect m_rect_rot;

    /// @private
    void gauge(Gauge* gauge) override;

    /// Minimum value of the needle.
    float m_min{0.0};

    /// Maximum value of the needle.
    float m_max{0.0};

    /// The value of the needle.
    float m_value{0.0};

    /// Starting angle of the needle.
    float m_angle_start{0.0};

    /// Stopping/Ending angle of the needle.
    float m_angle_stop{360.0};

    /// Does the needle rotate clockwise or counterclockwise.
    bool m_clockwise{true};

    /// Center point of the needle.
    PointF m_center;

    /// Rotate point of the needle on the gauge.
    PointF m_point;
};

/**
 * A Gauge Widget that is composed of GaugeLayer layers.
 *
 * In its simplest form, gauge layers are created using an Image. The images can
 * come from SVG files or other image file types, or even individual elements of
 * an SVG file using the SvgImage class.
 *
 * @image html gauge.png
 * @image latex gauge.png "gauge layers" width=5cm
 *
 * Creating a simple gauge with a base layer and a needle layer, from a single
 * SVG file is possible.
 *
 * @code{.cpp}
 * Gauge custom1;
 *
 * SvgImage custom1svg("gauge.svg", SizeF(200, 0));
 *
 * auto custom1_background = make_shared<GaugeLayer>(custom1svg.id("#base"));
 * custom1.add_layer(custom1_background);
 *
 * auto custom1_needle = make_shared<NeedleLayer>(custom1svg.id("#needle"),
 *                                                0, 100, 0, 180);
 * auto custom1_needle_point = custom1svg.id_box("#needlepoint").center();
 * custom1_needle->needle_point(custom1_needle_point);
 * custom1_needle->needle_center(custom1_needle_point);
 * custom1.add_layer(custom1_needle);
 * @endcode
 *
 * A GaugeLayer is actually a special Widget.  So, you can control its
 * properties just like any other widget and handle events from it as
 * well.
 *
 * Some [excellent tutorials](https://www.youtube.com/watch?v=VWFn1LOIScQ) can
 * be found online for creating your own gauges using
 * [Inkscape](https://inkscape.org/).  Of course, any tool that can create
 * standard SVG files works just as well.
 */
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
class EGT_API Gauge : public Frame
{
public:

    /**
    * @param[in] rect Initial rectangle of the widget.
    * @param[in] flags Widget flags.
    */
    explicit Gauge(const Rect& rect = {},
                   const Flags& flags = {}) noexcept;

    /**
     * @param[in] parent Parent Frame of the Widget.
     * @param[in] rect Initial rectangle of the widget.
     * @param[in] flags Widget flags.
     */
    Gauge(Frame& parent, const Rect& rect,
          const Flags& flags = {}) noexcept;

    using Frame::add;

    /**
     * Add a GaugeLayer to the Gauge.
     *
     * The Gauge will automatically be resized to the maximum size of any
     * GaugeLayer.
     *
     * @todo This does not account for adding the same layer multiple times.
     */
    void add(const std::shared_ptr<GaugeLayer>& layer);

    /**
     * Add a GaugeLayer to the Gauge.
     */
    void add(GaugeLayer& layer)
    {
        // Nasty, but it gets the job done.  If a widget is passed in as a
        // reference, we don't own it, so create a "pointless" shared_ptr that
        // will not delete it.
        auto w = std::shared_ptr<GaugeLayer>(&layer, [](GaugeLayer*) {});
        add(w);
    }

    using Frame::remove;

    /// Remove a layer from the gauge.
    void remove(GaugeLayer* layer);

    ~Gauge() noexcept override;

protected:

    /**
     * Get the size of all of the layers together.
     */
    EGT_NODISCARD Size super_size() const
    {
        Rect result = content_area().size();
        for (const auto& layer : m_layers)
            result = Rect::merge(result, layer->box() + Size(moat() * 2, moat() * 2));
        return result.size();
    }

    /// Type for an array of layers.
    using LayerArray = std::vector<std::shared_ptr<GaugeLayer>>;

    /// The layer's of the gauge.
    LayerArray m_layers;
};

}
}
}

#endif
