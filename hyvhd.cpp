#include "stdafx.h"
#include "hyvhd.h"
#include <time.h>
#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

#ifdef _DEBUG
#define VHDASSERT(x, ...) \
	if(!(x)){ __debugbreak(); }
#endif

#ifdef _OPTION_BIGENDIAN
union U8AND4 {
	u8_t u8;
	struct {
		u4_t h4;
		u4_t l4;
	};
}; 
#else
union U8AND4 {
	u8_t u8;
	struct {
		u4_t l4;
		u4_t h4;
	};
}; 
#endif

// seconds after Jan 1, 1970 00:00:00 GMT/UTC
const __time64_t _tm_2000_1_1_12_0_0_gmt = 0x00000000386D4380ll;
// 100-nanoseconds after Jan 1, 1601 00:00:00 GMT/UTC
const u8_t       _ft_2000_1_1_12_0_0_gmt = 0x01BF53EB256D4000ll;

__forceinline
void vhdtm2crttime(VHDTIME &vhdtm, tm *pTime)
{
	pTime->tm_year = vhdtm.year - 1900;
	pTime->tm_mon = vhdtm.month - 1;
	pTime->tm_mday = vhdtm.day;
	pTime->tm_hour = vhdtm.hour;
	pTime->tm_min = vhdtm.minute;
	pTime->tm_sec = vhdtm.second;
}

__hyvhdapi
u4_t __stdcall VhdConvertVhdTime(VHDTIME *pTime)
{
	tm crttm;
	vhdtm2crttime(*pTime, &crttm);
	__time64_t tm;
	_gmtime64_s(&crttm, &tm);
	return tm - _tm_2000_1_1_12_0_0_gmt;
}

__hyvhdapi
u4_t __stdcall VhdConvertFileTime(FILETIME *pTime)
{
	union TM
	{
		FILETIME f;
		u8_t     u;
	};
	TM tm = { *pTime };
	return (tm.u - _ft_2000_1_1_12_0_0_gmt) / 10000000;
}

__hyvhdapi
u4_t __stdcall VhdGetTimestamp(void)
{
	__time64_t current;
	_time64(&current);
	return current - _tm_2000_1_1_12_0_0_gmt;
}

__forceinline
void crttm2vhdtime(tm &systm, VHDTIME *pTime)
{
	pTime->year = systm.tm_year + 1900;
	pTime->month = systm.tm_mon + 1;
	pTime->day = systm.tm_mday;
	pTime->hour = systm.tm_hour;
	pTime->minute = systm.tm_min;
	pTime->second = systm.tm_sec;
}

__hyvhdapi
void __stdcall VhdMakeGmtTime(u4_t timestamp, VHDTIME *pTime)
{
	__time64_t time = timestamp + _tm_2000_1_1_12_0_0_gmt;
	tm systm;
	_gmtime64_s(&systm, &time);
	crttm2vhdtime(systm, pTime);
}

__hyvhdapi
void __stdcall VhdMakeLocalTime(u4_t timestamp, __in VHDTIME *pTime)
{
	__time64_t time = timestamp + _tm_2000_1_1_12_0_0_gmt;
	tm systm;
	_localtime64_s(&systm, &time);
	crttm2vhdtime(systm, pTime);
}

__hyvhdapi
u4_t __stdcall VhdFooterCalculateChecksum(PVHDFOOTER footer)
{
	u4_t oldcheck = footer->checksum;
	u4_t checksum = 0;
	footer->checksum = 0;
	u1_t *psb = (u1_t*)footer;
	for (u2_t u = 0 ; u < sizeof(VHDFOOTER); u++)
	{
		checksum += psb[u];
	}
	footer->checksum = oldcheck;
	return ~checksum;
}

__hyvhdapi
u4_t __stdcall VhdHeaderCalculateChecksum(PDYNVHDHEADER header)
{
	u4_t oldcheck = header->checksum;
	u4_t checksum = 0;
	header->checksum = 0;
	u1_t *psb = (u1_t*)header;
	for (u2_t u = 0 ; u < sizeof(DYNVHDHEADER); u++)
	{
		checksum += psb[u];
	}
	header->checksum = oldcheck;
	return ~checksum;
}

__hyvhdapi
u4_t __stdcall VhdCalculateCHS(u4_t totalSectors)
{	
	DISKCHS ret;
	u4_t sectorPerTrack;
	u4_t heads;
	u4_t cylinderTimesHeads;
	u4_t cylinders;

	if(totalSectors > 65536 * 16 * 255)
	{
		totalSectors = 65536 * 16 * 255;
	}

	if(totalSectors >= 65536 * 16 * 255)
	{
		sectorPerTrack = 255;
		heads = 16;
		cylinderTimesHeads = totalSectors / sectorPerTrack;
	}
	else
	{
		sectorPerTrack = 17;
		cylinderTimesHeads = totalSectors / sectorPerTrack;
		heads = (cylinderTimesHeads + 1023) / 1024;
		if(heads < 4)
			heads = 4;
		if(cylinderTimesHeads >= (heads * 1024) || heads > 16)
		{
			sectorPerTrack = 31;
			heads = 16;
			cylinderTimesHeads = totalSectors / sectorPerTrack;
		}
		if(cylinderTimesHeads >= (heads * 1024))
		{
			sectorPerTrack = 63;
			heads = 16;
			cylinderTimesHeads = totalSectors / sectorPerTrack;
		}
	}
	cylinders = cylinderTimesHeads / heads;

	ret.s.cylinder = cylinders;
	ret.s.heads = heads;
	ret.s.secpertrack = sectorPerTrack;
	return ret.u;
}

typedef HANDLE HVHD;

#pragma pack(push, 1)

struct _tagVHDIMAGE
{
#ifdef _OPTION_BIGENDIAN
	struct {
		u4_t c : 8;
		u4_t u : 24;
	} type;
#else
	struct {
		u4_t u : 24;
		u4_t c : 8;
	} type;
#endif
	HVHD hVhd;
	const wchar_t* filepath;
	VHDFOOTER footer;
	u4_t maxSectorIndex;
};

#ifdef _OPTION_BIGENDIAN
#define VHDIMG_TYPE_FIX_U 'Fix'
#define VHDIMG_TYPE_DYN_U 'Dyn'
#define VHDIMG_TYPE_DIF_U 'Dif'
#else
#define VHDIMG_TYPE_FIX_U 'xiF'
#define VHDIMG_TYPE_DYN_U 'nyD'
#define VHDIMG_TYPE_DIF_U 'fiD'
#endif

#define VHDIMG_SETPATHNEEDFREE(vhdptr) \
	vhdptr->type.c |= 0x80

#define VHDIMG_FREEPATH(vhdptr) \
	if(vhdptr->type.c & 0x80) { free((void*)vhdptr->filepath); }

