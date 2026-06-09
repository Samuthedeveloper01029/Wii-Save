#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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
    InitialiseVideo();
   
    printf("\n ======================================= ");
    printf("\n WII USB SAVE REDIRECTOR - STEP 1 ");
    printf("\n ======================================= \n\n");
   
    printf("[INFO] Initializing USB port...\n");
    VIDEO_WaitVSync();

    if (!fatInitDefault()) {
        printf("[ERROR] Failed to initialize FAT File System!\n");
        printf("Make sure your USB drive or Hard Disk is connected properly.\n");
    } else {
        printf("[SUCCESS] FAT File System successfully recognized.\n\n");
        printf("[INFO] Creating the folder for your Wii saves...\n");
       
        char cartella_salvataggi[] = "usb:/wii_saves";
        int risultato = mkdir(cartella_salvataggi, 0777);
       
        if (risultato == 0) {
            printf("[SUCCESS] Folder '%s' successfully created on your device!\n", cartella_salvataggi);
        } else {
            printf("[INFO] Folder '%s' already exists or USB is read-only.\n", cartella_salvataggi);
        }

        printf("[INFO] Writing test file...\n");
        char percorso_file[] = "usb:/wii_saves/save_test.txt";
       
        FILE *file = fopen(percorso_file, "w");
        if (file != NULL) {
            fprintf(file, "Partial EmuNAND test. If you can read this, your USB works correctly!\n");
            fclose(file);
            printf("[SUCCESS] Test file successfully created.\n");
        } else {
            printf("[ERROR] Unable to copy the save files to the USB device.\n");
        }
    }

    printf("\n---------------------------------------------------");
    printf("\nPress the HOME button on your Wii Remote to exit the application.\n");

    while(1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME) {
            break;
        }
        VIDEO_WaitVSync();
    }

    exit(0);
    return 0;
}

