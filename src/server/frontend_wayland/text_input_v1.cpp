/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "text_input_v1.h"

#include "wl_seat.h"
#include "wl_surface.h"
#include "mir/executor.h"
#include "mir/scene/text_input_hub.h"

#include "mir/log.h" // TODO - remove after debugging

#include <memory>
#include <boost/throw_exception.hpp>
#include <deque>
#include <utility>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;

auto wayland_to_mir_content_hint(uint32_t hint) -> ms::TextInputContentHint
{
#define WAYLAND_TO_MIR_HINT(wl_name, mir_name) \
    (hint & mw::TextInputV1::ContentHint::wl_name ? \
        ms::TextInputContentHint::mir_name : \
        ms::TextInputContentHint::none)

    return
        WAYLAND_TO_MIR_HINT(auto_completion, completion) |
        WAYLAND_TO_MIR_HINT(auto_correction, spellcheck) |
        WAYLAND_TO_MIR_HINT(auto_capitalization, auto_capitalization) |
        WAYLAND_TO_MIR_HINT(lowercase, lowercase) |
        WAYLAND_TO_MIR_HINT(uppercase, uppercase) |
        WAYLAND_TO_MIR_HINT(titlecase, titlecase) |
        WAYLAND_TO_MIR_HINT(hidden_text, hidden_text) |
        WAYLAND_TO_MIR_HINT(sensitive_data, sensitive_data) |
        WAYLAND_TO_MIR_HINT(latin, latin) |
        WAYLAND_TO_MIR_HINT(multiline, multiline);

#undef WAYLAND_TO_MIR_HINT
}

auto wayland_to_mir_content_purpose(uint32_t purpose) -> ms::TextInputContentPurpose
{
    switch (purpose)
    {
        case mw::TextInputV1::ContentPurpose::normal:
            return ms::TextInputContentPurpose::normal;

        case mw::TextInputV1::ContentPurpose::alpha:
            return ms::TextInputContentPurpose::alpha;

        case mw::TextInputV1::ContentPurpose::digits:
            return ms::TextInputContentPurpose::digits;

        case mw::TextInputV1::ContentPurpose::number:
            return ms::TextInputContentPurpose::number;

        case mw::TextInputV1::ContentPurpose::phone:
            return ms::TextInputContentPurpose::phone;

        case mw::TextInputV1::ContentPurpose::url:
            return ms::TextInputContentPurpose::url;

        case mw::TextInputV1::ContentPurpose::email:
            return ms::TextInputContentPurpose::email;

        case mw::TextInputV1::ContentPurpose::name:
            return ms::TextInputContentPurpose::name;

        case mw::TextInputV1::ContentPurpose::password:
            return ms::TextInputContentPurpose::password;

        case mw::TextInputV1::ContentPurpose::date:
            return ms::TextInputContentPurpose::date;

        case mw::TextInputV1::ContentPurpose::time:
            return ms::TextInputContentPurpose::time;

        case mw::TextInputV1::ContentPurpose::datetime:
            return ms::TextInputContentPurpose::datetime;

        case mw::TextInputV1::ContentPurpose::terminal:
            return ms::TextInputContentPurpose::terminal;

        default:
            BOOST_THROW_EXCEPTION(std::invalid_argument{"Invalid text content purpose " + std::to_string(purpose)});
    }
}

struct TextInputV1Ctx
{
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<ms::TextInputHub> const text_input_hub;
};

class TextInputManagerV1Global
    : public mw::TextInputManagerV1::Global
{
public:
    TextInputManagerV1Global(wl_display* display, std::shared_ptr<TextInputV1Ctx> ctx);

private:
    void bind(wl_resource* new_resource) override;

    std::shared_ptr<TextInputV1Ctx> const ctx;
};

class TextInputManagerV1
    : public mw::TextInputManagerV1
{
public:
    TextInputManagerV1(wl_resource* resource, std::shared_ptr<TextInputV1Ctx> ctx);

private:
    void create_text_input(wl_resource* id) override;

    std::shared_ptr<TextInputV1Ctx> const ctx;
};

