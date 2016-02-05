#include <iostream>
#include <stdlib.h>
#include <memory.h>
#include <cstring>

#include "../include/heappage.h"
#include "../include/heapfile.h"
#include "../include/bufmgr.h"
#include "../include/db.h"

using namespace std;

//------------------------------------------------------------------
// Constructor of HeapPage
//
// Input     : Page ID
// Output    : None
//------------------------------------------------------------------

void HeapPage::Init(PageID pageNo)
{
	pid = pageNo;
	nextPage = INVALID_PAGE;
	prevPage = INVALID_PAGE;

	numOfSlots = 0;
	fillPtr = 0;
	freeSpace = HEAPPAGE_DATA_SIZE;
	// type; //not initialised

	// slots grow toward the end of the page
	// starting point: slots[0]
	// data grow toward the beginning of the page
	// starting point: data[HEAPPAGE_DATA_SIZE - fillPtr]
}

void HeapPage::SetNextPage(PageID pageNo)
{
	nextPage = pageNo;
}

void HeapPage::SetPrevPage(PageID pageNo)
{
	prevPage = pageNo;
}

PageID HeapPage::GetNextPage()
{
	return nextPage;
}

PageID HeapPage::GetPrevPage()
{
	return prevPage;
}


//------------------------------------------------------------------
// HeapPage::InsertRecord
//
// Input     : Pointer to the record and the record's length 
// Output    : Record ID of the record inserted.
// Purpose   : Insert a record into the page
// Return    : OK if everything went OK, DONE if sufficient space 
//             does not exist
//------------------------------------------------------------------

Status HeapPage::InsertRecord(char *recPtr, int length, RecordID& rid)
{
	// cout << "insert" << "\n";
	// if not enough space, return DONE
	if (freeSpace < length)
	{
		return DONE;
	}

	// calculate new fillPtr
	fillPtr += length;
	// update freespace
	freeSpace -= length;
	freeSpace -= sizeof(Slot);
	// update slot info
	slots[numOfSlots].length = length;
	slots[numOfSlots].offset = HEAPPAGE_DATA_SIZE-fillPtr;
	// set rid
	rid.pageNo = pid;
	rid.slotNo = numOfSlots;
	numOfSlots++;
	// copy data over
	std::memcpy(&data[slots[numOfSlots-1].offset], recPtr, length);
	
	return OK;
}


//------------------------------------------------------------------
// HeapPage::DeleteRecord 
//
// Input    : Record ID
// Output   : None
// Purpose  : Delete a record from the page
// Return   : OK if successful, FAIL otherwise  
//------------------------------------------------------------------ 

Status HeapPage::DeleteRecord(const RecordID& rid)
{
	// cout << "delete" << "\n";
	// if page does not match
	if (rid.pageNo != pid)
	{
		return FAIL;
	}

	// if record does not exist
	if (slots[rid.slotNo].offset == -1)
	{
		return FAIL;
	}

	int roffset = slots[rid.slotNo].offset;
	int rlength = slots[rid.slotNo].length;
	// set slots offset to -1
	slots[rid.slotNo].offset = -1;
	// update free space and fillptr
	freeSpace += rlength;
	fillPtr -= rlength;
	// move each record toward the end of the page to close the gap
	for (int i = rid.slotNo+1; i < numOfSlots; i++)
	{
		std::memcpy(&data[slots[i].offset + rlength], &data[slots[i].offset], rlength);
		slots[i].offset += rlength;
	}

	return OK;
}


//------------------------------------------------------------------
// HeapPage::FirstRecord
//
// Input    : None
// Output   : record id of the first record on a page
// Purpose  : To find the first record on a page
// Return   : OK if successful, DONE otherwise
//------------------------------------------------------------------

Status HeapPage::FirstRecord(RecordID& rid)
{
	// if page is empty
	if (numOfSlots == 0)
	{
		return DONE;
	}

	// find first valid record
	int first = 0;
	while (first < numOfSlots)
	{
		if (slots[first].offset != -1)
		{
			rid.pageNo = pid;
			rid.slotNo = first;
			return OK;
		}
		first++;
	}
	// if no valid record exists
	return DONE;
}


