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
    
    /* add in lab5 ex3 */
    if(numSectors <= NumDirect - 1){
        for (int i = 0; i < numSectors; i++){
	        dataSectors[i] = freeMap->Find();
            printf("no.%d allocate %d sector\n",i, dataSectors[i]);
        }
    }
    else {
        for (int i = 0; i < NumDirect - 1; i++){
	        dataSectors[i] = freeMap->Find();
            printf("no.%d allocate %d sector\n",i, dataSectors[i]);
        }
        int secondDirect[NumDirectInOneSector];
        dataSectors[NumDirect - 1] = freeMap->Find();
        for(int i = 0 ; i < numSectors - NumDirect + 1; i++){
            secondDirect[i] = freeMap->Find();
            printf("no.%d allocate %d sector\n",i + NumDirect - 1, secondDirect[i]);
        }
        synchDisk->WriteSector(dataSectors[NumDirect - 1], (char*)secondDirect);
        /*int *test = new int[NumDirectInOneSector];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], (char*)test);
        printf("test:%d\n",test[0]);*/
    }
    
    /* end add */

    /*
    for (int i = 0; i < numSectors; i++){
	    dataSectors[i] = freeMap->Find();
        printf("no.%d allocate %d sector\n",i, dataSectors[i]);
    }*/
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
    /* mod in lab5 ex3*/
    if(numSectors <= NumDirect -1){
        for (int i = 0; i < numSectors; i++) {
	        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	        freeMap->Clear((int) dataSectors[i]);
        }
    }
    else{
        for (int i = 0; i < NumDirect - 1; i++) {
	        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	        freeMap->Clear((int) dataSectors[i]);
        }
        int* secondDirect = new int[NumDirectInOneSector];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], (char*)secondDirect);
        for(int i = 0; i < numSectors - NumDirect + 1; i++){
            ASSERT(freeMap->Test((int) secondDirect[i]));  // ought to be marked!
	        freeMap->Clear((int) secondDirect[i]);
        }
    }
    
    /* end mod */
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
    /*add in lab5 ex3*/
    int maxOffset = (NumDirect - 1) * SectorSize;   //should be 3200 for now
    if(offset < maxOffset)
        return(dataSectors[offset / SectorSize]);
    else{
        int *secondDirect = new int[NumDirectInOneSector];
        synchDisk->ReadSector(dataSectors[NumDirect - 1], (char*)secondDirect);
        int sectorInFile = offset / SectorSize;
        //printf("sector in file:%d\n",sectorInFile);
        return secondDirect[sectorInFile - NumDirect + 1];
    }
    /*end add*/
    //return(dataSectors[offset / SectorSize]);
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
    /*mod in lab5 ex3*/
    bool useSecondDirect = false;
    int *secondDirect;
    if(numSectors > NumDirect - 1){ // file is big, has used second direct
        useSecondDirect = true;
        secondDirect = new int[NumDirectInOneSector];
       // printf("datasector[25]=%d\n",dataSectors[NumDirect - 1]);
        synchDisk->ReadSector(dataSectors[NumDirect - 1], (char*)secondDirect);
       // printf("secondDirect[0]=%d\n",secondDirect[0]);
    }
    if(!useSecondDirect){
        for (i = 0; i < numSectors; i++)
	        printf("no.%d data sectoer:%d \n", i, dataSectors[i]);
    }
    else{
        for (i = 0; i < NumDirect - 1; i++)
	        printf("no.%d data sectoer:%d \n", i, dataSectors[i]);
        
        for(i = 0; i < numSectors - NumDirect + 1; i++){
            printf("no.%d data sectoer:%d \n", i + NumDirect - 1, secondDirect[i]);
        }
    }
    /*end mod*/
    printf("\nFile type: %d", type);
    printf("\nCreate Time:%d\nLast Use Time:%d\nLast Modify Time:%d", 
        createTime, lastUseTime, lastModifyTime);
    printf("\nFile contents:\n");
    //printf("numDirect:%d",NumDirect);
    /*mod in lab5 ex3*/
    if(!useSecondDirect){
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
    else{
        for (i = k = 0; i < NumDirect - 1; i++) {
            printf("sector %d\n",i);
            synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n"); 
        }
        // second direct
        for(i = 0; i < numSectors - NumDirect + 1; i++){
            printf("sector %d\n",i + NumDirect - 1);
            synchDisk->ReadSector(secondDirect[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n"); 
        }
    }
    /*end mod*/
    delete [] data;
}

/* add in lab5 ex5 */
/* append file length*/
bool FileHeader::Append(BitMap *bitmap, int extraLength)
{
    numBytes += extraLength;
    if(numBytes > MaxFileSize)
        return false;
    int oldNumSectors = numSectors;
    numSectors = divRoundUp(numBytes, SectorSize);
    if(oldNumSectors == numSectors)
        return true;
    else if(bitmap->NumClear() < numSectors - oldNumSectors)
        return false;
    else{
        // allocate new space
        if(numSectors < NumDirect - 1){

        }
        else if(oldNumSectors > NumDirect - 1){

        }
        else{

        }
        return true;
    }

}
/* end add */