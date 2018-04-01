#include <manager/package_manager.h>
#include <loader/sis.h>
#include <common/cvt.h>
#include <common/log.h>
#include <dirent.h>

namespace eka2l1 {
    namespace manager {

        struct sdb_header {
            char magic[4];
            uint32_t app_count;
            uint64_t uid_offset;
            uint64_t name_offset;
            uint64_t drive_offset;
            uint64_t vendor_offset;
        };

        bool package_manager::write_sdb(const std::string& path) {
            FILE* file = fopen(path.c_str(), "wb");

            if (!file) {
                return false;
            }

            int total_app = c_apps.size() + e_apps.size();

            uint64_t uid_offset = 40;
            uint64_t name_offset = 40 + total_app * 4;
            uint64_t drive_offset = name_offset + total_app * 4;

            for (auto& c_app: c_apps) {
                drive_offset += c_app.second.name.size() * 2;
            }

            for (auto& e_app: e_apps) {
                drive_offset += e_app.second.name.size() * 2;
            }

            uint64_t vendor_offset = drive_offset + total_app;

            fwrite("sdbf", 1, 4, file);
            fwrite(&total_app, 1, 4, file);
            fwrite(&uid_offset, 1, 8, file);
            fwrite(&name_offset, 1, 8, file);
            fwrite(&drive_offset, 1, 8, file);
            fwrite(&vendor_offset, 1, 8, file);

            for (auto& c_app: c_apps) {
                fwrite(&c_app.first, 1, 4, file);
            }

            for (auto& e_app: e_apps) {
                fwrite(&e_app.first, 1, 4, file);
            }

            for (auto& c_app: c_apps) {
                int size = c_app.second.name.size();

                fwrite(&size, 1, 4, file);
                fwrite(&c_app.second.name[0], 2, c_app.second.name.size(), file);
            }

            for (auto& e_app: e_apps) {
                int size = e_app.second.name.size();

                fwrite(&size, 1, 4, file);
                fwrite(&e_app.second.name[0], 2, e_app.second.name.size(), file);
            }

            for (auto& c_app: c_apps) {
                fwrite(&c_app.second.drive, 1, 1, file);
            }

            for (auto& e_app: e_apps) {
                fwrite(&e_app.second.drive, 1, 4, file);
            }

            for (auto& c_app: c_apps) {
                int size = c_app.second.name.size();

                fwrite(&size, 1, 4, file);
                fwrite(c_app.second.vendor_name.data(), 2, c_app.second.vendor_name.size(), file);
            }

            for (auto& e_app: e_apps) {
                int size = e_app.second.name.size();

                fwrite(&size, 1, 4, file);
                fwrite(&e_app.second.vendor_name[0], 2, e_app.second.vendor_name.size(), file);
            }

            fclose(file);

            return true;
        }

        // Load registry contains all installed apps
        // This is a quick format I designed when I was on my way
        // going home :P
        bool package_manager::load_sdb(const std::string& path) {
            FILE* file = fopen(path.c_str(), "rb");

            if (!file) {
                return false;
            }

            sdb_header header;

            fread(header.magic, 1, 4, file);

            if (strcmp(header.magic, "sdbf") != 0) {
                return false;
            }

            fread(&header.app_count, 1, 4, file);
            fread(&header.uid_offset, 1, 8, file);
            fread(&header.name_offset, 1, 8, file);

            std::vector<uid> uids(header.app_count);

            fseek(file, header.uid_offset, SEEK_SET);

            for (uint32_t i = 0; i < header.app_count; i++) {
                fread(&uids[i], 1, 4, file);
            }

            fseek(file, header.name_offset, SEEK_SET);

            std::vector<std::u16string> names(header.app_count);

            for (uint32_t i = 0; i < header.app_count; i++) {
                int name_len = 0;

                fread(&name_len, 1, 4, file);
                names[i].resize(name_len);

                fread(&(names[i])[0], 2, name_len, file);
            }

            std::vector<uint8_t> drives(header.app_count);

            fseek(file, header.drive_offset, SEEK_SET);

            for (uint32_t i = 0; i < header.app_count; i++) {
                fread(&drives[i], 1, 1, file);
            }


            std::vector<std::u16string> vendor_names(header.app_count);

            for (uint32_t i = 0; i < header.app_count; i++) {
                int name_len = 0;

                fread(&name_len, 1, 4, file);
                vendor_names[i].resize(name_len);

                fread(&(vendor_names[i])[0], 2, name_len, file);
            }

            // Load those into the manager
            for (uint32_t i = 0; i < header.app_count; i++) {
                app_info info {};

                info.drive = drives[i];
                info.name = names[i];
                info.vendor_name = vendor_names[i];

                // Drive C
                if (drives[i] == 0) {
                    c_apps.insert(std::make_pair(uids[i], info));
                } else {
                    e_apps.insert(std::make_pair(uids[i], info));
                }
            }

            fclose(file);

            return true;
        }

        bool package_manager::installed(uid app_uid) {
             auto res1 = c_apps.find(app_uid);
             auto res2 = e_apps.find(app_uid);

             return (res1 != c_apps.end()) || (res2 != e_apps.end());
        }

        std::u16string package_manager::app_name(uid app_uid) {
            auto res1 = c_apps.find(app_uid);
            auto res2 = e_apps.find(app_uid);

            if ((res1 == c_apps.end()) && (res2 == e_apps.end())) {
                LOG_WARN("App not installed on both drive, return null name.");
                return u"";
            }

            if (res1 != c_apps.end()) {
                return res1->second.name;
            }

            return res2->second.name;
        }

        bool package_manager::install_package(const std::u16string& path, uint8_t drive) {
            loader::sis_contents res =
                    loader::install_sis(common::ucs2_to_utf8(path), (loader::sis_drive)drive);

            app_info info {};

            info.vendor_name = res.controller.info.vendor_name.unicode_string;
            info.name = ((loader::sis_string*)(res.controller.info.names.fields[0].get()))->unicode_string;
            info.drive = drive;

            if (drive == 0) {
                c_apps.insert(std::make_pair(res.controller.info.uid.uid, info));
            } else {
                e_apps.insert(std::make_pair(res.controller.info.uid.uid, info));
            }

            write_sdb("apps_registry.sdb");

            return true;
        }

        bool package_manager::uninstall_package(uid app_uid) {
            c_apps.erase(app_uid);
            e_apps.erase(app_uid);

            write_sdb("apps_registry.sdb");

            return false;
        }
    }
}
