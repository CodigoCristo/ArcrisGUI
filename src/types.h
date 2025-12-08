#ifndef TYPES_H
#define TYPES_H

#include <gtk/gtk.h>
#include <adwaita.h>

// Enumeración para modos de particionado
typedef enum {
    DISK_MODE_AUTO_PARTITION,
    DISK_MODE_AUTO_BTRFS,
    DISK_MODE_CIFRADO,
    DISK_MODE_MANUAL_PARTITION
} DiskMode;



#endif /* TYPES_H */
