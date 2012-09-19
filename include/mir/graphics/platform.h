/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Thomas Guest  <thomas.guest@canonical.com>
 */

#ifndef MIR_GRAPHICS_PLATFORM_H_
#define MIR_GRAPHICS_PLATFORM_H_

#include <memory>

// Interface to platform specific support for graphics operations.

namespace mir
{
namespace compositor
{
class GraphicBufferAllocator;
}

namespace graphics
{

class Display;

class Platform
{
public:
    Platform() = default;
    Platform(const Platform& p) = delete;
    Platform& operator=(const Platform& p) = delete;
};

// Create and return a new graphics platform.
std::shared_ptr<Platform> create_platform();

// Create and return a new graphics buffer allocator.
std::shared_ptr<compositor::GraphicBufferAllocator> create_buffer_allocator(
        const std::shared_ptr<Platform>& platform);

//Create and return a new graphics display
std::shared_ptr<Display> create_display(
        const std::shared_ptr<Platform>& platform);
}
}

#endif // MIR_GRAPHICS_PLATFORM_H_
