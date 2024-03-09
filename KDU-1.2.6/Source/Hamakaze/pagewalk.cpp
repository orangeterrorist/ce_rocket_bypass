/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2018 - 2021
*
*  TITLE:       PAGEWALK.CPP
*
*  VERSION:     1.12
*
*  DATE:        16 May 2021
*
*  Function to translate virtual to physical addresses, x86-64.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/

#include "global.h"

#define PHY_ADDRESS_MASK                0x000ffffffffff000ull
#define PHY_ADDRESS_MASK_1GB_PAGES      0x000fffffc0000000ull
#define PHY_ADDRESS_MASK_2MB_PAGES      0x000fffffffe00000ull
#define VADDR_ADDRESS_MASK_1GB_PAGES    0x000000003fffffffull
#define VADDR_ADDRESS_MASK_2MB_PAGES    0x00000000001fffffull
#define VADDR_ADDRESS_MASK_4KB_PAGES    0x0000000000000fffull
#define ENTRY_PRESENT_BIT               1
#define ENTRY_PAGE_SIZE_BIT             0x0000000000000080ull

int PwEntryToPhyAddr(ULONG_PTR entry, ULONG_PTR* phyaddr)
{
    if (entry & ENTRY_PRESENT_BIT) {
        *phyaddr = entry & PHY_ADDRESS_MASK;
        return 1;
    }

    return 0;
}

BOOL PwVirtualToPhysical(
    _In_ HANDLE DeviceHandle,
    _In_ provQueryPML4 QueryPML4Routine,
    _In_ provReadPhysicalMemory ReadPhysicalMemoryRoutine,
    _In_ ULONG_PTR VirtualAddress,
    _Out_ ULONG_PTR* PhysicalAddress)
{
    ULONG_PTR   pml4_cr3, selector, table, entry = 0;
    INT         r, shift;

    *PhysicalAddress = 0;

    if (QueryPML4Routine(DeviceHandle, &pml4_cr3) == 0) {
        SetLastError(ERROR_DEVICE_HARDWARE_ERROR);
        return 0;
    }

    table = pml4_cr3 & PHY_ADDRESS_MASK;

    for (r = 0; r < 4; r++) {

        shift = 39 - (r * 9);
        selector = (VirtualAddress >> shift) & 0x1ff;

        if (ReadPhysicalMemoryRoutine(DeviceHandle,
            table + selector * 8,
            &entry,
            sizeof(ULONG_PTR)) == 0)
        {
            //
            // Last error set by called routine.
            //
            return 0;
        }

        if (PwEntryToPhyAddr(entry, &table) == 0) {
            SetLastError(ERROR_INVALID_ADDRESS);
            return 0;
        }

        if (entry & ENTRY_PAGE_SIZE_BIT)
        {
            if (r == 1) {
                table &= PHY_ADDRESS_MASK_1GB_PAGES;
                table += VirtualAddress & VADDR_ADDRESS_MASK_1GB_PAGES;
                *PhysicalAddress = table;
                return 1;
            }

            if (r == 2) {
                table &= PHY_ADDRESS_MASK_2MB_PAGES;
                table += VirtualAddress & VADDR_ADDRESS_MASK_2MB_PAGES;
                *PhysicalAddress = table;
                return 1;
            }
        }
    }

    table += VirtualAddress & VADDR_ADDRESS_MASK_4KB_PAGES;
    *PhysicalAddress = table;

    return 1;
}
