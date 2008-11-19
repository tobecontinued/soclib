dsx_segment_icu_start = 0x20200000;
dsx_segment_icu_end = 0x20200020;
dsx_segment_tty_start = 0x00200000;
dsx_segment_tty_end = 0x00200010;

dsx_segment_text_start = 0x60100000;
dsx_segment_text_end = 0x60200000;
dsx_segment_data_cached_start = 0x61100000;
dsx_segment_data_cached_end = 0x61200000;
dsx_segment_data_uncached_start = 0x62200000;
dsx_segment_data_uncached_end = 0x62300000;

m4_ifelse(CONFIG_CPU_MIPS, defined, `
dsx_segment_reset_start = 0xbfc00000;
dsx_segment_reset_end =   0xbfc00040;
')

m4_ifelse(CONFIG_CPU_PPC, defined, `
dsx_segment_reset_start = 0xffffffc0;
dsx_segment_reset_end =   0x100000000;
')

dsx_segment_excep_start = 0x80000000;
dsx_segment_excep_end = 0x80001000;
dsx_segment_sem_start = 0x40200000;
dsx_segment_sem_end = 0x40300000;
