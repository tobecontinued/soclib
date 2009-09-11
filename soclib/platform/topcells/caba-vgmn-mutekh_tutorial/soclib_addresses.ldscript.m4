dsx_segment_text_start = 0x60100000;
dsx_segment_text_end = 0x60200000;
dsx_segment_data_cached_start = 0x61100000;
dsx_segment_data_cached_end = 0x61200000;
dsx_segment_data_uncached_start = 0x62600000;
dsx_segment_data_uncached_end = 0x62700000;

m4_ifelse(CONFIG_CPU_MIPS, defined, `
dsx_segment_reset_start = 0xbfc00000;
dsx_segment_reset_end =   0xbfc00040;
')

m4_ifelse(CONFIG_CPU_PPC, defined, `
dsx_segment_reset_start = 0xffffff80;
dsx_segment_reset_end =   0x100000000;
')

m4_ifelse(CONFIG_CPU_ARM, defined, `
dsx_segment_reset_start = 0x000000000;
dsx_segment_reset_end =   0x000000400;
')

dsx_segment_excep_start = 0x80000000;
dsx_segment_excep_end = 0x80001000;