#define VHDIMG_SETMODIFIED(vhdptr) \
	vhdptr->type.c |= 0x20
#define VHDIMG_SETEXPANDED(vhdptr) \
	vhdptr->type.c |= 0x60

#define IS_VHDIMG_MODIFIED(vhdptr) \
	(vhdptr->type.c & 0x20)
#define IS_VHDIMG_EXPANDED(vhdptr) \
	(vhdptr->type.c & 0x40)

struct _tagFIXVHDIMAGE : public _tagVHDIMAGE
{
};

struct _tagDYNVHDIMAGE : public _tagVHDIMAGE
{
	DYNVHDHEADER header;
	u4_t *pBAT;
	u4_t sectorPerBAT;
	u4_t sectorBmpSize; // the sector bitmap size
	u4_t sectorBlockSize; // the bmp + headr->blockSize
	_tagVHDIMAGE* pParent;
};

typedef struct _tagVHDIMAGE VHDIMGDATA;
typedef struct _tagVHDIMAGE *PVHDIMGDATA;

typedef _tagFIXVHDIMAGE FIXVHDIMG;
typedef _tagFIXVHDIMAGE *PFIXVHDIMG;

typedef _tagDYNVHDIMAGE DYNVHDIMG;
typedef _tagDYNVHDIMAGE *PDYNVHDIMG;

typedef DYNVHDIMG DIFVHDIMG;
typedef DYNVHDIMG *PDIFVHDIMG;

#pragma pack(pop)

bool _VhdReadFooter(HVHD hVhd, PVHDFOOTER pVhdFooter, bool readHead)
{
	DWORD read;
	if(readHead)
		SetFilePointer(hVhd, 0, NULL, FILE_BEGIN);
	else
		SetFilePointer(hVhd, -512, NULL, FILE_END);
	return ReadFile(hVhd, pVhdFooter, 512, &read, NULL);
}

#define IS_VALID_HVHD(hVhd) hVhd && (hVhd != INVALID_HANDLE_VALUE)

PFIXVHDIMG _VhdOpenFixDisk(PVHDIMGDATA imgdata, VHDRETCODE *pretcode)
{
	PFIXVHDIMG image = NULL;
	u4_t checksum = _swapEndian4(imgdata->footer.checksum);
	u4_t calcsum = VhdFooterCalculateChecksum(&imgdata->footer);
	if(calcsum == checksum)
	{
		image = (PFIXVHDIMG)malloc(sizeof(FIXVHDIMG));
		memcpy(image, imgdata, sizeof(VHDIMGDATA));
		image->maxSectorIndex = _swapEndian4(image->footer.currentSize) / VHD_SECTORSIZE;		
	}
	else
	{
		*pretcode = RETCODE_F(VHDCODE_FOOTERCHECKSUM);
	}
	return image;
}

PDYNVHDIMG __VhdOpenDynDisk(PVHDIMGDATA imgdata, VHDRETCODE *pretcode)
{
	PDYNVHDIMG image = NULL;
	u4_t checksum = _swapEndian4(imgdata->footer.checksum);
	u4_t calcsum = VhdFooterCalculateChecksum(&imgdata->footer);

	u8_t offset;
	u4_t lo, hi;
	if(calcsum == checksum)
	{
		image = (PDYNVHDIMG)malloc(sizeof(DYNVHDIMG));
		SetFilePointer(imgdata->hVhd, 512, NULL, FILE_BEGIN);
		ReadFile(imgdata->hVhd, &image->header, 1024, (LPDWORD)&hi, NULL);		
		checksum = _swapEndian4(image->header.checksum);
		calcsum = VhdHeaderCalculateChecksum(&image->header);
		if(checksum == calcsum)
		{
			lo = _swapEndian4(image->header.maxTableEntries) * 4;
			image->pBAT = (u4_t*)malloc(lo);
			offset = _swapEndian8(image->header.tableOffset);
			hi = (offset >> 32);
			SetFilePointer(imgdata->hVhd, offset, (PLONG)&hi, FILE_BEGIN);
			ReadFile(imgdata->hVhd, image->pBAT, lo, (LPDWORD)&hi, NULL);
			memcpy(image, imgdata, sizeof(VHDIMGDATA));	
			image->maxSectorIndex = _swapEndian8(image->footer.currentSize) / VHD_SECTORSIZE;
			image->sectorPerBAT = _swapEndian4(image->header.blockSize) / VHD_SECTORSIZE;
			image->sectorBmpSize = image->sectorPerBAT / 8;
			if(image->sectorBmpSize < VHD_SECTORSIZE)
				image->sectorBmpSize = VHD_SECTORSIZE;
			else if(image->sectorPerBAT % 8)
				image->sectorBmpSize += VHD_SECTORSIZE;
			image->sectorBlockSize = image->sectorPerBAT * VHD_SECTORSIZE + image->sectorBmpSize;
			image->pParent = NULL;
		}
		else
		{
			free(image); image = NULL;
			*pretcode = RETCODE_F(VHDCODE_HEADERCHECKSUM);
		}
	}
	return image;
}

PDYNVHDIMG _VhdOpenDynDisk(PVHDIMGDATA imgdata, VHDRETCODE *pretcode)
{
	PDYNVHDIMG image = __VhdOpenDynDisk(imgdata, pretcode);
	u4_t hi;
	if(!image && (*pretcode == RETCODE_F(VHDCODE_FOOTERCHECKSUM)))
	{
		SetFilePointer(imgdata->hVhd, 0, NULL, FILE_BEGIN);
		ReadFile(imgdata->hVhd, &imgdata->footer, 512, (LPDWORD)&hi, NULL);
		switch(imgdata->footer.diskType)
		{
		case VHD_FOOTER_DISKTYPE_DYNAMIC:
		case VHD_FOOTER_DISKTYPE_DIFFERENCING:
			image = __VhdOpenDynDisk(imgdata, pretcode);
			break;
		default:
			*pretcode = RETCODE_F(VHDCODE_FILE);
			break;
		}
	}
	return image;
}

wchar_t* _VhdReadDifParentLocator(PVHDIMGDATA imgdata, PVHDPARENTLOCATOR locator)
{
	U8AND4 offset;
	offset.u8 = _swapEndian8(locator->platformDataOffset);
	u4_t len = _swapEndian4(locator->platformDataLength);
	if(len<1024) len = 1024;	
	SetFilePointer(imgdata->hVhd, offset.l4, (PLONG)&offset.h4, FILE_BEGIN);
	wchar_t* file = (wchar_t*)malloc(len);
	ReadFile(imgdata->hVhd, file, len, (LPDWORD)&offset.h4, NULL);
	if(locator->platformCode.u == VHD_DYNHEADER_PLATFORM_WINNTR)
	{
		wchar_t *tmp[2];
		tmp[1] = NULL;
		tmp[0] = (wchar_t*)malloc(wcslen(imgdata->filepath) * sizeof(wchar_t) + sizeof(wchar_t));
		wcscpy(tmp[0], imgdata->filepath);
		PathRemoveFileSpec(tmp[0]);
		PathResolve(file, (PZPCWSTR)tmp, PRF_FIRSTDIRDEF|PRF_DONTFINDLNK|PRF_REQUIREABSOLUTE);
		free(tmp[0]);
	}
	return file;
}

