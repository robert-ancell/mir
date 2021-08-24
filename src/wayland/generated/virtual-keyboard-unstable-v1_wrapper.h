/*
 * AUTOGENERATED - DO NOT EDIT
 *
 * This file is generated from virtual-keyboard-unstable-v1.xml
 * To regenerate, run the “refresh-wayland-wrapper” target.
 */

#ifndef MIR_FRONTEND_WAYLAND_VIRTUAL_KEYBOARD_UNSTABLE_V1_XML_WRAPPER
#define MIR_FRONTEND_WAYLAND_VIRTUAL_KEYBOARD_UNSTABLE_V1_XML_WRAPPER

#include <optional>

#include "mir/fd.h"
#include <wayland-server-core.h>

#include "mir/wayland/wayland_base.h"

namespace mir
{
namespace wayland
{

class VirtualKeyboardV1;
class VirtualKeyboardManagerV1;

class VirtualKeyboardV1 : public Resource
{
public:
    static char const constexpr* interface_name = "zwp_virtual_keyboard_v1";

    static VirtualKeyboardV1* from(struct wl_resource*);

    VirtualKeyboardV1(struct wl_resource* resource, Version<1>);
    virtual ~VirtualKeyboardV1();

    struct wl_client* const client;
    struct wl_resource* const resource;

    struct Error
    {
        static uint32_t const no_keymap = 0;
    };

    struct Thunks;

    static bool is_instance(wl_resource* resource);

private:
    virtual void keymap(uint32_t format, mir::Fd fd, uint32_t size) = 0;
    virtual void key(uint32_t time, uint32_t key, uint32_t state) = 0;
    virtual void modifiers(uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) = 0;
};

class VirtualKeyboardManagerV1 : public Resource
{
public:
    static char const constexpr* interface_name = "zwp_virtual_keyboard_manager_v1";

    static VirtualKeyboardManagerV1* from(struct wl_resource*);

    VirtualKeyboardManagerV1(struct wl_resource* resource, Version<1>);
    virtual ~VirtualKeyboardManagerV1();

    void destroy_and_delete() const;

    struct wl_client* const client;
    struct wl_resource* const resource;

    struct Error
    {
        static uint32_t const unauthorized = 0;
    };

    struct Thunks;

    static bool is_instance(wl_resource* resource);

    class Global : public wayland::Global
    {
    public:
        Global(wl_display* display, Version<1>);

        auto interface_name() const -> char const* override;

    private:
        virtual void bind(wl_resource* new_zwp_virtual_keyboard_manager_v1) = 0;
        friend VirtualKeyboardManagerV1::Thunks;
    };

private:
    virtual void create_virtual_keyboard(struct wl_resource* seat, struct wl_resource* id) = 0;
};

}
}

#endif // MIR_FRONTEND_WAYLAND_VIRTUAL_KEYBOARD_UNSTABLE_V1_XML_WRAPPER