// cpm.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>
#include "CPMDiskcon.h"
#include <iomanip>
#include <cstring>
#include <strsafe.h>


using namespace std;

// forward declarations
char menu();
string hexout(unsigned char ch);


//********************** Class CPMDisk *******************
class CPMDisk
	{
	private:
		string fname;
		bool readonly;      // Read only file
		bool sys;          // system file
		bool chg;          // disk changed - not used
		struct FCBlist {
			unsigned char fcb[16];    // 16 file control block numbers
			FCBlist* nxtptr;    // pointer to next fcb it needed
			int fcbnum;       // number of 128 byte records in this extant
			int extantnum;		// extant number
			} fcbfirst;
		int32_t fsize;     // file size
		CPMDisk* diskptr;  // pointer to next file name

	public:
		// Constructor
		CPMDisk() : fname(), readonly(false), sys(false), chg(false),  diskptr(0)
			{
			fcbfirst.nxtptr = nullptr;
			fcbfirst.fcb[0] = 'c';

			}

		// destructor
		~CPMDisk()
			{
			FCBlist *fcbptr, *nfcbptr;

			//cout << "Destructor" << endl;
			fcbptr = fcbfirst.nxtptr;
			while (fcbptr != nullptr)
				{
				nfcbptr = fcbptr->nxtptr;
				delete fcbptr;
				fcbptr = nfcbptr;
				}

			}

		// **********************  AddDir **************
		//Reads CP/M disk and creates directory listing
		void AddDir(char *bptr)
			{
			char temp[13];
			int j;
			FCBlist *fcbptr, *lfcbptr;

//			cout << "AddDir" << endl;
			if (*bptr != 0xe5)      // check if erased directory
				{
				for (j = 1; j < 9; j++)                    // get file name
					temp[j - 1] = (*(bptr + j));
				for (j = 9; j < 12; j++)
					temp[j - 1] = (*(bptr + j) & 0x7f);
				temp[11] = '\0';
				//			cout << temp << "Length: " << strlen(temp) << "j: " << j<< endl;;


				if (fname.empty())                       // first record
					{
					fname = temp;
					if ((unsigned char(*(bptr + 9) & 0x80)) > 0)               // file attributes
						readonly = true;
					if ((unsigned char(*(bptr + 10) & 0x80)) > 0)
						sys = true;
					if ((unsigned char(*(bptr + 11) & 0x80)) > 0)
						chg = true;
//					cout << "Added directory entry: " << temp << " readonly: " << std::boolalpha << readonly << std::dec << endl;

					fcbfirst.fcbnum = unsigned char(*(bptr + 15));      // allocation blocks
					fcbfirst.extantnum = unsigned char(*(bptr + 12));	// extant number
					fsize = int32_t(fcbfirst.fcbnum) * 128;
					fcbfirst.nxtptr = 0;
					
					for (j = 16; j < 32; j++)               // get allocation pointers
						fcbfirst.fcb[j - 16] = *(bptr + j);

					}
				else                          // need to walk list and add in file
					{
					if (temp == fname)          // need to add new FCB to first record
						{
						fcbptr = &fcbfirst;
						while (fcbptr->nxtptr != nullptr && fcbptr->extantnum < *(bptr + 12))
							{
							lfcbptr = fcbptr;
							fcbptr = fcbptr->nxtptr;        // walk the list
							}
						if (fcbptr->nxtptr == nullptr)
							{
																				// cout << "End of FCB list " << endl;
							fcbptr->nxtptr = new FCBlist;						// create new FCB
							fcbptr = fcbptr->nxtptr;							// set new pointer to new record
							for (j = 16; j < 32; j++)							// get allocation pointers
								fcbptr->fcb[j - 16] = *(bptr + j);
							fcbptr->fcbnum = unsigned char(*(bptr + 15));      // allocation blocks
							fsize += int32_t(fcbptr->fcbnum) * 128;			// add to toal file size	
							fcbptr->nxtptr = nullptr;
							}
						else
							{
																				// cout << "Insert FCB " << endl;
							lfcbptr->nxtptr = new FCBlist;
							lfcbptr->nxtptr->nxtptr = fcbptr;
							fcbptr = lfcbptr->nxtptr;
							for (j = 16; j < 32; j++)							// get allocation pointers
								fcbptr->fcb[j - 16] = *(bptr + j);
							fcbptr->fcbnum = unsigned char(*(bptr + 15));      // allocation blocks
							fsize += int32_t(fcbptr->fcbnum) * 128;			// add to toal file size	
							};
						}

					}
				}

			}

		// ************* Displays directory listing
		void DirCPM( CPMDisk *startptr)
			{
			CPMDisk * xcdptr;
			string temp;
			int j;

			cout << "CP\\M directory" << endl;
			cout << "   Filename       RSC Size\n";
			xcdptr = startptr;
			j = 0;
			do
				{
				temp.clear();
				cout << setw(2) << j << " "  << xcdptr->Getfname();
				if (xcdptr->readonly) temp.append("x"); else temp.append(" ");
				if (xcdptr->sys) temp.append("x");  else temp.append(" ");
				if (xcdptr->chg) temp.append("x");  else temp.append(" ");
				cout << "  " << temp << " " << xcdptr->fsize << endl;

				xcdptr = xcdptr->GetDiskptr();
				j++;
				} while (xcdptr != nullptr);
			}

		//*************** Copies CP/M file name to Windows system
		void CopyCPM(fstream & inptr, string fname , CPMDisk *cdptr, int ablock, int dirstart, int fcbsize)
			// inptr: file pointer to CP\M disk image
			// fname: name of Windows file to create
			// cdptr: pointer to CPMDisk record - not sure I need this
			// ablock: floppy disk block size
			// dirstart: loction of directory start. All calculations are from this point
			// fcbsize: 1 or 2. number of bytes for allocation block values
			{
			fstream  ofile;
			char buff[BuffLen];
			unsigned char *cptr, ch;
			FCBlist * fcbptr;
			int abnum, bytesOut, blockread ;
			long diskpos;

			ofile.open(fname, ios::out | ios::binary);
			if (!ofile)
				cout << "ERROR - opening file: " << fname << endl;
			else
				{
				bytesOut = 0;
				fcbptr = &fcbfirst;						// set pointer to fcbfirst to walk the fcb linked list
				cptr = fcbptr->fcb;						// cptr gets 1 or 2 bytes from fcb[] depending on fsize


				cout << "File Size is : " << cdptr->fsize << endl;	
				while (bytesOut < cdptr->fsize)					// figure out if last read is less than ablock size
					{
					if (cdptr->fsize - bytesOut < ablock)
						blockread = cdptr->fsize - bytesOut;
					  else
						blockread = ablock;
					if (fcbsize == 1)								// convert one byte into an int
						{
						ch = *cptr;
						cptr++;
						abnum = ch;
						}
					else
						{									// convert two bytes into an int
						abnum = *cptr;
						cptr = cptr + 2;
						};
					diskpos = dirstart + ablock * abnum;		// calculate file position to read. All reads are offset from dirstart
					inptr.seekg(diskpos, ios::beg);
					inptr.read(buff, blockread);				// read and write data
					ofile.write(buff, blockread);
					bytesOut += blockread;
					if (cptr - fcbptr->fcb > 15)				// Did we use all 16 bytes in fcb[]? count starts at 0
						{
						fcbptr = fcbptr->nxtptr;
						cptr = fcbptr->fcb;
						}
					}
				ofile.close();

				}


			//    return 0 ;
			}
		//***************** Returns file name as read from disk	
		string Getfname()
			{
			return fname;
			}

		// **************** Returns filename without spaces
		string GetWinfname()
			{
			char tname[15], name[15];
			string result;
			int j, k; 
			
			strcpy(name,  fname.c_str());
			name[11] = '\0';						// filename is always 11 characters. make sure it is zero terminated
			k = 0;
			for (j = 0; j < 8; j++)
				if (name[j] != ' ')
					{
					tname[k] = name[j];
					k++;
					}
			tname[k] = '.';	
			k++;
			for(j=8; j < 11;j++)
				{
				tname[k] = name[j];
				k++;
				};
			tname[k] = '\0';
			result.clear();
			result.append(tname);
			return result;
			}
		// returns pointer to next CPMDisk
		CPMDisk * GetDiskptr()
			{
			return diskptr;
			}
		// Set pointer to next CPMDisk
		void SetDiskptr(CPMDisk *ptr)
			{
			diskptr = ptr;
			}
	};

