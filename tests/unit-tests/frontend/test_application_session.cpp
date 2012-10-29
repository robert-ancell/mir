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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#include "mir/compositor/buffer_bundle.h"
#include "mir/frontend/application_session.h"
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

namespace
{

struct MockApplicationSurfaceOrganiser : public ms::ApplicationSurfaceOrganiser
{
    MOCK_METHOD1(create_surface, std::weak_ptr<ms::Surface>(const ms::SurfaceCreationParameters&));
    MOCK_METHOD1(destroy_surface, void(std::weak_ptr<ms::Surface> surface));
};

}

TEST(ApplicationSession, create_and_destroy_surface)
{
    using namespace ::testing;

    std::shared_ptr<mc::BufferBundle> buffer_bundle(
        new mc::MockBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            ms::a_surface().name,
            buffer_bundle));

    MockApplicationSurfaceOrganiser organizer;
    mf::ApplicationSession app_session(&organizer, "Foo");
    ON_CALL(organizer, create_surface(_)).WillByDefault(Return(dummy_surface));
    EXPECT_CALL(organizer, create_surface(_));
    EXPECT_CALL(organizer, destroy_surface(_));

    ms::SurfaceCreationParameters params;
    int surface_id = app_session.create_surface(params);
    assert(surface_id >= 0);

    app_session.destroy_surface(surface_id);
}


TEST(ApplicationSession, surface_ids_increment)
{
    using namespace ::testing;

    std::shared_ptr<mc::BufferBundle> buffer_bundle(
        new mc::MockBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            ms::a_surface().name,
            buffer_bundle));

    MockApplicationSurfaceOrganiser organizer;
    mf::ApplicationSession app_session(&organizer, "Foo");
    ON_CALL(organizer, create_surface(_)).WillByDefault(Return(dummy_surface));
    EXPECT_CALL(organizer, create_surface(_)).Times(2);
    EXPECT_CALL(organizer, destroy_surface(_)).Times(2);

    ms::SurfaceCreationParameters params;
    int surface_id = app_session.create_surface(params);
    assert(surface_id >= 0);
    int surface_id2 = app_session.create_surface(params);
    assert(surface_id2 > surface_id);

    app_session.destroy_surface(surface_id);
    app_session.destroy_surface(surface_id2);
}

TEST(ApplicationSession, hide_hides_surfaces)
{
    using namespace ::testing;

    std::shared_ptr<mc::BufferBundle> buffer_bundle(
        new mc::MockBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            ms::a_surface().name,
            buffer_bundle));

    MockApplicationSurfaceOrganiser organizer;
    mf::ApplicationSession app_session(&organizer, "TurboPascal");
    ON_CALL(organizer, create_surface(_)).WillByDefault(Return(dummy_surface));
    EXPECT_CALL(organizer, create_surface(_));

    ms::SurfaceCreationParameters params;
    app_session.create_surface(params);

    EXPECT_CALL(dummy_surface, hide(_)).Times(1);
    app_session.hide();
    assert(dummy_surface.visible() == false);

}
