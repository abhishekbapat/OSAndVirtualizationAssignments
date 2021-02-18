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
static VOID *AllocatePool(UINTN size, EFI_MEMORY_TYPE memoryType)
{
	VOID *ptr;
	EFI_STATUS ret = BootServices->AllocatePool(memoryType, size, &ptr);
	if (EFI_ERROR(ret))
		return NULL;
	return ptr;
}

static VOID FreePool(VOID *buf)
{
	BootServices->FreePool(buf);
}

static EFI_STATUS AllocatePages(EFI_ALLOCATE_TYPE type, EFI_MEMORY_TYPE memory_type, UINTN pages, UINTN *base)
{
	EFI_STATUS efi_status;
	efi_status = BootServices->AllocatePages(type, memory_type, pages, base);
	if (efi_status == EFI_INVALID_PARAMETER)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Could not allocate pages due to invalid parameters.\r\n");
	}
	else if (efi_status == EFI_OUT_OF_RESOURCES)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Could not allocate pages as enough resources are not available.\r\n");
	}
	else if (efi_status == EFI_NOT_FOUND)
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Could not allocate pages as the requested pages could not be found.\r\n");
	}
	return efi_status;
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

static EFI_STATUS ReadFileSize(EFI_FILE_PROTOCOL *fh, UINTN *file_size)
{
	EFI_STATUS efi_status;
	EFI_FILE_INFO *file_info;
	UINTN file_info_size = 0;

	//Read the file info size.
	efi_status = fh->GetInfo(fh, &gEfiFileInfoGuid, &file_info_size, NULL);
	if (efi_status == EFI_BUFFER_TOO_SMALL)
	{
		file_info = AllocatePool(file_info_size, EfiBootServicesData);
	}
	else
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get file size for KERNEL.\r\n");
		return efi_status;
	}

	efi_status = fh->GetInfo(fh, &gEfiFileInfoGuid, &file_info_size, file_info);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get file KERNEL file info.\r\n");
	}
	else
	{
		*file_size = file_info->FileSize;
	}
	FreePool(file_info);

	return efi_status;
}

static EFI_STATUS LoadKernel(EFI_FILE_PROTOCOL *fh, UINTN file_size, void *buffer)
{
	EFI_STATUS efi_status;

	efi_status = fh->Read(fh, &file_size, buffer);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot load KERNEL file.\r\n");
	}
	return efi_status;
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
	BootServices->Stall(5 * 1000000); //Wait for five seconds to display the message.
	return frameBufferDefault;
}

static EFI_STATUS ExitBootServicesHook(EFI_HANDLE imageHandle)
{
	UINTN descriptorSize;
	UINT32 descriptorVersion;
	EFI_MEMORY_DESCRIPTOR *memoryMap;
	EFI_STATUS efi_status;
	UINTN memoryMapSize = 0;
	UINTN memoryMapKey;
	memoryMap = NULL;

	do
	{
		efi_status = BootServices->GetMemoryMap(&memoryMapSize, memoryMap, &memoryMapKey, &descriptorSize, &descriptorVersion);
		if (efi_status == EFI_BUFFER_TOO_SMALL)
		{
			memoryMap = AllocatePool(memoryMapSize, EfiBootServicesData);
		}
		else if (efi_status == EFI_SUCCESS)
		{
			BootServices->ExitBootServices(imageHandle, memoryMapKey);
			break;
		}
		else
		{
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
											  L"Error reading the memory map size.\r\n");
			break;
		}
	} while (1);

	return efi_status;
}

/* Use System V ABI rather than EFI/Microsoft ABI. */
typedef void (*kernel_entry_t)(unsigned int *, int, int, EFI_PHYSICAL_ADDRESS *) __attribute__((sysv_abi));

EFI_STATUS EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
	EFI_FILE_PROTOCOL *vh, *fh;
	EFI_STATUS efi_status;
	UINT32 *fb;
	void *buffer;
	EFI_PHYSICAL_ADDRESS page_table_base = 0x0000000100000000ULL;
	UINTN page_table_pages = EFI_SIZE_TO_PAGES(SIZE_8MB + SIZE_16KB + SIZE_8KB + SIZE_4KB);//One extra page for buffer. 2055 4kb pages
	UINTN file_size = 0;

	ImageHandle = imageHandle;
	SystemTable = systemTable;
	BootServices = systemTable->BootServices;

	efi_status = OpenKernel(&vh, &fh);
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	efi_status = ReadFileSize(fh, &file_size);
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds0x0000000100000000ULL
		return efi_status;
	}

	buffer = AllocatePool(file_size, EfiPersistentMemory);
	efi_status = LoadKernel(fh, file_size, buffer);
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	CloseKernel(vh, fh);

	efi_status = AllocatePages(AllocateMaxAddress, EfiLoaderData, page_table_pages, &page_table_base);
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	fb = SetGraphicsMode(800, 600);

	// page_table_base = AllocatePool(page_table_size, EfiPersistentMemory);
	// page_table_base = (void *) (((unsigned long long) page_table_base + 4095) & (~4095ULL));

	efi_status = ExitBootServicesHook(ImageHandle);
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	// kernel's _start() is at base #0 (pure binary format)
	// cast the function pointer appropriately and call the function

	kernel_entry_t func = (kernel_entry_t)buffer;
	func(fb, 800, 600, &page_table_base);

	// FreePool(buffer);

	return EFI_SUCCESS;
}