// Forward declaration

CPMDisk* ReadDisk(fstream &fptr, int& ablock, int & dirstart, int& dirsize, int& fcbsize);
void SaveFile(fstream& fptr,  CPMDisk * startptr,int ablock, int dirstart, int dirsize, int fcbsize);
void SaveAllFile(fstream& fptr, CPMDisk * startptr, int ablock, int dirstart, int dirsize, int fcbsize, char dirname[]);
void GetNewDisk(fstream& fptr, char openfile[], int openfilesz);


//**************** menu *************
char menu(CPMDisk * cdptr)
	{
	char result;

	cout << endl <<"CP\\M Disk Menu\n" << endl;
	cout << "R Read CP\\M Disk" << endl;
	cout << "O Open a new CP\\M disk" << endl;	
	if (cdptr != nullptr)
		{
		cout << "D Display CP\\M Directory" << endl;
		cout << "S Save a specific file to Windows" << endl;
		cout << "A Save all files to Windows" << endl;

		}
	cout << "E Exit" << endl << endl;
	cout << "Enter Option: ";
	cin >> result;
	result = toupper(result);
	return result;

	};



// ******************************* main *************
int main(int argc, char *argv[])
	{
	fstream ioFile;
	char what;     // menu result
	CPMDisk *cdisk = nullptr;     // pointer to start of CPM Disk directory
	CPMDisk *ndisk;
	int ablock, dirstart, dirsize, fcbsize;					// CP\M file block size, directory start, directory size
	char openfile[15];
	//char b[200];

	if (argc ==2)
		{
		ioFile.open(argv[1], ios::in | ios::binary);
		if (!ioFile)
			{
			cout << "Error opening file - " << argv[1] << endl;
			return 0;
			}
		else
			{
			cout << "Opened file - " << argv[1] << endl; 
			strcpy_s(openfile, argv[1]);
			}
		}

	//ioFile.seekg(0, ios::beg);            // get disk identifier
	//ioFile.read(b, 20);
	do
		{
		what = menu(cdisk);
		switch (what)
			{
			case 'R':               // Directory
				if (cdisk == nullptr)
					if (ioFile.is_open())
						cdisk = ReadDisk(ioFile, ablock, dirstart, dirsize, fcbsize);
					  else
						cout << "No file open" << endl;

				else
					cout << " You can only read the disk once. Open a new disk if desired." << endl;
				break;
			case 'D':               // display directory
				cdisk->DirCPM(cdisk);
				break;
			case 'S':               // Save file
				SaveFile(ioFile, cdisk, ablock, dirstart,dirsize, fcbsize);
				break;
			case 'A':               // Save All files
				SaveAllFile(ioFile, cdisk, ablock, dirstart, dirsize, fcbsize, openfile);
				break;
			case 'O' :				// Delete current list and clean up memory
				if (cdisk != nullptr)
					{
					ndisk = cdisk->GetDiskptr();		// Delete current list and free memory
					while (cdisk != nullptr)
						{
						//cout << cdisk->Getfname();
						delete cdisk;
						cdisk = ndisk;
						if (cdisk != nullptr)
							ndisk = cdisk->GetDiskptr();
						}
					};
				ioFile.close();
				GetNewDisk( ioFile, openfile, sizeof(openfile));
				break;
			case 'E':              // Exit
				break;
			default:
				cout << "Unknown menu option" << endl;
			}
		} while (what != 'E');
	};