bool _VhdCheckDifParentTimestamp(PDIFVHDIMG image, VHDIMAGE parentImage)
{
	FILETIME writeTime;
	GetFileTime(parentImage->hVhd, NULL, NULL, &writeTime);
	SYSTEMTIME systm;
	FileTimeToSystemTime(&writeTime, &systm);
	
	VHDTIME vhdtm;
	VhdMakeGmtTimeSwap(image->header.parentModifiedTime, &vhdtm);

	return (systm.wYear == vhdtm.year) && 
		(systm.wMonth == vhdtm.month) &&
		(systm.wDay == vhdtm.day) &&
		(systm.wHour == vhdtm.hour) &&
		(systm.wMinute == vhdtm.minute) &&
		(systm.wSecond == vhdtm.second);
}

PVHDIMGDATA _VhdOpenDifParentDisk(PDYNVHDIMG image, VHDRETCODE *pretcode)
{
	PVHDPARENTLOCATOR w2ru = NULL, w2ku = NULL;
	for(int i = 0; i<8; i++)
	{
		switch(image->header.parentLocatorEntries[i].platformCode.u)
		{
		case VHD_DYNHEADER_PLATFORM_WINNTR:
			w2ru = image->header.parentLocatorEntries + i; break;
		case VHD_DYNHEADER_PLATFORM_WINNTK:
			w2ku = image->header.parentLocatorEntries + i; break;
		}
	}
	wchar_t * file = NULL;
	if(w2ru && !w2ku)
		file = _VhdReadDifParentLocator(image, w2ru);
	else if(w2ku)
		file = _VhdReadDifParentLocator(image, w2ku);
	
	VHDIMAGE parentImage = NULL;
	if(file)
	{
		if(PathFileExists(file))
		{
			if(!PathIsDirectory(file))
			{
				// check if the parent is readonly
				if(!(GetFileAttributes(file) & FILE_ATTRIBUTE_READONLY))
				{
					*pretcode = RETCODE_F(VHDCODE_MUTABLEPARENT);
					free(file);
					return parentImage;
				}
				// open the disk
				// parent disk is never wrote, set readonly flag always
				parentImage = (PDYNVHDIMG)VhdOpenDisk(file, VOF_READONLY, pretcode);				
				// check guid and timestamp
				if(!_VhdCheckDifParentTimestamp(image, parentImage) ||
					parentImage->footer.uniqueid.u8[0] != image->header.parentId.u8[0] ||
					parentImage->footer.uniqueid.u8[1] != image->header.parentId.u8[1])
				{
					VhdCloseDisk((PVHDIMAGE)&parentImage);
					*pretcode = RETCODE_F(VHDCODE_INVALIDPARENT);
				}
				else
				{
					VHDIMG_SETPATHNEEDFREE(parentImage);
					return parentImage;
				}
			}
		}
		free(file);
	}
	return parentImage;
}

PDIFVHDIMG _VhdOpenDifDisk(PVHDIMGDATA imgdata, VHDRETCODE *pretcode)
{
	PDIFVHDIMG image = _VhdOpenDynDisk(imgdata, pretcode);
	if(!(imgdata->type.c & VOF_IGNOREPARENT) && image)
	{
		image->pParent = _VhdOpenDifParentDisk(image, pretcode);		
	}
	return image;
}

__hyvhdapi
VHDIMAGE VhdOpenDisk(const wchar_t* fullPath, VHDOPENFLAG flags, VHDRETCODE *pretcode)
{
	*pretcode = RETCODE_S(VHDCODE_NONE);
	if(((flags & VOF_IGNOREPARENT) && !(flags & VOF_READONLY)) ||
		((flags & VOF_ALLOCONREAD) && (flags & VOF_READONLY)))
	{
		*pretcode = RETCODE_F(VHDCODE_FLAGS);
		return NULL;
	}
	
	// check readonly
	if((GetFileAttributes(fullPath) & FILE_ATTRIBUTE_READONLY) && !(flags & VOF_READONLY))
	{
		*pretcode = RETCODE_F(VHDCODE_ACCESSDENY);
		return NULL;
	}

	VHDIMAGE image = NULL;
	VHDIMGDATA vhd;	
	SetLastError(0);
	vhd.hVhd = CreateFile(fullPath, (flags & VOF_READONLY) ? GENERIC_READ : GENERIC_ALL, 
		(flags & VOF_READONLY) ? FILE_SHARE_READ : 0, NULL, OPEN_EXISTING, 0, NULL);
	DWORD err = GetLastError();
	if(IS_VALID_HVHD(vhd.hVhd))
	{
		if(_VhdReadFooter(vhd.hVhd, &vhd.footer, false))
		{
			if(vhd.footer.cookieValue != VHD_FOOTER_COOKIEVALUE_MICROSOFT)
			{
				if(_VhdReadFooter(vhd.hVhd, &vhd.footer, true))
				{
					if(vhd.footer.cookieValue != VHD_FOOTER_COOKIEVALUE_MICROSOFT)
					{
						// footer corrupt
						CloseHandle(vhd.hVhd);
						*pretcode = RETCODE_F(VHDCODE_BADIMAGE);
						return NULL;
					}
				}
			}
			vhd.filepath = fullPath;
			vhd.type.c = flags;
			switch(vhd.footer.diskType)
			{
			case VHD_FOOTER_DISKTYPE_FIXED: 
				vhd.type.u = VHDIMG_TYPE_FIX_U;
				image = _VhdOpenFixDisk(&vhd, pretcode); break;
			case VHD_FOOTER_DISKTYPE_DYNAMIC: 
				vhd.type.u = VHDIMG_TYPE_DYN_U;
				image = _VhdOpenDynDisk(&vhd, pretcode); break;
			case VHD_FOOTER_DISKTYPE_DIFFERENCING: 
				vhd.type.u = VHDIMG_TYPE_DIF_U;
				image = _VhdOpenDifDisk(&vhd, pretcode); break;
			}
		}
		if(image && VHD_F(*pretcode))
		{
			VhdCloseDisk(&image);
		}
	}
	else
	{
		*pretcode = RETCODE_F(VHDCODE_FILE);
	}
	return image;
}

