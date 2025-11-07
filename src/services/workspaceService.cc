#include "workspaceService.h"

#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <sys/stat.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "../utils/config.h"

namespace fs = std::filesystem;

namespace services {

static void addDirToArchive(archive* a, const std::string& path,
                            const std::string& prefix) {
  DIR* dir = opendir(path.c_str());
  if (!dir) {
    throw std::runtime_error("Cannot open directory");
  }

  try {
    struct dirent* ent;
    while ((ent = readdir(dir))) {
      std::string name = ent->d_name;
      if (name == "." || name == "..") {
        continue;
      }

      std::string full = path + "/" + name;
      std::string arch = prefix + "/" + name;
      struct stat st;
      if (stat(full.c_str(), &st) != 0) {
        continue;
      }

      archive_entry* entry = archive_entry_new();
      archive_entry_set_pathname(entry, arch.c_str());
      archive_entry_copy_stat(entry, &st);
      archive_write_header(a, entry);

      if (S_ISREG(st.st_mode)) {
        std::ifstream file(full, std::ios::binary);
        char buf[Config::FILE_BUFFER_SIZE];
        while (file.read(buf, sizeof(buf)) || file.gcount() > 0) {
          archive_write_data(a, buf, file.gcount());
        }
      }

      archive_entry_free(entry);
      if (S_ISDIR(st.st_mode)) {
        addDirToArchive(a, full, arch);
      }
    }
    closedir(dir);
  } catch (...) {
    closedir(dir);
    throw;
  }
}

void WorkspaceService::compress(const std::string& user) {
  std::string base = Config::PATH_HOME_BASE + user;
  std::string workspace = base + Config::PATH_WORKSPACE;
  std::string output = base + Config::PATH_ARCHIVE;

  if (!fs::exists(workspace)) {
    throw std::runtime_error("Workspace directory does not exist");
  }

  fs::remove(output);

  archive* a = archive_write_new();
  if (!a) {
    throw std::runtime_error("Failed to create archive");
  }

  try {
    archive_write_add_filter_gzip(a);
    archive_write_set_format_pax_restricted(a);
    if (archive_write_open_filename(a, output.c_str()) != ARCHIVE_OK) {
      throw std::runtime_error("Failed to open output");
    }
    addDirToArchive(a, workspace, "workspace");
    archive_write_close(a);
    archive_write_free(a);
  } catch (...) {
    archive_write_free(a);
    throw;
  }
}

void WorkspaceService::extract(const std::string& user) {
  std::string base = Config::PATH_HOME_BASE + user;
  std::string workspace = base + Config::PATH_WORKSPACE;
  std::string input = base + Config::PATH_ARCHIVE;

  if (!fs::exists(input)) {
    throw std::runtime_error("Archive file does not exist");
  }

  fs::remove_all(workspace);

  archive* a = archive_read_new();
  if (!a) {
    throw std::runtime_error("Failed to create reader");
  }

  try {
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);
    if (archive_read_open_filename(a, input.c_str(),
                                   Config::ARCHIVE_BLOCK_SIZE) != ARCHIVE_OK) {
      throw std::runtime_error("Failed to open archive");
    }

    archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
      std::string path = base + "/" + archive_entry_pathname(entry);
      archive_entry_set_pathname(entry, path.c_str());

      archive* ext = archive_write_disk_new();
      archive_write_disk_set_options(
          ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);

      if (archive_write_header(ext, entry) == ARCHIVE_OK) {
        const void* buf;
        size_t size;
        int64_t offset;
        while (archive_read_data_block(a, &buf, &size, &offset) == ARCHIVE_OK) {
          archive_write_data_block(ext, buf, size, offset);
        }
      }
      archive_write_free(ext);
    }
    archive_read_close(a);
    archive_read_free(a);

    fs::remove(input);
  } catch (...) {
    archive_read_free(a);
    throw;
  }
}

}  // namespace services
