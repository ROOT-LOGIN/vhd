#ifndef ___HYVHD_H_INCLUDE___
#define ___HYVHD_H_INCLUDE___

// Compile options:
// _OPTION_BIGENDIAN
//    the target machine is bigendian, 
//    default to littlendian(x86)
// _OPTION_CREATE_WRITEDISK
//    init the disk data to zero when creating the disk, 
//    defaut is do nothing

#pragma comment(lib, "shlwapi.lib")

// mark up one function is a API(exported function)
#define __hyvhdapi

// one signed byte
typedef signed char       i1_t;
// one unsigned byte
typedef unsigned char     u1_t;
// two signed bytes
typedef signed short      i2_t;
// two unsigned bytes
typedef unsigned short    u2_t;
// four signed bytes
typedef signed long       i4_t;
// four unsigned bytes
typedef unsigned long     u4_t;
// eight signed bytes
typedef signed __int64    i8_t;
// eight unsigned bytes
typedef unsigned __int64  u8_t;

struct st1_t {
	u1_t u[1];
};

struct st2_t {
	u1_t u[2];
};

struct st4_t {
	u1_t u[4];
};

struct st8_t {
	u1_t u[8];
};

__forceinline
u2_t __fastcall _swapEndian2(u2_t value)
{
	u1_t v;
	u1_t* ary = (u1_t*)&value;
	v = ary[1]; ary[1] = ary[0]; ary[0] = v;
	return value;
}

__forceinline
u4_t __fastcall _swapEndian4(u4_t value)
{
	u1_t v;
	u1_t* ary = (u1_t*)&value;
	v = ary[3]; ary[3] = ary[0]; ary[0] = v;
	v = ary[2]; ary[2] = ary[1]; ary[1] = v;
	return value;
}

__forceinline
u8_t __fastcall _swapEndian8(u8_t value)
{
	u1_t v;
	u1_t* ary = (u1_t*)&value;
	v = ary[7]; ary[7] = ary[0]; ary[0] = v; 
	v = ary[6]; ary[6] = ary[1]; ary[1] = v; 
	v = ary[5]; ary[5] = ary[2]; ary[2] = v;
	v = ary[4]; ary[4] = ary[3]; ary[3] = v;
	return value;
}

#pragma pack(push, 1)

union guid_t {
	GUID guid;
	struct {
		u4_t data1;
		u2_t data2;
		u2_t data3;
		u1_t data4[8];
	};
	u8_t u8[2];
};

union _tagCHS {
	u4_t u;
	struct {
		u2_t cylinder;
		u1_t heads;
		u1_t secpertrack;
	} s;
};
typedef _tagCHS DISKCHS;
typedef _tagCHS *PDISKCHS;

/* the VHD sector size is always 512 */
#define VHD_SECTORSIZE 512

/* representing a sector  */
union _tagVHDSECTOR
{
	u1_t bytes[VHD_SECTORSIZE];
	u2_t words[VHD_SECTORSIZE / 2];
	u4_t dwords[VHD_SECTORSIZE / 4];
	u8_t qwords[VHD_SECTORSIZE / 8];
};
typedef _tagVHDSECTOR VHDSECTOR;
typedef _tagVHDSECTOR *PVHDSECTOR;

/* representing the VHD footer */
struct _tagVHDFOOTER {
	union {
		u1_t cookie[8];
		u8_t cookieValue;
	};	
	u4_t features;
	u4_t formatVersion;
	u8_t dataOffset;
	// Since Jan. 1st, 2000 12:00:00 AM UTC/GMT
	u4_t createdTime;
	u4_t creatorApplication;
	u4_t creatorVersion;
	u4_t creatorHostOs;
	u8_t originSize;
	u8_t currentSize;
	DISKCHS diskGeometry;
	u4_t diskType;
	u4_t checksum;
	guid_t uniqueid;
	u1_t savedState;
	u1_t _reserved[427];
};

typedef _tagVHDFOOTER VHDFOOTER;
typedef _tagVHDFOOTER *PVHDFOOTER;

#ifdef _OPTION_BIGENDIAN

#define VHD_FOOTER_COOKIEVALUE_MICROSOFT (((u8_t)'cone'<<32) | 'ctix')

#define VHD_FOOTER_FEATURE_NONE      0x0
// indicate this vhd is a candidate for deletion on shutdown
#define VHD_FOOTER_FEATURE_TEMPORARY 0x1
// must always be set
#define VHD_FOOTER_FEATURE_RESERVED  0x2

