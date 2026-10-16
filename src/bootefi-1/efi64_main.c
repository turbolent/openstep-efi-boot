#include "loader_diagnostics.h"
/* Native EFI64 front end with a fixed-width bridge to the IA32 loader.
 * EFI handles remain opaque tokens; allocations and copied protocol data
 * passed to boot-2 are below 4 GiB. No OPENSTEP headers enter this unit. */
#include "efi64_bridge.h"
#include "firmware_console.h"
#include "efi_reservations.h"

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_HANDLE gImageHandle;
EFI_STATUS efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE *);
/* Keep a DIR64 relocation even when every code reference is RIP-relative. */
EFI_STATUS (*volatile efi64_entry)(EFI_HANDLE, EFI_SYSTEM_TABLE *) = efi_main;
extern const unsigned char core_image[];
extern void bridge_gate64(void);
extern void bridge_enter32(UINTN, UINTN, EFI64_BRIDGE *, void *);
static EFI64_BRIDGE bridge;
static UINT64 gdt[512];
static struct { UINT16 limit; UINT64 base; } __attribute__((packed)) gdtr;
static EFI_HANDLE handles[256];
static EFI_BLOCK_IO_PROTOCOL *blocks[256];
static UINT32 nhandles;
static UINTN last_key;
static UINT32 key_token;

#ifdef EFI64_HOST_TEST
#include <string.h>
#else
void *memcpy(void *d, const void *s, UINTN n)
{ UINT8 *p = d; const UINT8 *q = s; while (n--) *p++ = *q++; return d; }
void *memset(void *d, int c, UINTN n)
{ UINT8 *p = d; while (n--) *p++ = (UINT8)c; return d; }
#endif
static int equal(const EFI_GUID *a, const EFI_GUID *b)
{ UINTN i; for (i=0;i<sizeof(*a);i++) if (((UINT8 *)a)[i] != ((UINT8 *)b)[i]) return 0; return 1; }
static void say(CHAR16 *s) { gST->ConOut->OutputString(gST->ConOut, s); }
static void field(CHAR16 *name, UINT64 value)
{
    CHAR16 text[19];
    unsigned i;
    text[0] = '0'; text[1] = 'x'; text[18] = 0;
    for (i = 0; i < 16; i++) {
        unsigned digit = (unsigned)(value >> ((15-i)*4)) & 15;
        text[i+2] = (CHAR16)(digit < 10 ? '0'+digit : 'A'+digit-10);
    }
    say(name); say(text); say(L"\r\n");
}

static EFI_STATUS low_alloc(UINT32 size, void **out)
{
    UINT64 address = 0xffffffffULL;
    UINTN pages = ((UINTN)size + 16 + 4095) / 4096;
    EFI_STATUS st = gBS->AllocatePages(AllocateMaxAddress, EfiLoaderData, pages, &address);
    if (!EFI_ERROR(st)) {
        UINT64 *p = (void *)(UINTN)address;
        p[0] = pages;
        p[1] = 0;
        *out = p + 2;
    }
    return st;
}
static UINT32 token(EFI_HANDLE handle)
{
    UINT32 i;
    for (i=0;i<nhandles;i++) if (handles[i] == handle) return i+1;
    if (!handle || nhandles == 256) return 0;
    handles[nhandles++] = handle;
    return nhandles;
}
static UINT32 status32(EFI_STATUS st)
{ return (UINT32)(st & 0x7fffffffU) | ((st >> 63) ? 0x80000000U : 0); }
#define PTR(n) ((void *)(UINTN)a[n])
#define U32(n) ((UINT32 *)PTR(n))

