/*
 * Copyright (C) 2018 Microchip Technology Inc.  All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "detail/utf8text.h"
#include "egt/app.h"
#include "egt/button.h"
#include "egt/embed.h"
#include "egt/input.h"
#include "egt/keycode.h"
#include "egt/popup.h"
#include "egt/sizer.h"
#include "egt/virtualkeyboard.h"
#ifdef HAVE_LIBPINYIN
#include "detail/egtlog.h"
#endif

#ifdef SRCDIR
EGT_EMBED(internal_microphone, SRCDIR "/icons/32px/microphone.png")
#endif

namespace egt
{
inline namespace v1
{
#ifdef HAVE_LIBPINYIN
static constexpr inline bool isAsciiLetter(char32_t ch) {
    return (ch >= U'A' && ch <= U'Z') ||
            (ch >= U'a' && ch <= U'z');
}

static std::vector<uint32_t> utf8_to_codepoints(const std::string& str) {
    std::vector<uint32_t> result;
    size_t i = 0;
    while (i < str.size()) {
        unsigned char c = str[i];
        uint32_t codepoint = 0;
        size_t bytes = 0;

        if ((c & 0x80) == 0)
        {
            codepoint = c;
            bytes = 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            codepoint = c & 0x1F;
            bytes = 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            codepoint = c & 0x0F;
            bytes = 3;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            codepoint = c & 0x07;
            bytes = 4;
        }

        for (size_t j = 1; j < bytes; ++j)
            codepoint = (codepoint << 6) | (str[i + j] & 0x3F);

        result.push_back(codepoint);
        i += bytes;
    }
    return result;
}

#endif

VirtualKeyboard::Key::Key(uint32_t unicode, double length) noexcept
    : m_button(std::make_shared<Button>()),
      m_unicode(unicode),
      m_length(length)
{
    std::string tmp;
    utf8::append(unicode, std::back_inserter(tmp));
    m_button->text(tmp);
    m_button->autoresize(false);
}

VirtualKeyboard::Key::Key(const std::string& label, KeyboardCode keycode,
                          double length) noexcept
    : m_button(std::make_shared<Button>(label)),
      m_keycode(keycode),
      m_length(length)
{
    m_button->autoresize(false);
#ifdef HAVE_LIBPINYIN
    m_button->font(Font("Noto Sans CJK SC"));
#endif
}

VirtualKeyboard::Key::Key(const Image& img, KeyboardCode keycode,
                          double length) noexcept
    : m_button(std::make_shared<ImageButton>(img)),
      m_keycode(keycode),
      m_length(length)
{
    m_button->autoresize(false);
}

VirtualKeyboard::Key::Key(const std::string& label, int link,
                          double length) noexcept
    : m_button(std::make_shared<Button>(label)),
      m_length(length),
      m_link(link)
{
    m_button->autoresize(false);
}

VirtualKeyboard::Key::Key(const Image& img, int link,
                          double length) noexcept
    : m_button(std::make_shared<ImageButton>(img)),
      m_length(length),
      m_link(link)
{
    m_button->autoresize(false);
}

VirtualKeyboard::Key::Key(uint32_t unicode,
                          PanelKeys keys_multichoice,
                          double length) noexcept
    : m_button(std::make_shared<Button>()),
      m_unicode(unicode),
      m_length(length),
      m_keys_multichoice(std::move(keys_multichoice))
{
    std::string tmp;
    utf8::append(unicode, std::back_inserter(tmp));
    m_button->text(tmp);
    m_button->autoresize(false);
}

void VirtualKeyboard::Key::color(Palette::ColorId id, const Pattern& color, Palette::GroupId group)
{
    m_button->color(id, color, group);
}

void VirtualKeyboard::Key::font(const Font& font)
{
    m_button->font(font);
}

void VirtualKeyboard::initialize(const std::vector<PanelKeys>& keys)
{
    m_main_panel.align(AlignFlag::expand);
    add(m_main_panel);

    for (auto& keys_panel : keys)
    {
        m_max_rows = std::max(m_max_rows, unsigned(keys_panel.size()));

        for (auto& keys_row : keys_panel)
            m_max_cols = std::max(m_max_cols, unsigned(keys_row.size()));
    }

    for (auto& keys_panel : keys)
    {
        auto panel = std::make_shared<Panel>(keys_panel);
        panel->align(AlignFlag::expand);
        panel->update_key_space(m_key_space);
        m_main_panels.push_back(panel);

        auto main_panel = std::make_shared<NotebookTab>();
        main_panel->add(panel);
        main_panel->align(AlignFlag::expand);
        // By default NotebookTab are not transparent.
        main_panel->fill_flags().clear();
        m_main_panel.add(main_panel);

        for (auto& keys_row : keys_panel)
        {
            for (auto& key : keys_row)
            {
                key_link(key);
                key_input_value(key);
                if (!key->m_keys_multichoice.empty())
                {
                    key_multichoice(key);
                }
            }
        }
    }
}

VirtualKeyboard::VirtualKeyboard(const std::vector<PanelKeys>& keys, const Rect& rect)
    : Frame(rect)
{
    name("VirtualKeyboard" + std::to_string(m_widgetid));
    initialize(keys);
}

VirtualKeyboard::VirtualKeyboard(const Rect& rect) noexcept
    : VirtualKeyboard(
{
    QwertyLettersLowerCase(),
                           QwertyLettersUpperCase(),
                           QwertySymbols1(),
                           QwertySymbols2()
}, rect)
{}

VirtualKeyboard::VirtualKeyboard(Serializer::Properties& props, bool is_derived) noexcept
    : Frame(props, true)
{
    const std::vector<PanelKeys>& keys =
    {
        QwertyLettersLowerCase(),
        QwertyLettersUpperCase(),
        QwertySymbols1(),
        QwertySymbols2()
    };

    initialize(keys);

    if (!is_derived)
        deserialize_leaf(props);
}

void VirtualKeyboard::resize(const Size& s)
{
    auto key_base_size = Size(s.width() / m_max_cols, s.height() / m_max_rows);

    for (auto& main_panel : m_main_panels)
    {
        main_panel->update_key_size(key_base_size);
        main_panel->resize(s);
    }

    m_main_panel.resize(s);
    Frame::resize(s);
}

void VirtualKeyboard::key_space(unsigned key_space)
{
    m_key_space = key_space;
    for (auto& main_panel : m_main_panels)
        main_panel->update_key_space(key_space);
}

VirtualKeyboard::Panel::Panel(const PanelKeys& keys)
    : m_keys(keys)
{
    for (auto& row : keys)
    {
        auto hsizer = std::make_shared<HorizontalBoxSizer>();
        hsizer->align(AlignFlag::expand_horizontal | AlignFlag::top);
        add(hsizer);

        for (auto& key : row)
            hsizer->add(key->m_button);
    }
}

void VirtualKeyboard::Panel::update_key_space(unsigned key_space)
{
    for (auto& row : m_keys)
        for (auto& key : row)
            key->m_button->margin(key_space / 2);
}

void VirtualKeyboard::Panel::update_key_size(const Size& s)
{
    for (auto& row : m_keys)
        for (auto& key : row)
            key->m_button->resize(Size(s.width() * key->m_length, s.height()));
}

void VirtualKeyboard::key_link(const std::shared_ptr<Key>& k)
{
    k->m_button->on_event([this, k](Event&)
    {
        m_main_panel.selected(k->m_link);
    }, {EventId::pointer_click});
}

void VirtualKeyboard::key_input_value(const std::shared_ptr<Key>& k)
{
    k->m_button->on_event([this, k](Event&)
    {
#ifdef HAVE_LIBPINYIN
        if (EKEY_CN_EN == k->m_keycode)
        {
            if (!m_is_pinyin)
            {
                k->m_button->text("拼音");
                m_is_pinyin = true;
            }
            else
            {
                k->m_button->text("Eng");
                m_is_pinyin = false;
            }
        }

        if (!k->m_button->text().empty() && m_is_pinyin)
        {
            if (EKEY_BACKSPACE == k->m_keycode)
            {
                Event event2(EventId::keyboard_down);
                event2.key().unicode = k->m_unicode;
                event2.key().keycode = EKEY_PINYIN_BACKSPACE;
                m_in.dispatch(event2);

                event2.id(EventId::keyboard_up);
                event2.key().unicode = k->m_unicode;
                event2.key().keycode = EKEY_PINYIN_BACKSPACE;
                m_in.dispatch(event2);
                return 0;
            }

            if (isAsciiLetter(k->m_unicode))
            {
                Event event2(EventId::keyboard_down);
                event2.key().unicode = k->m_unicode;
                event2.key().keycode = EKEY_PINYIN_INPUT;
                m_in.dispatch(event2);

                event2.id(EventId::keyboard_up);
                event2.key().unicode = k->m_unicode;
                event2.key().keycode = EKEY_PINYIN_INPUT;
                m_in.dispatch(event2);
                return 0;
            }
        }
#endif

        if (!k->m_button->text().empty())
        {
            Event event2(EventId::keyboard_down);
            event2.key().unicode = k->m_unicode;
            event2.key().keycode = k->m_keycode;
            m_in.dispatch(event2);

            event2.id(EventId::keyboard_up);
            event2.key().unicode = k->m_unicode;
            event2.key().keycode = k->m_keycode;
            m_in.dispatch(event2);
        }

        return 0;
    }, {EventId::pointer_click});
}

void VirtualKeyboard::key_multichoice(const std::shared_ptr<Key>& k)
{
    k->m_multichoice_panel = std::make_shared<Panel>(k->m_keys_multichoice);
    k->m_multichoice_panel->align(AlignFlag::center);

    // Create a Popup to display the multichoice panel.
    k->m_button->on_event([this, k](Event&)
    {
        k->m_multichoice_panel->update_key_space(m_key_space);
        auto key_multichoice_size = Size(width() / m_max_cols, height() / m_max_rows);
        k->m_multichoice_panel->update_key_size(Size(key_multichoice_size.width() * m_key_size_multichoice_factor,
                                                key_multichoice_size.height() * m_key_size_multichoice_factor));

        m_multichoice_popup = std::make_shared<Popup>();
        m_multichoice_popup->resize(k->m_multichoice_panel->size());
        m_multichoice_popup->add(k->m_multichoice_panel);
        Application::instance().main_window()->add(m_multichoice_popup);

        auto display_origin = k->m_button->display_origin();
        auto main_window_origin = Application::instance().main_window()->display_to_local(display_origin);

        // Popup on top of the key.
        main_window_origin.y(main_window_origin.y() - m_multichoice_popup->height());
        // If it goes out of the main_window, move it at the bottom of the key.
        if (main_window_origin.y() < 0)
            main_window_origin.y(Application::instance().main_window()->display_to_local(display_origin).y() + k->m_button->height());

        // Popup aligned with key.
        main_window_origin.x(main_window_origin.x() - m_multichoice_popup->width() / 2);
        main_window_origin.x(main_window_origin.x() + k->m_button->width() / 2);
        // Update the position if it goes out of the main_window.
        if (main_window_origin.x() < 0)
            main_window_origin.x(0);

        if (main_window_origin.x() + m_multichoice_popup->width() > Application::instance().main_window()->width())
            main_window_origin.x(Application::instance().main_window()->width() - m_multichoice_popup->width());

        m_multichoice_popup->move(main_window_origin);
        m_multichoice_popup->show_modal();
    }, {EventId::pointer_hold});

    // Create the Buttons for the multichoice panel.
    for (auto& key_multichoice_row : k->m_keys_multichoice)
    {
        for (auto& key_multichoice : key_multichoice_row)
        {
            key_multichoice->m_button->on_event([this, k, key_multichoice](Event&)
            {
                m_multichoice_popup->hide();
                Application::instance().main_window()->remove(m_multichoice_popup.get());

                if (!key_multichoice->m_button->text().empty())
                {
                    Event down(EventId::keyboard_down);
                    down.key().unicode = key_multichoice->m_unicode;
                    down.key().keycode = key_multichoice->m_keycode;
                    m_in.dispatch(down);
                    Event up(EventId::keyboard_up);
                    up.key().unicode = key_multichoice->m_unicode;
                    up.key().keycode = key_multichoice->m_keycode;
                    m_in.dispatch(up);
                    // the modal popup caught the raw_pointer_up event
                    k->m_button->active(false);
                }
                // User may just move his finger so prefer the raw_pointer_up event to the pointer_click one.
            }, {EventId::raw_pointer_up});
        }
    }
}

#ifdef HAVE_LIBPINYIN
void VirtualKeyboard::dispatch(Event event)
{
    m_in.dispatch(event);
}
#endif

static PopupVirtualKeyboard* the_popup_virtual_keyboard = nullptr;

PopupVirtualKeyboard::PopupVirtualKeyboard(const std::shared_ptr<VirtualKeyboard>& keyboard, Size size) noexcept
#ifdef HAVE_LIBPINYIN
    : m_keyboard(keyboard)
#endif
{
    // Make the keyboard partially transparent.
    fill_flags(Theme::FillFlag::blend);
#ifdef HAVE_LIBPINYIN
    color(Palette::ColorId::bg, Color(Palette::transparent, 20));
#else
    color(Palette::ColorId::bg, Color(Palette::transparent, 80));
#endif

    const auto screen_size = Application::instance().screen()->size();
    if (size.empty())
#ifdef HAVE_LIBPINYIN
        size = Size(screen_size.width(), screen_size.height() * 0.47);
#else
        size = Size(screen_size.width(), screen_size.height() * 0.4);
#endif

    resize(size);
    auto y_keyboard_position = screen_size.height() - size.height();
    move(Point(0, y_keyboard_position));

    m_vsizer.align(AlignFlag::expand);
    add(m_vsizer);

#ifdef HAVE_LIBPINYIN
    m_pinyin_input = std::make_shared<Label>("n");
    m_pinyin_input->font(Font("Noto Sans", 17, Font::Weight::normal));
    m_pinyin_input->color(Palette::ColorId::label_bg, Color(0x4d4d4dee), Palette::GroupId::normal);
    m_pinyin_input->color(Palette::ColorId::label_text, Palette::white);
    m_pinyin_input->color(Palette::ColorId::bg, Color(0x4d4d4dee), Palette::GroupId::normal);
    m_pinyin_input->color(Palette::ColorId::border, Color(0x4d4d4dee), Palette::GroupId::normal);
    m_pinyin_input->border(2);
    m_pinyin_input->border_radius(4);
    m_pinyin_input->fill_flags(Theme::FillFlag::blend);
    m_pinyin_input->margin(4);
    m_pinyin_input->align(AlignFlag::left);
    m_pinyin_input->hide();
    m_vsizer.add(m_pinyin_input);

    m_hsizer.align(AlignFlag::top);
    m_hsizer.resize(Size(size.width(), size.height() * 0.15));

    m_candidates_hsizer.align(AlignFlag::left);
    m_candidates_hsizer.resize(Size(size.width() * 0.8, size.height() * 0.15));
    m_hsizer.add(m_candidates_hsizer);

    m_icons_hsizer.align(AlignFlag::right);
    m_icons_hsizer.resize(Size(size.width() * 0.2, size.height() * 0.15));
    m_hsizer.add(m_icons_hsizer);
#else
    m_hsizer.align(AlignFlag::top | AlignFlag::right);
    m_hsizer.resize(Size(size.width() * 0.2, size.height() * 0.15));
#endif
    m_vsizer.add(m_hsizer);

#ifdef HAVE_LIBPINYIN
    for (auto i=0; i<5; i++)
    {
        auto candidate = std::make_shared<egt::Label>("你好");
        candidate->font(egt::Font("Noto Sans CJK SC", 18, egt::Font::Weight::normal));
        candidate->color(egt::Palette::ColorId::label_text, egt::Palette::white);
        candidate->color(Palette::ColorId::label_bg, Color(0x4d4d4dee), Palette::GroupId::normal);
        candidate->color(Palette::ColorId::bg, Color(0x4d4d4dee), Palette::GroupId::normal);
        candidate->color(Palette::ColorId::border, Color(0x4d4d4dee), Palette::GroupId::normal);
        candidate->border(2);
        candidate->border_radius(4);
        candidate->fill_flags(Theme::FillFlag::blend);
        candidate->margin(4);
        candidate->align(AlignFlag::left);
        candidate->hide();
        m_candidates_hsizer.add(left(candidate));
        m_candidates.emplace_back(candidate);
        m_candidates[i]->on_event([this, i, &keyboard](egt::Event&)
        {
            for (uint32_t unicode : utf8_to_codepoints(m_candidates[i]->text()))
            {
                Event event2(EventId::keyboard_down);
                event2.key().unicode = unicode;
                event2.key().keycode = EKEY_UNKNOWN;
                keyboard->dispatch(event2);

                event2.id(EventId::keyboard_up);
                event2.key().unicode = unicode;
                event2.key().keycode = EKEY_UNKNOWN;
                keyboard->dispatch(event2);
            }
            reset_pinyin_input();
        }, {egt::EventId::pointer_click});
    }

    m_prev_button.margin(5);
    m_prev_button.show();
    m_prev_button.on_event([this](Event&)
    {
        if (m_candidates_cur_idx <= 5)
            return;

        int32_t i;
        int32_t margin = m_candidates_list.size() % 5;
        int32_t back_step = m_candidates_cur_idx % 5 ? (5 + margin) : 10;
        for (i = 0; i < 5; i++)
        {
            m_candidates[i]->text(m_candidates_list[m_candidates_cur_idx - back_step + i]);
            m_candidates[i]->show();
        }
        m_candidates_cur_idx -= (back_step == 10) ? 10 : margin;
    }, {EventId::pointer_click});
    m_icons_hsizer.add(expand(m_prev_button));

    m_next_button.margin(5);
    m_next_button.show();
    m_next_button.on_event([this](Event&)
    {
        int32_t i, len;
        int32_t margin = m_candidates_list.size() % 5;
        if (m_candidates_cur_idx < m_candidates_list.size())
        {
            len = (m_candidates_cur_idx + 5 > m_candidates_list.size()) ? margin : 5;
            for (i = 0; i < len; i++)
            {
                m_candidates[i]->text(m_candidates_list[m_candidates_cur_idx+i]);
                m_candidates[i]->show();
            }
            m_candidates_cur_idx += i;
            if (len < 5)
            {
                for (i = 0; i < 5 - len; i++)
                    m_candidates[4-i]->hide();
            }
        }
    }, {EventId::pointer_click});
    m_icons_hsizer.add(expand(m_next_button));
#endif

    m_top_bottom_button.margin(5);
    m_top_bottom_button.on_event([this, y_keyboard_position](Event&)
    {
        if (m_bottom_positionned)
        {
            move(Point(0, 0));
            m_top_bottom_button.image(Image("res:internal_arrow_down"));
        }
        else
        {
            move(Point(0, y_keyboard_position));
            m_top_bottom_button.image(Image("res:internal_arrow_up"));
        }

        m_bottom_positionned = !m_bottom_positionned;
    }, {EventId::pointer_click});
#ifdef HAVE_LIBPINYIN
    m_icons_hsizer.add(expand(m_top_bottom_button));
#else
    m_hsizer.add(expand(m_top_bottom_button));
#endif

    m_close_button.margin(5);
    m_close_button.on_event([this, y_keyboard_position](Event&)
    {
        hide();
        // By default, the virtual keyboard is displayed at the bottom of the screen.
        move(Point(0, y_keyboard_position));
        m_top_bottom_button.image(Image("res:internal_arrow_up"));
        m_bottom_positionned = true;
#ifdef HAVE_LIBPINYIN
        reset_pinyin_input();
#endif
    }, {EventId::pointer_click});
#ifdef HAVE_LIBPINYIN
    m_icons_hsizer.add(expand(m_close_button));
#else
    m_hsizer.add(expand(m_close_button));
#endif

    keyboard->align(AlignFlag::expand);
    m_vsizer.add(keyboard);

    the_popup_virtual_keyboard = this;

#ifdef HAVE_LIBPINYIN
    detail::vkbd_register_event(this);

    /* Construct libpinyin engine */
    m_pinyin_context = pinyin_init(LIBPINYIN_DATA_PATH, "");
    if (m_pinyin_context == NULL)
    {
        detail::error("Context not allocted for pinyin...\n");
        return;
    }

    m_pinyin_instance = pinyin_alloc_instance(m_pinyin_context);
    if (m_pinyin_instance == NULL)
        detail::error("Pinyin instance not allocted...\n");