//------------------------------------------------------------------
// HeapPage::NextRecord
//
// Input    : ID of the current record
// Output   : ID of the next record
// Return   : Return DONE if no more records exist on the page; 
//            otherwise OK
//------------------------------------------------------------------

Status HeapPage::NextRecord (RecordID curRid, RecordID& nextRid)
{	
	// if page does not match
	if (curRid.pageNo != pid)
	{
		return DONE;
	}
	// find first valid record after the current one
	int next = curRid.slotNo + 1;
	while  (next < numOfSlots) {
		if (slots[next].offset != -1)
		{
			nextRid.pageNo = pid;
			nextRid.slotNo = next;
			return OK;
		}
		next++;
	}
	// if no valid record exists
	return DONE;
}


//------------------------------------------------------------------
// HeapPage::GetRecord
//
// Input    : Record ID
// Output   : Records length and a copy of the record itself
// Purpose  : To retrieve a _copy_ of a record with ID rid from a page
// Return   : OK if successful, FAIL otherwise
//------------------------------------------------------------------

Status HeapPage::GetRecord(RecordID rid, char *recPtr, int& length)
{
	// if page does not match
	if (rid.pageNo != pid)
	{
		return FAIL;
	}

	// if record no longer valid
	if (rid.slotNo >= numOfSlots || slots[rid.slotNo].offset == -1)
	{
		return FAIL;
	}

	length = slots[rid.slotNo].length;
	std::memcpy(recPtr, &data[slots[rid.slotNo].offset], length);
    return OK;
}


//------------------------------------------------------------------
// HeapPage::ReturnRecord
//
// Input    : Record ID
// Output   : pointer to the record, record's length
// Purpose  : To output a _pointer_ to the record
// Return   : OK if successful, FAIL otherwise
//------------------------------------------------------------------

Status HeapPage::ReturnRecord(RecordID rid, char*& recPtr, int& length)
{
	// if page does not match
	if (rid.pageNo != pid)
	{
		return FAIL;
	}

	// if record no longer valid
	if (rid.slotNo >= numOfSlots || slots[rid.slotNo].offset == -1)
	{
		return FAIL;
	}

	length = slots[rid.slotNo].length;
	recPtr = &data[slots[rid.slotNo].offset];
    return OK;
}


//------------------------------------------------------------------
// HeapPage::AvailableSpace
//
// Input    : None
// Output   : None
// Purpose  : To return the amount of available space
// Return   : The amount of available space on the heap file page.
//------------------------------------------------------------------

int HeapPage::AvailableSpace(void)
{
	return freeSpace;
}


//------------------------------------------------------------------
// HeapPage::IsEmpty
// 
// Input    : None
// Output   : None
// Purpose  : Check if there is any record in the page.
// Return   : true if the HeapPage is empty, and false otherwise.
//------------------------------------------------------------------

bool HeapPage::IsEmpty(void)
{
	if (numOfSlots == 0)
	{
		return true;
	}

	for (int i = 0; i < numOfSlots; i++)
	{
		if (slots[i].offset != -1)
		{
			return false;
		}
	}
	return true;
}


void HeapPage::CompactSlotDir()
{
	int start = 0, end = numOfSlots-1;
	while (end < numOfSlots && slots[end].offset == -1)
	{
		end--;
	}
	while (start < end)
	{
		if (slots[start].offset != -1)
		{
			start++;
		}
		else
		{
			if (slots[end].offset == -1)
			{
				end--;
			}
			else
			{
				slots[start].offset = slots[end].offset;
				slots[start].length = slots[end].length;
				start++;
				end--;
			}
		}
	}

	freeSpace += sizeof(Slot)*(numOfSlots-start);
	numOfSlots = start;
}

int HeapPage::GetNumOfRecords()
{
	int toReturn = numOfSlots;
	// minus all empty slots
	for (int i = 0; i < numOfSlots; i++)
	{
		if (slots[i].offset == -1)
		{
			toReturn--;
		}
	}
	return toReturn;
}