static EFI_STATUS dispatch(UINT32 *a)
{
    EFI_STATUS st;
    switch (a[0]) {
    case BR_ALLOC: {
        void *p = 0;
        st = low_alloc(a[1], &p);
        *U32(2) = (UINT32)(UINTN)p;
        return st;
    }
    case BR_FREE: {
        UINT64 *p = (UINT64 *)PTR(1) - 2;
        return gBS->FreePages((UINTN)p, p[0]);
    }
    case BR_PAGES:
        /* The fixed kernel/linker range contains executable sarld code. */
        return gBS->AllocatePages(a[1], EfiLoaderCode, a[3], PTR(4));
    case BR_MAP: {
        UINTN size = *U32(1), stride = sizeof(EFI_MEMORY_DESCRIPTOR), key = 0;
        st = gBS->GetMemoryMap(&size, PTR(2), &key, &stride, PTR(5));
        if (size > 0xffffffffU || stride > 0xffffffffU) return EFI_UNSUPPORTED;
        *U32(1) = (UINT32)size; *U32(4) = (UINT32)stride;
        if (!EFI_ERROR(st)) { last_key = key; *U32(3) = ++key_token; }
        return st;
    }
    case BR_LOCATE: {
        EFI_HANDLE found[256];
        UINTN size = sizeof(found), i;
        st = gBS->LocateHandle(ByProtocol, PTR(1), 0, &size, found);
        if (st == EFI_BUFFER_TOO_SMALL) return EFI_OUT_OF_RESOURCES;
        if (EFI_ERROR(st)) return st;
        size = size / sizeof(EFI_HANDLE) * 4;
        if (!a[3] || *U32(2) < size) { *U32(2) = (UINT32)size; return EFI_BUFFER_TOO_SMALL; }
        for (i=0;i<size/4;i++) {
            UINT32 id = token(found[i]);
            if (!id) return EFI_OUT_OF_RESOURCES;
            U32(3)[i] = id;
        }
        *U32(2) = (UINT32)size;
        return EFI_SUCCESS;
    }
    case BR_PROTOCOL: {
        static const EFI_GUID block_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
        static const EFI_GUID image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
        static const EFI_GUID path_guid = EFI_DEVICE_PATH_PROTOCOL_GUID;
        void *native = 0, *copy = 0;
        UINT32 id = a[1];
        if (!id || id > nhandles) return EFI_UNSUPPORTED;
        st = gBS->HandleProtocol(handles[id-1], PTR(2), &native);
        if (EFI_ERROR(st)) return st;
        if (equal(PTR(2), &block_guid)) {
            EFI_BLOCK_IO_PROTOCOL *bio = native;
            EFI32_BLOCK *block;
            st = low_alloc(sizeof(*block), &copy);
            if (EFI_ERROR(st)) return st;
            block = copy;
            memset(block, 0, sizeof(*block));
            block->revision = bio->Revision;
            block->media = (UINT32)(UINTN)&block->media_value;
            block->read = a[4];
            block->media_value = *bio->Media;
            block->handle = id;
            blocks[id-1] = bio;
        } else if (equal(PTR(2), &image_guid)) {
            EFI_LOADED_IMAGE_PROTOCOL *image = native;
            UINT32 device = token(image->DeviceHandle);
            if (!device) return EFI_OUT_OF_RESOURCES;
            st = low_alloc(sizeof(EFI32_IMAGE), &copy);
            if (EFI_ERROR(st)) return st;
            memset(copy, 0, sizeof(EFI32_IMAGE));
            ((EFI32_IMAGE *)copy)->device = device;
        } else if (equal(PTR(2), &path_guid)) {
            UINT8 *path = native;
            UINT32 size = 0, len;
            do {
                len = path[size+2] | ((UINT32)path[size+3] << 8);
                if (len < 4 || size + len > 65536) return EFI_UNSUPPORTED;
                size += len;
            } while (path[size-len] != 0x7f || path[size-len+1] != 0xff);
            st = low_alloc(size, &copy);
            if (EFI_ERROR(st)) return st;
            memcpy(copy, path, size);
        } else return EFI_UNSUPPORTED;
        *U32(3) = (UINT32)(UINTN)copy;
        return EFI_SUCCESS;
    }
    case BR_READ: {
        EFI_BLOCK_IO_PROTOCOL *bio;
        if (!a[1] || a[1] > nhandles || !(bio = blocks[a[1]-1])) return EFI_UNSUPPORTED;
        return bio->ReadBlocks(bio, a[2], a[3] | ((UINT64)a[4] << 32), a[5], PTR(6));
    }
    case BR_OUTPUT: return gST->ConOut->OutputString(gST->ConOut, PTR(1));
    case BR_INPUT: return gST->ConIn->ReadKeyStroke(gST->ConIn, PTR(1));
    case BR_STALL: return gBS->Stall(a[1]);
    case BR_WATCHDOG: return gBS->SetWatchdogTimer(a[1], 0, 0, 0);
    case BR_EXIT:
        if (a[1] != key_token) return EFIERR(2);
        return gBS->ExitBootServices(gImageHandle, last_key);
    case BR_GRAPHICS: return firmware_framebuffer(a[1], a[2], PTR(3));
    case BR_CHECKPOINT:
        if (OPENSTEP_DIAGNOSTICS) say(L"[B4] IA32 core called back into native EFI64.\r\n");
        return EFI_SUCCESS;
    default: return EFI_UNSUPPORTED;
    }
}

