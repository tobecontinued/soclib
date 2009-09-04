/*****************************************************************************

  vfs_file.c  -- File performing the mount of the VFS file system
   - input_vfs_init

  Thales Author:
  Pierre-Edouard Beaucamps THALES COM - AAL, 2009

*****************************************************************************/

/****************************************************************************
  Include section
****************************************************************************/
#include "vfs_file.h"

/****************************************************************************
  Variables and structures
****************************************************************************/
static struct device_s bd_dev;


void input_vfs_init()
{
  /*************************************
     VFS management
  *************************************/
  device_init(&bd_dev);
#if defined(CONFIG_ARCH_EMU)
  block_file_emu_init(&bd_dev, "emu_img.bin");
#else
  block_ramdisk_init(&bd_dev, (void*) DSX_SEGMENT_RAMDISK_ADDR);
#endif

#if defined(CONFIG_VFS_LIBC_STREAM)
  printf("  -- INPUT VFS INIT -- VFS Initialization\n");
  if (vfs_init(&bd_dev, VFS_VFAT_TYPE, 20, 20, NULL) != 0){
    printf("\033[1A  -- INPUT VFS INIT -- Aborted : problem during the initialization\n");
    abort();
  }
  printf("\033[1A  -- INPUT VFS INIT -- VFS Initialized\033[K\n");
#endif
}

