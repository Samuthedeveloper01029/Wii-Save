#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void InitialiseVideo() {
    VIDEO_Init();
    WPAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    console_init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
}

int main(int argc, char **argv) {
    // Rimosso il Reload dell'IOS per evitare il blocco DSI iniziale
    InitialiseVideo();
   
    printf("\n ======================================= ");
    printf("\n WII USB SAVE EXTRACTOR v1.3 ");
    printf("\n ======================================= \n\n");
   
    printf("[INFO] Current IOS in use: %d\n\n", IOS_GetVersion());

    printf("[INFO] Initializing Wii NAND Filesystem...\n");
    s32 isfs_status = ISFS_Initialize();
    if (isfs_status != 0) {
        printf("[ERROR] Failed to initialize NAND access! Code: %d\n", isfs_status);
        printf("Please ensure your Homebrew Channel has AHBPROT enabled.\n");
    } else {
        printf("[SUCCESS] Wii NAND successfully mounted.\n\n");
    }

    printf("[INFO] Initializing USB Drive...\n");
    if (!fatInitDefault()) {
        printf("[ERROR] Failed to initialize FAT File System on USB!\n");
        printf("Please check your USB device connection.\n");
    } else {
        printf("[SUCCESS] USB Storage recognized.\n\n");
        mkdir("usb:/wii_saves", 0777);
        printf("[SUCCESS] Target folder verified.\n");
    }

    printf("\n---------------------------------------------------");
    printf("\nPress the HOME button on your Wii Remote to exit.\n");

    while(1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME) {
            break;
        }
        VIDEO_WaitVSync();
    }

    ISFS_Deinitialize();
    exit(0);
    return 0;
}