#define VHD_FOOTER_FORMATVERSION 0x00010000

#define VHD_FOOTER_DATAOFFSET_FIXED 0xFFFFFFFFul

#define VHD_FOOTER_CREATORAPP_VIRTUALPC     'vpc 'ul
#define VHD_FOOTER_CREATORAPP_VIRTUALSERVER 'vs  'ul

#define VHD_FOOTER_CREATORVER_VIRTUALPC     0x00010000ul
#define VHD_FOOTER_CREATORVER_VIRTUALSERVER 0x00050000ul

#define VHD_FOOTER_CREATORHOS_WIN 'Wi2k'ul
#define VHD_FOOTER_CREATORHOS_MAC 'Mac 'ul

#define VHD_FOOTER_DISKTYPE_NONE           0x0
#define VHD_FOOTER_DISKTYPE_RESERVEDv1     0x1
#define VHD_FOOTER_DISKTYPE_FIXED          0x2
#define VHD_FOOTER_DISKTYPE_DYNAMIC        0x3
#define VHD_FOOTER_DISKTYPE_DIFFERENCING   0x4
#define VHD_FOOTER_DISKTYPE_RESERVEDv5     0x5
#define VHD_FOOTER_DISKTYPE_RESERVEDv6     0x6
#else
#define VHD_FOOTER_COOKIEVALUE_MICROSOFT (((u8_t)'xitc'<<32) | 'enoc')

#define VHD_FOOTER_FEATURE_NONE      0x0
// indicate this vhd is a candidate for deletion on shutdown
#define VHD_FOOTER_FEATURE_TEMPORARY 0x01000000
// must always be set
#define VHD_FOOTER_FEATURE_RESERVED  0x02000000

#define VHD_FOOTER_FORMATVERSION 0x00000100

#define VHD_FOOTER_DATAOFFSET_FIXED 0xFFFFFFFFul

#define VHD_FOOTER_CREATORAPP_VIRTUALPC     ' cpv'ul
#define VHD_FOOTER_CREATORAPP_VIRTUALSERVER '  sv'ul

#define VHD_FOOTER_CREATORVER_VIRTUALPC     0x00000100ul
#define VHD_FOOTER_CREATORVER_VIRTUALSERVER 0x00000500ul

#define VHD_FOOTER_CREATORHOS_WIN 'k2iW'ul
#define VHD_FOOTER_CREATORHOS_MAC ' caM'ul

#define VHD_FOOTER_DISKTYPE_NONE           0x0
#define VHD_FOOTER_DISKTYPE_RESERVEDv1     0x01000000ul
#define VHD_FOOTER_DISKTYPE_FIXED          0x02000000ul
#define VHD_FOOTER_DISKTYPE_DYNAMIC        0x03000000ul
#define VHD_FOOTER_DISKTYPE_DIFFERENCING   0x04000000ul
#define VHD_FOOTER_DISKTYPE_RESERVEDv5     0x05000000ul
#define VHD_FOOTER_DISKTYPE_RESERVEDv6     0x06000000ul
#endif

/* representing the VHD parent locator */
struct _tagVHDPARENTLOCATOR {
	union {
		u4_t u;
		u1_t c[4];		
	} platformCode;	
	// the number of 512-byte sectors needed 
	// to store the parent hard disk locator
	u4_t platformDataSpace;
	// the actual length of the parent hard
	// disk locator in bytes
	u4_t platformDataLength;
	u4_t _reserverd;
	// the absolute file offset in bytes where the 
	// platform specific file locator data is stored.
	u8_t platformDataOffset;
};
typedef _tagVHDPARENTLOCATOR VHDPARENTLOCATOR;
typedef _tagVHDPARENTLOCATOR *PVHDPARENTLOCATOR;

/* representing the dynamic allocate VHD header */
struct _tagDYNVHDHEADER
{
	union {
		u1_t cookie[8];
		u8_t cookieValue;
	};
	// absolutely offset to the next data structure.
	u8_t dataOffset;
	// absolutely offset to the BAT
	u8_t tableOffset;
	u4_t headerVersion;
	u4_t maxTableEntries;
	u4_t blockSize;
	u4_t checksum;
	guid_t parentId;
	// Since Jan. 1st, 2000 12:00:00 AM UTC/GMT
	u4_t parentModifiedTime;
	u4_t _reserverd;
	u1_t parentUnicodeName[512];
	VHDPARENTLOCATOR parentLocatorEntries[8];
	u1_t _reserverd_f[256];
};

