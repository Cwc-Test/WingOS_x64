#include "userspace_fs.h"
#include <device/local_data.h>
#include <filesystem/file_system.h>
#include <liballoc.h>
#include <logging.h>
#include <utility.h>
lock_type handle_lock;
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
enum file_system_file_state
{
    FS_STATE_ERROR = 0,
    FS_STATE_FREE = 1,
    FS_STATE_USED = 2,
    FS_STATE_RESERVED = 3, // may be used later

};
ram_file rmf;
ram_dir *ram_directory[1];
void init_process_userspace_fs(per_process_userspace_fs &target)
{
    lock(&handle_lock);
    for (int i = 0; i < per_process_userspace_fs::ram_files_count; i++)
    {
        target.ram_files[i] = &rmf;
    }

    target.ram_files[0] = new std_zero_file();
    target.ram_files[1] = new std_stdout_file();
    target.ram_files[2] = new std_stderr_file();
    target.ram_files[3] = new std_stdin_file();
    unlock(&handle_lock);
}

ram_file *process_ramdir::get(const char *msg)
{
    msg += strlen("/proc/");

    auto pid = atoi(msg);
    if (upid_to_kpid(pid) == (uint64_t)-1)
    {
        log("ram_dir", LOG_INFO) << "invalid pid for path /proc/" << pid << "for path+" << msg;
        return nullptr;
    }
    for (; *msg != '/'; msg++)
        ;
    if (strncmp(msg, "/fd/", 4) == 0)
    {
        msg += 4;
        int fd = atoi(msg);

        log("ram_dir", LOG_INFO) << "getting buffer" << fd << "of process" << pid << "for path+" << msg;
        return process_array[upid_to_kpid(pid)].ufs.ram_files[fd];
    }
    else
    {
        return nullptr;
    }
}

size_t ram_file::read(void *buffer, size_t offset, size_t count)
{
    log("ram file", LOG_ERROR) << "can't use " << __PRETTY_FUNCTION__ << " in file " << get_npath() << " in process" << get_current_cpu_process()->process_name;
    return -1;
}
size_t ram_file::write(const void *buffer, size_t offset, size_t count)
{
    log("ram file", LOG_ERROR) << "can't use " << __PRETTY_FUNCTION__ << " in file " << get_npath() << " in process" << get_current_cpu_process()->process_name;
    return -1;
}

size_t std_zero_file::read(void *dbuffer, size_t offset, size_t count)
{
    memset(dbuffer, 0, count);
    return offset;
}
size_t std_zero_file::write(const void *dbuffer, size_t offset, size_t count)
{
    log("stdzero", LOG_WARNING) << "trying to write to" << get_npath();
    return 0;
}
size_t std_stdbuf_file::read(void *dbuffer, size_t offset, size_t count)
{
    size_t final_count = count;
    if (size < offset)
    {
        return 0;
    }
    else if (size < final_count + offset)
    {
        final_count = size - offset;
    }
    memcpy(dbuffer, buffer + offset, count);
    return final_count;
}
size_t std_stdbuf_file::write(const void *dbuffer, size_t offset, size_t count)
{
    realocate(offset + count);
    uint8_t *dbuf = (uint8_t *)dbuffer;
    for (size_t i = offset; i < offset + count; i++)
    {
        if (dbuf[i - offset] == '\0')
        {
            buffer[i] = ' ';
        }
        else
        {
            buffer[i] = dbuf[i - offset];
        }
    }
    for (; logging_pos < offset + count; logging_pos++)
    {
        if (buffer[logging_pos] == '\n')
        {
            log("std", LOG_INFO) << range_str(buffer + logging_pos_start, logging_pos - logging_pos_start);
            logging_pos_start = logging_pos + 1;
        }
    }
    return count;
}