void _VhdCloseFixDisk(PFIXVHDIMG fiximg, bool writeBack)
{
	if(writeBack && IS_VHDIMG_MODIFIED(fiximg))
	{		
		VHDASSERT(false);
		/*fiximg->footer.checksum = _swapEndian4(VhdFooterCalculateChecksum(&fiximg->footer));
		SetFilePointer(fiximg->hVhd, -512, NULL, FILE_END);
		DWORD wrote;
		WriteFile(fiximg, &fiximg->footer, 512, &wrote, NULL);*/
		/*FlushFileBuffers(fiximg->hVhd);
		SetEndOfFile(fiximg->hVhd);*/
	}
	VHDIMG_FREEPATH(fiximg);
	CloseHandle(fiximg->hVhd);
	free(fiximg);
}

void _VhdCloseDynDisk(PDYNVHDIMG dynimg, bool writeBack)
{
	if(writeBack && IS_VHDIMG_MODIFIED(dynimg))
	{
		VHDASSERT(false);
/*
		U8AND4 offset;
		// update BAT
		offset.u8 = _swapEndian8(dynimg->header.tableOffset);
		SetFilePointer(dynimg->hVhd, offset.l4, (PLONG)&offset.h4, FILE_BEGIN);
		WriteFile(dynimg, dynimg->pBAT, _swapEndian4(dynimg->header.maxTableEntries) * 4, (LPDWORD)&offset.h4, NULL);

		if(IS_VHDIMG_EXPANDED(dynimg))
		{
			// update footer
			dynimg->footer.checksum = _swapEndian4(VhdFooterCalculateChecksum(&dynimg->footer));
			// the end of the file
			SetFilePointer(dynimg->hVhd, 0, NULL, FILE_END);
			WriteFile(dynimg, &dynimg->footer, 512, (LPDWORD)&offset.h4, NULL); // footer
			// the begining of the file
			SetFilePointer(dynimg->hVhd, 0, NULL, FILE_BEGIN);
			WriteFile(dynimg, &dynimg->footer, 512, (LPDWORD)&hi, NULL); // copy of footer
		}
*/
	}
	// free BAT
	free(dynimg->pBAT);
	VHDIMG_FREEPATH(dynimg);
	CloseHandle(dynimg->hVhd);
	free(dynimg);
}

void _VhdCloseDifParentDisk(PVHDIMGDATA vhdimg)
{
	if(vhdimg == NULL) return;
	
	switch(vhdimg->type.u)
	{
	case VHDIMG_TYPE_DIF_U: 
		_VhdCloseDifParentDisk(((PDIFVHDIMG)vhdimg)->pParent); 
	case VHDIMG_TYPE_DYN_U: 
		_VhdCloseDynDisk((PDYNVHDIMG)vhdimg, false);
		break;
	case VHDIMG_TYPE_FIX_U:
		_VhdCloseFixDisk((PFIXVHDIMG)vhdimg, false);
		break;
	}
}

void _VhdCloseDifDisk(PDIFVHDIMG difimg)
{
	if(difimg == NULL) return;

	// Close parent
	_VhdCloseDifParentDisk(difimg->pParent);

	// Close this file
	_VhdCloseDynDisk(difimg, true);
}

__hyvhdapi
void __stdcall VhdCloseDisk(PVHDIMAGE pVhd)
{
	if(pVhd == NULL) return;
	VHDIMAGE vhd = *pVhd;
	if(vhd == NULL) return;

	switch(vhd->type.u)
	{
	case VHDIMG_TYPE_FIX_U: _VhdCloseFixDisk((PFIXVHDIMG)vhd, true); break;
	case VHDIMG_TYPE_DYN_U: _VhdCloseDynDisk((PDYNVHDIMG)vhd, true); break;
	case VHDIMG_TYPE_DIF_U: _VhdCloseDifDisk((PDIFVHDIMG)vhd); break;
	}
	*pVhd = NULL;
}

//=========================================

__hyvhdapi
VHDRETCODE __stdcall VhdCreateFixedDisk(
	const wchar_t* vhdLocation, u8_t diskSize)
{
	if((diskSize % 0x200) != 0)
		return RETCODE_F(VHDCODE_DISKSIZE);

	HVHD hVhd = CreateFile(vhdLocation, GENERIC_ALL, 0, NULL, CREATE_NEW, 0, NULL);
	if(!IS_VALID_HVHD(hVhd))
		return RETCODE_F(VHDCODE_ACCESSDENY);

	VHDFOOTER footer = { 0 };
	u4_t writen;
	footer.cookieValue = VHD_FOOTER_COOKIEVALUE_MICROSOFT;
	footer.createdTime = VhdGetTimestampSwap();
	footer.dataOffset = 0xFFFFFFFFFFFFFFFF;
	footer.features = VHD_FOOTER_FEATURE_RESERVED;
	footer.creatorApplication = 'dhyh';
	footer.creatorHostOs = 'k2iW';
	footer.creatorVersion = 0x00000100;		
	footer.diskGeometry.u = VhdCalculateCHS(diskSize / VHD_SECTORSIZE);
	footer.diskGeometry.s.cylinder = _swapEndian2(footer.diskGeometry.s.cylinder);
	footer.diskType = VHD_FOOTER_DISKTYPE_FIXED;
	footer.originSize = _swapEndian8(diskSize);
	footer.currentSize = footer.originSize;
	footer.formatVersion = VHD_FOOTER_FORMATVERSION;
	UuidCreate((UUID*)&footer.uniqueid);
	footer.checksum = _swapEndian4(VhdFooterCalculateChecksum(&footer));
	
#ifdef _OPTION_CREATE_WRITEDISK
	VHDSECTOR sector = { 0 };
	for(u8_t u=0; u < diskSize / VHD_SECTORSIZE; u++)
	{
		WriteFile(hVhd, &sector, VHD_SECTORSIZE, (LPDWORD)&writen, NULL);
	}
#else
	u4_t lo = diskSize, hi = (diskSize >> 32);
	SetFilePointer(hVhd, lo, (PLONG)&hi, FILE_BEGIN);
#endif

	WriteFile(hVhd, &footer, sizeof(footer), (LPDWORD)&writen, NULL);
	FlushFileBuffers(hVhd);
	SetEndOfFile(hVhd);
	CloseHandle(hVhd);
	return RETCODE_S(VHDCODE_NONE);
}

typedef void (*SetParentVHDInfoCallback)(PDYNVHDHEADER pHeader, void *pState);
typedef bool (*SetParentLocationCallback)(u4_t platformCode, PVHDSECTOR pBuffer, void *pState);

struct DYNAMICDISKMODIFIER
{
	void *pState;
	SetParentVHDInfoCallback setParentInfo;
	SetParentLocationCallback setParentLocation;
};

