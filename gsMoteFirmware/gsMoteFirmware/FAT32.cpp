/*
 * FAT32.cpp
 *
 * Created: 9/7/2013 4:51:15 PM
 *  Author: Vlad
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "FAT32.h"
#include "E-000001-000009_firmware_rev_1_0.h"

//***************************************************************************
//Function: to read data from boot sector of SD card, to determine important
//parameters like bytesPerSector, sectorsPerCluster etc.
//Arguments: none
//return: none
//***************************************************************************
unsigned char getBootSectorData (void)
{
struct BS_Structure *bpb; //mapping the buffer onto the structure
struct MBRinfo_Structure *mbr;
struct partitionInfo_Structure *partition;
unsigned long dataSectors;

unusedSectors = 0;

SD_read_block(0,SDBuffer);
bpb = (struct BS_Structure *)SDBuffer;

if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB)   //check if it is boot sector
{
  mbr = (struct MBRinfo_Structure *) SDBuffer;       //if it is not boot sector, it must be MBR
  
  if(mbr->signature != 0xaa55) return 1;       //if it is not even MBR then it's not FAT32
  	
  partition = (struct partitionInfo_Structure *)(mbr->partitionData);//first partition
  unusedSectors = partition->firstSector; //the unused sectors, hidden to the FAT
  
  SD_read_block(partition->firstSector,SDBuffer);//read the bpb sector
  bpb = (struct BS_Structure *)SDBuffer;
  if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB) return 1; 
}

bytesPerSector = bpb->bytesPerSector;
sectorPerCluster = bpb->sectorPerCluster;
reservedSectorCount = bpb->reservedSectorCount;
rootCluster = bpb->rootCluster;
firstDataSector = bpb->hiddenSectors + reservedSectorCount + (bpb->numberofFATs * bpb->FATsize_F32);

dataSectors = bpb->totalSectors_F32
              - bpb->reservedSectorCount
              - ( bpb->numberofFATs * bpb->FATsize_F32);
totalClusters = dataSectors / sectorPerCluster;


if((getSetFreeCluster (TOTAL_FREE, GET, 0)) > totalClusters)  //check if FSinfo free clusters count is valid
     freeClusterCountUpdated = 0;
else
	 freeClusterCountUpdated = 1;
return 0;
}

//***************************************************************************
//Function: to calculate first sector address of any given cluster
//Arguments: cluster number for which first sector is to be found
//return: first sector address
//***************************************************************************
unsigned long getFirstSector(unsigned long clusterNumber)
{
  return (((clusterNumber - 2) * sectorPerCluster) + firstDataSector);
}

//***************************************************************************
//Function: get cluster entry value from FAT to find out the next cluster in the chain
//or set new cluster entry in FAT
//Arguments: 1. current cluster number, 2. get_set (=GET, if next cluster is to be found or = SET,
//if next cluster is to be set 3. next cluster number, if argument#2 = SET, else 0
//return: next cluster number, if if argument#2 = GET, else 0
//****************************************************************************
unsigned long getSetNextCluster (unsigned long clusterNumber,
                                 unsigned char get_set,
                                 unsigned long clusterEntry)
{
unsigned int FATEntryOffset;
unsigned long *FATEntryValue;
unsigned long FATEntrySector;
unsigned char retry = 0;

//get sector number of the cluster entry in the FAT
FATEntrySector = unusedSectors + reservedSectorCount + ((clusterNumber * 4) / bytesPerSector) ;

//get the offset address in that sector number
FATEntryOffset = (unsigned int) ((clusterNumber * 4) % bytesPerSector);

//read the sector into a buffer
SD_read_block(FATEntrySector,SDBuffer);

//get the cluster address from the buffer
FATEntryValue = (unsigned long *) &SDBuffer[FATEntryOffset];

if(get_set == GET)
  return ((*FATEntryValue) & 0x0fffffff);


*FATEntryValue = clusterEntry;   //for setting new value in cluster entry in FAT

SD_write_block(FATEntrySector,SDBuffer,512);

return (0);
}

//********************************************************************************************
//Function: to get or set next free cluster or total free clusters in FSinfo sector of SD card
//Arguments: 1.flag:TOTAL_FREE or NEXT_FREE, 
//			 2.flag: GET or SET 
//			 3.new FS entry, when argument2 is SET; or 0, when argument2 is GET
//return: next free cluster, if arg1 is NEXT_FREE & arg2 is GET
//        total number of free clusters, if arg1 is TOTAL_FREE & arg2 is GET
//		  0xffffffff, if any error or if arg2 is SET
//********************************************************************************************
unsigned long getSetFreeCluster(unsigned char totOrNext, unsigned char get_set, unsigned long FSEntry)
{
struct FSInfo_Structure *FS = (struct FSInfo_Structure *) &SDBuffer;

SD_read_block(unusedSectors + 1,SDBuffer);

if((FS->leadSignature != 0x41615252) || (FS->structureSignature != 0x61417272) || (FS->trailSignature !=0xaa550000))
  return 0xffffffff;

 if(get_set == GET)
 {
   if(totOrNext == TOTAL_FREE)
      return(FS->freeClusterCount);
   else // when totOrNext = NEXT_FREE
      return(FS->nextFreeCluster);
 }
 else
 {
   if(totOrNext == TOTAL_FREE)
      FS->freeClusterCount = FSEntry;
   else // when totOrNext = NEXT_FREE
	  FS->nextFreeCluster = FSEntry;
 
   SD_write_block(unusedSectors + 1,SDBuffer,512);	//update FSinfo
 }
 return 0xffffffff;
}

//***************************************************************************
//Function: to get DIR/FILE list or a single file address (cluster number) or to delete a specified file
//Arguments: #1 - flag: GET_LIST, GET_FILE or DELETE #2 - pointer to file name (0 if arg#1 is GET_LIST)
//return: first cluster of the file, if flag = GET_FILE
//        print file/dir list of the root directory, if flag = GET_LIST
//		  Delete the file mentioned in arg#2, if flag = DELETE
//****************************************************************************
struct dir_Structure* findFiles (unsigned char flag, unsigned char *fileName)
{
unsigned long cluster, sector, firstSector, firstCluster, nextCluster;
struct dir_Structure *dir;
unsigned int i;
unsigned char j;

cluster = rootCluster; //root cluster

while(1)
{
   firstSector = getFirstSector (cluster);

   for(sector = 0; sector < sectorPerCluster; sector++)
   {
     SD_read_block (firstSector + sector,SDBuffer);
	

     for(i=0; i<bytesPerSector; i+=32)
     {
	    dir = (struct dir_Structure *) &SDBuffer[i];

        if(dir->name[0] == EMPTY) //indicates end of the file list of the directory
		{
		  return 0;   
		}
		if((dir->name[0] != DELETED) && (dir->attrib != ATTR_LONG_NAME))
        {
          if((flag == GET_FILE) || (flag == DELETE))
          {
            for(j=0; j<11; j++)
            if(dir->name[j] != fileName[j]) break;
            if(j == 11)
			{
			  if(flag == GET_FILE)
              {
			    appendFileSector = firstSector + sector;
				appendFileLocation = i;
				appendStartCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
				fileSize = dir->fileSize;
			    return (dir);
			  }	
			  else    //when flag = DELETE
			  {
				 firstCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
                
				 //mark file as 'deleted' in FAT table
				 dir->name[0] = DELETED;    
				 SD_write_block (firstSector+sector,SDBuffer,512);
				 			 
				 freeMemoryUpdate (ADD, dir->fileSize);

				 //update next free cluster entry in FSinfo sector
				 cluster = getSetFreeCluster (NEXT_FREE, GET, 0); 
				 if(firstCluster < cluster)
				     getSetFreeCluster (NEXT_FREE, SET, firstCluster);

				 //mark all the clusters allocated to the file as 'free'
			     while(1)  
			     {
			        nextCluster = getSetNextCluster (firstCluster, GET, 0);
					getSetNextCluster (firstCluster, SET, 0);
					if(nextCluster > 0x0ffffff6) 
					   {//file deleted
						   return 0;}
					firstCluster = nextCluster;
			  	 } 
			  }
            }
          }
          else  //invalid flag
		  {
			  return 0;
		  }
       }
     }
   }

   cluster = (getSetNextCluster (cluster, GET, 0));

   if(cluster > 0x0ffffff6)
   	 return 0;
   if(cluster == 0) 
   {//error in getting cluster
	   return 0;}
 }
return 0;
}

//***************************************************************************
//Function: if flag=READ then to read file from SD card and save contents to FRAMRead Buffer 
//if flag=VERIFY then functions will verify whether a specified file is already existing
//Arguments: flag (READ or VERIFY) and pointer to the file name
//return: 0, if normal operation or flag is READ
//	      1, if file is already existing and flag = VERIFY; or if flag=READ and file does not exist
//		  2, if file name is incompatible
//***************************************************************************
unsigned char readFile (unsigned char flag, const char *fileName)
{
struct dir_Structure *dir;
unsigned long cluster, byteCounter = 0, fileSize, firstSector;
unsigned char j, error;

error = convertFileName (fileName); //convert fileName into FAT format
if(error) return 2;

dir = findFiles (GET_FILE, Filename); //get the file location
if(dir == 0) 
{
  if(flag == READ) return (1);
  else return (0);
}

if(flag == VERIFY) return (1);	//specified file name is already existing

cluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;

fileSize = dir->fileSize;


while(1)
{
  firstSector = getFirstSector (cluster);

  for(j=0; j<sectorPerCluster; j++)
  {
    SD_read_block(firstSector + j,FRAMReadBuffer);
	/********************************************************************************* IMPORTANT!!!!!!!!!!!!!!! **********************************************************************************************/
	//put in code here to send the read data sector over Radio since the FRAM buffer is not big enough to hold even one cluster of data
	
    /*
	for(k=0; k<512; k++)
    {
      transmitByte(buffer[k]);
      if ((byteCounter++) >= fileSize ) return 0;
    }
	*/
  }
  cluster = getSetNextCluster (cluster, GET, 0);
  if(cluster == 0) {//Error in getting cluster
	  return 0;}
}
return 0;
}