//**************************** SaveFile **************************
// ** Save a specxific file to the corrent directory

void SaveFile(fstream& fptr, CPMDisk * startptr, int ablock, int dirstart, int dirsize, int fcbsize)
	{
	string fname;
	int j,  fnum;
	
	CPMDisk * xcdptr;
	
	if (startptr == nullptr)
		{
		cout << "No Files in the Directory" << endl;
		return;
		};

	startptr->DirCPM(startptr);
	printf("\nEnter file name number to copy: ");
	cin >> fnum;

	xcdptr = startptr;
	for (j = 0; j < fnum; j++)				// walk CPMDisk list to the user supplied number	
		xcdptr = xcdptr->GetDiskptr();
	fname = xcdptr->GetWinfname();
	
	cout << "Copying file " << xcdptr->Getfname() <<" to: " << fname<< endl ;
	xcdptr->CopyCPM(fptr, fname, xcdptr, ablock, dirstart, fcbsize);
		
	}
//********************** SaveAllFile() ****************************************
void SaveAllFile(fstream& fptr, CPMDisk * startptr, int ablock, int dirstart, int dirsize, int fcbsize, char dirname[])
	{
	string fname;
	string temp;

	int j, k;
	TCHAR		newdir[10];
	CPMDisk		* xcdptr;
	DWORD       dwRet;
	TCHAR       szNewPath[MAX_PATH], SavePath[MAX_PACKAGE_NAME];

	if (startptr == nullptr)
		{
		cout << "No Files in the Directory" << endl;
		return;
		};
	dwRet = GetCurrentDirectory(MAX_PATH, szNewPath);
	if (dwRet == 0)
		{
		cout << "Current Directory error" << endl;
		return;
		}
	_tcscpy_s( SavePath, szNewPath);			// Save current directory

	k = 1;
	newdir[0] = '\\';
	for (j = 0; j < 8; j++)					// get new directory name by removing filename spaces
		{
		if (dirname[j] == '.')				// end loop when a period occurs
			break;
		newdir[k] = dirname[j];
		k++;
		};
	newdir[k] = '\0';
	_tcsncat_s(szNewPath, newdir, k);

	szNewPath[_tcslen(szNewPath) + 1] = '\0';


	//_tcscpy(temp, newdir, k);
	if (CreateDirectory(szNewPath, 0))
		{
		// directory created
		}
	else
		{
		if (ERROR_ALREADY_EXISTS == GetLastError())
			wcout << "Directory " << newdir << " already exists. Error number: " << GetLastError() << endl;
		  else
			cout << "Directory creation failed" << endl;
		}
	dwRet = SetCurrentDirectory(szNewPath);


	if (dwRet != 0)
		{
		// directory created
		}
	else
		{
		wcout << "Can not change to Directory: " << newdir << ". Error number: " << GetLastError() << endl;
		dwRet = GetCurrentDirectory(MAX_PATH, szNewPath);
		wcout << szNewPath << endl;
		return;
		}

	xcdptr = startptr;
	do
		{
		fname = xcdptr->GetWinfname();
		cout << "Copying file " << xcdptr->Getfname() << " to: " << fname << endl;
		xcdptr->CopyCPM(fptr, fname, xcdptr, ablock, dirstart, fcbsize);
		xcdptr = xcdptr->GetDiskptr();
		} while (xcdptr != nullptr);
	dwRet = SetCurrentDirectory(SavePath);

	return;
	};

