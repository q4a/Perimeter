static uint32_t shader[]={
	0xffff0104, 0x00000051, 0xa00f0000, 0x3f000000, 0x3f000000, 0x3f000000, 
	0x3f000000, 0x00000051, 0xa00f0003, 0xbf400000, 0xbf400000, 0xbf400000, 
	0xbf400000, 0x00000042, 0x800f0001, 0xb0e40001, 0x00000042, 0x800f0002, 
	0xb0e40002, 0x00000040, 0x80070003, 0xb0e40003, 0x00000040, 0x80070004, 
	0xb0e40004, 0x00000008, 0x80170000, 0x80e40002, 0x80e40003, 0x00000005, 
	0x80070000, 0x80e40000, 0xa0e40005, 0x00000002, 0x80070000, 0x80e40000, 
	0xa0e40004, 0x00000005, 0x80070000, 0x80e40000, 0x80e40001, 0x00000008, 
	0x80170001, 0x80e40002, 0x80e40004, 0x00000004, 0x82170001, 0x80e40001, 
	0x80e40001, 0xa0e40003, 0x00000004, 0x80070001, 0x80e40001, 0xa0e40006, 
	0x80e40000, 0x0000fffd, 0x00000042, 0x800f0000, 0xb0e40000, 0x00000004, 
	0x80070000, 0x80e40000, 0xa0e40000, 0xa0ff0000, 0x00000004, 0x80070000, 
	0x80e40001, 0x80e40000, 0x90e40000, 0x0000ffff, 
};
	Compile("object_scene_bump.psh",shader);