//***************************************************************************
//Function: to convert normal short file name into FAT format
//Arguments: pointer to the file name
//return: 0, if successful else 1.
//***************************************************************************
unsigned char convertFileName (const char *fileName)
{
bool NoExtension = FALSE;
unsigned char fileNameFAT[11];
unsigned char j, k;

//store the string variable into the file name array if it is of a proper length

if(strlen(fileName) > 15) return 1;
int i=0;
for(; i < strlen(fileName); i++){
	Filename[i] = fileName[i];
}
for(; i < 15; i++) Filename[i] = ' ';


for(j=0; j<12; j++)
if(Filename[j] == '.') break;

if(j>8 && j<12) {//Invalid fileName
	return 1;}
else if (j==12) NoExtension=TRUE;	

for(k=0; k<j; k++) //setting file name
  fileNameFAT[k] = Filename[k];

for(k=j; k<=7; k++) //filling file name trail with blanks
  fileNameFAT[k] = ' ';

if(!NoExtension) j++;
for(k=8; k<11; k++) //setting file extension
{
  if(Filename[j] != 0)
    fileNameFAT[k] = Filename[j++];
  else //filling extension trail with blanks
    while(k<11)
      fileNameFAT[k++] = ' ';
}

for(j=0; j<11; j++) //converting small letters to caps
  if((fileNameFAT[j] >= 0x61) && (fileNameFAT[j] <= 0x7a))
    fileNameFAT[j] -= 0x20;

for(j=0; j<11; j++)
  Filename[j] = fileNameFAT[j];

return 0;
}