void std_stdbuf_file::realocate(size_t new_size)
{
    if (buffer == nullptr)
    {
        buffer = (char *)malloc(new_size);
        memset(buffer, 'f', new_size);
        size = new_size;
        return;
    }
    else
    {
        if (new_size > size)
        {
            buffer = (char *)realloc(buffer, new_size);
        }
        size = new_size;
    }
}
#define MAX_FILE_HANDLE 255
filesystem_file_t fs_handle_table[MAX_FILE_HANDLE];

void init_userspace_fs()
{
    ram_directory[0] = new process_ramdir();
    for (int i = 0; i < MAX_FILE_HANDLE; i++)
    {
        fs_handle_table[i].state = FS_STATE_FREE;
    }
}

filesystem_file_t *get_if_valid_handle(int fd, bool check_free = true)
{
    if (fd > MAX_FILE_HANDLE)
    {
        log("fs", LOG_WARNING) << "process " << get_current_cpu_process()->process_name << " trying to use an invalid file descriptor";

        return nullptr;
    }
    else if (fs_handle_table[fd].state != file_system_file_state::FS_STATE_USED && check_free)
    {
        log("fs", LOG_WARNING) << "process " << get_current_cpu_process()->process_name << " trying to use a file descriptor already free";

        return nullptr;
    }
    else if (fs_handle_table[fd].rpid != get_current_cpu_process()->upid && check_free)
    {
        log("fs", LOG_WARNING) << "process " << get_current_cpu_process()->process_name << " trying to use a file descriptor used by another process";

        return nullptr;
    }
    return &fs_handle_table[fd];
}

int get_free_handle()
{
    for (int i = 0; i < MAX_FILE_HANDLE; i++)
    {
        if (fs_handle_table[i].state == file_system_file_state::FS_STATE_FREE)
        {
            return i;
        }
    }
    return -1;
}

int set_free_handle(int fd)
{
    auto handle = get_if_valid_handle(fd);
    if (handle == nullptr)
    {
        log("fs", LOG_WARNING) << "process " << get_current_cpu_process()->process_name << " trying to free an invalid file descriptor";

        return -1;
    }
    handle->state = file_system_file_state::FS_STATE_FREE;
    return fd;
}

int set_used_handle(int fd)
{
    auto handle = get_if_valid_handle(fd, false);

    if (handle == nullptr)
    {
        log("fs", LOG_WARNING) << "process " << get_current_cpu_process()->process_name << " trying to set used an invalid file descriptor";

        return -1;
    }
    handle->state = file_system_file_state::FS_STATE_USED;
    return fd;
}

size_t fs_read(int fd, void *buffer, size_t count)
{
    filesystem_file_t *file = get_if_valid_handle(fd);
    if (file == nullptr)
    {
        log("fs", LOG_ERROR) << "process " << get_current_cpu_process()->process_name << " can't read file";
        return 0;
    }
    if (file->ram_file)
    {
        auto result = file->file->read(buffer, file->cur, count);
        file->cur += result;
        return result;
    }
    else
    {
        auto result = main_fs_system::the()->main_fs()->read_file(file->path, file->cur, count, (uint8_t *)buffer);
        file->cur += result;
        return result;
    }
}

size_t fs_lseek(int fd, size_t offset, int whence)
{

    filesystem_file_t *file = get_if_valid_handle(fd);
    if (file == nullptr)
    {
        log("fs", LOG_ERROR) << "process " << get_current_cpu_process()->process_name << " can't seek file invalid fd";
        return 0;
    }
    if (whence == SEEK_SET)
    {
        file->cur = offset;
    }
    if (whence == SEEK_CUR)
    {
        file->cur += offset;
    }
    if (whence == SEEK_END)
    {
        if (file->ram_file)
        {
            file->cur = file->file->size;
        }
        else
        {
            file->cur = main_fs_system::the()->main_fs()->get_file_length(file->path);
        }
        // unimplemented :^(
    }
    return file->cur;
}

