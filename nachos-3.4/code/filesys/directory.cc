// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	    table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++){
        /* lab5 ex2
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
	        return i;
        */
        int length = strlen(name);
        if(length != table[i].nameLength) 
            continue;
        else{
            char *cmpName = new char[length + 1];
            OpenFile *nameFile = new OpenFile(2);
            //printf("in findindex\n");
            //nameFile->Print();
            nameFile->ReadAt(cmpName, length, table[i].nameInFilePosition);
            cmpName[length] = '\0';
            if(!strcmp(cmpName, name)){
                delete []cmpName;
                delete nameFile;
                return i;
            }
            delete []cmpName;
            delete nameFile;
        }
    }
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
	    return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector)
{ 
    int fnameLength = strlen(name);
    char* fname = new char[fnameLength];
    int pos, fpos = 0;
    for(int i = fnameLength - 1; i >= 0; i--){
        if(name[i] == '/'){
            pos = i + 1;
            break;
        }
    }
    for(int i = pos; i < fnameLength; i++ ){
        fname[fpos++] = name[pos++];
    }
    fname[fpos] = '\0';

    if (FindIndex(fname) != -1)
	    return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            //strncpy(table[i].name, name, FileNameMaxLen); 
            table[i].sector = newSector;
            /* add in lab5 ex2 */ 
            OpenFile *nameFile = new OpenFile(2); // need to delete?
            //printf("in directory::add\n");
            //nameFile->Print();
            table[i].nameInFilePosition = nameFile->getPosition();
            table[i].nameLength = strlen(fname);
            //printf("name:%s, length:%d, pos:%d\n", name, table[i].nameLength, table[i].nameInFilePosition);
            nameFile->Write(fname, table[i].nameLength);
            delete nameFile;
            /* end add */
            return TRUE;
	    }
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);

    if (i == -1)
	return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
   for (int i = 0; i < tableSize; i++){
	    if (table[i].inUse){
            char *name = new char[table[i].nameLength + 1];
            OpenFile *nameFile = new OpenFile(2);
            nameFile->ReadAt(name, table[i].nameLength, table[i].nameInFilePosition);
            name[table[i].nameLength] = '\0';
            delete nameFile;
	        printf("%s\n", name);
        }
   }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
        //printf("length:%d, pos:%d\n",table[i].nameLength, table[i].nameInFilePosition);
        char *name = new char[table[i].nameLength + 1];
        OpenFile *nameFile = new OpenFile(2);
        nameFile->ReadAt(name, table[i].nameLength, table[i].nameInFilePosition);
        name[table[i].nameLength] = '\0';
        delete nameFile;
	    printf("Name: %s, Sector: %d\n", name, table[i].sector);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}


/* add in lab5 ex4 */
/* parse path, return header sector*/
int Directory::parsePath(char* name)
{
    int sector;
    OpenFile* dirHeader = new OpenFile(1); //root dir file
    Directory* dir = new Directory(10);
    dir->FetchFrom(dirHeader);
    int namePos = 0;
    int nextLevelDirPos = 0;
    char nextLevelDir[10];
    int length = strlen(name);
    while(namePos < length){
        nextLevelDir[nextLevelDirPos++] = name[namePos];
        if(name[namePos == '/']){
            nextLevelDir[nextLevelDirPos] = '\0';
            sector = dir->Find(nextLevelDir);
            if(sector == -1){
                printf("directory not exists\n");
                ASSERT(FALSE);
            }
            dirHeader = new OpenFile(sector);
            dir->FetchFrom(dirHeader);
            namePos++;
            nextLevelDirPos = 0;
        }
    }
    return sector;
}
/* end add */