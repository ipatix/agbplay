#include "OS.h"

#include "Xcept.h"

#include <filesystem>
#include <iostream>

#if defined(_WIN32)
// if we compile for Windows native

#include <windows.h>
#include <shlobj.h>

void OS::LowerThreadPriority()
{
    // ignore errors if this fails
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
}

const std::filesystem::path OS::GetMusicDirectory()
{
    PWSTR folderPath = NULL;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_Music, KF_FLAG_DEFAULT, NULL, &folderPath);

    if (result != S_OK)
        throw Xcept("SHGetKnownFolderPath: Failed to retrieve AppData folder");

    std::filesystem::path retval(folderPath);
    CoTaskMemFree(folderPath);
    return retval;
}

const std::filesystem::path OS::GetLocalConfigDirectory()
{
    PWSTR folderPath = NULL;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &folderPath);

    if (result != S_OK)
        throw Xcept("SHGetKnownFolderPath: Failed to retrieve AppData folder");

    std::filesystem::path retval(folderPath);
    CoTaskMemFree(folderPath);
    return retval;
}

const std::filesystem::path OS::GetGlobalConfigDirectory()
{
    PWSTR folderPath = NULL;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_DEFAULT, NULL, &folderPath);

    if (result != S_OK)
        throw Xcept("SHGetKnownFolderPath: Failed to retrieve ProgramData folder");

    std::filesystem::path retval(folderPath);
    CoTaskMemFree(folderPath);
    return retval;
}

#elif __has_include(<unistd.h>)
// if we compile for a UNIX'oid

#include <unistd.h>
#include <pwd.h>
#include <string.h>

void OS::LowerThreadPriority()
{
    // we don't really care about errors here, so ignore errno
    nice(15);
}

const std::filesystem::path OS::GetMusicDirectory()
{
    passwd *pw = getpwuid(getuid());
    if (!pw)
        throw Xcept("getpwuid failed: %s", strerror(errno));

    std::filesystem::path retval(pw->pw_dir);
    return retval / "Music";
}

const std::filesystem::path OS::GetLocalConfigDirectory()
{
    passwd *pw = getpwuid(getuid());
    if (!pw)
        throw Xcept("getpwuid failed: %s", strerror(errno));

    std::filesystem::path retval(pw->pw_dir);
    return retval / ".config";
}

const std::filesystem::path OS::GetGlobalConfigDirectory()
{
    return std::filesystem::path("/etc");
}

#else
// Unsupported OS
#error "Apparently your OS is neither Windows nor appears to be a UNIX variant (no unistd.h). You will have to add support for your OS in src/OS.cpp :/"
#endif

#ifdef __APPLE__
#include <libproc.h>
#include <unistd.h>
#include <sys/sysctl.h>

// http://www.objectpark.net/en/parentpid.html

#define OPProcessValueUnknown UINT_MAX

static int OPParentIDForProcessID(int pid)
/*" Returns the parent process id
     for the given process id (pid). "*/
{
    struct kinfo_proc info;
    size_t length = sizeof(struct kinfo_proc);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
    if (sysctl(mib, 4, &info, &length, NULL, 0) < 0)
        return OPProcessValueUnknown;
    if (length == 0)
        return OPProcessValueUnknown;
    return info.kp_eproc.e_ppid;
}

// adapted from https://stackoverflow.com/a/8149198
/*
 * Attempts to check for Apple Terminal.
 * Apple Terminal performs hilariously bad in comparison
 * to iTerm2 to the point where it is almost unusable.
 */
void OS::CheckTerminal() {
    pid_t pid = getpid();
    int ret;
    pid_t oldpid = getpid();
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    do  {
        ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf));
        if (strncmp(pathbuf, "/Applications/Utilities/Terminal.app", 37) == 0) {
            std::cout << "It appears that you are using macOS's built-in Terminal.app." << std::endl << std::endl <<
                        "This terminal is prone to some serious lag with agbplay." << std::endl <<
                        "It is recommended to use iTerm2 (https://www.iterm2.com/), as it can" << std::endl <<
                        "run agbplay without any lag." << std::endl << std::endl <<
                        "Press Return to continue, or Ctrl+C to exit.";
            std::cin.ignore();
            return;
        }
        oldpid = pid;
        pid = (pid_t)OPParentIDForProcessID(pid); } while (ret > 0 && pid > 1 /* launchd */ && pid != oldpid && pid != OPProcessValueUnknown); }
#else
void OS::CheckTerminal()
{
    return;
}
#endif
