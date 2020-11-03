#include "OS.h"

#include "Xcept.h"

#include <filesystem>
#include <utility>

#if defined(_WIN32)
// if we compile for Windows native

#include <windows.h>

const std::filesystem::path OS::GetLocalConfigDirectory()
{
    PWSTR *folderPath = NULL;
    HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &folderPath);

    if (result != S_OK)
        throw Xcept("SHGetKnownFolderPath: Failed to retrieve AppData folder");

    std::filesystem::path retval(folderPath);
    CoTaskMemFree(folderPath);
    return std::move(retval);
}

const std::filesystem::path OS::GetGlobalConfigDirectory()
{
    PWSTR *folderPath = NULL;
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

const std::filesystem::path OS::GetLocalConfigDirectory()
{
    passwd *pw = getpwuid(getuid());
    if (!pw)
        throw Xcept("getpwuid failed: %s", strerror(errno));

    std::filesystem::path retval(pw->pw_dir);
    retval /= ".config";
    return retval;
}

const std::filesystem::path OS::GetGlobalConfigDirectory()
{
    return std::filesystem::path("/etc");
}

#else
// Unsupported OS
#error "Apparently your OS is neither Windows nor appears to be a UNIX variant (no unistd.h). You will have to add support for your OS in src/OS.cpp :/"
#endif