//************************************************************************************
//Function: to create a file in FAT32 format in the root directory if given 
//			file name does not exist; if the file already exists then append the data
//Arguments: pointer to the file name
//return: 1 - invalid filename, 2 - no free cluster, 3 - end of cluster chain, 4 - error in getting cluster
//************************************************************************************
unsigned char writeFile (const char* fileName,uint8_t* dataArray,uint32_t lengthOfData){
unsigned char j, data, error, fileCreatedFlag = 0, start = 0, appendFile = 0, sector=0;
unsigned int firstClusterHigh=0, firstClusterLow=0, startBlock=0;  //value 0 is assigned just to avoid warning in compilation
struct dir_Structure *dir;
unsigned long cluster, nextCluster, prevCluster, firstSector, clusterCount, extraMemory;


j = readFile (VERIFY, fileName);

if(j == 1) 
{
  //File already exists, appending data
  appendFile = 1;
  //set cluster to write to as the first cluster of the file
  cluster = appendStartCluster;
  clusterCount=0;
  //iterate through the clusters in the file until the last incomplete cluster is found and set that to cluster
  while(1)
  {
    nextCluster = getSetNextCluster (cluster, GET, 0);
    if(nextCluster == EOF) break;
	cluster = nextCluster;
	clusterCount++;
  }
  //using the size of the file and how many clusters it occupies, deduce the sector offset within the last incomplete cluster
  sector = (fileSize - (clusterCount * sectorPerCluster * bytesPerSector)) / bytesPerSector; //last sector number of the last cluster of the file
  start = 1;
}
else if(j == 2) 
   return 1; //invalid file name

else
{
 //Creating File

  cluster = getSetFreeCluster (NEXT_FREE, GET, 0);
  if(cluster > totalClusters)
     cluster = rootCluster;

  cluster = searchNextFreeCluster(cluster);
   if(cluster == 0)
   {
	   // No free cluster!
	  return 2;
   }
  getSetNextCluster(cluster, SET, EOF);   //set last cluster of the file, marked EOF
   
  firstClusterHigh = (unsigned int) ((cluster & 0xffff0000) >> 16 );
  firstClusterLow = (unsigned int) ( cluster & 0x0000ffff);
  fileSize = 0;
}

//start writing data here

if(start){
  start = 0;
  startBlock = getFirstSector (cluster) + sector;
  SD_read_block (startBlock,SDBuffer);
  j = sector;
}
else{
  startBlock = getFirstSector (cluster);
  j=0;
}
uint32_t writtenData = 0;
uint32_t dataToWrite = 0;
while(lengthOfData!=0){
	//write 1 sector (512 bytes) to the cluster and increase file size by 512 bytes
	if(lengthOfData >= 512 ){
		 writtenData += 512;
		 dataToWrite = 512;
		 lengthOfData -= 512;
	}
	else{
		writtenData += lengthOfData;
		dataToWrite = lengthOfData%512;
		lengthOfData = 0;
	}		   
	fileSize += 512;	//always increment size of file by 512 to avoid losing data when appending to the file. Otherwise if there is data in an unfilled sector it will be overwritten.
	SD_write_block (startBlock,dataArray+writtenData-dataToWrite,dataToWrite);
	j++;
	//if the cluster is filled up, find the next free cluster and set it as the current cluster of the file, also link another free cluster to the file and mark it as the end of file cluster
	if(j == sectorPerCluster) {
		j = 0; 
 
		prevCluster = cluster;
		cluster = searchNextFreeCluster(prevCluster); //look for a free cluster starting from the current cluster
		if(cluster == 0){
		  //No free cluster!
		  return 2;
	   }
		getSetNextCluster(prevCluster, SET, cluster);
		getSetNextCluster(cluster, SET, EOF);   //last cluster of the file, marked EOF
	}
	//otherwise increment the sector offset 
	else startBlock++;       
	getSetFreeCluster (NEXT_FREE, SET, cluster); //update FSinfo next free cluster entry
}

if(appendFile)  //executes this loop if file is to be appended (updates file information and reduce count of free memory)
{
  SD_read_block (appendFileSector,SDBuffer);    
  dir = (struct dir_Structure *) &SDBuffer[appendFileLocation]; 

  dir->lastAccessDate = 0;   //date of last access ignored
  //dir->writeTime = timeFAT;  //setting new time of last write, obtained from RTC
  //dir->writeDate = dateFAT;  //setting new date of last write, obtained from RTC
  extraMemory = fileSize - dir->fileSize;
  dir->fileSize = fileSize;
  SD_write_block (appendFileSector,SDBuffer,512);
  freeMemoryUpdate (REMOVE, extraMemory); //updating free memory count in FSinfo sector;

 //File appended!
  return 0;
}

//executes following portion when new file is created

prevCluster = rootCluster; //root cluster

while(1)
{
   firstSector = getFirstSector (prevCluster);

   for(sector = 0; sector < sectorPerCluster; sector++)
   {
     SD_read_block (firstSector + sector,SDBuffer);
	

     for(int i=0; i<bytesPerSector; i+=32)
     {
	    dir = (struct dir_Structure *) &SDBuffer[i];
		//if file info successfully updated, return from function
		if(fileCreatedFlag)   //to mark last directory entry with 0x00 (empty) mark
		 { 					  //indicating end of the directory file list
		   //dir->name[0] = EMPTY;
		   //SD_writeSingleBlock (firstSector + sector);
           return 0;
         }

        if((dir->name[0] == EMPTY) || (dir->name[0] == DELETED))  //looking for an empty slot to enter file info
		{
		  for(j=0; j<11; j++)
  			dir->name[j] = Filename[j];
		  dir->attrib = ATTR_ARCHIVE;	//settting file attribute as 'archive'
		  dir->NTreserved = 0;			//always set to 0
		  dir->timeTenth = 0;			//always set to 0
		  //dir->createTime = timeFAT; 	//setting time of file creation, obtained from RTC
		  //dir->createDate = dateFAT; 	//setting date of file creation, obtained from RTC
		  dir->lastAccessDate = 0;   	//date of last access ignored
		  //dir->writeTime = timeFAT;  	//setting new time of last write, obtained from RTC
		  //dir->writeDate = dateFAT;  	//setting new date of last write, obtained from RTC
		  dir->firstClusterHI = firstClusterHigh;
		  dir->firstClusterLO = firstClusterLow;
		  dir->fileSize = fileSize;

		  SD_write_block (firstSector + sector,SDBuffer,512);
		  fileCreatedFlag = 1;

		  //File Created!

		  freeMemoryUpdate (REMOVE, fileSize); //updating free memory count in FSinfo sector
	     
        }
     }
   }

   cluster = getSetNextCluster (prevCluster, GET, 0);

   if(cluster > 0x0ffffff6)
   {
      if(cluster == EOF)   //this situation will come when total files in root is multiple of (32*sectorPerCluster)
	  {  
		cluster = searchNextFreeCluster(prevCluster); //find next cluster for root directory entries
		getSetNextCluster(prevCluster, SET, cluster); //link the new cluster of root to the previous cluster
		getSetNextCluster(cluster, SET, EOF);  //set the new cluster as end of the root directory
      } 

      else
      {	
	    //End of Cluster Chain 
	    return 3;
      }
   }
   if(cluster == 0) {//Error in getting cluster 
	   return 4;
	}
   
   prevCluster = cluster;
 }
 
 return 0;
}


