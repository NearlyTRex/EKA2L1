/*
 * Copyright (c) 2019 EKA2L1 Team
 * 
 * This file is part of EKA2L1 project.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <epoc/loader/mbm.h>
#include <epoc/services/window/common.h>
#include <epoc/utils/uid.h>
#include <epoc/ptr.h>

namespace eka2l1::epoc {
    struct bitwise_bitmap {
        enum settings_flag {
            dirty_bitmap = 0x00010000,
            violate_bitmap = 0x00020000
        };

        uid                uid_;

        struct settings {
            // The first 8 bits are reserved for initial display mode
            // The next 8 bits are reserved for current display mode
            // 16 bits left are for flags
            std::uint32_t flags_ { 0 };

            display_mode initial_display_mode() const;
            display_mode current_display_mode() const;

            void current_display_mode(const display_mode &mode);
            void initial_display_mode(const display_mode &mode);

            bool dirty_bitmap() const;
            void dirty_bitmap(const bool is_it);

            bool violate_bitmap() const;
            void violate_bitmap(const bool is_it);
        } settings_;

        eka2l1::ptr<void>  allocator_;
        eka2l1::ptr<void>  pile_;
        int                byte_width_;
        loader::sbm_header header_;
        int                spare1_;
        int                data_offset_;
        bool               compressed_in_ram_;
    };

    constexpr epoc::uid bitwise_bitmap_uid = 0x10000040;
}
