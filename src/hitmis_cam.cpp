#include "CameraUnit_ANDORUSB.hpp"
#include "CameraUnit_PI.hpp"
#include "CameraUnit.hpp"
#include "meb_print.h"
#include "ini.h"
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>

#include <windows.h>
#include <shlobj_core.h>
#pragma comment(lib, "Shell32.lib")

static bool dirExists(const char *dirName)
{
    DWORD ftyp = GetFileAttributesA(dirName);
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false; //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true; // this is a directory!

    return false; // this is not a directory!
}

static int checknmakedir(const char *path)
{
    if (!dirExists(path))
    {
        int mkDir_ret = SHCreateDirectoryExA(NULL, path, NULL);
        switch (mkDir_ret)
        {
#define CREATEDIR_ERROR(x)                                         \
    case x:                                                        \
    {                                                              \
        dbprintlf(RED_FG "Error %d: " #x " creating %s", x, path); \
        return 0;                                                  \
        break;                                                     \
    }
            CREATEDIR_ERROR(ERROR_BAD_PATHNAME);
            CREATEDIR_ERROR(ERROR_FILENAME_EXCED_RANGE);
            CREATEDIR_ERROR(ERROR_PATH_NOT_FOUND);
            CREATEDIR_ERROR(ERROR_FILE_EXISTS);
            CREATEDIR_ERROR(ERROR_ALREADY_EXISTS);
            CREATEDIR_ERROR(ERROR_CANCELLED);
        case ERROR_SUCCESS:
        {
            return 1;
            break;
        }
        default:
        {
            dbprintlf(FATAL "Unknown error %d", mkDir_ret);
            return 0;
            break;
        }
        }
    }
    else
    {
        return 1;
    }
}

typedef struct
{
    const char *progname;
    const char *savedir;
    float cadence,
        maxexposure,
        percentile;
    int maxbin,
        value,
        uncertainty;
} hitmis_config;

static int inihandler(void *user, const char *section, const char *name, const char *value)
{
    hitmis_config *pconfig = (hitmis_config *)user;

#define MATCH(s, n) (strcmp(section, s) == 0 && strcmp(name, #n) == 0)
#define ASSIGNF(n) pconfig->##n = atof(value)
#define ASSIGNI(n) pconfig->##n = atol(value)
#define ASSIGNS(n) pconfig->##n = strdup(value)

#define CHECKS(s, n)      \
    else if (MATCH(s, n)) \
    {                     \
        ASSIGNS(n);       \
    }

#define CHECKF(s, n)      \
    else if (MATCH(s, n)) \
    {                     \
        ASSIGNF(n);       \
    }

#define CHECKI(s, n)      \
    else if (MATCH(s, n)) \
    {                     \
        ASSIGNI(n);       \
    }

    if (MATCH("PROGRAM", name))
    {
        pconfig->progname = strdup(value);
    }
    CHECKS("CONFIG", savedir)
    CHECKF("CONFIG", cadence)
    CHECKF("CONFIG", maxexposure)
    CHECKF("CONFIG", percentile)
    CHECKI("CONFIG", maxbin)
    CHECKI("CONFIG", value)
    CHECKI("CONFIG", uncertainty)
    else
    {
        dbprintlf(RED_FG "%s -> %s: %s not accounted for.", section, name, value);
        return 0;
    }
    return 1;
}

CCameraUnit *InitCamera()
{
    CCameraUnit *cam = nullptr;
    // PiCam
    cam = new CCameraUnit_PI(); // try PI
    int cam_number = 0;
    if (!cam->CameraReady())
    {
        tprintlf("PI Pixis not detected. Checking for Andor iKon");
        delete cam;
        cam = new CCameraUnit_ANDORUSB();
    }
    if (!cam->CameraReady())
    {
        delete cam;
        dbprintlf(FATAL "PI Pixis or Andor iKon not found");
    }
    return cam;
}

volatile sig_atomic_t done = 0;
void sighandler(int sig)
{
    done = 1;
}

static inline char *get_date()
{
#ifndef _MSC_VER
    static __thread char buf[128];
#else
    __declspec(thread) static char buf[128];
#endif
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buf, sizeof(buf), "%04d%02d%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return buf;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("Usage:\n%s [config file]\n\n", argv[0]);
    }
    else if (argc > 2)
    {
        printf("Usage:\n%s [config file]\n\n", argv[0]);
        return 0;
    }
    signal(SIGINT, sighandler);
    CCameraUnit *cam = nullptr;
    long retryCount = 10;
    do
    {
        cam = InitCamera();
    } while (retryCount-- && cam == nullptr);
    if (cam == nullptr)
    {
        dbprintlf(FATAL "Error opening camera");
        return 0;
    }
    if (!cam->CameraReady())
    {
        dbprintlf(FATAL "%s not ready", cam->CameraName());
        delete cam;
        return 0;
    }
    hitmis_config config;
    const char *ininame = argc == 2 ? argv[1] : "config.ini";
    if (ini_parse(ininame, inihandler, &config) < 0)
    {
        dbprintlf(RED_FG "Could not load '%s'", ininame);
        return 0;
    }
    // defaults
    char _savepath[256];
    char savepath[256];
    if (config.cadence < 1)
        config.cadence = 1;
    if (config.cadence > 20)
        config.cadence = 20;
    if (config.maxbin != 1)
        config.maxbin = 1;
    if (config.maxexposure < 10)
        config.maxexposure = 10;
    if (config.maxexposure > 120)
        config.maxexposure = 120;
    if (config.percentile < 90)
        config.percentile = 90;
    if (config.percentile > 100)
        config.percentile = 100;
    if (config.value > 40000)
        config.value = 40000;
    if (config.value < 10000)
        config.value = 10000;
    if (config.uncertainty > 10000)
        config.uncertainty = 10000;
    if (config.uncertainty < 1000)
        config.uncertainty = 1000;
    if (strlen(config.savedir) == 0)
    {
        if (!GetCurrentDirectoryA(sizeof(_savepath), _savepath))
        {
            dbprintlf(FATAL "Error getting current directory");
            goto ret;
        }
        _snprintf(savepath, sizeof(savepath), "%s\\fits", _savepath);
        config.savedir = savepath;
    }
    retryCount = 10;
    while (retryCount--)
    {  
        if (checknmakedir(config.savedir))
            break;
        if (!GetCurrentDirectoryA(sizeof(_savepath), _savepath))
        {
            dbprintlf(FATAL "Error getting current directory");
            goto ret;
        }
        _snprintf(savepath, sizeof(savepath), "%s\\fits", _savepath);
        config.savedir = savepath;
    }
    if (checknmakedir(config.savedir) == 0)
    {
        dbprintlf(FATAL "Save dir not available, exiting");
        goto ret;
    }
    cam->SetExposure(0.1);
    cam->SetBinningAndROI(1, 1);
    while (!done)
    {
        static char dirname[256];
        static char fname[512];
        static float exposure = 0.1;
        CImageData img = cam->CaptureImage(retryCount);
        _snprintf(dirname, sizeof(dirname), "%s\\%s", config.savedir, get_date());
        checknmakedir(dirname);
        img.SaveFits(NULL, dirname);
        img.FindOptimumExposure(exposure, config.percentile, config.value, config.maxexposure, 0, config.uncertainty);
        cam->SetExposure(exposure);
        unsigned char *ptr;
        int sz;
        img.GetJPEGData(ptr, sz);
        _snprintf(fname, sizeof(fname), "%s\\%" PRIu64 ".jpg", dirname, img.GetTimestamp());
        FILE *fp = fopen(fname, "wb");
        if (fp != NULL)
        {
            fwrite(ptr, sz, 1, fp);
            fclose(fp);
        }
        int sleeptime = 1000 * (config.cadence - exposure);
        if (sleeptime > 0)
            Sleep(sleeptime);
    }
ret:
    delete cam;
    return 0;
}