//***************************************************************************
//Function: to search for the next free cluster in the root directory
//          starting from a specified cluster
//Arguments: Starting cluster
//return: the next free cluster
//****************************************************************
unsigned long searchNextFreeCluster (unsigned long startCluster)
{
  unsigned long cluster, *value, sector;
  unsigned char i;
    
	startCluster -=  (startCluster % 128);   //to start with the first file in a FAT sector
    for(cluster =startCluster; cluster <totalClusters; cluster+=128) 
    {
      sector = unusedSectors + reservedSectorCount + ((cluster * 4) / bytesPerSector);
      SD_read_block(sector,SDBuffer);
      for(i=0; i<128; i++)
      {
       	 value = (unsigned long *) &SDBuffer[i*4];
         if(((*value) & 0x0fffffff) == 0)
            return(cluster+i);
      }  
    } 

 return 0;
}


//********************************************************************
//Function: to delete a specified file from the root directory
//Arguments: pointer to the file name
//return: none
//********************************************************************
void deleteFile (unsigned char *fileName)
{
  unsigned char error;

  error = convertFileName (fileName);
  if(error) return;

  findFiles (DELETE, Filename);
}

//********************************************************************
//Function: update the free memory count in the FSinfo sector. 
//			Whenever a file is deleted or created, this function will be called
//			to ADD or REMOVE clusters occupied by the file
//Arguments: #1.flag ADD or REMOVE #2.file size in Bytes
//return: none
//********************************************************************
void freeMemoryUpdate (unsigned char flag, unsigned long size)
{
  unsigned long freeClusters;
  //convert file size into number of clusters occupied
  if((size % 512) == 0) size = size / 512;
  else size = (size / 512) +1;
  if((size % 8) == 0) size = size / 8;
  else size = (size / 8) +1;

  if(freeClusterCountUpdated)
  {
	freeClusters = getSetFreeCluster (TOTAL_FREE, GET, 0);
	if(flag == ADD)
  	   freeClusters = freeClusters + size;
	else  //when flag = REMOVE
	   freeClusters = freeClusters - size;
	getSetFreeCluster (TOTAL_FREE, SET, freeClusters);
  }
}