//***************************** ReadDisk() ************************************

CPMDisk* ReadDisk(fstream &fptr, int& ablock, int& dirstart, int& dirsize, int& fcbsize)
	{

	char buff[BuffLen], *bptr;        // file buffer
	long seekpt;           // point to seek in the file
	char ch;               // temp char
	int j, k,fcbofs, tempint;
	string temp;

	CPMDisk *startptr, *cdptr, *lcdptr, *tcdptr;   // c= current, l = last, t = temporary



	startptr = nullptr;                    // set start of directory list

	fptr.seekg(0, ios::beg);            // get disk identifier

	fptr.read( buff, 256);
	ch = buff[H37disktype];
	tempint = int(ch);

	for (j = 0; j < 4; j++)
		{
		for (k = 0; k < 5; k++)
			cout << DiskType[j][k] << " ";
		cout << endl;
		};

	cout << "Disk type: " << hexout(ch) << "h "<< tempint << "d"<<endl;
	j = 0;
	while(j < DISK_ROW)
		{
		if (DiskType[j][0] == tempint)
			{
			ablock = DiskType[j][1];
			dirstart = DiskType[j][2];
			fcbsize = DiskType[j][3];
			dirsize = DiskType[j][4];
			break;
			};
		j++;
		};

	if (j == DISK_ROW)			// didn't find value in table, so set to default
		{
		cout << "\nUNKOWN Disk format\n" << endl;
		return startptr;
		ablock = DiskType[j][1];				// dead code. No default disk type
		dirstart = DiskType[j][2];
		fcbsize = DiskType[j][3];
		dirsize = DiskType[j][4];
		};
/*
	switch (ch)
		{
		case H37e:
			ablock = H37eAB;
			dirstart = H37eDir;
			dirsize = H37eDirSz;
			fcbsize = H37efcb;
			break;
		case H37d:
			ablock = H37dAB;
			dirstart = H37dDir;
			dirsize = H37dDirSz;
			fcbsize = H37dfcb;
			break;
		case H37s:
			ablock = H37sAB;
			dirstart = H37sDir;
			dirsize = H37sDirSz;
			fcbsize = H37sfcb;
			break;
		default:
			ablock = CDR_AB;
			dirstart = CDRDir;
			dirsize = sizeof(buff);
			fcbsize = CDRfcb;
			break;
		}
	*/
	seekpt = dirstart;       // start of directory

	fptr.seekg(seekpt, ios::beg);
	fptr.read(buff, sizeof(buff));			// Causes subscript out of range assertion errors in MSVCP140D.dll

	fcbofs = 0;                     // file control block offset in buffer

	while (fcbofs < dirsize)
		{

		if ((unsigned char)buff[fcbofs] != (0xe5))               // check for erased directory
			{
			temp.clear();

			for (j = 1; j < 9; j++)                    // get file name to walk dir list
				temp = temp + char(buff[fcbofs + j]);
			for (j = 9; j < 12; j++)
				temp = temp + char(buff[fcbofs + j] & 0x7f);

			if (startptr == nullptr)
				{
				startptr = new CPMDisk;        // create new record
				bptr = &buff[fcbofs];
				startptr->AddDir(bptr);
				cdptr = startptr;
				lcdptr = cdptr;
				}
			else
				{
				cdptr = startptr;
				lcdptr = cdptr;          // last ptr = current ptr
										 // start of list
				// Walk the list

				while (temp.compare(cdptr->Getfname()) > 0 && cdptr->GetDiskptr() != nullptr)
					{
					lcdptr = cdptr;
					cdptr = cdptr->GetDiskptr();                // get next record
					}
				bptr = &buff[fcbofs];
				if ((cdptr->Getfname()) == temp)                            // file name match, add FCB data
					cdptr->AddDir(bptr);

				tcdptr = new CPMDisk;                // need to add new record
				if (temp.compare(cdptr->Getfname()) < 0)
					// insert into list, cdptr points to record to insert prior
					{
					if (startptr == cdptr)					// first record in list, reset statrtptr
						startptr = tcdptr;
					else
						lcdptr->SetDiskptr(tcdptr);
					tcdptr->SetDiskptr(cdptr);
					tcdptr->AddDir(bptr);
					}
				else if (cdptr->GetDiskptr() == nullptr)          // add record to end of list
					{
					cdptr->SetDiskptr(tcdptr);
					tcdptr->AddDir(bptr);
					}

				}
			}
		fcbofs += 32;        // advance to next file control block
		};

	return startptr;
	};


	//********************** void GetNewDisk() ****************
	// Opens new Windows file for processing