VHDRETCODE _VhdCreateDynamicDisk(
	const wchar_t* vhdLocation, u8_t diskSize, DYNAMICDISKMODIFIER *pModifier)
{
	if((diskSize % 0x200) != 0)
		return RETCODE_F(VHDCODE_DISKSIZE);
	
	HVHD hVhd = CreateFile(vhdLocation, GENERIC_ALL, 0, NULL, CREATE_NEW, 0, NULL);
	if(!IS_VALID_HVHD(hVhd))
		return RETCODE_F(VHDCODE_ACCESSDENY);
	
	VHDFOOTER footer = { 0 };
	DYNVHDHEADER header = { 0 };
		
	footer.cookieValue = VHD_FOOTER_COOKIEVALUE_MICROSOFT;
	footer.createdTime = VhdGetTimestampSwap();
	footer.features = VHD_FOOTER_FEATURE_RESERVED;
	footer.dataOffset = sizeof(footer);
	footer.dataOffset = _swapEndian8(footer.dataOffset);
	footer.creatorApplication = 'dhyh';
	footer.creatorHostOs = 'k2iW';
	footer.creatorVersion = 0x00000100;
	footer.diskGeometry.u = VhdCalculateCHS(diskSize / VHD_SECTORSIZE);
	footer.diskGeometry.s.cylinder = _swapEndian2(footer.diskGeometry.s.cylinder);
	if(pModifier)
		footer.diskType = VHD_FOOTER_DISKTYPE_DIFFERENCING;
	else
		footer.diskType = VHD_FOOTER_DISKTYPE_DYNAMIC;
	footer.originSize = _swapEndian8(diskSize);
	footer.currentSize = footer.originSize;
	footer.formatVersion = VHD_FOOTER_FORMATVERSION;
	UuidCreate((UUID*)&footer.uniqueid);
	footer.checksum = _swapEndian4(VhdFooterCalculateChecksum(&footer));
		
	header.cookieValue = VHD_DYNHEADER_COOKIE_VALUE;		
	header.tableOffset = sizeof(footer)+sizeof(header);//+sizeof(footer)*3;	
	header.dataOffset = 0xFFFFFFFFFFFFFFFF;
	header.maxTableEntries = _swapEndian4(header.maxTableEntries);		
	header.headerVersion = VHD_DYNHEADER_VERSION;
	header.blockSize = 0;
	header.maxTableEntries = 0;
	if(pModifier && pModifier->setParentInfo)
	{
		pModifier->setParentInfo(&header, pModifier->pState);
	}
	else
	{
		header.blockSize = VHD_DYNHEADER_BLOCKDEFSIZE;
		header.maxTableEntries = diskSize / VHD_DEF_BLOCKSIZE;
	}
	VHDASSERT(header.blockSize != 0, "BlockSize");
	VHDASSERT(header.maxTableEntries != 0, "maxTableEntries");

	header.tableOffset = _swapEndian8(header.tableOffset);
	header.checksum = _swapEndian4(VhdHeaderCalculateChecksum(&header));

	u4_t lo = 0xFFFFFFFF, hi;
	//write footer and header
	WriteFile(hVhd, &footer, sizeof(footer), (LPDWORD)&hi, NULL);
	WriteFile(hVhd, &header, sizeof(header), (LPDWORD)&hi, NULL);
	/*WriteFile(hVhd, &footer, sizeof(footer), (LPDWORD)&hi, NULL);
	WriteFile(hVhd, &footer, sizeof(footer), (LPDWORD)&hi, NULL);
	WriteFile(hVhd, &footer, sizeof(footer), (LPDWORD)&hi, NULL);*/
		
	// write parent location data
	if(pModifier && pModifier->setParentLocation)
	{
		void *ptr = NULL; 
		for(int i = 0; i<8; i++)
		{
			if(header.parentLocatorEntries[i].platformCode.u != VHD_DYNHEADER_PLATFORM_NONE)
			{
				lo = _swapEndian4(header.parentLocatorEntries[i].platformDataSpace) * VHD_SECTORSIZE;
				ptr = malloc(lo);
				if(pModifier->setParentLocation(header.parentLocatorEntries[i].platformCode.u, (PVHDSECTOR)ptr, pModifier->pState))
				{
					WriteFile(hVhd, ptr, lo, (LPDWORD)&hi, NULL);
				}
				free(ptr);
			}
		}
	}

	//write BAT
	if(pModifier)
		hi = diskSize / _swapEndian4(header.blockSize) * 4;
	else
		hi = diskSize / VHD_DEF_BLOCKSIZE * 4;
	lo = hi / VHD_SECTORSIZE;
	lo = lo * VHD_SECTORSIZE;
	if(lo < hi)
	{
		lo += VHD_SECTORSIZE;
	}
	u4_t data = 0xFFFFFFFF;
	for(u8_t u = 0; u < lo / 4; u++)
	{			
		WriteFile(hVhd, &data, 4, (LPDWORD)&hi, NULL);
	}
		
	//write footer
	WriteFile(hVhd, &footer, sizeof(footer), (LPDWORD)&hi, NULL);
	FlushFileBuffers(hVhd);
	SetEndOfFile(hVhd);
	CloseHandle(hVhd);
	return RETCODE_S(VHDCODE_NONE);
}

__hyvhdapi
VHDRETCODE __stdcall VhdCreateDynamicDisk(const wchar_t* vhdLocation, u8_t diskSize)
{
	return _VhdCreateDynamicDisk(vhdLocation, diskSize, NULL);
}

__forceinline
u4_t wcsbytelen(const wchar_t* lpsz)
{
	u4_t len = sizeof(wchar_t);
	for( ; *lpsz!='\0'; lpsz++)
		len += sizeof(wchar_t);
	return len;
}

struct CREATEDIFDISKUSERSTATE
{
	PVHDIMGDATA pParent;
	const wchar_t *vhdLocation;	
	VHDSECTOR w2ru;
	VHDSECTOR w2ku;
};

