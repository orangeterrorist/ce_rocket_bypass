/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2020 - 2022
*
*  TITLE:       MAPMEM.H
*
*  VERSION:     1.26
*
*  DATE:        15 Oct 2022
*
*  MAPMEM driver interface header.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/

#pragma once

//
// GIGABYTE GDRV driver interface for CVE-2018-19320.
//

#define GDRV_DEVICE_TYPE        (DWORD)0xC350

#define GDRV_VIRTUALTOPHYSICAL  (DWORD)0xA03
#define GRV_IOCTL_INDEX         (DWORD)0x800 

#define IOCTL_GDRV_VIRTUALTOPHYSICAL            \
    CTL_CODE(GDRV_DEVICE_TYPE, GDRV_VIRTUALTOPHYSICAL, METHOD_BUFFERED, FILE_ANY_ACCESS) //0xC350280C

#define IOCTL_GDRV_MAP_USER_PHYSICAL_MEMORY     \
    CTL_CODE(GDRV_DEVICE_TYPE, GRV_IOCTL_INDEX+1, METHOD_BUFFERED, FILE_ANY_ACCESS) //0xC3502004

#define IOCTL_GDRV_UNMAP_USER_PHYSICAL_MEMORY   \
    CTL_CODE(GDRV_DEVICE_TYPE, GRV_IOCTL_INDEX+2, METHOD_BUFFERED, FILE_ANY_ACCESS) //0xC3502008

//
// SuperMicro SUPERBMC driver interface.
//

#define SUPERBMC_DEVICE_TYPE  (DWORD)0x8010

#define SUPERBMC_MAP_FUNCID   (DWORD)0x88E
#define SUPERBMC_UNMAP_FUNCID (DWORD)0x890

#define IOCTL_SUPERBMC_MAP_USER_PHYSICAL_MEMORY      \
    CTL_CODE(SUPERBMC_DEVICE_TYPE, SUPERBMC_MAP_FUNCID, METHOD_BUFFERED, FILE_ANY_ACCESS) //0x80102238

#define IOCTL_SUPERBMC_UNMAP_USER_PHYSICAL_MEMORY    \
    CTL_CODE(SUPERBMC_DEVICE_TYPE, SUPERBMC_UNMAP_FUNCID, METHOD_BUFFERED, FILE_ANY_ACCESS) //0x80102240

//
// Codesys driver interface (basically copy-paste from mapmem ddk).
//

#define FILE_DEVICE_MAPMEM            (DWORD)0x00008000
#define MAPMEM_IOCTL_INDEX            (DWORD)0x800

#define IOCTL_MAPMEM_MAP_USER_PHYSICAL_MEMORY   CTL_CODE(FILE_DEVICE_MAPMEM , \
                                                         MAPMEM_IOCTL_INDEX,  \
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)

#define IOCTL_MAPMEM_UNMAP_USER_PHYSICAL_MEMORY CTL_CODE(FILE_DEVICE_MAPMEM,  \
                                                         MAPMEM_IOCTL_INDEX+1,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)

typedef struct _GIO_VIRTUAL_TO_PHYSICAL {
    ULARGE_INTEGER Address;
} GIO_VIRTUAL_TO_PHYSICAL, * PGIO_VIRTUAL_TO_PHYSICAL;

typedef struct _MAPMEM_PHYSICAL_MEMORY_INFO {
    INTERFACE_TYPE   InterfaceType; 
    ULONG            BusNumber;     
    PHYSICAL_ADDRESS BusAddress;
    ULONG            AddressSpace;  
    ULONG            Length;        
} MAPMEM_PHYSICAL_MEMORY_INFO, * PMAPMEM_PHYSICAL_MEMORY_INFO;

BOOL WINAPI MapMemVirtualToPhysical(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG_PTR VirtualAddress,
    _Out_ ULONG_PTR* PhysicalAddress);

BOOL WINAPI MapMemReadPhysicalMemory(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG_PTR PhysicalAddress,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength);

BOOL WINAPI MapMemWritePhysicalMemory(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG_PTR PhysicalAddress,
    _In_reads_bytes_(NumberOfBytes) PVOID Buffer,
    _In_ ULONG NumberOfBytes);

BOOL WINAPI MapMemWriteKernelVirtualMemory(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG_PTR Address,
    _Out_writes_bytes_(NumberOfBytes) PVOID Buffer,
    _In_ ULONG NumberOfBytes);

BOOL WINAPI MapMemReadKernelVirtualMemory(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG_PTR Address,
    _Out_writes_bytes_(NumberOfBytes) PVOID Buffer,
    _In_ ULONG NumberOfBytes);

BOOL WINAPI MapMemQueryPML4Value(
    _In_ HANDLE DeviceHandle,
    _Out_ ULONG_PTR* Value);

BOOL WINAPI MapMemRegisterDriver(
    _In_ HANDLE DeviceHandle,
    _In_opt_ PVOID Param);
