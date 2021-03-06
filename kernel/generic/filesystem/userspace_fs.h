#ifndef USER_FS_H
#define USER_FS_H
#include <stddef.h>
#include <stdint.h>

class ram_file
{ // ram_file are file that are stored in the ram such as stdout stdin ...
public:
    char *buffer = nullptr;
    size_t size = 0;
    virtual const char *get_npath() { return "invalid ram file"; };
    virtual size_t read(void *dbuffer, size_t offset, size_t count);
    virtual size_t write(const void *dbuffer, size_t offset, size_t count);
};
struct filesystem_file_t
{
    const char *path;
    int mode;
    size_t cur;
    uint64_t rpid;
    int state;
    bool ram_file;
    class ram_file *file;
};
class std_zero_file : public ram_file
{
public:
    virtual const char *get_npath() { return "/dev/zero"; };
    virtual size_t read(void *dbuffer, size_t offset, size_t count);
    virtual size_t write(const void *dbuffer, size_t offset, size_t count);
};

class std_stdbuf_file : public ram_file
{
protected:
    void realocate(size_t new_size);
    size_t logging_pos = 0;
    size_t logging_pos_start = 0;

public:
    virtual size_t read(void *dbuffer, size_t offset, size_t count);
    virtual size_t write(const void *dbuffer, size_t offset, size_t count);
};

class std_stdin_file : public std_stdbuf_file
{
public:
    virtual const char *get_npath() { return "/dev/stdin"; };
};
class std_stderr_file : public std_stdbuf_file
{
public:
    virtual const char *get_npath() { return "/dev/stderr"; };
};
class std_stdout_file : public std_stdbuf_file
{
public:
    virtual const char *get_npath() { return "/dev/stdout"; };
};

class ram_dir
{
public:
    virtual const char *get_path() { return "invalid path"; };
    virtual ram_file *get(const char *msg) { return nullptr; };
};

class process_ramdir : public ram_dir
{
public:
    virtual const char *get_path() { return "/proc/"; };
    virtual ram_file *get(const char *msg);
};

struct per_process_userspace_fs
{
    static const int ram_files_count = 16;
    ram_file *ram_files[ram_files_count]; // in this we have stdio / stdin / stderr ...
};

bool is_ram_file(const char *path);
ram_file *get_ram_file(const char *path);

size_t fs_read(int fd, void *buffer, size_t count);
size_t fs_write(int fd, const void *buffer, size_t count);
int fs_open(const char *path_name, int flags, int mode);
int fs_close(int fd);
size_t fs_lseek(int fd, size_t offset, int whence);
void init_userspace_fs();
void init_process_userspace_fs(per_process_userspace_fs &target);
#endif // USER_FS_H