void _VhdCreateDifferencingDisk_SetHeader(PDYNVHDHEADER pHeader, void *pState)
{
	CREATEDIFDISKUSERSTATE *pData = (CREATEDIFDISKUSERSTATE*)pState;
	
	// get the last write time of parent
	FILETIME writeTime;
	GetFileTime(pData->pParent->hVhd, NULL, NULL, &writeTime);
	u4_t paretWriteTime = VhdConvertFileTimeSwap(&writeTime);
	VHDTIME vhdTm;
	VhdMakeGmtTimeSwap(paretWriteTime, &vhdTm);
	VhdMakeLocalTimeSwap(paretWriteTime, &vhdTm);

	pHeader->parentId = pData->pParent->footer.uniqueid;
	pHeader->parentModifiedTime = paretWriteTime;
	pHeader->parentLocatorEntries[0].platformCode.u = VHD_DYNHEADER_PLATFORM_WINNTR;
	PathRelativePathTo((LPTSTR)&(pData->w2ru), pData->vhdLocation, 0, pData->pParent->filepath, 0);
	pHeader->parentLocatorEntries[0].platformDataLength = _swapEndian4(wcsbytelen((const wchar_t*)&(pData->w2ru)));
	pHeader->parentLocatorEntries[0].platformDataOffset = pHeader->tableOffset;
	pHeader->parentLocatorEntries[0].platformDataOffset = _swapEndian8(pHeader->parentLocatorEntries[0].platformDataOffset);
	pHeader->parentLocatorEntries[0].platformDataSpace = _swapEndian4(1);
	pHeader->parentLocatorEntries[1].platformCode.u = VHD_DYNHEADER_PLATFORM_WINNTK;
	pHeader->parentLocatorEntries[1].platformDataLength = wcsbytelen(pData->pParent->filepath);
	CopyMemory(&pData->w2ku, pData->pParent->filepath, pHeader->parentLocatorEntries[1].platformDataLength);
	pHeader->parentLocatorEntries[1].platformDataLength = _swapEndian4(pHeader->parentLocatorEntries[1].platformDataLength);
	pHeader->parentLocatorEntries[1].platformDataOffset = pHeader->tableOffset + 512;
	pHeader->parentLocatorEntries[1].platformDataOffset = _swapEndian8(pHeader->parentLocatorEntries[1].platformDataOffset);
	pHeader->parentLocatorEntries[1].platformDataSpace = _swapEndian4(1);

	pHeader->tableOffset += (512 * 2);

	switch(pData->pParent->type.u)		
	{
	case VHDIMG_TYPE_DYN_U:
	case VHDIMG_TYPE_DIF_U:
		pHeader->blockSize = ((PDYNVHDIMG)(pData->pParent))->header.blockSize;
		break;
	case VHDIMG_TYPE_FIX_U:
	default:
		pHeader->blockSize = VHD_DYNHEADER_BLOCKDEFSIZE;
		break;
	}
	pHeader->maxTableEntries = _swapEndian4(
		_swapEndian8(pData->pParent->footer.currentSize) / _swapEndian4(pHeader->blockSize));
}

bool _VhdCreateDifferencingDisk_GetParentLocation(u4_t platformCode, PVHDSECTOR pBuffer, void *pState)
{
	CREATEDIFDISKUSERSTATE *pData = (CREATEDIFDISKUSERSTATE*)pState;
	switch(platformCode)
	{
	case VHD_DYNHEADER_PLATFORM_WINNTK: CopyMemory(pBuffer, &pData->w2ku, VHD_SECTORSIZE); return true;
	case VHD_DYNHEADER_PLATFORM_WINNTR: CopyMemory(pBuffer, &pData->w2ru, VHD_SECTORSIZE); return true;
	}
	return false;
}

__hyvhdapi
VHDRETCODE __stdcall VhdCreateDifferencingDisk(
	const wchar_t* vhdLocation, const wchar_t* parentVhd)
{
	VHDRETCODE retcode;
	PVHDIMGDATA parent = VhdOpenDisk(parentVhd, VHDOPENFLAG(VOF_READONLY|VOF_IGNOREPARENT), &retcode);
	if(!parent) return RETCODE_F(VHDCODE_INVALIDPARENT);

	CREATEDIFDISKUSERSTATE state = { 0 };
	state.vhdLocation = vhdLocation;
	state.pParent = parent;

	DYNAMICDISKMODIFIER modifier;
	modifier.pState = &state;
	modifier.setParentInfo = _VhdCreateDifferencingDisk_SetHeader;
	modifier.setParentLocation = _VhdCreateDifferencingDisk_GetParentLocation;
	retcode = _VhdCreateDynamicDisk(vhdLocation, 
		_swapEndian8(parent->footer.currentSize), &modifier);
	VhdCloseDisk(&parent);
	if(VHD_S(retcode))
	{		
		// some software(like VirtualBox) will modifidy the parent disk,
		// so set it readonly
		SetFileAttributes(parentVhd, GetFileAttributes(parentVhd) | FILE_ATTRIBUTE_READONLY);
	}	
	return retcode;
}

//=========================================

#pragma pack(push, 1)
struct VHDSECTORMASK
{
	u1_t bytes[1];

	inline bool get(u4_t sectorIndex) {	
		return (bytes[sectorIndex / 8] >> (sectorIndex % 8)) & 0x1;
	}

	inline void set(u4_t sectorIndex) {
		bytes[sectorIndex / 8] |= 0x1 << (sectorIndex % 8);
	}
};

struct VHDRWDISKDATA
{
	u4_t fixSectorNo; // the absolutely sector no
	u4_t fixSecCount; // the sectors to read
	u4_t bat; // the index of BAT
	u4_t firstSec; // [0, 65536], the first sector index in current block 
	u4_t lastP1Sec; // [1, 65537], the last plus 1 secotor index in current block
	VHDSECTORMASK *pSecmsk;
	void* pBlock; // the buffer to store Block
	PVHDSECTOR pBuffer; // the target sector buffer
};

#pragma pack(pop)

VHDRETCODE _VhdReadSector_Fix(__in PFIXVHDIMG vhd,
	__in u4_t firstSector, u4_t count, __in PVHDSECTOR buffer)
{	
	U8AND4 offset;
	offset.u8 = firstSector;
	offset.u8 *= VHD_SECTORSIZE;	
	SetFilePointer(vhd->hVhd, offset.l4, (PLONG)&offset.h4, FILE_BEGIN);
	offset.l4 = count;
	offset.l4 *= VHD_SECTORSIZE;
	ReadFile(vhd->hVhd, buffer, offset.l4, (LPDWORD)&offset.h4, NULL);
	VHDASSERT(offset.l4 == offset.h4, "readFile");
	return RETCODE_S(VHDCODE_NONE);
}

__inline
VHDRETCODE _VhdReadSector_Fix(__in PFIXVHDIMG vhd, __in VHDRWDISKDATA *pData)
{	
	return _VhdReadSector_Fix(vhd, pData->fixSectorNo, pData->fixSecCount, pData->pBuffer);
}

VHDRETCODE _VhdReadSector_DifBlock(__in PDIFVHDIMG vhd, __in VHDRWDISKDATA *pData)
{
	U8AND4 offset;
	offset.u8 = _swapEndian4(vhd->pBAT[pData->bat]);
	offset.u8 *= VHD_SECTORSIZE;
	SetFilePointer(vhd->hVhd, offset.l4, (PLONG)offset.h4, FILE_BEGIN);
	ReadFile(vhd->hVhd, pData->pBlock, vhd->sectorBlockSize, (LPDWORD)&offset.h4, NULL);
	VHDSECTORMASK *pmask = (VHDSECTORMASK*)pData->pBlock;
	PVHDSECTOR psec = (PVHDSECTOR)((u1_t*)pmask + vhd->sectorBmpSize);	
	u4_t secidx;
	for(u4_t u = pData->firstSec; u < pData->lastP1Sec; u++)
	{
		// only read doesn't represent in differencing disk
		if(!pData->pSecmsk->get(u))
		{
			if(pmask->get(u))
			{
				// representing, read
				secidx = u - pData->firstSec;
				CopyMemory(pData->pBuffer + secidx, psec + u, VHD_SECTORSIZE);
				pData->pSecmsk->set(secidx);
			}
		}
	}
	return RETCODE_S(VHDCODE_NONE);
}

