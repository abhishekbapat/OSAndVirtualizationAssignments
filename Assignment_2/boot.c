/*
 * A UEFI bootloader implementation that loads a kernel binary into memory.
 * Code written for ECE6504 OsAndVirtualization assignment 1.
 * Code belongs to Abhishek Bapat. PID: abapat28.
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

// Wrapper method to allocate memory.
static VOID *AllocatePool(UINTN size, EFI_MEMORY_TYPE memoryType)
{
	VOID *ptr;
	EFI_STATUS ret = BootServices->AllocatePool(memoryType, size, &ptr);
	if (EFI_ERROR(ret))
		return NULL;
	return ptr;
}

// Wrapper method to free memory buffer.
static VOID FreePool(VOID *buf)
{
	BootServices->FreePool(buf);
}

// Wrapper method used to allocate memory in contiguous 4kb pages.
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

// Opens the kernel file.
static EFI_STATUS OpenFile(EFI_FILE_PROTOCOL **pvh, EFI_FILE_PROTOCOL **pfh, CHAR16 *full_file_path)
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

	efi_status = vh->Open(vh, pfh, full_file_path,
						  EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(efi_status))
	{
		SystemTable->ConOut->OutputString(SystemTable->ConOut,
										  L"Cannot get the file handle!\r\n");
		return efi_status;
	}

	return EFI_SUCCESS;
}

// Closer the file handles.
static void CloseFile(EFI_FILE_PROTOCOL *vh, EFI_FILE_PROTOCOL *fh)
{
	vh->Close(vh);
	fh->Close(fh);
}

// Reads the size of the file whose handle is passed to it.
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

// Loads the binary file into memory buffer from the file handle.
static EFI_STATUS LoadBinaryFileInBuffer(EFI_FILE_PROTOCOL *fh, UINTN file_size, void *buffer)
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

// Method to set the graphics mode as BGRA.
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

// Method thats fetches the memory map and calls ExitBootServices.
static EFI_STATUS ExitBootServicesHook(EFI_HANDLE imageHandle)
{
	UINTN descriptorSize;
	UINT32 descriptorVersion;
	EFI_MEMORY_DESCRIPTOR *memoryMap;
	EFI_STATUS efi_status;
	UINTN memoryMapSize = 0;
	UINTN memoryMapKey;
	memoryMap = NULL;
	while (1)
	{
		efi_status = BootServices->GetMemoryMap(&memoryMapSize, memoryMap, &memoryMapKey, &descriptorSize, &descriptorVersion);
		if (efi_status == EFI_BUFFER_TOO_SMALL)
		{
			memoryMap = AllocatePool(memoryMapSize, EfiBootServicesData);
		}
		else if (EFI_ERROR(efi_status))
		{
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
											  L"Error getting memory map size.\r\n");
			return efi_status;
		}
		efi_status = BootServices->GetMemoryMap(&memoryMapSize, memoryMap, &memoryMapKey, &descriptorSize, &descriptorVersion);
		if (EFI_ERROR(efi_status))
		{
			FreePool(memoryMap);
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
											  L"Error getting memory map.\r\n");
			return efi_status;
		}

		efi_status = BootServices->ExitBootServices(imageHandle, memoryMapKey);
		if (efi_status == EFI_INVALID_PARAMETER)
		{
			FreePool(memoryMap);
		}
		else if (EFI_ERROR(efi_status))
		{
			SystemTable->ConOut->OutputString(SystemTable->ConOut,
											  L"Error exiting boot services.\r\n");
			FreePool(memoryMap);
			return efi_status;
		}
		else
		{
			break;
		}
	}

	return efi_status;
}

/* Use System V ABI rather than EFI/Microsoft ABI. */
typedef void (*kernel_entry_t)(unsigned int *, int, int, void *) __attribute__((sysv_abi));

// Entry point of the boot loader.
EFI_STATUS EFIAPI
efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE *systemTable)
{
	// Initialize some local variables.
	EFI_FILE_PROTOCOL *kvh, *kfh, *uvh, *ufh;
	EFI_STATUS efi_status;
	UINT32 *fb;
	void *kernel_buffer;
	void *user_buffer;
	EFI_PHYSICAL_ADDRESS page_table_base = 0x0ULL;
	EFI_PHYSICAL_ADDRESS kernel_base = 0x0ULL;
	EFI_PHYSICAL_ADDRESS user_base = 0x0ULL;
	UINTN page_table_pages = EFI_SIZE_TO_PAGES(SIZE_8MB + SIZE_16KB + SIZE_8KB + SIZE_4KB); //One extra page for buffer. 2055 4kb pages
	UINTN kernel_file_size = 0;
	UINTN user_file_size = 0;

	// Set global variables.
	ImageHandle = imageHandle;
	SystemTable = systemTable;
	BootServices = systemTable->BootServices;

	efi_status = OpenFile(&kvh, &kfh, L"\\EFI\\BOOT\\KERNEL"); // Open the kernel file for reading.
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	efi_status = ReadFileSize(kfh, &kernel_file_size); // Read the kernel file size.
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	efi_status = AllocatePages(AllocateAnyPages, EfiLoaderCode, EFI_SIZE_TO_PAGES(kernel_file_size), &kernel_base); // Page aligned memory for kernel.
	if(EFI_ERROR(efi_status))
	{
		BootServices->Stall(5*1000000); // 5 seconds
		return efi_status;
	}
	kernel_buffer = (void *)kernel_base;

	efi_status = LoadBinaryFileInBuffer(kfh, kernel_file_size, kernel_buffer); // Load the kernel binary into memory.
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	CloseFile(kvh, kfh); // Close the kernel file.

	efi_status = OpenFile(&uvh, &ufh, L"\\EFI\\BOOT\\USER"); // Open the kernel file for reading.
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	efi_status = ReadFileSize(ufh, &user_file_size); // Read the kernel file size.
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	efi_status = AllocatePages(AllocateAnyPages, EfiLoaderCode, EFI_SIZE_TO_PAGES(user_file_size), &user_base); // Page aligned memory for kernel.
	if(EFI_ERROR(efi_status))
	{
		BootServices->Stall(5*1000000); // 5 seconds
		return efi_status;
	}
	user_buffer = (void *)user_base;

	efi_status = LoadBinaryFileInBuffer(ufh, user_file_size, user_buffer); // Load the kernel binary into memory.
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	CloseFile(uvh, ufh); // Close the kernel file.

	// Allocate pages for the kernel to initialize page table.
	efi_status = AllocatePages(AllocateAnyPages, EfiLoaderData, page_table_pages, &page_table_base);
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	fb = SetGraphicsMode(800, 600); // Set the graphics mode to 800x600 BGRA.

	efi_status = ExitBootServicesHook(ImageHandle); // Call ExitBootServices.
	if (EFI_ERROR(efi_status))
	{
		BootServices->Stall(5 * 1000000); // 5 seconds
		return efi_status;
	}

	// kernel's _start() is at base #0 (pure binary format)
	// cast the function pointer appropriately and call the function
	kernel_entry_t func = (kernel_entry_t)kernel_buffer;
	func(fb, 800, 600, (void *)page_table_base); // call the kernel function.

	return EFI_SUCCESS;
}
