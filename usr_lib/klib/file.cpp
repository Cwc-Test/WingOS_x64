#include "file.h"
#include <klib/syscall.h>
#include <stdio.h>
namespace sys
{

    file stdin;
    file stdout;
    file stderr;
    file::file()
    {
        opened = false;
    }
    file::file(const char *path)
    {
        opened = false;
        fpath = path;
        open(fpath);
    }

    void file::seek(uint64_t at)
    {
        sys$lseek(fid, at, SEEK_SET);
    }
    size_t file::read(uint8_t *buffer, uint64_t length)
    {
        if (opened == false)
        {
            printf("file is not opened \n");
            return 0;
        }

        size_t readed = sys$read(fid, buffer, length);
        fcurrent_seek_pos += length;
        return readed;
    }
    size_t file::write(const uint8_t *buffer, uint64_t length)
    {
        if (opened == false)
        {
            printf("file is not opened \n");
            return 0;
        }

        size_t writed = sys$write(fid, buffer, length);
        fcurrent_seek_pos += length;
        return writed;
    }

    void file::open(const char *path)
    {
        if (opened == true)
        {
            close();
        }
        fcurrent_seek_pos = 0;

        opened = true;
        fpath = path;
        fid = sys$open(path, 0, 0);
    }
    void file::close()
    {
        opened = false;
        sys$close(fid);
        fid = 0;
    }
    uint64_t file::get_file_length()
    {
        if (opened == false)
        {
            return 0;
        }
        fcurrent_seek_pos = sys$lseek(fid, 0, SEEK_CUR);
        size_t size = sys$lseek(fid, 0, SEEK_END);
        sys$lseek(fid, fcurrent_seek_pos, SEEK_SET);

        return size;
    }
} // namespace sys
