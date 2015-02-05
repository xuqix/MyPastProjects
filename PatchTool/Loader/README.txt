动态补丁，支持内存读写方式和调试寄存器方式补丁

需要工具修正的节为.sdata节
#pragma data_seg(".sdata")
DWORD	wTypeOfPatch = 0;			/指示补丁类型
DWORD	dwPatchNum = 2;				//补丁数量
//偏移8
TCHAR	szFileName[MAX_PATH] = { 0 };

//偏移528
DWORD	dwPatchAddress[16] = { 0}  //////////////////////利用调试寄存器打丁///////////////////////////////////////////////////////
/////////////打此类补丁应在补丁地址第一个地址填上希望中断的地址以确保所有地址数据已解码，以保证补丁正确性///////////

//偏移592
BYTE	byOldData[16] = { 0};		//补丁处旧数据和新数据
//偏移608
BYTE	byNewData[16] = { 0};

#pragma data_seg()
#pragma comment(linker, "/SECTION:.sdata,ERW")

其中加入了CRC32验证，需要补丁工具在PE头前4个字节写上目标文件的CRC32