void GetNewDisk(fstream & fptr, char openfile[], int namelen)
	{
	const int FileNmeLen = 50;
	TCHAR name[FileNmeLen], fname[FileNmeLen];
	string temp;
	int strlen;

	WIN32_FIND_DATA FileData;
	LARGE_INTEGER	filesize;
	HANDLE          hSearch;
	DWORD           dwRet;
	TCHAR        szPath[MAX_PATH], SavePath[MAX_PATH];

	if (fptr.is_open())					// Close file pointer if open
		fptr.close();
	dwRet = GetCurrentDirectory(MAX_PATH, szPath);
	if (dwRet == 0)
		{
		cout << "Current Directory error" << endl;
		return;
		}
	_tcscpy_s(SavePath, szPath);			// Save current directory
	cout << "Enter file serach string (for example: *.bin): ";
	wcin >> name;
	if (name[0] == 0)
		StringCchCopy(fname, MAX_PATH, TEXT("\\*.bin"));
	else
		{
		StringCchCopy(fname, MAX_PATH, TEXT("\\"));
		StringCchCat(fname, MAX_PATH, name);
		};
	StringCchCat(szPath, MAX_PATH, fname);
	hSearch = FindFirstFile(szPath, &FileData);
	if (INVALID_HANDLE_VALUE == hSearch)
		{
		wcout << "Can't find file: " << fname;
		return ;
		};
	do                         // Display files in Directory
		{
		if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			_tprintf(TEXT("  %s  <DIR> \n"), FileData.cFileName);
		  else
			{
			filesize.LowPart = FileData.nFileSizeLow;
			filesize.HighPart = FileData.nFileSizeHigh;
			_tprintf(TEXT("  %s   %lld bytes\n"), FileData.cFileName, filesize.QuadPart);
			}
		} while (FindNextFile(hSearch, &FileData) != 0);
	cout << "Enter CP\\M file name to read: ";
	cin >> temp;
	fptr.open(temp, ios::in | ios::binary);
	if (!fptr)
		{
		cout << "Error opening file - " << temp << endl;
		}
	else
		{
		cout << "Opened file - " << temp << endl; 
		}
	strlen = temp.length();
	if (strlen > namelen)
		strncpy_s(openfile, namelen - 1,temp.c_str(), namelen - 1);
	  else
		strcpy_s(openfile, namelen-1, temp.c_str());

	};

	//************** hexout() *******************
	// displays character in hex format
string hexout(unsigned char ch)
	{
	const unsigned char hexs[] = "0123456789ABCDEF";
	char temp[3];

	temp[0] = hexs[(ch >> 4) & 0x0f];
	temp[1] = hexs[ch & 0x0f];
	temp[2] = 0;

	return temp;

	}

