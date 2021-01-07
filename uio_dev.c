/**
 * @file libuio.c
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief Function declarations for libuio userspace library.
 * @version 0.1
 * @date 2020-10-29
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include "uio_dev.h"

int uio_init(uio_dev *dev, int uio_id)
{
#ifdef UIO_DEBUG
        char ffname[256];
        snprintf(ffname, 256, "/sys/class/uio/uio%d/name", uio_id);
        FILE *ffp = fopen(ffname, "r");
        fscanf(ffp, "%s", &ffname);
        fprintf(stderr, "%s Line %d: %s %s\n", __func__, __LINE__, "UIO device name: ", ffname);
#endif
    dev->pfd = (struct pollfd *) malloc (sizeof(struct pollfd));
    if (dev->pfd == NULL)
    {
        fprintf(stderr, "%s: ", __func__);
        perror("malloc failed");
        return -1;
    }
    dev->pfd->events = POLLIN;
    if (uio_id < 0)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO ID is negative, aborting...\n");
        return UIO_ID_NEGATIVE;
    }

    if (dev == NULL)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO Device is unallocated, aborting...\n");
        return UIO_DEV_NULL;
    }

    dev->mapped = 0; // ensure that memory is NOT mapped

    char fname[256]; // 64 bytes max
    if (snprintf(fname, 256, "/dev/uio%d", uio_id) < 0)
    {
#ifdef UIO_DEBUG
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO fname could not be generated, aborting...\n");
#endif
        return UIO_FNAME_ERROR;
    }
    dev->fd = open(fname, O_RDWR);
    if (dev->fd < 0)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO device file could not be accessed, aborting...\n");
        return UIO_FD_OPEN_ERROR;
    }

    // access size register
    if (snprintf(fname, 256, "/sys/class/uio/uio%d/maps/map0/size", uio_id) < 0)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO regmap size fname could not be generated, aborting...\n");
        return UIO_FNAME_ERROR;
    }
    FILE *fp;
    ssize_t size;

    fp = fopen(fname, "r");
    if (!fp)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO regmap size file could not be opened, aborting...\n");
        return UIO_FD_OPEN_ERROR;
    }

    int ret = fscanf(fp, "0x%lx", &size);
#ifdef UIO_DEBUG
    fprintf(stderr, "%s Line %d: %s %d\n", __func__, __LINE__, "UIO regmap size read: ", size);
#endif 
    fclose(fp);
    if (ret < 0)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO regmap size read error, aborting...\n");
        return UIO_FILE_READ_ERROR;
    }

    if (size < 0 || size > 0x10000) // AXI4 Lite max address size = 64KiB
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO regmap size negative, aborting...\n");
        return UIO_MMAP_SIZE_ERROR;
    }

    dev->len = size;

    int offset = 0;
    // access size register
    if (snprintf(fname, 256, "/sys/class/uio/uio%d/maps/map0/offset", uio_id) < 0)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO regmap offset fname could not be generated, aborting...\n");
        return UIO_FNAME_ERROR;
    }

    fp = fopen(fname, "r");
    if (!fp)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO regmap offset file could not be opened, aborting...\n");
        return UIO_FD_OPEN_ERROR;
    }

    ret = fscanf(fp, "0x%x", &offset);
    fprintf(stderr, "%s Line %d: %s %d\n", __func__, __LINE__, "UIO regmap offset read: ", offset);
    fclose(fp);
    if (ret < 0)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO regmap offset read error, aborting...\n");
        return UIO_FILE_READ_ERROR;
    }

    if (offset < 0 || offset > 0xffff) // AXI4 Lite max address size = 64KiB
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO regmap size negative, aborting...\n");
        return UIO_MMAP_OFFSET_ERROR;
    }

    dev->addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dev->fd, offset);

    if (dev->addr == MAP_FAILED)
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "UIO register mmap failed, aborting...\n");
        return UIO_MMAP_FAILED;
    }
    // everything successful
    dev->pfd->fd = dev->fd;
    dev->mapped = 1;
    dev->len = 0x1000;
    return 1;
}

void uio_destroy(uio_dev *dev)
{
    if (dev->mapped)
        munmap(dev->addr, dev->len);
    free(dev->pfd);
    close(dev->fd);
}

int uio_write(uio_dev *dev, int offset, uint32_t data)
{
    if (offset < 0 || offset > dev->len - 1)
    {
        fprintf(stderr, "%s Line %d: ", __func__, __LINE__);
        fprintf(stderr, "accessing register beyond limit, offset %d. Aborting...\n", offset);
        return UIO_ACCESS_VIOLATION;
    }
    *((uint32_t *)(dev->addr + offset)) = data;
    return 1;
}

int uio_read(uio_dev *dev, int offset, uint32_t *data)
{
    if (offset < 0 || offset > dev->len - 1)
    {
        fprintf(stderr, "%s Line %d: ", __func__, __LINE__);
        fprintf(stderr, "accessing register beyond limit, offset %d. Aborting...\n", offset);
        return UIO_ACCESS_VIOLATION;
    }
    *data = (*((uint32_t *)(dev->addr + offset)));
    return 1;
}

int uio_unmask_irq(uio_dev *dev)
{
    uint32_t umask = 1;
    ssize_t rv = write(dev->fd, &umask, sizeof(umask));
    if (rv != (ssize_t)sizeof(umask))
    {
        fprintf(stderr, "%s Line %d: ", __func__, __LINE__);
        fprintf(stderr, "Could not unmask interrupt. Wrote %ld of %ld...\n", rv, sizeof(umask));
        return UIO_UMASK_IRQ_FAILED;
    }
    return 1;
}

int uio_mask_irq(uio_dev *dev)
{
    uint32_t umask = 0;
    ssize_t rv = write(dev->fd, &umask, sizeof(umask));
    if (rv != (ssize_t)sizeof(umask))
    {
        fprintf(stderr, "%s Line %d: ", __func__, __LINE__);
        fprintf(stderr, "Could not mask interrupt. Wrote %ld of %ld...\n", rv, sizeof(umask));
        return UIO_MASK_IRQ_FAILED;
    }
    return 1;
}

int uio_wait_irq(uio_dev *dev, int32_t tout_ms)
{
    int rv = poll(dev->pfd, 1, tout_ms);

    if (rv >= 1)
    {
        uint32_t info, ret;
        ret = read(dev->fd, &info, sizeof(info)); // clear the interrupt
#ifdef UIO_DEBUG
        fprintf(stderr, "%s Line %d: ", __func__, __LINE__);
        fprintf(stderr, "Received interrupt %d.\n", info);
#endif
        return (info + 1); // to guarantee a non-zero value
    }
    else if (rv == 0)
    {
#ifdef UIO_DEBUG
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "Interrupt timed out.\n");
#endif
        return 0;
    }
    else
    {
        fprintf(stderr, "%s Line %d: %s", __func__, __LINE__, "Error polling for interrupt. ");
        perror("poll");
        return -1;
    }
    return 1; // will never reach this point
}