typedef _tagDYNVHDHEADER DYNVHDHEADER;
typedef _tagDYNVHDHEADER *PDYNVHDHEADER;

#ifdef _OPTION_BIGENDIAN
#define VHD_DYNHEADER_COOKIE_VALUE (((u8_t)'cxsp'<<32) | 'arse')

#define VHD_DYNHEADER_DATAOFFSET 0xFFFFFFFFul

#define VHD_DYNHEADER_VERSION 0x00010000ul

//indicating a block size of 2MB
#define VHD_DYNHEADER_BLOCKDEFSIZE 0x00200000

#define VHD_DYNHEADER_PLATFORM_NONE      (u4_t)0x0
#define VHD_DYNHEADER_PLATFORM_WIN98R    (u4_t)'Wi2r'
#define VHD_DYNHEADER_PLATFORM_WIN98K    (u4_t)'Wi2k'
#define VHD_DYNHEADER_PLATFORM_WINNTR    (u4_t)'W2ur'
#define VHD_DYNHEADER_PLATFORM_WINNTK    (u4_t)'W2uk'
#define VHD_DYNHEADER_PLATFORM_MAC       (u4_t)'Mac '
#define VHD_DYNHEADER_PLATFORM_MACX      (u4_t)'MacX'
#else
#define VHD_DYNHEADER_COOKIE_VALUE (((u8_t)'esra'<<32) | 'psxc')

#define VHD_DYNHEADER_DATAOFFSET 0xFFFFFFFFul

#define VHD_DYNHEADER_VERSION 0x00000100ul

//indicating a block size of 2MB
#define VHD_DYNHEADER_BLOCKDEFSIZE 0x00002000

#define VHD_DYNHEADER_PLATFORM_NONE      (u4_t)0x0
#define VHD_DYNHEADER_PLATFORM_WIN98R    (u4_t)'r2iW'
#define VHD_DYNHEADER_PLATFORM_WIN98K    (u4_t)'k2iW'
#define VHD_DYNHEADER_PLATFORM_WINNTR    (u4_t)'ur2W'
#define VHD_DYNHEADER_PLATFORM_WINNTK    (u4_t)'uk2W'
#define VHD_DYNHEADER_PLATFORM_MAC       (u4_t)' caM'
#define VHD_DYNHEADER_PLATFORM_MACX      (u4_t)'XcaM'
#endif

#define VHD_DEF_BLOCKSIZE 0x200000

// typdedef for the return code
typedef u4_t VHDRETCODE;

/* representing the decoded VHD timestamp */
struct VHDTIME
{
	u4_t year : 24;   // [2000, infinite)
	u1_t month;       // [1, 12]
	u1_t day;         // [1, 31]
	u1_t hour;        // [0, 23]
	u1_t minute;      // [0, 59]
	u1_t second;      // [0, 59]
};

#pragma pack(pop)

/* Gets the current timestamp for vhd footer and header */
__hyvhdapi
u4_t __stdcall 
VhdGetTimestamp(
	void
);

__hyvhdapi __forceinline
u4_t VhdGetTimestampSwap(void) {
	return _swapEndian4(VhdGetTimestamp());
}

/* Make VHDTIME by the timestamp representing in GMT/UTC */
__hyvhdapi
void __stdcall 
VhdMakeGmtTime(
	u4_t timestamp, 
	__in VHDTIME *pTime
);

__hyvhdapi __forceinline
void VhdMakeGmtTimeSwap(u4_t timestamp, __in VHDTIME *pTime) {
	return VhdMakeGmtTime(_swapEndian4(timestamp), pTime);
}

/* Make VHDTIME by the timestamp representing in local */
__hyvhdapi
void __stdcall 
VhdMakeLocalTime(
	u4_t timestamp, 
	__in VHDTIME *pTime
);

__hyvhdapi __forceinline
void VhdMakeLocalTimeSwap(u4_t timestamp, __in VHDTIME *pTime) {
	VhdMakeLocalTime(_swapEndian4(timestamp), pTime);
}

/* Converts VHDTIME to GMT/UTC timestamp */
__hyvhdapi
u4_t __stdcall 
VhdConvertVhdTime(
	VHDTIME *pTime
);

__hyvhdapi __forceinline
u4_t VhdConvertVhdTimeSwap(VHDTIME *pTime) {
	return _swapEndian4(VhdConvertVhdTime(pTime));
}

/* Converts FILETIME to GMT/UTC timestamp */
__hyvhdapi
u4_t __stdcall 
VhdConvertFileTime(
	FILETIME *pTime
);

