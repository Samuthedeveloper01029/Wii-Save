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

int CopyFile(const char *srcPath, const char *destPath) {
    FILE *src = fopen(srcPath, "rb");
    if (!src) return -1;

    FILE *dest = fopen(destPath, "wb");
    if (!dest) {
        fclose(src);
        return -2;
    }

    char buffer;
    size_t bytesRead;
    while ((bytesRead = fread(&buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(&buffer, 1, bytesRead, dest);
    }

    fclose(src);
    fclose(dest);
    return 0;
}

void ScanAndBackup(const char *nandSaveDir, const char *usbTargetDir) {
    u32 numEntries = 0;
    static char nameList[ISFS_MAXPATH] __attribute__((aligned(32)));
    
    printf("[INFO] Scanning: %s...\n", nandSaveDir);
    s32 ret = ISFS_ReadDir(nandSaveDir, nameList, &numEntries);
    
    if (ret < 0) {
        if (ret == -106) {
            printf("[SKIP] Folder does not exist on this Wii.\n");
        } else {
            printf("[SKIP] Access denied or locked. Code: %d\n", ret);
        }
        return;
    }
    
    if (numEntries == 0) {
        printf("[INFO] No data here.\n");
        return;
    }

    printf("[SUCCESS] Found %d game entries!\n", numEntries);
    char *currentEntry = nameList;
    
    for (u32 i = 0; i < numEntries; i++) {
        if (strcmp(currentEntry, ".") != 0 && strcmp(currentEntry, "..") != 0) {
            char usbGameDir[ISFS_MAXPATH];
            snprintf(usbGameDir, sizeof(usbGameDir), "%s/%s", usbTargetDir, currentEntry);
            mkdir(usbGameDir, 0777);
            
            char nandFilePath[ISFS_MAXPATH];
            char usbFilePath[ISFS_MAXPATH];
            
            snprintf(nandFilePath, sizeof(nandFilePath), "%s/%s/data/data.bin", nandSaveDir, currentEntry);
            snprintf(usbFilePath, sizeof(usbFilePath), "%s/data.bin", usbGameDir);
            
            int copyResult = CopyFile(nandFilePath, usbFilePath);
            if (copyResult == 0) {
                printf(" -> %s [COPIED]\n", currentEntry);
            } else {
                printf(" -> %s [NO DATA.BIN]\n", currentEntry);
            }
        }
        currentEntry += strlen(currentEntry) + 1;
    }
}

int main(int argc, char **argv) {
    InitialiseVideo();
   
    printf("\n ======================================= ");
    printf("\n       WII UNIVERSAL SAVE EXTRACTOR v1.1.4 ");
    printf("\n ======================================= \n\n");

    printf("Current IOS active from Loader: %d\n\n", IOS_GetVersion());

    printf("[INFO] Initializing Wii NAND Filesystem...\n");
    s32 isfs_status = ISFS_Initialize();
    if (isfs_status != 0) {
        printf("[ERROR] NAND initialization failed! Code: %d\n", isfs_status);
    } else {
        printf("[SUCCESS] Wii NAND successfully mounted.\n");
    }

    printf("[INFO] Initializing USB Drive...\n");
    
    int usb_retry = 0;
    bool usb_mounted = false;
    while (usb_retry < 5 && !usb_mounted) {
        if (fatInitDefault()) {
            usb_mounted = true;
        } else {
            usb_retry++;
            printf("[RETRY] USB initialization attempt %d failed, retrying...\n", usb_retry);
            sleep(1);
        }
    }

    if (!usb_mounted) {
        printf("[ERROR] Failed to initialize FAT File System on USB!\n");
    } else {
        printf("[SUCCESS] USB Storage recognized.\n\n");
        
        mkdir("usb:/wii_saves", 0777);
        
        ScanAndBackup("/title/00010001", "usb:/wii_saves");
        ScanAndBackup("/title/00010000", "usb:/wii_saves");
        ScanAndBackup("/title/00010004", "usb:/wii_saves");
        ScanAndBackup("/title/00010007", "usb:/wii_saves");
        
        printf("\n[SUCCESS] Universal backup process completed.\n");
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

