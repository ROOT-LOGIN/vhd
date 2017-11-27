// vhdman.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../hyvhd/hyvhd.h"
#include <time.h>

#ifdef _M_X64
#ifdef _DEBUG
#include <crtdbg.h>
#pragma comment(lib, "../x64/Debug/hyvhd.lib")
#else
#pragma comment(lib, "../x64/Release/hyvhd.lib")
#endif
#elif defined(_M_IX86)
#ifdef _DEBUG
#include <crtdbg.h>
#pragma comment(lib, "../Debug/hyvhd.lib")
#else
#pragma comment(lib, "../Release/hyvhd.lib")
#endif
#endif

int _tmain(int argc, _TCHAR* argv[])
{
#if 0
	tm _t = {0};
	_t.tm_year = 100; 
	_t.tm_mon = 0;
	_t.tm_mday = 1;
	_t.tm_hour = 0;
	__time64_t ss = _mkgmtime64(&_t);

	u4_t t = VhdGetTimestamp();
	VHDTIME gmt, local;
	VhdMakeGmtTime(t, &gmt);
	VhdMakeLocalTime(t, &local);

	SYSTEMTIME systm = { 0 };
	systm.wYear = 2000;
	systm.wMonth = 1;
	systm.wDay = 1;
	union DD{
		FILETIME f;
		u8_t u;
	};
	DD filetm;
	SystemTimeToFileTime(&systm, &filetm.f);
	filetm.u /= 10000000;
	return 0;
#endif

	if(argc != 2) {
		wprintf(L"vhdman vhdFilePath\n");
		return 0;
	}
	
	const wchar_t* vhdFile = //L"E:\\$ROOM\\Kubuntu.vhd";
		//L"E:\\$ROOM\\$temp.vhd";
		//L"A:\\dif-temp.vhd";
		//L"D:\\PowerToy\\Xp\\444.vhd";
		//L"D:\\PowerToy\\Xp\\$temp-dyn.vhd";
		//L"E:\\$ROOM\\$temp-dyn.vhd";
		//L"D:\\PowerToy\\Xp\\Windows XP Mode base.vhd";
		//L"D:\\PowerToy\\Xp\\Windows XP Mode.vhd";
		//L"A:\\temp.vhd";
		//L"A:\\dif-fix-temp.vhd";
		//L"A:\\dyn-fix-temp.vhd";
		L"A:\\dif-dif-temp.vhd";
		//L"A:\\dif-dyn-temp.vhd";
		//argv[1];

	VHDRETCODE retcode = 0;
#if 2
	VHDIMAGE vhd = VhdOpenDisk(vhdFile, VOF_READONLY, &retcode);
	/*VHDFOOTER footer;
	HANDLE file = (HANDLE)*((u4_t*)vhd+1);
	SetFilePointer(file, 0, 0, FILE_BEGIN);
	DWORD lo;
	ReadFile(file, &footer, 512, &lo, NULL);
	SetFilePointer(file, 0, 0, FILE_END);
	WriteFile(file, &footer, 512, &lo, NULL);*/
#define SECTOR_COUNT 1
	u4_t secno = SECTOR_COUNT;
	//VHDSECTOR *psector = new VHDSECTOR[SECTOR_COUNT];
	VHDSECTOR psector[SECTOR_COUNT];
	/*for(u8_t u = 0; u<VHD_SECTORSIZE/8; u++)
		psector[0].qwords[u] = u;
		*/
	retcode = VhdReadSector(vhd, 0x0, &secno, psector);
	//retcode = VhdWriteSector(vhd, 0x0, &secno, psector);
	//delete psector;
	VhdCloseDisk(&vhd);
#elif 1
	// fixed disk with 8M
	retcode = VhdCreateFixedDisk(L"A:\\fix-temp.vhd", 1024*1024*8);
	// dynamic disk with 8M
	retcode = VhdCreateDynamicDisk(L"A:\\dyn-temp.vhd", 1024*1024*8);
	//_CrtDumpMemoryLeaks();
	// differencing disk with fixed disk parent
	retcode = VhdCreateDifferencingDisk(L"A:\\dif-fix-temp.vhd", L"A:\\fix-temp.vhd");
	// differencing disk with dynamic disk parent
	retcode = VhdCreateDifferencingDisk(L"A:\\dif-dyn-temp.vhd", L"A:\\dyn-temp.vhd");
	//_CrtDumpMemoryLeaks();
	// differencing disk with differencing disk parent
	retcode = VhdCreateDifferencingDisk(L"A:\\dif-dif-temp.vhd", L"A:\\dif-dyn-temp.vhd");
#endif
	/*HANDLE hFile = CreateFile(vhdFile, GENERIC_ALL, 0, NULL, CREATE_NEW, 0, NULL);
	if(!hFile || hFile == INVALID_HANDLE_VALUE)
		hFile = CreateFile(vhdFile, GENERIC_ALL, 0, NULL, OPEN_EXISTING, 0, NULL);
	VHDSECTOR *psec = new VHDSECTOR[12];
	if(hFile && hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dw;
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		ReadFile(hFile, psec, 1024*6, &dw, NULL);
		VHDSECTOR sector;
		memset(&sector, 0x7d, 512);		
		//DWORD ptr = SetFilePointer(hFile, 2048, 0, FILE_END);
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		WriteFile(hFile, &sector, 512, &dw, NULL);
		SetFilePointer(hFile, -512, NULL, FILE_END);
		WriteFile(hFile, &sector, 512, &dw, NULL);
		FlushFileBuffers(hFile);
		SetEndOfFile(hFile);
		CloseHandle(hFile);
	}
	delete[] psec;*/
	_CrtDumpMemoryLeaks();
	return (int)retcode;
}