__hyvhdapi __forceinline
u4_t VhdConvertFileTimeSwap(FILETIME *pTime) {
	return _swapEndian4(VhdConvertFileTime(pTime));
}

/* Calculate the checksum for the VHDFOOTER */
__hyvhdapi
u4_t __stdcall
VhdFooterCalculateChecksum(
	PVHDFOOTER footer
);

/* Calculate the checksum for the VHDHEADER */
__hyvhdapi
u4_t __stdcall
VhdHeaderCalculateChecksum(
	PDYNVHDHEADER header
);

/* Calculate the CHS for the LBA disk */
__hyvhdapi
u4_t __stdcall 
VhdCalculateCHS(
	u4_t totalSectors
);

//==================================

/* typedef for representing a VHD disk image */
typedef struct _tagVHDIMAGE *VHDIMAGE;
/* typedef for representing a VHD disk image pointer */
typedef struct _tagVHDIMAGE **PVHDIMAGE;

// Make a VHDRETCODE indicats a Successful operation
#define RETCODE_S(code) (code & 0x00FFFFFF)
// Make a VHDRETCODE indicats a Fail operation
#define RETCODE_F(code) ((code & 0x00FFFFFF) | 0x80000000)
// Check whether a VHDRETCODE indicats a Successful operation
#define VHD_S(code)     ((code & 0x80000000) == 0)
// Check whether a VHDRETCODE indicats a Fail operation
#define VHD_F(code)     ((code & 0x80000000) != 0)

// No extra infomation
#define VHDCODE_NONE                0x0
// The file is already existing
#define VHDCODE_FILE                0x1
// Footer checksum is corrupt
#define VHDCODE_FOOTERCHECKSUM      0x2
// Header checksum is corrupt
#define VHDCODE_HEADERCHECKSUM      0x3
// Timestamp is corrupt
#define VHDCODE_TIMESTAMP           0x4
// Parent disk is corrupt
#define VHDCODE_INVALIDPARENT       0x5
// Access is denied
#define VHDCODE_ACCESSDENY          0x6
// Exclusive flags
#define VHDCODE_FLAGS               0x7
// Disk size is not mutilple of sector size
#define VHDCODE_DISKSIZE            0x8
// Parent disk is not readonly
#define VHDCODE_MUTABLEPARENT       0x9
// the sector index or sectors count is to large
#define VHDCODE_OUTOFRANGE          0xA
// the argument is NULL or points to incorrect area
#define VHDCODE_ARGUMETENS          0xB
// the VHD disk is ruin
#define VHDCODE_BADIMAGE            0xC

/* the flag for open the VHD disk file */
enum VHDOPENFLAG
{
	// no flags
	VOF_NONE,
	// read only, write is not allowed
	VOF_READONLY = 0x1,
	// alloc sector if it's not representing
	VOF_ALLOCONREAD = 0x2,
	// don't open parent disk.
	VOF_IGNOREPARENT = 0x4
};

/* Opens the VHD disk file */
__hyvhdapi
VHDIMAGE VhdOpenDisk(
	const wchar_t* fullPath, 
	VHDOPENFLAG flags,
	VHDRETCODE *pretcode
);

/* Closes the VHD disk image */
__hyvhdapi
void __stdcall 
VhdCloseDisk(
	__in PVHDIMAGE pVhd
);

/* Reads sectors from the VHD disk image */
__hyvhdapi
VHDRETCODE __stdcall 
VhdReadSector(
	__in VHDIMAGE vhd,
	u4_t firstSector,
	__inout u4_t *pCount,
	__in PVHDSECTOR buffer
);

/* Writes sectors to the VHD disk image */
__hyvhdapi
VHDRETCODE __stdcall 
VhdWriteSector(
	__in VHDIMAGE vhd,
	u4_t firstSector,
	__inout u4_t *pCount,
	__in PVHDSECTOR buffer
);

//===================================

/* Creates a fixed size VHD disk file */
__hyvhdapi
VHDRETCODE __stdcall 
VhdCreateFixedDisk(
	const wchar_t* vhdLocation, 
	u8_t diskSize
);

/* Creates a dynamic allcate VHD disk file */
__hyvhdapi
VHDRETCODE __stdcall
VhdCreateDynamicDisk(
	const wchar_t* vhdLocation, 
	u8_t diskSize
);

/* Creates a differencing VHD disk file */
__hyvhdapi
VHDRETCODE __stdcall
VhdCreateDifferencingDisk(
	const wchar_t* vhdLocation, 
	const wchar_t* parentVhd
);

#endif

