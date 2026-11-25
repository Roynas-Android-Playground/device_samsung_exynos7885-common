/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "libshim_ion"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/dma-heap.h>
#include <log/log.h>

#define DMA_HEAP_ROOT "/dev/dma_heap/"
#define ION_FLAG_CACHED 1

/*
 * Legacy Exynos ION heap IDs and the DMA-BUF heaps that replace them,
 * matching the table used by libion_exynos.
 */
static const char *const dma_heaps[] = {
    [0] = "system",
    [1] = "crypto",
    [3] = "system-secure-vstream",
    [5] = "vframe",
    [6] = "vscaler",
    [8] = "gpu_crc",
    [9] = "system-secure-gpu_buffer",
    [10] = "camera",
    [11] = "secure_camera",
    [13] = "ext_ui",
    [14] = "system",
};

#define NUM_DMA_HEAPS (sizeof(dma_heaps) / sizeof(dma_heaps[0]))

/*
 * Callers treat a zero descriptor as "not opened", so hand out a real
 * one. It is only used as a handle and is never passed to an ioctl.
 */
int ion_open(void) {
    int fd = open(DMA_HEAP_ROOT "system", O_RDONLY | O_CLOEXEC);

    if (fd < 0) {
        ALOGE("open " DMA_HEAP_ROOT "system failed: %s", strerror(errno));
        return -errno;
    }

    return fd;
}

int ion_close(int fd) {
    if (close(fd) < 0)
        return -errno;

    return 0;
}

static int dma_heap_alloc(const char *name, int cached, size_t len) {
    struct dma_heap_allocation_data data = {
        .len = len,
        .fd_flags = O_RDWR | O_CLOEXEC,
    };
    char path[64];
    int heap_fd, ret;

    snprintf(path, sizeof(path), DMA_HEAP_ROOT "%s%s", name, cached ? "" : "-uncached");

    heap_fd = open(path, O_RDONLY | O_CLOEXEC);
    if (heap_fd < 0)
        return -errno;

    ret = ioctl(heap_fd, DMA_HEAP_IOCTL_ALLOC, &data);
    if (ret < 0)
        ret = -errno;
    else
        ret = data.fd;

    close(heap_fd);

    return ret;
}

int ion_alloc_fd(int fd, size_t len, size_t align, unsigned int heap_mask, unsigned int flags,
                 int *handle_fd) {
    int ret = -ENODEV;
    unsigned int id;

    (void)fd;
    (void)align;

    if (!handle_fd)
        return -EINVAL;

    for (id = 0; id < NUM_DMA_HEAPS; id++) {
        if (!(heap_mask & (1U << id)) || !dma_heaps[id])
            continue;

        ret = dma_heap_alloc(dma_heaps[id], flags & ION_FLAG_CACHED, len);
        if (ret >= 0) {
            *handle_fd = ret;
            return 0;
        }
    }

    ALOGE("%s: len %zu heap_mask %#x flags %#x failed: %s", __func__, len, heap_mask, flags,
          strerror(-ret));

    return ret;
}