#if 0
int _tmain(int argc, _TCHAR* argv[])
{
	if(argc != 2) {
		wprintf(L"vhdman vhdFilePath\n");
		return 0;
	}

	const wchar_t* vhdFile = //L"E:\\$ROOM\\Kubuntu.vhd";
		//L"E:\\$ROOM\\$temp.vhd";
		//L"D:\\PowerToy\\Xp\\444.vhd";
		//L"D:\\PowerToy\\Xp\\$temp-dyn.vhd";
		L"E:\\$ROOM\\$temp-dyn.vhd";
		//argv[1];
	HVHD hVhd;
	VHDFOOTER footer = { 0 };
	DYNVHDHEADER header = { 0 };
	u4_t *pbats = NULL;
	u4_t batc;
	u1_t *pblock0 = NULL;
	u4_t cb;
	u4_t blksz;
	VHDSECTOR sector0 = { 0 };
	VHDSECTOR sector0x = { 0 };
	/*struct _4kdisk
	{
		VHDFOOTER footer0;
		DYNVHDHEADER header;
		VHDFOOTER footer1;
		VHDFOOTER footer2;
		VHDFOOTER footer3;
		VHDSECTOR sector;
		VHDFOOTER footer4;
	};
	_4kdisk disk;*/
	if(VhdOpenDisk(vhdFile, &hVhd))
	{		
		VhdReadFooter(hVhd, &footer);
		if(footer.diskType == _swapEndian4(VHD_FOOTER_DISKTYPE_DYNAMIC))
		{
			VhdReadDynDiskHeader(hVhd, &header);
			batc = _swapEndian4(header.maxTableEntries);
			pbats = (u4_t*)malloc(batc * 4);
			VhdReadDynDiskBAT(hVhd, &header, pbats, &batc);
			for(u4_t u=0; u<batc; u++)
			{
				pbats[u] = _swapEndian4(pbats[u]);
			}
			cb = (pbats[1]-pbats[0]) * 512;
			pblock0 = (u1_t*)malloc(cb);
			VhdReadDynDiskBlock(hVhd, pbats[0], cb, pblock0);
			blksz = _swapEndian4(header.blockSize);
			VhdReadDynDiskSector(hVhd, pbats, VhdDynDiskCalculateBlockBmpSectorCount(blksz), blksz / VHD_SECTORSIZE, 0, &sector0);
			VhdReadDynDiskSectorEx(hVhd, &header, 0, &sector0x);
			/*SetFilePointer(hVhd, 0, NULL, FILE_BEGIN);
			ReadFile(hVhd, &disk, 1024 * 4, (LPDWORD)&blksz, NULL);*/
		}
		VhdCloseDisk(hVhd);
	}
	
	u8_t size = _swapEndian8(footer.currentSize);
	
	free(pbats);
	free(pblock0);

	//VhdCreateFixedDisk(L"E:\\$ROOM\\$temp.vhd", 1024 * 1024 * 16);
	VhdCreateDynamicDisk(L"E:\\$ROOM\\$temp-dyn.vhd", 1024 * 1024 * 16);
	return argc;
}

#endif
