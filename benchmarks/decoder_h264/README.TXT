			----------------------
			-TO LAUNCH SIMULATION-
			----------------------

Authors : Fabien Colas-Bigey & Pierre-Edouard Beaucamps (Thales Communications)
Date    : September 2009


1) Verify links
Verify that you dispose of the correct links for yuv_test.bin.
You can download images at : http://www.soclib.fr/files/disk_img.tgz

2) Paths
Verify that you have correctly set your paths: SystemC, cross compilers and others.

3) Mutekh
Create links in mutekh directory to soclib_addresses.
   % cd tools_path/mutekh
   % ln –s path_to_platform/soclib_addresses.h
   % ln –s path_to_platform/soclib_addresses.ldscript.m4

4) Compile the platform and software
   % cd platform_mutekh_h264_ramdisk_caba_non_coherent_cache
   % make

5) Launch simulation
   % ./h264_platform.x mutekh/kernel-soclib-mips.out


In case of a problem, please post on : dev@soclib.fr