#endif
}

PopupVirtualKeyboard*& popup_virtual_keyboard()
{
    return the_popup_virtual_keyboard;
}

#ifdef HAVE_LIBPINYIN
PopupVirtualKeyboard::~PopupVirtualKeyboard() noexcept
{
    pinyin_free_instance(m_pinyin_instance);
    pinyin_fini(m_pinyin_context);
}

void PopupVirtualKeyboard::reset_pinyin_input(void)
{
    if (!m_pinyin_str.empty())
        m_pinyin_str.clear();

    m_pinyin_input->hide();
    m_pinyin_input->text("n");

    for (auto candidate : m_candidates)
    {
        candidate->hide();
        candidate->text("reset");
    }

    m_candidates_list.clear();
}

void PopupVirtualKeyboard::handle(Event& event)
{
    Widget::handle(event);

    switch (event.id())
    {
        case EventId::keyboard_up:
        case EventId::keyboard_down:
        case EventId::keyboard_repeat:
            handle_pinyin_input(event);
            event.stop();
        default:
            break;
    }
}

void PopupVirtualKeyboard::handle_pinyin_input(Event& event)
{
    if (!m_keyboard->get_pinyin_input())
        return;

    switch (event.key().keycode)
    {
        case EKEY_PINYIN_BACKSPACE:
        {
            if (!m_pinyin_str.empty())
                m_pinyin_str.pop_back();
            else
            {
                Event event1(EventId::keyboard_down);
                event1.key().unicode = event.key().unicode;
                event1.key().keycode = EKEY_BACKSPACE;

                Event event2(EventId::keyboard_up);
                event2.key().unicode = event.key().unicode;
                event2.key().keycode = EKEY_BACKSPACE;

                if (Application::check_instance())
                {
                    asio::post(Application::instance().event().io(), [this, event1, event2]()
                    {
                        m_keyboard->dispatch(event1);
                        m_keyboard->dispatch(event2);
                    });
                }
                return;
            }

            break;
        }

        case EKEY_PINYIN_INPUT:
        {
            m_pinyin_str.push_back(event.key().unicode);
            break;
        }

        default:
            break;
    }

    if (m_pinyin_str.empty())
    {
        m_pinyin_input->hide();
        return;
    }
    else
    {
        m_pinyin_input->text(m_pinyin_str);
        m_pinyin_input->show();
    }

    pinyin_parse_more_full_pinyins (m_pinyin_instance, m_pinyin_str.c_str());
    pinyin_guess_sentence (m_pinyin_instance);
    guint number;
    pinyin_guess_candidates(m_pinyin_instance, 0, {});
    pinyin_get_n_candidate (m_pinyin_instance, &number);
    detail::info("Pinyin {} has {} candidates\n", m_pinyin_str.c_str(), number);

    if (!number)
        return;

    int32_t i;
    lookup_candidate_t * candidate;
    const gchar *display_string;
    m_candidates_list.clear();
    for (i = 0; i < std::min(MAX_CANDIDATES_NUM, static_cast<int>(number)); i++)
    {
        pinyin_get_candidate(m_pinyin_instance, i, &candidate);
        pinyin_get_candidate_string(m_pinyin_instance, candidate, &display_string);
        std::string cpp_string(display_string);
        m_candidates_list.emplace_back(cpp_string);
    }

    for (i = 0; i < std::min(5, static_cast<int>(m_candidates_list.size())); i++)
    {
        m_candidates[i]->text(m_candidates_list[i]);
        m_candidates[i]->show();
    }
    m_candidates_cur_idx = i;
}
#endif

