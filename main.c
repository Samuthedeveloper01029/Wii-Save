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
   
    printf("[INFO] Inizialization of the USB door...\n");
    VIDEO_WaitVSync();

    if (!fatInitDefault()) {
        printf("[ERROR] Impossible to initialize File System FAT!\n");
        printf("Make sure that your USB/Hard Disk is in the external USB door.\n");
    } else {
        printf("[SUCCESS] File System FAT succesfully recognized.\n\n");
        printf("[INFO] Creating your folder for the Wii savings...\n");
       
        char cartella_salvataggi[] = "usb:/wii_saves";
        int risultato = mkdir(cartella_salvataggi, 0777);
       
        if (risultato == 0) {
            printf("[SUCCESS] The folder '%s' has succesfully created on your device!\n", cartella_salvataggi);
        } else {
            printf("[INFO] The folder '%s' already exists or the USB can be only read.\n", cartella_salvataggi);
        }

        printf("[INFO] Test file downloading...\n");
        char percorso_file[] = "usb:/wii_saves/test_salvataggio.txt";
       
        FILE *file = fopen(percorso_file, "w");
        if (file != NULL) {
            fprintf(file, "Partial EmuNAND test. If you can read this, your USB works correctly!\n");
            fclose(file);
            printf("[SUCCESS] File succesfully created.\n");
        } else {
            printf("[ERROR] Impossible to copy the saving files on the USB/Hard Disk.\n");
        }
    }

    printf("\n---------------------------------------------------");
    printf("\nPress the home button on your Wii Remote to get out of the app.\n");

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
