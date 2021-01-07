/**
 * @file libuio.h
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @brief UIO memory access based function prototypes and datatypes.
 * @version 0.1
 * @date 2020-10-29
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef __LIB_UIO_H
#define __LIB_UIO_H

#include <sys/types.h>
#include <stdint.h>

typedef enum
{
    UIO_UMASK_IRQ_FAILED = -12, /// Failed to unmask interrupt vector
    UIO_MASK_IRQ_FAILED,        /// Failed to mask interrupt vector
    UIO_ACCESS_VIOLATION,       /// Register access violation, offset out of range
    UIO_MMAP_FAILED,            /// Register memory map failed
    UIO_MMAP_OFFSET_ERROR,      /// UIO register offset error (in /sys/class/uio/uioX/maps/map0/offset)
    UIO_MMAP_SIZE_ERROR,        /// UIO register map size negative or larger than 64KiB which is maximum allowed for AXI4-Lite bus
    UIO_FILE_READ_ERROR,        /// UIO register map properties file read error
    UIO_FD_OPEN_ERROR,          /// UIO device file (/dev/uioX) open error
    UIO_FNAME_ERROR,            /// UIO file name error (from snprintf)
    UIO_DEV_NULL,               /// uio_dev device memory not allocated
    UIO_ID_NEGATIVE,            /// UIO device ID negative
} UIO_ERROR;

typedef struct
{
    int fd;             /// File descriptor for the UIO device
    void *addr;         /// Address to the memory map of the UIO device config space
    size_t len;         /// Length of the UIO device config space
    int mapped;         /// Indicates whether the memory map has been allocated
    struct pollfd *pfd; /// Poll file descriptor for the UIO device
} uio_dev;
/**
 * @brief Initialize a UIO device with given ID
 * 
 * @param dev uio_dev descriptor for the UIO device. Memory of this struct
 * has to be pre-allocated.
 * @param uio_id ID of the UIO device being accessed.
 * @return int Returns positive on success and negative on error.
 */
int uio_init(uio_dev *dev, int uio_id);
/**
 * @brief Unmap configuration space and disable a UIO device
 * 
 * @param dev uio_dev descriptor for the UIO device. Memory of this struct is
 * NOT freed by this function, the caller has to take care of memory management.
 */
void uio_destroy(uio_dev *dev);
/**
 * @brief Write data to UIO device configuration space at offset provided.
 * 
 * @param dev Descriptor for the UIO device.
 * @param offset Offset to the register.
 * @param data Data to be written to the register
 * 
 * @returns 1 on success, UIO_ACCESS_VIOLATION on error
 */
int uio_write(uio_dev *dev, int offset, uint32_t data);
/**
 * @brief Read data from UIO device register at offset specified.
 * 
 * @param dev Descriptor for the UIO device.
 * @param offset Offset to the register.
 * @param data Data to be read from the register.
 * 
 * @returns 1 on success, UIO_ACCESS_VIOLATION on error
 */
int uio_read(uio_dev *dev, int offset, uint32_t *data);
/**
 * @brief Unmasks (enables) the interrupt vector associated with the UIO device.
 * 
 * @param dev Descriptor for the UIO device.
 * @return int Returns positive on success and negative on error.
 */
int uio_unmask_irq(uio_dev *dev);
/**
 * @brief Masks (disables) the interrupt vector associated with the UIO device.
 * 
 * @param dev Descriptor for the UIO device.
 * @return int Returns positive on success and negative on error.
 */
int uio_mask_irq(uio_dev *dev);
/**
 * @brief Waits on interrupt from the UIO device until interrupt or timeout
 * occurrs.
 * 
 * @param dev Descriptor for the UIO device.
 * @param tout_ms Timeout in milliseconds. After this amount of time has
 * elapsed, the call will return even if an interrupt had not occurred.
 * @return int Positive on receiving interrupt, zero on timeout, negative on
 * error.
 */
int uio_wait_irq(uio_dev *dev, int32_t tout_ms);
#endif // __LIB_UIO_H