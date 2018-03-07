// cpm.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>
#include "CPMDiskcon.h"

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
	};
	FCBlist* fcbfirst; // pointer to File Control Block list
	int32_t fsize;     // file size
	CPMDisk* diskptr;  // pointer to next file name
public:
	// Constructor
	CPMDisk() : fname(), readonly(false), sys(false), chg(false), fcbfirst(0), diskptr(0)
	{
		cout << "Constructor: " << fname << endl;  // debug print
	}
	// destructor
	~CPMDisk()
	{
		cout << "Destructor" << endl;			// debug print
	}
	// **********************  AddDir **************
	//Reads CP/M disk and creates directory listing
	void AddDir( char *bptr)
	{
		string temp;
		int j;
		unsigned char ch, tch;
		FCBlist *fcbptr;

//		cout << "AddDir" << endl;
		temp.empty();
		if (*bptr != 0xe5)      // check if erased directory
		{
			for (j = 1; j < 9; j++)                    // get file name
				temp += (*(bptr + j));
			for (j = 9; j < 12; j++)
				temp += (*(bptr + j) & 0x7f);
			temp[j] = 0;

//			cout << "File name : " << temp << " record filename: " << fname << " " << fname.empty() << endl;
//			cin >> tch;
			if (fname.empty())                       // first record
			{
				fname = temp;
//				cout << unsigned char(*(bptr + 9)) << " " << unsigned int(*(bptr + 9) & 0x80) << " "<< unsigned char(*(bptr + 9) & 0x7f) << endl;
//				cin >> ch;
				if ( (unsigned char(*(bptr + 9) & 0x80)) > 0)               // file attributes
					readonly = true;
				if ((unsigned char(*(bptr + 10) & 0x80)) > 0)
					sys = true;
				if ((unsigned char(*(bptr + 11) & 0x80)) > 0)
					chg = true;
				cout << "Added directory entry: " << temp << " " << std::boolalpha<< readonly << std::dec << endl;
				fcbfirst = new FCBlist;
				fcbfirst->fcbnum = unsigned char (*(bptr + 15));      // allocation blocks
				fsize = int32_t(fcbfirst->fcbnum) * 128;
				cout << fsize << endl;
				fcbfirst->nxtptr = 0;
				for (j = 16; j < 32; j++)               // get allocation pointers
					fcbfirst->fcb[j-16] = *(bptr + j);

			}
			else                          // need to walk list and add in file
			{
				if (temp == fname)          // need to add new FCB to first record
				{
					cout << "adding FCB " << endl;
					fcbptr = fcbfirst;
					cout << std::hex << "fcbptr " << fcbptr << " nxtptr " << fcbptr->nxtptr << std::dec << endl;
					while (fcbptr->nxtptr != nullptr) fcbptr = fcbptr->nxtptr;        // walk the list
					fcbptr->nxtptr = new FCBlist;
					fcbptr = fcbptr->nxtptr;
					fcbptr->fcbnum += *(bptr + 15);      // allocation blocks
					fsize = int32_t(fcbfirst->fcbnum) * 128;
					cout << fsize << endl;
					fcbptr->nxtptr = nullptr;
					for (j = 16; j < 32; j++)               // get allocation pointers
						fcbptr->fcb[j - 16] = *(bptr + j);

				}

			}
		}


	}
	// Displays directory listing
	void DirCPM(int blocksz, CPMDisk *startptr)
	{
		CPMDisk * xcdptr;
		string temp ;

		cout << "CP\\M directory" << endl;
		cout << " Filename      RSC Size\n";
		xcdptr = startptr;
		do
		{
			temp.empty();
			cout << xcdptr->Getfname();
			if (readonly) temp.append("x"); else temp.append(" ");
			if (sys) temp.append("x");  else temp.append(" ");
			if (chg) temp.append("x");  else temp.append(" ");
			cout << "  " << temp << " "<< fsize << endl;

			xcdptr = xcdptr->GetDiskptr();
		} while (xcdptr != nullptr);
	}
	// Copies CP/M file name to Windows system
	void CopyCPM()
	{
		//    return 0 ;
	}
	// Copies all CP/M files to Windows system
	void CopyAll()
	{
		//    return 0 ;
	}
	string Getfname()
	{
		return fname;
	}
	CPMDisk * GetDiskptr()
	{
		return diskptr;
	}
	void SetDiskptr(CPMDisk *ptr)
	{
		diskptr = ptr;
	}
};

