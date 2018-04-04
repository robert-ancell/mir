/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_BASIC_EVENT_SINK_H_
#define MIR_FRONTEND_BASIC_EVENT_SINK_H_

#include "null_event_sink.h"

#include "mir/executor.h"
#include "mir_toolkit/event.h"

#include <wayland-server-core.h>

#include <atomic>
#include <functional>
#include <memory>

namespace mir
{
namespace frontend
{
class WlSurface;
class WlSeat;
class WlAbstractMirWindow;

class BasicSurfaceEventSink : public NullEventSink
{
public:
    BasicSurfaceEventSink(WlSeat* seat, wl_client* client, wl_resource* target, WlAbstractMirWindow* window);
    ~BasicSurfaceEventSink();

    void handle_event(EventUPtr&& event) override;

    void latest_client_size(geometry::Size window_size)
    {
        this->window_size = window_size;
    }

    auto latest_timestamp_ns() const -> uint64_t
    {
        return timestamp_ns;
    }

    auto is_active() const -> bool
    {
        return has_focus;
    }

    auto state() const -> MirWindowState
    {
        return current_state;
    }

protected:
    WlSeat* const seat;
    wl_client* const client;
    WlSurface* const surface;
    WlAbstractMirWindow* window;
    std::atomic<geometry::Size> window_size;
    std::atomic<int64_t> timestamp_ns{0};
    std::atomic<geometry::Size> requested_size;
    std::atomic<bool> has_focus{false};
    std::atomic<MirWindowState> current_state{mir_window_state_unknown};
    std::shared_ptr<bool> const destroyed;

private:
    void handle_resize_event(MirResizeEvent const* event);
    void handle_input_event(MirInputEvent const* event);
    void handle_keymap_event(MirKeymapEvent const* event);
    void handle_window_event(MirWindowEvent const* event);

};
}
}

#endif //MIR_FRONTEND_BASIC_EVENT_SINK_H_
