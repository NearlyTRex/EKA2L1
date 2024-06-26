/*
 * Copyright (c) 2021 EKA2L1 Team
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

#include <services/msv/operations/change.h>

#include <services/msv/common.h>
#include <services/msv/entry.h>
#include <services/msv/msv.h>

#include <common/chunkyseri.h>
#include <utils/err.h>

namespace eka2l1::epoc::msv {
    change_operation::change_operation(const msv_id operation_id, const operation_buffer &buffer,
        epoc::notify_info complete_info)
        : operation(operation_id, buffer, complete_info) {
    }

    void change_operation::execute(msv_server *server, const kernel::uid process_uid) {
        state(operation_state_pending);
        entry target_entry;

        common::chunkyseri seri(buffer_.data(), buffer_.size(), common::SERI_MODE_READ);
        server->absorb_entry_to_buffer(seri, target_entry);

        const bool result = server->indexer()->change_entry(target_entry);

        local_operation_progress *progress = progress_data<local_operation_progress>();

        progress->operation_ = epoc::msv::local_operation_changed;
        progress->number_of_entries_ = 1;

        if (!result) {
            LOG_ERROR(SERVICE_MSV, "Error changing entry!");

            progress->number_failed_++;
            progress->number_remaining_ = 1;
            progress->error_ = epoc::error_general;
        } else {
            progress->mid_ = target_entry.id_;
            progress->number_completed_++;

            // Queue entry changed event
            msv_event_data created;
            created.nof_ = epoc::msv::change_notification_type_entries_changed;
            created.ids_.push_back(target_entry.id_);
            created.arg1_ = target_entry.parent_id_;

            server->queue(created);
        }

        state(operation_state_success);
        complete_info_.complete(epoc::error_none);
    }

    std::int32_t change_operation::system_progress(system_progress_info &progress) {
        local_operation_progress *localprg = progress_data<local_operation_progress>();

        progress.err_code_ = localprg->error_;
        progress.id_ = localprg->mid_;

        return epoc::error_none;
    }
}