// Forward declaration

CPMDisk* ReadDisk(fstream& fptr, int & ablock);

//**************** test function for buffer read. Not production
void test(fstream& fptr, char fname[])
{
	char  *buffer = new char [9000];
	char ch;
	long length = fptr.tellg();
	int j;
	fstream fp;

	fptr.seekg(0, fptr.end);	
	cout << "test seek, length: " << length << endl;
	fptr.seekg(0, fptr.beg);
	cout << "test read" << endl;
	if (fptr.is_open()) cout << "File open" << endl;
	fptr.get (buffer[0]);
	cout << "Single char" << endl;
//	fptr.read(buffer, 5);
	fptr.close();
	cout << "file closed" << endl;
	fp.open(fname, ios::in | ios::binary);
	cout << "file opened: " << fname << endl;
	for (j = 0; j < 8192; j++)
		{
		fp.get(ch); buffer[j] = ch;
		};
	cin >> ch;
	fp.read(buffer, 8192);
	cout << hexout(buffer[5]);

}

//**************** menu *************
char menu()
{
	char result;

	cout << "CP\\M Disk Menu" << endl;
	cout << "R Read CP\\M Disk" << endl;
	cout << "D Display CP\\M Directory" << endl;
	cout << "S Save a specific file to Windows" << endl;
	cout << "A Save all files to Windows" << endl;
	cout << "E Exit" << endl << endl;
	cout << "Enter Option: ";
	cin >> result;
	result = toupper(result);
	return result;

};



// ******************************* main *************
int main(int argc, char *argv[])
{
	char what;     // menu result
	CPMDisk *cdisk = nullptr;     // pointer to start of CPM Disk directory
	int ablock;					// CP\M file block size
	char b[100];




	if (argc == 0)
		{
		cout << "ERROR - No filename!" << endl;
		return 0;
		}
	// only care about first filename
	cout << argv[1] << endl;				// debug print

	fstream ioFile(argv[1], ios::in | ios::binary);

	//  fstream ioFile("g:\\dev\\dev-cpp\\cpmdiskcon\\bin\\debug\\CPM224.BIN", ios::in| ios::binary);
	if (!ioFile)
		{
		cout << "Error opening file - " << argv[1] << endl;
		return 0;
		}
	  else
		{
		cout << "Opened file - " << argv[1] << endl; //argv[1] << endl << endl;
		}
	
	ioFile.seekg(0, ios::beg);            // get disk identifier
	ioFile.read(b, 20);


	do
	{
		what = menu();
		switch (what)
		{
		case 'R':               // Directory
								//*cdisk.AddDir(ioFile, cdisk);        // read CP\M disk
//			test(ioFile, argv[1]);								// debug disk read
			cdisk = ReadDisk(ioFile, ablock);
			break;
		case 'D':               // display directory
			cdisk->DirCPM(ablock, cdisk);
			break;
		case 'S':               // Save file
			break;
		case 'A':               // Save All files
			break;
		case 'E':              // Exit
			break;
		default:
			cout << "Unknown menu option" << endl;
		}
	} while (what != 'E');
};


//***************************** ReadDisk() ************************************