size_t fs_write(int fd, const void *buffer, size_t count)
{
    filesystem_file_t *file = get_if_valid_handle(fd);
    if (file == nullptr)
    {
        log("fs", LOG_ERROR) << "process " << get_current_cpu_process()->process_name << " can't read file";
        return 0;
    }
    if (file->ram_file)
    {
        auto result = file->file->write(buffer, file->cur, count);
        file->cur += result;
        return result;
    }
    else
    {

        auto result = main_fs_system::the()->main_fs()->write_file(file->path, file->cur, count, (uint8_t *)buffer);
        file->cur += result;
        return result;
    }
}

int fs_open(const char *path_name, int flags, int mode)
{
    lock(&handle_lock);
    int fd = get_free_handle();
    if (fd == -1)
    {
        log("fs", LOG_ERROR) << "process " << get_current_cpu_process()->process_name << " can't get free handle";

        return 0;
    }
    set_used_handle(fd);
    unlock(&handle_lock);
    fs_handle_table[fd].rpid = get_current_cpu_process()->upid;
    fs_handle_table[fd].path = path_name;
    fs_handle_table[fd].mode = mode;
    fs_handle_table[fd].cur = 0;
    fs_handle_table[fd].ram_file = false;
    if (is_ram_file(path_name))
    {

        //  log("fs", LOG_INFO) << "process " << get_current_cpu_process()->process_name << " open (ram file) " << path_name;
        fs_handle_table[fd].ram_file = true;
        fs_handle_table[fd].file = get_ram_file(path_name);
        return fd;
    }
    else if (main_fs_system::the()->main_fs()->get_file_length(path_name) == (uint64_t)-1)
    {
        ram_dir *target = nullptr;
        for (size_t i = 0; i < sizeof(ram_directory) / sizeof(ram_directory[0]); i++)
        {
            if (strncmp(ram_directory[i]->get_path(), path_name, strlen(ram_directory[i]->get_path())) == 0)
            {
                target = ram_directory[i];
                break;
            }
        }
        if (target == nullptr)
        {

            log("fs", LOG_WARNING) << "process " << get_current_cpu_process()->process_name << " trying to open file" << path_name << "but it doesn't exits";

            return -1;
        }
        else
        {
            fs_handle_table[fd].ram_file = true;
            fs_handle_table[fd].file = target->get(path_name);
            return fd;
        }
    }
    else
    {

        if (main_fs_system::the()->main_fs()->get_file_length(path_name) == (uint64_t)-1)
        {
            log("fs", LOG_WARNING) << "process " << get_current_cpu_process()->process_name << " trying to open file" << path_name << "but it doesn't exits";

            return -1;
        }
        log("fs", LOG_INFO) << "process " << get_current_cpu_process()->process_name << " open " << path_name;

        return fd;
    }
}

int fs_close(int fd)
{
    auto file = get_if_valid_handle(fd);
    if (file == nullptr)
    {
        log("fs", LOG_ERROR) << "process " << get_current_cpu_process()->process_name << " invalid fd for closing file ";

        return 0;
    }
    lock(&handle_lock);

    set_free_handle(fd);
    log("fs", LOG_INFO) << "process " << get_current_cpu_process()->process_name << " closed " << file->path;

    unlock(&handle_lock);
    return 1;
}

bool is_ram_file(const char *path)
{

    for (int i = 0; i < per_process_userspace_fs::ram_files_count; i++)
    {
        if (strcmp(get_current_cpu_process()->ufs.ram_files[i]->get_npath(), path) == 0)
        {
            return true;
        }
    }

    return false;
}
ram_file *get_ram_file(const char *path)
{

    for (int i = 0; i < per_process_userspace_fs::ram_files_count; i++)
    {
        if (strcmp(get_current_cpu_process()->ufs.ram_files[i]->get_npath(), path) == 0)
        {
            return get_current_cpu_process()->ufs.ram_files[i];
        }
    }
    return nullptr;
}
