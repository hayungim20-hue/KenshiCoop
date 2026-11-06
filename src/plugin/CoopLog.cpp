// CoopLog implementation. See CoopLog.h for rationale.
//
// VS2010 (v100) compatible: Win32 CRITICAL_SECTION + GetLocalTime, plain stdio.

#define _CRT_SECURE_NO_WARNINGS 1

#include "CoopLog.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

namespace coop {
namespace {

FILE*            g_fp   = 0;
CRITICAL_SECTION g_cs;
bool             g_init = false;
char             g_tag[16] = { 0 };
volatile long    g_fakeSkewMs = 0;

void writeLine(const char* level, const char* msg) {
    if (!g_init) return;
    EnterCriticalSection(&g_cs);
    if (g_fp) {
        // Derive the stamp from wallClockMs() (real clock + injected skew) so
        // log timestamps and the wire time-sync share one clock.
        unsigned long ms = wallClockMs();
        unsigned long hh = (ms / 3600000ul) % 24ul;
        unsigned long mm = (ms / 60000ul) % 60ul;
        unsigned long ss = (ms / 1000ul) % 60ul;
        unsigned long mmm = ms % 1000ul;
        std::fprintf(g_fp, "[%02lu:%02lu:%02lu.%03lu] [%s] %s: %s\n",
                     hh, mm, ss, mmm,
                     g_tag, level, msg ? msg : "");
        std::fflush(g_fp);
    }
    LeaveCriticalSection(&g_cs);
}

// local2h: every launch used to truncate the log (fopen "w"), so restarting after a
// crash destroyed the only record of it - and Kenshi overwrites its crashDump zip the
// same way. Before opening, archive the previous log (and any crash zip) into
// <dir>\KenshiCoop-logs\ stamped with the file's own write time, keeping the newest 10.
bool fileStamp(const std::string& path, char* out, size_t outLen) {
    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fa)) return false;
    if (fa.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return false;
    FILETIME lt; SYSTEMTIME t;
    if (!FileTimeToLocalFileTime(&fa.ftLastWriteTime, &lt) || !FileTimeToSystemTime(&lt, &t))
        return false;
    _snprintf(out, outLen - 1, "%04u%02u%02u-%02u%02u%02u",
              (unsigned)t.wYear, (unsigned)t.wMonth, (unsigned)t.wDay,
              (unsigned)t.wHour, (unsigned)t.wMinute, (unsigned)t.wSecond);
    out[outLen - 1] = '\0';
    return true;
}

void pruneArchive(const std::string& dir, const std::string& pattern, size_t keep) {
    std::vector<std::string> names;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + "\\" + pattern).c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) names.push_back(fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    if (names.size() <= keep) return;
    std::sort(names.begin(), names.end()); // stamped names sort chronologically
    for (size_t i = 0; i + keep < names.size(); ++i)
        DeleteFileA((dir + "\\" + names[i]).c_str());
}

void archivePrevious(const char* path) {
    std::string p(path);
    std::string::size_type slash = p.find_last_of("\\/");
    std::string dir  = (slash == std::string::npos) ? std::string(".") : p.substr(0, slash);
    std::string file = (slash == std::string::npos) ? p : p.substr(slash + 1);
    std::string stem = file;
    if (stem.size() > 4 && _stricmp(stem.c_str() + stem.size() - 4, ".log") == 0)
        stem.erase(stem.size() - 4);
    std::string arch = dir + "\\KenshiCoop-logs";
    CreateDirectoryA(arch.c_str(), 0); // fine if it already exists
    char st[32];
    if (fileStamp(p, st, sizeof(st)))
        MoveFileExA(p.c_str(), (arch + "\\" + stem + "." + st + ".log").c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
    // Kenshi's own crash package, same overwrite-on-next-crash problem. Copy, never move:
    // it is Kenshi's file.
    std::string dump = dir + "\\crashDump1.0.65_x64.zip";
    if (fileStamp(dump, st, sizeof(st)))
        CopyFileA(dump.c_str(), (arch + "\\crashDump." + st + ".zip").c_str(), TRUE);
    pruneArchive(arch, stem + ".*.log", 10);
    pruneArchive(arch, "crashDump.*.zip", 10);
}

} // namespace

unsigned long wallClockMs() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    long ms = (long)((((unsigned long)st.wHour * 60ul + st.wMinute) * 60ul + st.wSecond) * 1000ul
                     + st.wMilliseconds);
    ms += g_fakeSkewMs;
    // Wrap into [0, 24h) so a skew across midnight still formats sanely.
    const long DAY = 24l * 3600l * 1000l;
    ms %= DAY;
    if (ms < 0) ms += DAY;
    return (unsigned long)ms;
}

void logSetFakeSkewMs(long skewMs) { g_fakeSkewMs = skewMs; }

void logInit(const char* path, const char* modeTag) {
    if (g_init) return;
    InitializeCriticalSection(&g_cs);
    g_init = true;

    if (modeTag) {
        size_t i = 0;
        for (; modeTag[i] && i < sizeof(g_tag) - 1; ++i) g_tag[i] = modeTag[i];
        g_tag[i] = '\0';
    }

    if (path && path[0]) {
        archivePrevious(path);         // keep the last run's log (local2h)
        g_fp = std::fopen(path, "w"); // fresh file each run
    }
    writeLine("INFO", "log opened");
}

void logSetTag(const char* modeTag) {
    if (!g_init || !modeTag) return;
    EnterCriticalSection(&g_cs);
    size_t i = 0;
    for (; modeTag[i] && i < sizeof(g_tag) - 1; ++i) g_tag[i] = modeTag[i];
    g_tag[i] = '\0';
    LeaveCriticalSection(&g_cs);
}

void logLine(const char* msg)    { writeLine("INFO",  msg); }
void logErrLine(const char* msg) { writeLine("ERROR", msg); }

void logClose() {
    if (!g_init) return;
    EnterCriticalSection(&g_cs);
    if (g_fp) {
        std::fflush(g_fp);
        std::fclose(g_fp);
        g_fp = 0;
    }
    LeaveCriticalSection(&g_cs);
}

} // namespace coop
