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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display.h"
#include "mir/graphics/renderer.h"
#include "mir/display_server.h"

#include "display_server_test_environment.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
struct MockDisplay : mg::Display
{
public:
    MOCK_METHOD0(view_area, geom::Rectangle ());
    MOCK_METHOD0(notify_update, void());
};

}

TEST_F(DisplayServerTestEnvironment, notify_sync_and_see_paint)
{
    using namespace testing;

    MockDisplay display;

    in_server_process([&]() -> void
    {
        EXPECT_CALL(display, notify_update()).Times(1);

        EXPECT_CALL(display, view_area()).Times(AtLeast(1))
                .WillRepeatedly(Return(geom::Rectangle()));

        display_server()->render(&display);
    });
}

TEST_F(DisplayServerTestEnvironment, failing_server_side_test)
{
    MockDisplay display;

    in_server_process([&]() -> void
    {
        using namespace testing;
        FAIL() << "Proving a test can fail";
    });
}