class TextInputV1
    : public mw::TextInputV1,
      private mf::WlSeat::FocusListener
{
public:
    TextInputV1(wl_resource* resource, std::shared_ptr<TextInputV1Ctx> ctx);
    ~TextInputV1();

private:
    struct Handler : ms::TextInputChangeHandler
    {
        Handler(TextInputV1* const text_input, std::shared_ptr<mir::Executor> const wayland_executor)
            : text_input{text_input},
              wayland_executor{wayland_executor}
        {
        }

        void text_changed(ms::TextInputChange const& change) override
        {
            wayland_executor->spawn([text_input=text_input, change]()
                {
                    if (text_input)
                    {
                        text_input.value().send_text_change(change);
                    }
                });
        }

        mw::Weak<TextInputV1> const text_input;
        std::shared_ptr<mir::Executor> const wayland_executor;
    };

    static size_t constexpr max_remembered_serials{10};

    std::shared_ptr<TextInputV1Ctx> const ctx;
    std::optional<mf::WlSeat*> seat;
    std::shared_ptr<Handler> const handler;
    mw::Weak<mf::WlSurface> current_surface;
    /// Set to true if and only if the text input has been enabled since the last commit
    bool on_new_input_field{false};
    /// nullopt if the state is inactive, otherwise holds the pending and/or committed state
    std::optional<ms::TextInputState> pending_state;
    /// The first value is the number of commits we had received when a state was submitted to the text input hub. The
    /// second value is the serial the hub gave us for that state. When we get a change from the input method we match
    /// it's state serial to the corresponding commit count, which is the serial we send to the client. We only remember
    /// a small number of serials.
    std::deque<std::pair<uint32_t, ms::TextInputStateSerial>> state_serials;
    /// The number of commits we've received
    uint32_t commit_count{0};

    /// Sends the text change to the client if possible
    void send_text_change(ms::TextInputChange const& change);

    /// Returns the client serial (aka the commit count) that corresponds to the given state serial
    auto find_client_serial(ms::TextInputStateSerial state_serial) const -> std::optional<uint32_t>;

   /// From WlSeat::FocusListener
   void focus_on(mf::WlSurface* surface) override;

    /// From wayland::TextInputV1
    /// @{
    void activate(wl_resource* seat, wl_resource* surface) override;
    void deactivate(wl_resource* seat) override;
    void show_input_panel() override;
    void hide_input_panel() override;
    void reset() override;
    void set_surrounding_text(std::string const& text, uint32_t cursor, uint32_t anchor) override;
    void set_content_type(uint32_t hint, uint32_t purpose) override;
    void set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height) override;
    void set_preferred_language(std::string const& language) override;
    void commit_state(uint32_t serial) override;
    void invoke_action(uint32_t button, uint32_t index) override;
    /// @}
};

auto mf::create_text_input_manager_v1(
    wl_display *display,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<scene::TextInputHub> text_input_hub)
-> std::shared_ptr<wayland::TextInputManagerV1::Global>
{
    auto ctx = std::make_shared<TextInputV1Ctx>(TextInputV1Ctx{std::move(wayland_executor), std::move(text_input_hub)});
    return std::make_shared<TextInputManagerV1Global>(display, std::move(ctx));
}

TextInputManagerV1Global::TextInputManagerV1Global(
    wl_display* display,
    std::shared_ptr<TextInputV1Ctx> ctx)
    : Global{display, Version<1>()},
      ctx{std::move(ctx)}
{
    mir::log_info("TextInputManagerV1Global created!");
}

void TextInputManagerV1Global::bind(wl_resource* new_resource)
{
    mir::log_info("TextInputManagerV1Global::bind() called!");
    new TextInputManagerV1{new_resource, ctx};
}

TextInputManagerV1::TextInputManagerV1(
    wl_resource *resource,
    std::shared_ptr<TextInputV1Ctx> ctx)
    : mw::TextInputManagerV1{resource, Version<1>()},
      ctx{std::move(ctx)}
{
    mir::log_info("TextInputManagerV1 created!");
}

void TextInputManagerV1::create_text_input(wl_resource *id)
{
    mir::log_info("TextInputManagerV1::create_text_input() called!");
    new TextInputV1{id, ctx};
}

TextInputV1::TextInputV1(
    wl_resource *resource,
    std::shared_ptr<TextInputV1Ctx> ctx)
    : mw::TextInputV1{resource, Version<1>()},
      ctx{std::move(ctx)},
      seat{}
{
    mir::log_info("TextInputV1 created!");
}