VirtualKeyboard::PanelKeys& multichoice_e()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0117),
            std::make_shared<VirtualKeyboard::Key>(0x0119),
            std::make_shared<VirtualKeyboard::Key>(0x011b),
            std::make_shared<VirtualKeyboard::Key>(0x0115),
            std::make_shared<VirtualKeyboard::Key>(0x04d9),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00e8),
            std::make_shared<VirtualKeyboard::Key>(0x00e9),
            std::make_shared<VirtualKeyboard::Key>(0x00ea),
            std::make_shared<VirtualKeyboard::Key>(0x00eb),
            std::make_shared<VirtualKeyboard::Key>(0x0113),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_E()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0116),
            std::make_shared<VirtualKeyboard::Key>(0x0118),
            std::make_shared<VirtualKeyboard::Key>(0x011a),
            std::make_shared<VirtualKeyboard::Key>(0x0115),
            std::make_shared<VirtualKeyboard::Key>(0x04d8),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00c8),
            std::make_shared<VirtualKeyboard::Key>(0x00c9),
            std::make_shared<VirtualKeyboard::Key>(0x00ca),
            std::make_shared<VirtualKeyboard::Key>(0x00cb),
            std::make_shared<VirtualKeyboard::Key>(0x0112),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_r()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0155),
            std::make_shared<VirtualKeyboard::Key>(0x0159),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_R()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0154),
            std::make_shared<VirtualKeyboard::Key>(0x0158),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_t()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0fe),
            std::make_shared<VirtualKeyboard::Key>(0x0165),
            std::make_shared<VirtualKeyboard::Key>(0x021b),
            std::make_shared<VirtualKeyboard::Key>(0x0163),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_T()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0fe),
            std::make_shared<VirtualKeyboard::Key>(0x0164),
            std::make_shared<VirtualKeyboard::Key>(0x021a),
            std::make_shared<VirtualKeyboard::Key>(0x0162),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_y()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0fd),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_Y()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0dd),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_u()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0173),
            std::make_shared<VirtualKeyboard::Key>(0x0171),
            std::make_shared<VirtualKeyboard::Key>(0x016f),
            std::make_shared<VirtualKeyboard::Key>(0x016b),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00fc),
            std::make_shared<VirtualKeyboard::Key>(0x00fb),
            std::make_shared<VirtualKeyboard::Key>(0x00fa),
            std::make_shared<VirtualKeyboard::Key>(0x00f9),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_U()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0172),
            std::make_shared<VirtualKeyboard::Key>(0x0170),
            std::make_shared<VirtualKeyboard::Key>(0x016e),
            std::make_shared<VirtualKeyboard::Key>(0x016a),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00dc),
            std::make_shared<VirtualKeyboard::Key>(0x00db),
            std::make_shared<VirtualKeyboard::Key>(0x00da),
            std::make_shared<VirtualKeyboard::Key>(0x00d9),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_i()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0131),
            std::make_shared<VirtualKeyboard::Key>(0x012e),
            std::make_shared<VirtualKeyboard::Key>(0x012b),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00ef),
            std::make_shared<VirtualKeyboard::Key>(0x00ee),
            std::make_shared<VirtualKeyboard::Key>(0x00ed),
            std::make_shared<VirtualKeyboard::Key>(0x00ec),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_I()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0130),
            std::make_shared<VirtualKeyboard::Key>(0x012d),
            std::make_shared<VirtualKeyboard::Key>(0x012a),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00cf),
            std::make_shared<VirtualKeyboard::Key>(0x00ce),
            std::make_shared<VirtualKeyboard::Key>(0x00cd),
            std::make_shared<VirtualKeyboard::Key>(0x00cc),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_o()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0153),
            std::make_shared<VirtualKeyboard::Key>(0x0151),
            std::make_shared<VirtualKeyboard::Key>(0x00f8),
            std::make_shared<VirtualKeyboard::Key>(0x00f6),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00f5),
            std::make_shared<VirtualKeyboard::Key>(0x00f4),
            std::make_shared<VirtualKeyboard::Key>(0x00f3),
            std::make_shared<VirtualKeyboard::Key>(0x00f2),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_O()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0152),
            std::make_shared<VirtualKeyboard::Key>(0x0150),
            std::make_shared<VirtualKeyboard::Key>(0x00d8),
            std::make_shared<VirtualKeyboard::Key>(0x00d6),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00d5),
            std::make_shared<VirtualKeyboard::Key>(0x00d4),
            std::make_shared<VirtualKeyboard::Key>(0x00d3),
            std::make_shared<VirtualKeyboard::Key>(0x00d2),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_a()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x00e5),
            std::make_shared<VirtualKeyboard::Key>(0x00e6),
            std::make_shared<VirtualKeyboard::Key>(0x0101),
            std::make_shared<VirtualKeyboard::Key>(0x0103),
            std::make_shared<VirtualKeyboard::Key>(0x0105),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00e0),
            std::make_shared<VirtualKeyboard::Key>(0x00e1),
            std::make_shared<VirtualKeyboard::Key>(0x00e2),
            std::make_shared<VirtualKeyboard::Key>(0x00e3),
            std::make_shared<VirtualKeyboard::Key>(0x00e4),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_A()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x00c5),
            std::make_shared<VirtualKeyboard::Key>(0x00c6),
            std::make_shared<VirtualKeyboard::Key>(0x0100),
            std::make_shared<VirtualKeyboard::Key>(0x0102),
            std::make_shared<VirtualKeyboard::Key>(0x0104),
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x00c0),
            std::make_shared<VirtualKeyboard::Key>(0x00c1),
            std::make_shared<VirtualKeyboard::Key>(0x00c2),
            std::make_shared<VirtualKeyboard::Key>(0x00c3),
            std::make_shared<VirtualKeyboard::Key>(0x00c4),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_s()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x00df),
            std::make_shared<VirtualKeyboard::Key>(0x00a7),
            std::make_shared<VirtualKeyboard::Key>(0x015b),
            std::make_shared<VirtualKeyboard::Key>(0x0161),
            std::make_shared<VirtualKeyboard::Key>(0x015f),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_S()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x00df),
            std::make_shared<VirtualKeyboard::Key>(0x00a7),
            std::make_shared<VirtualKeyboard::Key>(0x015a),
            std::make_shared<VirtualKeyboard::Key>(0x0160),
            std::make_shared<VirtualKeyboard::Key>(0x015e),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_d()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x010f),
            std::make_shared<VirtualKeyboard::Key>(0x0111),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_D()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x010e),
            std::make_shared<VirtualKeyboard::Key>(0x0110),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_g()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0123),
            std::make_shared<VirtualKeyboard::Key>(0x011f),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_G()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0122),
            std::make_shared<VirtualKeyboard::Key>(0x011e),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_k()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0137),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_K()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0136),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_l()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0142),
            std::make_shared<VirtualKeyboard::Key>(0x013e),
            std::make_shared<VirtualKeyboard::Key>(0x013c),
            std::make_shared<VirtualKeyboard::Key>(0x013a),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_L()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0141),
            std::make_shared<VirtualKeyboard::Key>(0x013d),
            std::make_shared<VirtualKeyboard::Key>(0x013b),
            std::make_shared<VirtualKeyboard::Key>(0x0139),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_z()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x017a),
            std::make_shared<VirtualKeyboard::Key>(0x017c),
            std::make_shared<VirtualKeyboard::Key>(0x017e),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_Z()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0179),
            std::make_shared<VirtualKeyboard::Key>(0x017b),
            std::make_shared<VirtualKeyboard::Key>(0x017d),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_c()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x00e7),
            std::make_shared<VirtualKeyboard::Key>(0x0107),
            std::make_shared<VirtualKeyboard::Key>(0x010d),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_C()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x00c7),
            std::make_shared<VirtualKeyboard::Key>(0x0106),
            std::make_shared<VirtualKeyboard::Key>(0x010c),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_n()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0148),
            std::make_shared<VirtualKeyboard::Key>(0x0146),
            std::make_shared<VirtualKeyboard::Key>(0x0144),
            std::make_shared<VirtualKeyboard::Key>(0x00f1),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& multichoice_N()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0147),
            std::make_shared<VirtualKeyboard::Key>(0x0145),
            std::make_shared<VirtualKeyboard::Key>(0x0143),
            std::make_shared<VirtualKeyboard::Key>(0x00d1),
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& QwertyLettersLowerCase()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0031),
            std::make_shared<VirtualKeyboard::Key>(0x0032),
            std::make_shared<VirtualKeyboard::Key>(0x0033),
            std::make_shared<VirtualKeyboard::Key>(0x0034),
            std::make_shared<VirtualKeyboard::Key>(0x0035),
            std::make_shared<VirtualKeyboard::Key>(0x0036),
            std::make_shared<VirtualKeyboard::Key>(0x0037),
            std::make_shared<VirtualKeyboard::Key>(0x0038),
            std::make_shared<VirtualKeyboard::Key>(0x0039),
            std::make_shared<VirtualKeyboard::Key>(0x0030)
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x0071),
            std::make_shared<VirtualKeyboard::Key>(0x0077),
            std::make_shared<VirtualKeyboard::Key>(0x0065, multichoice_e()),
            std::make_shared<VirtualKeyboard::Key>(0x0072, multichoice_r()),
            std::make_shared<VirtualKeyboard::Key>(0x0074, multichoice_t()),
            std::make_shared<VirtualKeyboard::Key>(0x0079, multichoice_y()),
            std::make_shared<VirtualKeyboard::Key>(0x0075, multichoice_u()),
            std::make_shared<VirtualKeyboard::Key>(0x0069, multichoice_i()),
            std::make_shared<VirtualKeyboard::Key>(0x006f, multichoice_o()),
            std::make_shared<VirtualKeyboard::Key>(0x0070)
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x0061, multichoice_a()),
            std::make_shared<VirtualKeyboard::Key>(0x0073, multichoice_s()),
            std::make_shared<VirtualKeyboard::Key>(0x0064, multichoice_d()),
            std::make_shared<VirtualKeyboard::Key>(0x0066),
            std::make_shared<VirtualKeyboard::Key>(0x0067, multichoice_g()),
            std::make_shared<VirtualKeyboard::Key>(0x0068),
            std::make_shared<VirtualKeyboard::Key>(0x006a),
            std::make_shared<VirtualKeyboard::Key>(0x006b, multichoice_k()),
            std::make_shared<VirtualKeyboard::Key>(0x006c, multichoice_l())
        }, {
            std::make_shared<VirtualKeyboard::Key>("\ua71b", 1, 1.5),
            std::make_shared<VirtualKeyboard::Key>(0x007a, multichoice_z()),
            std::make_shared<VirtualKeyboard::Key>(0x0078),
            std::make_shared<VirtualKeyboard::Key>(0x0063, multichoice_c()),
            std::make_shared<VirtualKeyboard::Key>(0x0076),
            std::make_shared<VirtualKeyboard::Key>(0x0062),
            std::make_shared<VirtualKeyboard::Key>(0x006e, multichoice_n()),
            std::make_shared<VirtualKeyboard::Key>(0x006d),
            std::make_shared<VirtualKeyboard::Key>("\u2190", EKEY_BACKSPACE, 1.5)
        }, {
            std::make_shared<VirtualKeyboard::Key>("!#\u263a", 2, 1.5),
#ifdef HAVE_LIBPINYIN
            std::make_shared<VirtualKeyboard::Key>("Eng", EKEY_CN_EN),
#else
            std::make_shared<VirtualKeyboard::Key>(Image("res:internal_microphone"), EKEY_UNKNOWN),
#endif
            std::make_shared<VirtualKeyboard::Key>(0x0020, 5.0),
            std::make_shared<VirtualKeyboard::Key>(0x002e),
            std::make_shared<VirtualKeyboard::Key>("Enter", EKEY_ENTER, 1.5)
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& QwertyLettersUpperCase()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0031),
            std::make_shared<VirtualKeyboard::Key>(0x0032),
            std::make_shared<VirtualKeyboard::Key>(0x0033),
            std::make_shared<VirtualKeyboard::Key>(0x0034),
            std::make_shared<VirtualKeyboard::Key>(0x0035),
            std::make_shared<VirtualKeyboard::Key>(0x0036),
            std::make_shared<VirtualKeyboard::Key>(0x0037),
            std::make_shared<VirtualKeyboard::Key>(0x0038),
            std::make_shared<VirtualKeyboard::Key>(0x0039),
            std::make_shared<VirtualKeyboard::Key>(0x0030)
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x0051),
            std::make_shared<VirtualKeyboard::Key>(0x0057),
            std::make_shared<VirtualKeyboard::Key>(0x0045, multichoice_E()),
            std::make_shared<VirtualKeyboard::Key>(0x0052, multichoice_R()),
            std::make_shared<VirtualKeyboard::Key>(0x0054, multichoice_T()),
            std::make_shared<VirtualKeyboard::Key>(0x0059, multichoice_Y()),
            std::make_shared<VirtualKeyboard::Key>(0x0055, multichoice_U()),
            std::make_shared<VirtualKeyboard::Key>(0x0049, multichoice_I()),
            std::make_shared<VirtualKeyboard::Key>(0x004f, multichoice_O()),
            std::make_shared<VirtualKeyboard::Key>(0x0050)
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x0041, multichoice_A()),
            std::make_shared<VirtualKeyboard::Key>(0x0053, multichoice_S()),
            std::make_shared<VirtualKeyboard::Key>(0x0044, multichoice_D()),
            std::make_shared<VirtualKeyboard::Key>(0x0046),
            std::make_shared<VirtualKeyboard::Key>(0x0047, multichoice_G()),
            std::make_shared<VirtualKeyboard::Key>(0x0048),
            std::make_shared<VirtualKeyboard::Key>(0x004a),
            std::make_shared<VirtualKeyboard::Key>(0x004b, multichoice_K()),
            std::make_shared<VirtualKeyboard::Key>(0x004c, multichoice_L())
        }, {
            std::make_shared<VirtualKeyboard::Key>("\ua71b", 0, 1.5),
            std::make_shared<VirtualKeyboard::Key>(0x005a, multichoice_Z()),
            std::make_shared<VirtualKeyboard::Key>(0x0058),
            std::make_shared<VirtualKeyboard::Key>(0x0043, multichoice_C()),
            std::make_shared<VirtualKeyboard::Key>(0x0056),
            std::make_shared<VirtualKeyboard::Key>(0x0042),
            std::make_shared<VirtualKeyboard::Key>(0x004e, multichoice_N()),
            std::make_shared<VirtualKeyboard::Key>(0x004d),
            std::make_shared<VirtualKeyboard::Key>("\u2190", EKEY_BACKSPACE, 1.5)
        }, {
            std::make_shared<VirtualKeyboard::Key>("!#\u263a", 2, 1.5),
#ifdef HAVE_LIBPINYIN
            std::make_shared<VirtualKeyboard::Key>("Eng", EKEY_CN_EN),
#else
            std::make_shared<VirtualKeyboard::Key>(Image("res:internal_microphone"), EKEY_UNKNOWN),
#endif
            std::make_shared<VirtualKeyboard::Key>(0x0020, 5.0),
            std::make_shared<VirtualKeyboard::Key>(0x002e),
            std::make_shared<VirtualKeyboard::Key>("Enter", EKEY_ENTER, 1.5)
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& QwertySymbols1()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0031),
            std::make_shared<VirtualKeyboard::Key>(0x0032),
            std::make_shared<VirtualKeyboard::Key>(0x0033),
            std::make_shared<VirtualKeyboard::Key>(0x0034),
            std::make_shared<VirtualKeyboard::Key>(0x0035),
            std::make_shared<VirtualKeyboard::Key>(0x0036),
            std::make_shared<VirtualKeyboard::Key>(0x0037),
            std::make_shared<VirtualKeyboard::Key>(0x0038),
            std::make_shared<VirtualKeyboard::Key>(0x0039),
            std::make_shared<VirtualKeyboard::Key>(0x0030)
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x002b),
            std::make_shared<VirtualKeyboard::Key>(0x0078),
            std::make_shared<VirtualKeyboard::Key>(0x00f7),
            std::make_shared<VirtualKeyboard::Key>(0x003d),
            std::make_shared<VirtualKeyboard::Key>(0x002f),
            std::make_shared<VirtualKeyboard::Key>(0x005f),
            std::make_shared<VirtualKeyboard::Key>(0x20ac),
            std::make_shared<VirtualKeyboard::Key>(0x00a3),
            std::make_shared<VirtualKeyboard::Key>(0x00a5),
            std::make_shared<VirtualKeyboard::Key>(0x20a9)
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x0021),
            std::make_shared<VirtualKeyboard::Key>(0x0040),
            std::make_shared<VirtualKeyboard::Key>(0x0023),
            std::make_shared<VirtualKeyboard::Key>(0x0024),
            std::make_shared<VirtualKeyboard::Key>(0x0025),
            std::make_shared<VirtualKeyboard::Key>(0x005e),
            std::make_shared<VirtualKeyboard::Key>(0x0026),
            std::make_shared<VirtualKeyboard::Key>(0x002a),
            std::make_shared<VirtualKeyboard::Key>(0x0028),
            std::make_shared<VirtualKeyboard::Key>(0x0029)
        }, {
            std::make_shared<VirtualKeyboard::Key>("1/2", 3, 1.5),
            std::make_shared<VirtualKeyboard::Key>(0x002d),
            std::make_shared<VirtualKeyboard::Key>(0x0027),
            std::make_shared<VirtualKeyboard::Key>(0x0022),
            std::make_shared<VirtualKeyboard::Key>(0x003a),
            std::make_shared<VirtualKeyboard::Key>(0x003b),
            std::make_shared<VirtualKeyboard::Key>(0x002c),
            std::make_shared<VirtualKeyboard::Key>(0x003f),
            std::make_shared<VirtualKeyboard::Key>("\u2190", EKEY_BACKSPACE, 1.5)
        }, {
            std::make_shared<VirtualKeyboard::Key>("ABC", 0, 1.5),
#ifdef HAVE_LIBPINYIN
            std::make_shared<VirtualKeyboard::Key>("Eng", EKEY_CN_EN),
#else
            std::make_shared<VirtualKeyboard::Key>(Image("res:internal_microphone"), EKEY_UNKNOWN),
#endif
            std::make_shared<VirtualKeyboard::Key>(0x0020, 5.0),
            std::make_shared<VirtualKeyboard::Key>(0x002e),
            std::make_shared<VirtualKeyboard::Key>("Enter", EKEY_ENTER, 1.5)
        }
    };
    return keys;
}