CPMDisk* ReadDisk(fstream &fptr, int& ablock)
{

	char buff[BuffLen], *bptr;        // file buffer
	long seekpt;           // point to seek in the file
	char ch;               // temp char
	int j, fcbofs, dirstart, dirsize;
	string temp;

	CPMDisk *startptr, *cdptr, *lcdptr, *tcdptr;   // c= current, l = last, t = temporary



	startptr = nullptr;                    // set start of directory list

	fptr.seekg(5, ios::beg);            // get disk identifier
	cout << "Reading CP\\M Disk" << " " <<fptr.tellg() << endl;
	fptr.get(ch);						// Causes subscript out of range assertion errors in MSVCP140D.dll
// Workaround
//	fptr.read( buff, BuffLen);
	//ch = buff[H37disktype];

// end workaround

	cout << "Disk type: " << hexout(ch) << endl;
//	cin >> j;
	switch (ch)
	{
	case H37e:
		ablock = H37eAB;
		dirstart = H37eDir;
		dirsize = H37eDirSz;
		break;
	case H37d:
		ablock = H37dAB;
		dirstart = H37dDir;
		dirsize = H37dDirSz;
		break;
	case H37s:
		ablock = H37sAB;
		dirstart = H37sDir;
		dirsize = H37sDirSz;
		break;
	default:
		ablock = CDR_AB;
		dirstart = CDRDir;
		dirsize = sizeof(buff);
		break;
	}
	seekpt = dirstart;       // start of directory

	fptr.seekg(seekpt, ios::beg);
	// ** Workaround code follows
	// fptr.read(buff, sizeof(buff));			// Causes subscript out of range assertion errors in MSVCP140D.dll
	for (j = 0; j < BuffLen; j++)
		fptr.get(buff[j]);
	// end workaround

	cout << "file read at H37Dir char: " << hexout(buff[1]) << " Directory size: " << dirsize << endl;

	fcbofs = 0;                     // file control block offset in buffer

	while (fcbofs < dirsize)
	{

		if ((unsigned char) buff[fcbofs] !=  (0xe5))               // check for erased directory
			{
			temp.clear();

			for (j = 1; j < 9; j++)                    // get file name to walk dir list
				{
				temp = temp + char (buff[fcbofs + j]);
//				cout << temp << " length: " << temp.length() << " Offset: " << fcbofs + j << endl;
				}
			for (j = 9; j < 12; j++)
				{
				temp = temp + char(buff[fcbofs + j] & 0x7f);
//				cout << temp << " length: " << temp.length() << " Offset: " << fcbofs + j << endl;
				}

//			cout << "New filename (temp): " << temp << endl;
			if (startptr == nullptr)
				{
//				cout << "startptr " << endl;
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

//				cout << "First compare (temp): " << temp << " cdptr: " << cdptr->Getfname() << " result:" << temp.compare(cdptr->Getfname()) << endl;

				while (temp.compare(cdptr->Getfname()) > 0 && cdptr->GetDiskptr() != nullptr)
					{
					cout << "Current fname: " << cdptr->Getfname() << "  Temp: " << temp << " Compare: " << temp.compare(cdptr->Getfname()) << endl;
					lcdptr = cdptr;
					cdptr = cdptr->GetDiskptr();                // get next record
					}
				bptr = &buff[fcbofs];
				//cout << "bptr: " << *(bptr+1) << " Hex: " << hexout(*(bptr+1)) << endl;
//				cin >> ch;
				if ((cdptr->Getfname()) == temp)                            // file name match, add FCB data
					cdptr->AddDir(bptr);

				tcdptr = new CPMDisk;                // need to add new record
				if (temp.compare(cdptr->Getfname()) < 0)
					// insert into list, cdptr points to record to insert prior
				   {
					cout << " Insert into CPM Disk list BEFORE cdptr: " << lcdptr->Getfname() << " cdptr: " << cdptr->Getfname() << endl;

					if (startptr == cdptr)					// first record in list, reset statrtptr
						startptr = tcdptr; 
					  else				
						lcdptr->SetDiskptr(tcdptr);
					tcdptr->SetDiskptr(cdptr);
					tcdptr->AddDir(bptr);
					}
				  else if (cdptr->GetDiskptr() == nullptr)          // add record to end of list
					{
					cout << "End of CPM dir list " << endl;
					cdptr->SetDiskptr(tcdptr);
					tcdptr->AddDir(bptr);
					}

				}
			}
		fcbofs += 32;        // advance to next file control block
	};

	return startptr;
};

string hexout(unsigned char ch)
{
	const unsigned char hexs[] = "0123456789ABCDEF";
	string temp;

	temp.empty();
	temp += char(hexs[(ch >> 4) & 0x0f]);
	temp[1] = hexs[ch & 0x0f];
	temp[2] = 0;

	return temp;

}

