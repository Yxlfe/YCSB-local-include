#ifndef __DISKINFO_HH__
#define __DISKINFO_HH__

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "../define.hh"
#include "../configManager.hh"
#include "../util/debug.hh"

struct DiskInfo {
    disk_id_t diskId;                   // disk id
    char diskPath[128];                 // disk path
    offset_t capacity, remaining;       // total capacity, and unused capacity
    bool isLog;                         // log disk
    bool dirty;                         // whether data is written since last sync
    bool alive;                         // whether the disk is alive

    int fd;                             // file descriptor for the disk

    offset_t skipOffset;                // offset to skip from the starting point of the disk

    DiskInfo() {
        diskId = INVALID_DISK;
        strcpy(diskPath, "");
        capacity = 0;
        remaining = 0;
        fd = 0;
        dirty = false;
        alive = true;
        skipOffset = 0;
    };

    DiskInfo(disk_id_t diskId, const char* diskPath, ULL capacity, bool isLogDisk = false, offset_t skip = 0) {
        this->diskId = diskId;
        strncpy(this->diskPath, diskPath, 127);
        this->capacity = capacity;

        // Todo: For benchmark. Assume disk is brand new when starting
        this->remaining = capacity - skip;

        this->skipOffset = skip;

        isLog = isLogDisk;
        alive = true;

        int flag = O_RDWR|O_CREAT;
#ifdef DISK_DIRECT_IO
        flag |= O_DIRECT;
#ifdef ENABLE_DISK_DIRECT_IO
        char temp_diskPath[128];
        strncpy(temp_diskPath, diskPath, 127);  // 先拷贝路径到缓冲区，最多拷贝127个字符
        temp_diskPath[127] = '\0';  // 确保字符串终止

        // 现在添加后缀
        const char* suffix = "/direct";
        size_t remaining_space = 127 - strlen(temp_diskPath);  // 计算剩余空间
        strncat(temp_diskPath, suffix, remaining_space);  // 安全地添加后缀
        temp_diskPath[127] = '\0';  // 再次确保字符串终止
        strncpy(this->diskPath, temp_diskPath, 127);
#endif
#endif
#ifdef DISK_DSYNC
        flag |= O_DSYNC;
#endif

        bool useFS = ConfigManager::getInstance().segmentAsFile();
        if (!useFS) {
            fd = open(this->diskPath, flag, 0666);
            if (fd < 2) {
                // debug_error("zc***********open failed fd %d flag %d diskPath %s\n", fd, flag, this->diskPath);
                perror(strerror(errno));
                assert(0);
            }
        }
    }

    void resetDisk() {
        this->remaining = capacity - skipOffset;
    }


    void setDisk(bool status) {
        alive = status;
        if (alive == false) {
            resetDisk();
        }
    }

    void disconnect() {
        close(fd);
        fd = 0;
    }

};

#endif // __DISKINFO_HH__
