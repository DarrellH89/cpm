// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
//******************************


using namespace std;
// test bit 7 for the following filename chars
#define FRead   9      //  Read only
#define FSys    10     // 1 = true
#define FChg    11     // bit 7 means file changed
#define FMask   0x80   // bit mask
#define BuffLen 0x2000		// buffer size

// Disk types
#define H37disktype 5			// location of H37 disk type
							// Disk type, Allocation block size, Directory start, Allocation block number size, dir size 
#define DISK_ROW	5
#define DISK_VAL	5
const int DiskType[DISK_ROW][DISK_VAL] 
						 = {{0x6f, 0x800, 0x2800, 2, 0x2000}  ,		// H37 96tpi ED DS
							{0x62, 0x400, 0x2000, 1, 0x1000 },		// H37 48tpi DD DS
							{0x63, 0x400, 0x2000, 1, 0x2000 },		// H37 48tpi DD DS
							{0x6b, 0x800, 0x2000, 2, 0x2000 },		// H37 48tpi ED DS
							{0x00, 0x100, 0x1e00, 1, 0x800}			// H17 48tpi SD SS
							};		
#define H37e		0x6f	       // H37 96tpi ED DS
#define H37eAB		0x800       // Allocation block 2k
#define H37eDir		0x2800      // directory start
#define H37efcb		2			// allocation block number size
#define H37eDirSz   0x2000		// directory size
#define H37d		0x62        // H37 48tpi DD DS
#define H37dAB		0x400       // Allocation block 1k
#define H37dDir		0x2000		// start of directory
#define H37dDirSz   0x1000		// Directory size
#define H37dfcb		1			// allocation block number size
#define H37s		0x6b		// H37 48tpi ED DS
#define H37sAB		0x800		// Allocation block 1k
#define H37sDir		0x2000		// start of directory
#define H37sDirSz	0x2000		// Directory size
#define H37sfcb		2			// allocation block number size
/*
H37 disk identification at byte 6 on the first sector
MSB = 6 for H37
LSB
Bit 4 1 = 48tpi in 96tpi drive
Bit 3 0 = 48 tpi drive, 1 = 96 tpi
Bit 2 1 = extended double density
Bit 1 1 = double density, 0 = single density
Bit 0 1 = double sided, 0 = single sided
*/

#define CDR     0x0    // CDR format
#define CDR_AB  0x400
#define CDRDir  0x3100  // CDR directory start
#define CDRfcb	1			// allocation block number size


// From Starting out with C++ -- not really used
inline char * addr (void * p)
{
	return reinterpret_cast < char *> (p);
}