/* ConsoleControl is separate from GOP/UGA display resolution. OpenCore's
 * OcConsoleControlEntryModeLib and rEFInd's egSetGraphicsModeEnabled use it
 * to make firmware text visible. Protocol ABI: Include/Intel/Protocol/
 * ConsoleControl.h in OpenCorePkg. No renderer or firmware hooks are installed.
 */
#include "firmware_console.h"

typedef struct console_control CONSOLE_CONTROL;
struct console_control {
    EFI_STATUS (*GetMode)(CONSOLE_CONTROL *, UINT32 *, BOOLEAN *, BOOLEAN *);
    EFI_STATUS (*SetMode)(CONSOLE_CONTROL *, UINT32);
    void *LockStdIn;
};
static EFI_GUID console_guid =
    {0xf42f7782,0x012e,0x4c12,{0x99,0x56,0x49,0xf9,0x43,0x04,0xf7,0x21}};

EFI_STATUS firmware_console_text(FIRMWARE_CONSOLE *state)
{
    CONSOLE_CONTROL *console = 0;
    EFI_HANDLE handles[16];
    UINTN bytes = sizeof(handles), i;
    EFI_STATUS status;
    state->protocol = 0;
    state->restore = 0;
    status = gBS->HandleProtocol(gST->ConsoleOutHandle, &console_guid, (void **)&console);
    if (EFI_ERROR(status)) {
        status = gBS->LocateHandle(ByProtocol, &console_guid, 0, &bytes, handles);
        if (EFI_ERROR(status)) return status;
        if (bytes > sizeof(handles)) return EFI_UNSUPPORTED;
        for (i = 0; i < bytes/sizeof(*handles); i++) {
            status = gBS->HandleProtocol(handles[i], &console_guid, (void **)&console);
            if (!EFI_ERROR(status)) break;
        }
    }
    if (EFI_ERROR(status) || !console) return EFI_NOT_FOUND;
    status = console->GetMode(console, &state->original_mode, 0, 0);
    if (EFI_ERROR(status)) return status;
    if (state->original_mode == 0) return EFI_SUCCESS;
    if (state->original_mode != 1) return EFI_UNSUPPORTED;
    status = console->SetMode(console, 0); /* EfiConsoleControlScreenText */
    state->protocol = console;
    state->restore = !EFI_ERROR(status);
    return status;
}

void firmware_console_restore(FIRMWARE_CONSOLE *state)
{
    CONSOLE_CONTROL *console = state->protocol;
    if (state->restore) {
        console->SetMode(console, state->original_mode);
        state->restore = 0;
    }
}
