/*
 * boot.c - a boot loader template (Assignment 1, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

#include <Uefi.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/FileInfo.h>

/* Use GUID names 'gEfi...' that are already declared in Protocol headers. */
EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiSimpleFileSystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiGraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiFileInfoGuid = EFI_FILE_INFO_ID;

/* Keep these variables global. */
static EFI_HANDLE ImageHandle;
static EFI_SYSTEM_TABLE *SystemTable;
static EFI_BOOT_SERVICES *BootServices;

// Currently uses only EfiBootServicesData
// The function can be extended to accept any type as necessary
static VOID *AllocatePool(UINTN size)
{
	VOID *ptr;
	EFI_STATUS ret = BootServices->AllocatePool(EfiBootServicesData, size, &ptr);
	if (EFI_ERROR(ret))
		return NULL;
	return ptr;
}

static VOID FreePool(VOID *buf)
{
	BootServices->FreePool(buf);
}

static EFI_STATUS OpenKernel(EFI_FILE_PROTOCOL **pvh, EFI_FILE_PROTOCOL **pfh)
{
	EFI_LOADED_IMAGE *li = NULL;
	EFI_FILE_IO_INTERFACE *fio = NULL;
	EFI_FILE_PROTOCOL *vh;
	EFI_STATUS efi_status;

	*pvh = NULL;
	*pfh = NULL;

	efi_status = BootServices->HandleProtocol(ImageHandle,
											  &gEfiLoadedImageProtocolGuid, (void **)&li);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get LoadedImage for BOOTx64.EFI\r\n");
		return efi_status;
	}

	efi_status = BootServices->HandleProtocol(li->DeviceHandle,
											  &gEfiSimpleFileSystemProtocolGuid, (void **)&fio);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get fio\r\n");
		return efi_status;
	}

	efi_status = fio->OpenVolume(fio, pvh);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get the volume handle!\r\n");
		return efi_status;
	}
	vh = *pvh;

	efi_status = vh->Open(vh, pfh, L"\\EFI\\BOOT\\KERNEL",
						  EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get the file handle!\r\n");
		return efi_status;
	}

	return EFI_SUCCESS;
}

static void CloseKernel(EFI_FILE_PROTOCOL *vh, EFI_FILE_PROTOCOL *fh)
{
	vh->Close(vh);
	fh->Close(fh);
}

static UINTN ReadFileSize(EFI_FILE_PROTOCOL *fh)
{
	EFI_STATUS efi_status;
	UINTN file_size;

	//Read the file size.
	efi_status = fh->GetInfo(fh, &gEfiFileInfoGuid, &file_size, NULL);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get file size for KERNEL.\r\n");
	}
	return file_size;
}

static void LoadKernel(EFI_FILE_PROTOCOL *fh, UINTN file_size, void *buffer)
{
	EFI_STATUS efi_status;

	efi_status = fh->Read(fh, &file_size, buffer);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot load KERNEL file.\r\n");
	}
}

static UINT32 *SetGraphicsMode(UINT32 width, UINT32 height)
{
	EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;
	EFI_STATUS efi_status;
	UINT32 mode;

	efi_status = BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
											  NULL, (VOID **)&graphics);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get the GOP handle!\r\n");
		return NULL;
	}

	if (!graphics->Mode || graphics->Mode->MaxMode == 0)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Incorrect GOP mode information!\r\n");
		return NULL;
	}

	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN size = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
	UINT32 *frameBufferDefault = (UINT32 *)graphics->Mode->FrameBufferBase;

	for (mode = 0; mode < graphics->Mode->MaxMode; mode++)
	{
		// Locate BlueGreenRedReserved, aka BGRA (8-bit per color)
		// Resolution width x height (800x600 in our code)
		efi_status = graphics->QueryMode(graphics, mode, &size, &info);
		if (EFI_ERROR(efi_status))
		{
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
											  L"Cannot get graphics information!\r\n");
			break;
		}
		if (info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)
			continue;
		if (info->VerticalResolution != height || info->HorizontalResolution != width)
			continue;

		// Activate (set) this graphics mode
		efi_status = graphics->SetMode(graphics, mode);
		if (EFI_ERROR(efi_status))
		{
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
											  L"Cannot set graphics mode!\r\n");
			break;
		}

		// Return the frame buffer base address
		return (UINT32 *)graphics->Mode->FrameBufferBase;
	}
	
	//Could not set specified graphics mode.
	SystemTable->ConOut->OutputString(SystemTable->ConOut,
											  L"Cannot set the specified graphics mode!\r\n");
	BootServices->Stall(5*1000000);//Wait for five seconds to display the message.
	return frameBufferDefault;
}

/* Use System V ABI rather than EFI/Microsoft ABI. */
typedef void (*kernel_entry_t)(unsigned int *, int, int) __attribute__((sysv_abi));

EFI_STATUS EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
	EFI_FILE_PROTOCOL *vh, *fh;
	EFI_STATUS efi_status;
	UINT32 *fb;
	void *buffer;
	UINTN file_size;

	ImageHandle = imageHandle;
	SystemTable = systemTable;
	BootServices = systemTable->BootServices;

	efi_status = OpenKernel(&vh, &fh);
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	file_size = ReadFileSize(fh);
	buffer = AllocatePool(file_size);
	LoadKernel(fh, file_size, buffer);

	CloseKernel(vh, fh);

	fb = SetGraphicsMode(800, 600);

	// kernel's _start() is at base #0 (pure binary format)
	// cast the function pointer appropriately and call the function
	//
	kernel_entry_t func = (kernel_entry_t)buffer;
	func(fb, 800, 600);

	FreePool(buffer);

	return EFI_SUCCESS;
}