UINT32 bridge_dispatch(UINT32 *arguments)
{
    EFI_STATUS st;
    __asm__ volatile("sti");
    st = dispatch(arguments);
    __asm__ volatile("cli");
    return status32(st);
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *table)
{
    UINT32 pe, optional, sections, entry, image_size, image_base, i, count;
    UINT64 base;
    void *stack = 0, *config = 0;
    UINT8 *loaded;
    FIRMWARE_CONSOLE console = {0};
    EFI_STATUS status;
    CHAR16 *error;
    UINTN core_pages = 0;
    gST = table; gBS = table->BootServices; gImageHandle = image;
    if (!gST->ConOut) return EFI_UNSUPPORTED;
    firmware_console_text(&console);
    if (OPENSTEP_DIAGNOSTICS) say(L"[B0] OPENSTEP EFI64: native entry, text console requested.\r\n");
    if (OPENSTEP_DIAGNOSTICS) field(L"Native entry address: ", (UINTN)efi64_entry);
    if ((UINTN)&bridge_gate64 > 0xffffffffULL || (UINTN)&bridge > 0xffffffffULL) {
        error = L"EFI64 bridge must be loaded below 4 GiB\r\n";
        status = EFI_UNSUPPORTED; goto failed;
    }
    /* Trusted embedded PE32, validated while building. Copy virtual sections,
     * including zero-filled BSS, into the core's fixed link address. */
    pe = *(const UINT32 *)(core_image + 0x3c);
    optional = pe + 24;
    count = *(const UINT16 *)(core_image + pe + 6);
    sections = optional + *(const UINT16 *)(core_image + pe + 20);
    entry = *(const UINT32 *)(core_image + optional + 16);
    image_base = *(const UINT32 *)(core_image + optional + 28);
    image_size = *(const UINT32 *)(core_image + optional + 56);
    base = image_base;
    status = gBS->AllocatePages(AllocateAddress, EfiLoaderCode,
                               (image_size + 4095) / 4096, &base);
    if (EFI_ERROR(status)) {
        error = L"Cannot reserve IA32 loader address\r\n"; goto failed;
    }
    core_pages = (image_size + 4095) / 4096;
    if (OPENSTEP_DIAGNOSTICS) say(L"[B1] IA32 core range reserved; copying image.\r\n");
    loaded = (void *)(UINTN)base;
    memset(loaded, 0, image_size);
    for (i=0;i<count;i++) {
        const UINT32 *section = (const UINT32 *)(core_image + sections + i*40);
        memcpy(loaded + section[3], core_image + section[5], section[4]);
    }
    status = low_alloc(256*1024, &stack);
    if (EFI_ERROR(status)) { error = L"Cannot reserve low stack\r\n"; goto failed; }
    status = table->NumberOfTableEntries > 256 ? EFI_UNSUPPORTED :
        low_alloc((UINT32)table->NumberOfTableEntries * 20, &config);
    if (EFI_ERROR(status)) { error = L"Cannot copy configuration tables\r\n"; goto failed; }
    if (OPENSTEP_DIAGNOSTICS) field(L"IA32 stack base: ", (UINTN)stack);
    bridge.tables = (UINT32)(UINTN)config;
    for (i=0;i<table->NumberOfTableEntries;i++) {
        EFI_CONFIGURATION_TABLE *t = &table->ConfigurationTable[i];
        if ((UINTN)t->VendorTable <= 0xffffffffULL) {
            UINT8 *dest = (UINT8 *)config + bridge.table_count++ * 20;
            memcpy(dest, &t->VendorGuid, 16);
            *(UINT32 *)(dest + 16) = (UINT32)(UINTN)t->VendorTable;
        }
    }
    status = efi_reserve_ranges();
    if (EFI_ERROR(status)) {
        efi_reservation_wait();
        error = L"Fixed workspace unavailable; boot stopped before IA32.\r\n";
        goto failed;
    }
    /* Retain firmware selectors for its IDT/exception handlers. Append flat
     * compatibility, long-mode and data descriptors without replacing any. */
    __asm__ volatile("sgdt %0" : "=m"(gdtr));
    count = (gdtr.limit + 8) / 8;
    if (count > 509) {
        error = L"Firmware GDT is too large\r\n";
        status = EFI_UNSUPPORTED; goto failed;
    }
    memcpy(gdt, (void *)(UINTN)gdtr.base, gdtr.limit + 1);
    gdt[count] = 0x00cf9a000000ffffULL;
    gdt[count+1] = 0x00af9a000000ffffULL;
    gdt[count+2] = 0x00cf92000000ffffULL;
    bridge.code32 = (UINT16)(count*8);
    bridge.code64 = (UINT16)((count+1)*8);
    bridge.data = (UINT16)((count+2)*8);
    bridge.gate = (UINT32)(UINTN)bridge_gate64;
    bridge.image = token(image);
    gdtr.base = (UINTN)gdt;
    gdtr.limit = (UINT16)((count+3)*8-1);
    if (OPENSTEP_DIAGNOSTICS) say(L"[B2] Core, stack, tables and GDT prepared; still in native EFI.\r\n");
#ifdef OPENSTEP_PAUSE_BEFORE_HANDOFF
    say(L"Photograph this screen. B = boot OPENSTEP; Esc = return to firmware.\r\n");
    if (!gST->ConIn) {
        status = EFI_UNSUPPORTED; error = L"Firmware keyboard unavailable.\r\n"; goto failed;
    }
    for (;;) {
        EFI_INPUT_KEY key;
        status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
        if (!EFI_ERROR(status)) {
            if (key.ScanCode == 0x17 || key.UnicodeChar == 27) {
                error = L"Boot cancelled before the IA32 transition.\r\n"; goto failed;
            }
            if (key.UnicodeChar == 'b' || key.UnicodeChar == 'B') break;
        } else if (status != EFIERR(6)) {
            error = L"Firmware keyboard read failed.\r\n"; goto failed;
        }
        gBS->Stall(10000);
    }
#endif
    if (OPENSTEP_DIAGNOSTICS) say(L"[B3] Entering IA32 compatibility mode.\r\n");
    bridge_enter32(image_base + entry, ((UINTN)stack + 256*1024) & ~(UINTN)15, &bridge, &gdtr);
    return EFI_UNSUPPORTED;
failed:
    /* All failures above precede LGDT/CLI and the compatibility-mode jump.
     * Keep firmware input/timers alive and return control to the caller. */
    say(error);
    field(L"EFI status: ", status);
    say(L"Returning to firmware in 5 seconds.\r\n");
    efi_release_ranges();
    if (config) {
        UINT64 *p = (UINT64 *)config - 2;
        gBS->FreePages((UINTN)p, p[0]);
    }
    if (stack) {
        UINT64 *p = (UINT64 *)stack - 2;
        gBS->FreePages((UINTN)p, p[0]);
    }
    if (core_pages) gBS->FreePages(base, core_pages);
    gBS->Stall(5000000);
    firmware_console_restore(&console);
    return status;
}