TextInputV1::~TextInputV1()
{
    if (seat)
    {
        seat.value()->remove_focus_listener(client, this);
        delete seat.value();
    }

    on_new_input_field = false;
    pending_state.reset();
    delete this->seat.value();
}

void TextInputV1::send_text_change(ms::TextInputChange const& change)
{
    auto const client_serial = find_client_serial(change.serial);
    if (!pending_state || !current_surface || !client_serial)
    {
        // We are no longer enabled, or we don't have a valid serial
        return;
    }
    if (change.preedit_text || change.preedit_cursor_begin || change.preedit_cursor_end)
    {
        send_preedit_cursor_event(change.preedit_cursor_begin.value_or(0));
        // TODO - Is using .value() directly unsafe here?
        send_preedit_string_event(client_serial.value(), change.preedit_text.value_or(""), "");
    }
    if (change.delete_before || change.delete_after)
    {
        send_delete_surrounding_text_event(change.delete_before.value_or(0), change.delete_after.value_or(0));
    }
    if(change.commit_text)
    {
        // TODO - Is using .value() directly unsafe here?
        send_commit_string_event(client_serial.value(), change.commit_text.value());
    }
}

auto TextInputV1::find_client_serial(ms::TextInputStateSerial state_serial) const -> std::optional<uint32_t>
{
    // Loop in reverse order because the serial we're looking for will generally be at the end
    for (auto it = state_serials.rbegin(); it != state_serials.rend(); it++)
    {
        if (it->second == state_serial)
        {
            return it->first;
        }
    }
    return std::nullopt;
}

void TextInputV1::focus_on(mf::WlSurface* surface)
{
    if (current_surface)
    {
        send_leave_event();
    }
    current_surface = mw::make_weak(surface);
    if (surface)
    {
        send_enter_event(surface->resource);
    }
    else
    {
        deactivate(nullptr); // TODO - Is this right?
    }
}

void TextInputV1::activate(wl_resource *seat, wl_resource *surface)
{
    (void)surface;

    auto const wl_seat = mf::WlSeat::from(seat);
    if (!wl_seat)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to resolve WlSeat activating TextInputV1"));
    }
    this->seat = std::optional<mf::WlSeat*>(wl_seat);
    this->seat.value()->add_focus_listener(client, this);

    if (current_surface)
    {
        on_new_input_field = true;
        pending_state.emplace();
    }
}

void TextInputV1::deactivate(wl_resource *seat)
{
    // TODO - test and decide if this is correct (implemented from V2)
    (void)seat;
    delete this;
}

void TextInputV1::show_input_panel()
{
    commit_count++;
    pending_state.emplace(); // TODO - This makes sure that pending_state is ALWAYS true, making the next check redundant. Consult with Sophie.

    if (pending_state && current_surface)
    {
        auto const new_serial = ctx->text_input_hub->set_handler_state(handler, on_new_input_field, *pending_state);
        state_serials.push_back({commit_count, new_serial});
        while (state_serials.size() > max_remembered_serials)
        {
            state_serials.pop_front();
        }
    }
    else
    {
        ctx->text_input_hub->deactivate_handler(handler);
    }
    on_new_input_field = false;
}

void TextInputV1::hide_input_panel()
{
    ctx->text_input_hub->deactivate_handler(handler);
}

void TextInputV1::reset()
{
    // TODO - Is this right? Don't think so!
    pending_state.reset();
}

void TextInputV1::set_surrounding_text(const std::string &text, uint32_t cursor, uint32_t anchor)
{
    if (pending_state)
    {
        pending_state->surrounding_text = text;
        pending_state->cursor = cursor;
        pending_state->anchor = anchor;
    }
}

void TextInputV1::set_content_type(uint32_t hint, uint32_t purpose)
{
    if (pending_state)
    {
        pending_state->content_hint.emplace(wayland_to_mir_content_hint(hint));
        pending_state->content_purpose = wayland_to_mir_content_purpose(purpose);
    }
}

void TextInputV1::set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void TextInputV1::set_preferred_language(const std::string &language)
{
    // Ignored, input methods decide language for themselves
    (void)language;
}

void TextInputV1::commit_state(uint32_t serial)
{
    // TODO
    (void)serial;
}

void TextInputV1::invoke_action(uint32_t button, uint32_t index)
{
    // TODO
    (void)button;
    (void)index;
}
