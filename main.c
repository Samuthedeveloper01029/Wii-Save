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

    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytesRead, dest);
    }

    fclose(src);
    fclose(dest);
    return 0;
}

int main(int argc, char **argv) {
    if (IOS_GetVersion() != 249) {
        IOS_ReloadIOS(249);
    }

    InitialiseVideo();
   
    printf("\n ======================================= ");
    printf("\n WII USB SAVE EXTRACTOR v1.1 ");
    printf("\n ======================================= \n\n");
   
    printf("[INFO] Checking hardware permissions...\n");
    if (IOS_GetVersion() == 249) {
        printf("[SUCCESS] cIOS 249 successfully active.\n");
    } else {
        printf("[WARNING] Custom IOS not active. Retrying with default permissions...\n");
    }
    printf("Current IOS in use: %d\n\n", IOS_GetVersion());

    printf("[INFO] Initializing Wii NAND Filesystem...\n");
    s32 isfs_status = ISFS_Initialize();
    if (isfs_status != 0) {
        printf("[ERROR] Failed to initialize NAND access! Code: %d\n", isfs_status);
        printf("Please ensure cIOS 249 is installed on your Wii.\n");
    } else {
        printf("[SUCCESS] Wii NAND successfully mounted.\n\n");
    }

    printf("[INFO] Initializing USB Drive...\n");
    if (!fatInitDefault()) {
        printf("[ERROR] Failed to initialize FAT File System on USB!\n");
        printf("Check your USB connection. Use the outer/bottom port near the edge.\n");
    } else {
        printf("[SUCCESS] USB Storage recognized.\n\n");
        
        mkdir("usb:/wii_saves", 0777);
        const char *nandSaveDir = "/title/00010000";
        printf("[INFO] Scanning internal saves in %s...\n", nandSaveDir);
        
        u32 numEntries = 0;
        static char nameList[ISFS_MAXPATH] __attribute__((aligned(32)));
        
        s32 ret = ISFS_ReadDir(nandSaveDir, nameList, &numEntries);
        
        if (ret < 0) {
            printf("[ERROR] Cannot read Wii saves directory. Error code: %d\n", ret);
            if (ret == -102) printf("Reason: Access denied (Permissions issue).\n");
        } else if (numEntries == 0) {
            printf("[INFO] No game saves found on this console.\n");
        } else {
            printf("[INFO] Found %d game folders. Starting backup...\n\n", numEntries);
            
            char *currentEntry = nameList;
            for (u32 i = 0; i < numEntries; i++) {
                if (strcmp(currentEntry, ".") != 0 && strcmp(currentEntry, "..") != 0) {
                    printf("-> Backing up game ID: %s... ", currentEntry);
                    
                    char usbGameDir[ISFS_MAXPATH];
                    snprintf(usbGameDir, sizeof(usbGameDir), "usb:/wii_saves/%s", currentEntry);
                    mkdir(usbGameDir, 0777);
                    
                    char nandFilePath[ISFS_MAXPATH];
                    char usbFilePath[ISFS_MAXPATH];
                    snprintf(nandFilePath, sizeof(nandFilePath), "%s/%s/data/data.bin", nandSaveDir, currentEntry);
                    snprintf(usbFilePath, sizeof(usbFilePath), "%s/data.bin", usbGameDir);
                    
                    int copyResult = CopyFile(nandFilePath, usbFilePath);
                    if (copyResult == 0) {
                        printf("[OK]\n");
                    } else {
                        printf("[SKIPPED/NO DATA]\n");
                    }
                }
                currentEntry += strlen(currentEntry) + 1;
            }
            printf("\n[SUCCESS] Backup process finished!\n");
        }
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

