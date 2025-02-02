// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space
    
    if (numSectors <= 8) {
        for (int i = 0; i < numSectors; i++)
    	dataSectors[i] = freeMap->Find();
    }
    else {
        for (int i = 0; i <= 8; i++)
            dataSectors[i] = freeMap->Find();
        int ind_index[32];
        for (int i = 0; i < numSectors - 8; i++)
            ind_index[i] = freeMap->Find();
        synchDisk->WriteSector(dataSectors[8], (char*)ind_index);
    }
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    if (numSectors <= 8){
        for (int i = 0; i < numSectors; i++) {
        	ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
        	freeMap->Clear((int) dataSectors[i]);
        }
    }
    else{
        int ind_index[32];
        synchDisk->ReadSector(dataSectors[8], (char*)ind_index);
        for (int i = 0; i < numSectors - 8; i++)
            freeMap->Clear((int)ind_index[i]);
        for (int i = 0; i < 8; i++) {
            freeMap->Clear((int)dataSectors[i]);
        }
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    if (offset < 1024)
        return(dataSectors[offset / SectorSize]);
    else {
        int pos = (offset - 1024) / 128;
        int ind_index[32];
        synchDisk->ReadSector(dataSectors[8], (char*)ind_index);
        return ind_index[pos]; 
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    printf("File Type: %s\n", type);
    printf("Create Time: %s\n", cr_time);
    printf("Last Visited Time: %s\n", last_vis_time);
    printf("Last Modified Time: %s\n", last_mod_time);

    if (numSectors <= 8) {
        for (i = 0; i < numSectors; i++)
    	printf("%d ", dataSectors[i]);
    }
    else {
        for (i = 0; i < 8; i++)
            printf("%d ", dataSectors[i]);
        int ind_index[32];
        synchDisk->ReadSector(dataSectors[8], (char*)ind_index);
        for (i = 0; i < numSectors - 8; i++) 
            printf("%d ", ind_index[i]);
    }
    printf("\nFile contents:\n");
    if (numSectors <= 8) {
        for (i = k = 0; i < numSectors; i++) {
    	synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
    	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
    		printf("%c", data[j]);
                else
    		printf("\\%x", (unsigned char)data[j]);
    	}
            printf("\n"); 
        }
    }
    else {
        for (i = k = 0; i < 8; i++) {
            printf("Sector: %d\n", dataSectors[i]);
        synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
            if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
            printf("%c", data[j]);
                else
            printf("\\%x", (unsigned char)data[j]);
        }
            printf("\n"); 
        }
        int ind_index[32];
        synchDisk->ReadSector(dataSectors[8], (char*)ind_index);
        for (i = 0; i < numSectors - 8; i++) {
        printf("Sector: %d\n", ind_index[i]);
        synchDisk->ReadSector(ind_index[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
            if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
            printf("%c", data[j]);
                else
            printf("\\%x", (unsigned char)data[j]);
        }
            printf("\n"); 
        }

    }
    delete [] data;
}

void
FileHeader::set_cr_time()
{
    time_t t;
    time(&t);
    strncpy(cr_time, asctime(gmtime(&t)), 25);
    cr_time[24] = '\0';
}

void
FileHeader::set_last_vis_time()
{
    time_t t;
    time(&t);
    strncpy(last_vis_time, asctime(gmtime(&t)), 25);
    last_vis_time[24] = '\0';
}

void
FileHeader::set_last_mod_time()
{
    time_t t;
    time(&t);
    strncpy(last_mod_time, asctime(gmtime(&t)), 25);
    last_mod_time[24] = '\0';
}

bool
FileHeader::DynamicAdd(int bytes, BitMap* freeMap)
{
    //printf("DynamicADD! bytes to add = %d\n", bytes);
    int newsize = numBytes + bytes;
    int newsector = divRoundUp(newsize, SectorSize);
    if (newsector == numSectors) {
        //printf("no need to extend!\n");
        numBytes = newsize;
        numSectors = newsector;
        return TRUE;
    }
    if (freeMap->NumClear() < newsector - numSectors){
        printf("no enough sectors！\n");
        return FALSE;
    }
    if (newsector > 40) {
        printf("file too big!\n");
        return FALSE;
    }
    printf("Extend File by %d Sectors!\n", newsector - numSectors);
    int i = numSectors;
    for (; i < newsector; i++) {
        printf("sector id:%d\n", i);
        if (i > 8)
            break;
        dataSectors[i] = freeMap->Find();
    }
    int start = 0;
    if (numSectors > 8)
        start = numSectors - 9;
    int ind_index[32];
    if (newsector - 8 > 0){
        synchDisk->ReadSector(dataSectors[8], (char*)ind_index);
        for (i = start; i < newsector - 8; i++)
            ind_index[i] = freeMap->Find();
        synchDisk->WriteSector(dataSectors[8], (char*)ind_index);
    }
    numBytes = newsize;
    numSectors = newsector;
    return TRUE;
}