VirtualKeyboard::PanelKeys& QwertySymbols2()
{
    static VirtualKeyboard::PanelKeys keys =
    {
        {
            std::make_shared<VirtualKeyboard::Key>(0x0031),
            std::make_shared<VirtualKeyboard::Key>(0x0032),
            std::make_shared<VirtualKeyboard::Key>(0x0033),
            std::make_shared<VirtualKeyboard::Key>(0x0034),
            std::make_shared<VirtualKeyboard::Key>(0x0035),
            std::make_shared<VirtualKeyboard::Key>(0x0036),
            std::make_shared<VirtualKeyboard::Key>(0x0037),
            std::make_shared<VirtualKeyboard::Key>(0x0038),
            std::make_shared<VirtualKeyboard::Key>(0x0039),
            std::make_shared<VirtualKeyboard::Key>(0x0030)
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x0060),
            std::make_shared<VirtualKeyboard::Key>(0x007e),
            std::make_shared<VirtualKeyboard::Key>(0x005c),
            std::make_shared<VirtualKeyboard::Key>(0x007c),
            std::make_shared<VirtualKeyboard::Key>(0x003c),
            std::make_shared<VirtualKeyboard::Key>(0x003e),
            std::make_shared<VirtualKeyboard::Key>(0x007b),
            std::make_shared<VirtualKeyboard::Key>(0x007d),
            std::make_shared<VirtualKeyboard::Key>(0x005b),
            std::make_shared<VirtualKeyboard::Key>(0x005d)
        }, {
            std::make_shared<VirtualKeyboard::Key>(0x25c4),
            std::make_shared<VirtualKeyboard::Key>(0x25b2),
            std::make_shared<VirtualKeyboard::Key>(0x25bc),
            std::make_shared<VirtualKeyboard::Key>(0x25ba),
            std::make_shared<VirtualKeyboard::Key>(0x25a0),
            std::make_shared<VirtualKeyboard::Key>(0x25a1),
            std::make_shared<VirtualKeyboard::Key>(0x25cf),
            std::make_shared<VirtualKeyboard::Key>(0x25cb),
            std::make_shared<VirtualKeyboard::Key>(0x25cc),
            std::make_shared<VirtualKeyboard::Key>(0x25ca)
        }, {
            std::make_shared<VirtualKeyboard::Key>("2/2", 2, 1.5),
            std::make_shared<VirtualKeyboard::Key>(0x263a),
            std::make_shared<VirtualKeyboard::Key>(0x263b),
            std::make_shared<VirtualKeyboard::Key>(0x263c),
            std::make_shared<VirtualKeyboard::Key>(0x00ab),
            std::make_shared<VirtualKeyboard::Key>(0x00bb),
            std::make_shared<VirtualKeyboard::Key>(0x00a1),
            std::make_shared<VirtualKeyboard::Key>(0x00bf),
            std::make_shared<VirtualKeyboard::Key>("\u2190", EKEY_BACKSPACE, 1.5)
        }, {
            std::make_shared<VirtualKeyboard::Key>("ABC", 0, 1.5),
#ifdef HAVE_LIBPINYIN
            std::make_shared<VirtualKeyboard::Key>("Eng", EKEY_CN_EN),
#else
            std::make_shared<VirtualKeyboard::Key>(Image("res:internal_microphone"), EKEY_UNKNOWN),
#endif
            std::make_shared<VirtualKeyboard::Key>(0x0020, 5.0),
            std::make_shared<VirtualKeyboard::Key>(0x002e),
            std::make_shared<VirtualKeyboard::Key>("Enter", EKEY_ENTER, 1.5)
        }
    };
    return keys;
}

}
}