VHDRETCODE _VhdReadSector_DynBlock(__in PDYNVHDIMG vhd, __in VHDRWDISKDATA *pData, bool isParentDisk)
{
	U8AND4 offset;
	if(vhd->pBAT[pData->bat] == 0xFFFFFFFF)
	{
		// parent disk is immutable
		if(isParentDisk)
			return RETCODE_S(VHDCODE_NONE);

		// hasn't allocated
		if(vhd->type.c & VOF_ALLOCONREAD)
		{
			// alloc block
			offset.l4 = -512;
			offset.h4 = 0;
			offset.l4 = SetFilePointer(vhd->hVhd, offset.l4, (PLONG)&offset.h4, FILE_END);
			vhd->pBAT[pData->bat] = _swapEndian4(u4_t(offset.u8 / VHD_SECTORSIZE));
			SetFilePointer(vhd->hVhd, vhd->sectorBlockSize + VHD_SECTORSIZE, NULL, FILE_CURRENT);
		}
		for(u4_t u = pData->firstSec; u < pData->lastP1Sec; u++)
		{
			ZeroMemory(pData->pBuffer, VHD_SECTORSIZE);
			pData->pBuffer++;
			pData->pSecmsk->set(u - pData->firstSec);
		}
		pData->pBuffer -= pData->fixSecCount;
	}
	else
	{
		_VhdReadSector_DifBlock(vhd, pData);
	}
	return RETCODE_S(VHDCODE_NONE);
}

VHDRETCODE _VhdReadSector_Dyn(__in PDYNVHDIMG vhd,
	__in u4_t firstSector, u4_t count, __in PVHDSECTOR buffer)
{
	VHDRWDISKDATA readData;
	readData.bat = firstSector / vhd->sectorPerBAT;
	readData.firstSec = firstSector % vhd->sectorPerBAT;
	readData.pBlock = malloc(vhd->sectorBlockSize);
	readData.pSecmsk = (VHDSECTORMASK*)malloc(vhd->sectorBmpSize);
	readData.pBuffer = buffer;
	while(count)
	{	
		ZeroMemory(readData.pSecmsk, vhd->sectorBmpSize);

		readData.fixSecCount = vhd->sectorPerBAT - readData.firstSec - 1;
		if(count < readData.fixSecCount)
			readData.fixSecCount = count;
		
		readData.lastP1Sec = readData.fixSecCount + readData.firstSec;
		
		_VhdReadSector_DynBlock(vhd, &readData, false);
		count -= readData.fixSecCount;
		for(u4_t u = 0; u<readData.firstSec; u++)
		{
			if(!readData.pSecmsk->get(u))
			{
				ZeroMemory(readData.pBuffer+u, VHD_SECTORSIZE);
			}
		}		
		readData.pBuffer += readData.fixSecCount;
		readData.firstSec = 0;
		readData.fixSectorNo += readData.fixSecCount;
		readData.bat++;
	}
	free(readData.pBlock);
	free(readData.pSecmsk);
	return RETCODE_S(VHDCODE_NONE);
}


VHDRETCODE __VhdReadSector_Dif(__in PDIFVHDIMG vhd, __in VHDRWDISKDATA *pData, bool isParentDisk)
{
	VHDRETCODE retcode;	
	switch(vhd->type.u)
	{
	case VHDIMG_TYPE_FIX_U: 
		retcode = _VhdReadSector_Fix((PFIXVHDIMG)vhd, pData);
		break;
	case VHDIMG_TYPE_DYN_U: 
		retcode = _VhdReadSector_DynBlock((PDYNVHDIMG)vhd, pData, isParentDisk);
		break;
	case VHDIMG_TYPE_DIF_U:
		retcode = _VhdReadSector_DynBlock((PDYNVHDIMG)vhd, pData, isParentDisk);
		retcode = __VhdReadSector_Dif((PDIFVHDIMG)vhd->pParent, pData, true);
		break;
	}
	return retcode;
}

VHDRETCODE _VhdReadSector_Dif(__in PDIFVHDIMG vhd,
	__in u4_t firstSector, u4_t count, __in PVHDSECTOR buffer)
{
	U8AND4 offset;

	VHDRWDISKDATA readData;
	readData.fixSectorNo = firstSector;
	readData.bat = firstSector / vhd->sectorPerBAT;
	readData.firstSec = firstSector % vhd->sectorPerBAT;;
	readData.lastP1Sec = vhd->sectorPerBAT;
	if(count < vhd->sectorPerBAT - readData.firstSec)
		readData.lastP1Sec = count;
	readData.pBlock = malloc(vhd->sectorBlockSize);
	readData.pSecmsk = (VHDSECTORMASK*)malloc(vhd->sectorBmpSize);
	readData.pBuffer = buffer;
	
	while(count)
	{
		ZeroMemory(readData.pSecmsk, vhd->sectorBmpSize);
		
		readData.fixSecCount = vhd->sectorPerBAT - readData.firstSec - 1;
		if(count < readData.fixSecCount)
			readData.fixSecCount = count;
		
		readData.lastP1Sec = readData.fixSecCount + readData.firstSec;

		__VhdReadSector_Dif(vhd, &readData, false);
		count -= readData.fixSecCount;
		for(u4_t u = 0; u<readData.fixSecCount; u++)
		{
			if(!readData.pSecmsk->get(u))
			{
				ZeroMemory(readData.pBuffer+u, VHD_SECTORSIZE);
			}
		}
		readData.pBuffer += readData.fixSecCount;
		readData.firstSec = 0;
		readData.fixSectorNo += readData.fixSecCount;
		readData.bat++;
	}
	free(readData.pBlock);
	free(readData.pSecmsk);
	return RETCODE_S(VHDCODE_NONE);
}

__hyvhdapi
VHDRETCODE __stdcall VhdReadSector(__in VHDIMAGE vhd,
	u4_t firstSector, __inout u4_t *pCount, __in PVHDSECTOR buffer)
{
	if(!vhd || !pCount || !buffer)
		return RETCODE_F(VHDCODE_ARGUMETENS);
		
	if(firstSector >= 0xFFFFFFFFul)
		return RETCODE_F(VHDCODE_OUTOFRANGE);
	
	if(*pCount == 0)
		return RETCODE_S(VHDCODE_NONE);
	else if(*pCount > 0xFFFFul)
		return RETCODE_F(VHDCODE_OUTOFRANGE);

	VHDRETCODE retcode;
	u4_t count = vhd->maxSectorIndex - firstSector;
	if(*pCount > count) *pCount = count;
	else count = *pCount;

	switch(vhd->type.u)
	{
	case VHDIMG_TYPE_FIX_U: retcode = _VhdReadSector_Fix((PFIXVHDIMG)vhd, firstSector, count, buffer); break;
	case VHDIMG_TYPE_DYN_U: retcode = _VhdReadSector_Dyn((PDYNVHDIMG)vhd, firstSector, count, buffer); break;
	case VHDIMG_TYPE_DIF_U: retcode = _VhdReadSector_Dif((PDIFVHDIMG)vhd, firstSector, count, buffer); break;
	default: retcode = RETCODE_F(VHDCODE_NONE); break;
	}
	return retcode;
}

VHDRETCODE _VhdWriteSector_Fix(__in PFIXVHDIMG vhd,
	u4_t firstSector, __in u4_t count, __in PVHDSECTOR buffer)
{
	U8AND4 offset;
	offset.u8 = firstSector;
	offset.u8 *= VHD_SECTORSIZE;
	SetFilePointer(vhd->hVhd, offset.l4, (PLONG)offset.h4, FILE_BEGIN);
	offset.l4 = count * VHD_SECTORSIZE;
	WriteFile(vhd->hVhd, buffer, offset.l4, (LPDWORD)&offset.h4, NULL);
	VHDASSERT(offset.l4 == offset.h4, "WriteFile");
	VHDIMG_SETMODIFIED(vhd);
	return RETCODE_S(VHDCODE_NONE);
}

VHDRETCODE _VhdWriteSector_DynBlock(__in PDYNVHDIMG vhd, __in VHDRWDISKDATA *pData)	
{
	U8AND4 offset;
	VHDSECTORMASK *pmask;
	PVHDSECTOR psec;
	if(vhd->pBAT[pData->bat] == 0xFFFFFFFF)
	{		
		// alloc block
		offset.u8 = -512;
		offset.l4 = SetFilePointer(vhd->hVhd, offset.l4, (PLONG)&offset.h4, FILE_END);
		vhd->pBAT[pData->bat] = _swapEndian4(u4_t(offset.u8 / VHD_SECTORSIZE));
		//SetFilePointer(vhd->hVhd, vhd->sectorBlockSize + VHD_SECTORSIZE, NULL, FILE_CURRENT);
		ZeroMemory(pData->pBlock, vhd->sectorBlockSize);
		VHDIMG_SETEXPANDED(vhd);
	}
	else
	{
		offset.u8 = _swapEndian4(vhd->pBAT[pData->bat]);
		offset.u8 *= VHD_SECTORSIZE;
		SetFilePointer(vhd->hVhd, offset.l4, (PLONG)offset.h4, FILE_BEGIN);
		ReadFile(vhd->hVhd, pData->pBlock, vhd->sectorBlockSize, (LPDWORD)&offset.h4, NULL);
		offset.u8 = -vhd->sectorBlockSize;
		SetFilePointer(vhd->hVhd, offset.l4, (PLONG)offset.h4, FILE_CURRENT);
		VHDIMG_SETMODIFIED(vhd);
	}
	pmask = (VHDSECTORMASK*)pData->pBlock;
	psec = (PVHDSECTOR)((u1_t*)pmask + vhd->sectorBmpSize);
	
	u4_t secidx;
	for(u4_t u = pData->firstSec; u < pData->lastP1Sec; u++)
	{
		secidx = u - pData->firstSec;
		CopyMemory(psec + u, pData->pBuffer + secidx, VHD_SECTORSIZE);
		pmask->set(u);
	}
	offset.u8 = vhd->sectorBlockSize;
	WriteFile(vhd->hVhd, pData->pBlock, offset.l4, (LPDWORD)&offset.h4, NULL);
	VHDASSERT(offset.l4 == offset.h4, "WriteFile");
	return RETCODE_S(VHDCODE_NONE);
}

VHDRETCODE _VhdWriteSector_Dyn(__in PDYNVHDIMG vhd,
	u4_t firstSector, __inout u4_t count, __in PVHDSECTOR buffer)
{
	U8AND4 offset;

	VHDRWDISKDATA readData;
	readData.fixSectorNo = firstSector;
	readData.bat = firstSector / vhd->sectorPerBAT;
	readData.firstSec = firstSector % vhd->sectorPerBAT;;
	readData.lastP1Sec = vhd->sectorPerBAT;
	if(count < vhd->sectorPerBAT - readData.firstSec)
		readData.lastP1Sec = count;
	readData.pBlock = malloc(vhd->sectorBlockSize);
	readData.pSecmsk = NULL;
	readData.pBuffer = buffer;
	
	while(count)
	{	
		readData.fixSecCount = vhd->sectorPerBAT - readData.firstSec - 1;
		if(count < readData.fixSecCount)
			readData.fixSecCount = count;
		
		readData.lastP1Sec = readData.fixSecCount + readData.firstSec;

		_VhdWriteSector_DynBlock(vhd, &readData);
		count -= readData.fixSecCount;
		readData.pBuffer += readData.fixSecCount;
		readData.firstSec = 0;
		readData.fixSectorNo += readData.fixSecCount;
		readData.bat++;
	}
	free(readData.pBlock);
	return RETCODE_S(VHDCODE_NONE);
}

__hyvhdapi
VHDRETCODE __stdcall VhdWriteSector(__in VHDIMAGE vhd,
	u4_t firstSector, __inout u4_t *pCount, __in PVHDSECTOR buffer)
{
	if(!vhd || !pCount || !buffer)
		return RETCODE_F(VHDCODE_ARGUMETENS);

	if(vhd->type.c & VOF_READONLY)
		return RETCODE_F(VHDCODE_ACCESSDENY);

	if(firstSector >= 0xFFFFFFFFul)
		return RETCODE_F(VHDCODE_OUTOFRANGE);
	
	if(*pCount == 0)
		return RETCODE_S(VHDCODE_NONE);
	else if(*pCount > 0xFFFFul)
		return RETCODE_F(VHDCODE_OUTOFRANGE);

	VHDRETCODE retcode;
	u4_t count = vhd->maxSectorIndex - firstSector;
	if(*pCount > count) *pCount = count;
	else count = *pCount;

	switch (vhd->type.u)
	{
	case VHDIMG_TYPE_FIX_U: retcode = _VhdWriteSector_Fix((PFIXVHDIMG)vhd, firstSector, count, buffer); break;
	case VHDIMG_TYPE_DYN_U: 
	case VHDIMG_TYPE_DIF_U: retcode = _VhdWriteSector_Dyn((PDYNVHDIMG)vhd, firstSector, count, buffer); break;
	default: retcode = RETCODE_F(VHDCODE_NONE); break;
	}
	return retcode;
}


