/**
 * @file file_dialog.cpp
 * @brief File dialog implementation
 */

#include "tinyvk/core/file_dialog.h"
#include "tinyvk/core/log.h"

#include <fstream>
#include <sstream>
#include <filesystem>

#ifdef TVK_PLATFORM_APPLE
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#include <AppKit/AppKit.h>
#endif
#endif

#ifdef TVK_PLATFORM_WINDOWS
#include <windows.h>
#include <shobjidl.h>
#include <commdlg.h>
#endif

namespace fs = std::filesystem;

namespace tvk {

// macOS implementation
#ifdef TVK_PLATFORM_APPLE

std::optional<std::string> FileDialog::OpenFile(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        
        if (!defaultPath.empty()) {
            NSString* path = [NSString stringWithUTF8String:defaultPath.c_str()];
            [panel setDirectoryURL:[NSURL fileURLWithPath:path]];
        }
        
        // Set file type filters
        if (!filters.empty()) {
            NSMutableArray* types = [NSMutableArray array];
            for (const auto& filter : filters) {
                // Parse pattern like "*.png;*.jpg"
                std::string pattern = filter.pattern;
                size_t pos = 0;
                while ((pos = pattern.find(';')) != std::string::npos || !pattern.empty()) {
                    std::string ext;
                    if (pos != std::string::npos) {
                        ext = pattern.substr(0, pos);
                        pattern = pattern.substr(pos + 1);
                    } else {
                        ext = pattern;
                        pattern.clear();
                    }
                    // Remove "*." prefix
                    if (ext.length() > 2 && ext[0] == '*' && ext[1] == '.') {
                        ext = ext.substr(2);
                        [types addObject:[NSString stringWithUTF8String:ext.c_str()]];
                    }
                    if (pos == std::string::npos) break;
                }
            }
            if ([types count] > 0) {
                [panel setAllowedFileTypes:types];
            }
        }
        
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] firstObject];
            return std::string([[url path] UTF8String]);
        }
    }
    return std::nullopt;
}

std::vector<std::string> FileDialog::OpenFiles(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    std::vector<std::string> result;
    
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:YES];
        
        if (!defaultPath.empty()) {
            NSString* path = [NSString stringWithUTF8String:defaultPath.c_str()];
            [panel setDirectoryURL:[NSURL fileURLWithPath:path]];
        }
        
        if ([panel runModal] == NSModalResponseOK) {
            for (NSURL* url in [panel URLs]) {
                result.push_back([[url path] UTF8String]);
            }
        }
    }
    return result;
}

std::optional<std::string> FileDialog::SaveFile(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        
        if (!defaultPath.empty()) {
            fs::path p(defaultPath);
            if (fs::is_directory(p)) {
                [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:defaultPath.c_str()]]];
            } else {
                [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:p.parent_path().c_str()]]];
                [panel setNameFieldStringValue:[NSString stringWithUTF8String:p.filename().c_str()]];
            }
        }
        
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [panel URL];
            return std::string([[url path] UTF8String]);
        }
    }
    return std::nullopt;
}

std::optional<std::string> FileDialog::SelectFolder(const std::string& defaultPath) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:NO];
        [panel setCanChooseDirectories:YES];
        [panel setAllowsMultipleSelection:NO];
        [panel setCanCreateDirectories:YES];
        
        if (!defaultPath.empty()) {
            NSString* path = [NSString stringWithUTF8String:defaultPath.c_str()];
            [panel setDirectoryURL:[NSURL fileURLWithPath:path]];
        }
        
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] firstObject];
            return std::string([[url path] UTF8String]);
        }
    }
    return std::nullopt;
}

#endif // TVK_PLATFORM_APPLE

// Windows implementation
#ifdef TVK_PLATFORM_WINDOWS

std::optional<std::string> FileDialog::OpenFile(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    // Build filter string
    std::string filterStr;
    for (const auto& f : filters) {
        filterStr += f.name + '\0';
        std::string pattern = f.pattern;
        // Replace ; with \0
        for (char& c : pattern) {
            if (c == ';') c = '\0';
        }
        filterStr += pattern + '\0';
    }
    filterStr += '\0';
    ofn.lpstrFilter = filterStr.c_str();
    
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = defaultPath.empty() ? NULL : defaultPath.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn)) {
        CoUninitialize();
        return std::string(szFile);
    }
    
    CoUninitialize();
    return std::nullopt;
}

std::vector<std::string> FileDialog::OpenFiles(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    std::vector<std::string> result;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    OPENFILENAMEA ofn;
    char szFile[4096] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    std::string filterStr;
    for (const auto& f : filters) {
        filterStr += f.name + '\0' + f.pattern + '\0';
    }
    filterStr += '\0';
    ofn.lpstrFilter = filterStr.c_str();
    
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = defaultPath.empty() ? NULL : defaultPath.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn)) {
        char* ptr = szFile;
        std::string dir = ptr;
        ptr += dir.length() + 1;
        
        if (*ptr == '\0') {
            result.push_back(dir);
        } else {
            while (*ptr) {
                result.push_back(dir + "\\" + ptr);
                ptr += strlen(ptr) + 1;
            }
        }
    }
    
    CoUninitialize();
    return result;
}

std::optional<std::string> FileDialog::SaveFile(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    
    std::string filterStr;
    for (const auto& f : filters) {
        filterStr += f.name + '\0' + f.pattern + '\0';
    }
    filterStr += '\0';
    ofn.lpstrFilter = filterStr.c_str();
    
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = defaultPath.empty() ? NULL : defaultPath.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    
    if (GetSaveFileNameA(&ofn)) {
        CoUninitialize();
        return std::string(szFile);
    }
    
    CoUninitialize();
    return std::nullopt;
}

std::optional<std::string> FileDialog::SelectFolder(const std::string& defaultPath) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    
    if (SUCCEEDED(hr)) {
        DWORD options;
        pfd->GetOptions(&options);
        pfd->SetOptions(options | FOS_PICKFOLDERS);
        
        if (SUCCEEDED(pfd->Show(NULL))) {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR path;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                    char buffer[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer, MAX_PATH, NULL, NULL);
                    CoTaskMemFree(path);
                    psi->Release();
                    pfd->Release();
                    CoUninitialize();
                    return std::string(buffer);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    
    CoUninitialize();
    return std::nullopt;
}

#endif // TVK_PLATFORM_WINDOWS

// Linux implementation (fallback using zenity or kdialog)
#ifdef TVK_PLATFORM_LINUX

std::optional<std::string> FileDialog::OpenFile(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    std::string cmd = "zenity --file-selection";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return std::nullopt;
    
    char buffer[512];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    
    int status = pclose(pipe);
    if (status != 0) return std::nullopt;
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result.empty() ? std::nullopt : std::optional(result);
}

std::vector<std::string> FileDialog::OpenFiles(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    std::string cmd = "zenity --file-selection --multiple --separator='\n'";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    
    std::vector<std::string> result;
    char buffer[512];
    std::string current;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        current += buffer;
        size_t pos;
        while ((pos = current.find('\n')) != std::string::npos) {
            result.push_back(current.substr(0, pos));
            current = current.substr(pos + 1);
        }
    }
    if (!current.empty()) {
        result.push_back(current);
    }
    
    pclose(pipe);
    return result;
}

std::optional<std::string> FileDialog::SaveFile(
    const std::vector<FileFilter>& filters,
    const std::string& defaultPath) {
    
    std::string cmd = "zenity --file-selection --save --confirm-overwrite";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return std::nullopt;
    
    char buffer[512];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    
    int status = pclose(pipe);
    if (status != 0) return std::nullopt;
    
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result.empty() ? std::nullopt : std::optional(result);
}

std::optional<std::string> FileDialog::SelectFolder(const std::string& defaultPath) {
    std::string cmd = "zenity --file-selection --directory";
    if (!defaultPath.empty()) {
        cmd += " --filename=\"" + defaultPath + "\"";
    }
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return std::nullopt;
    
    char buffer[512];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    
    int status = pclose(pipe);
    if (status != 0) return std::nullopt;
    
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result.empty() ? std::nullopt : std::optional(result);
}

#endif // TVK_PLATFORM_LINUX

// FileSystem implementation (cross-platform using std::filesystem)

bool FileSystem::Exists(const std::string& path) {
    return fs::exists(path);
}

std::string FileSystem::GetExtension(const std::string& path) {
    return fs::path(path).extension().string();
}

std::string FileSystem::GetFilename(const std::string& path) {
    return fs::path(path).filename().string();
}

std::string FileSystem::GetStem(const std::string& path) {
    return fs::path(path).stem().string();
}

std::string FileSystem::GetDirectory(const std::string& path) {
    return fs::path(path).parent_path().string();
}

std::optional<std::string> FileSystem::ReadText(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

std::optional<std::vector<u8>> FileSystem::ReadBinary(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return std::nullopt;
    
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<u8> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return std::nullopt;
    }
    return data;
}

bool FileSystem::WriteText(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return true;
}

bool FileSystem::WriteBinary(const std::string& path, const std::vector<u8>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

std::string FileSystem::GetCurrentDirectory() {
    return fs::current_path().string();
}

std::vector<std::string> FileSystem::ListDirectory(const std::string& path, bool recursive) {
    std::vector<std::string> result;
    
    if (!fs::exists(path) || !fs::is_directory(path)) {
        return result;
    }
    
    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            result.push_back(entry.path().string());
        }
    } else {
        for (const auto& entry : fs::directory_iterator(path)) {
            result.push_back(entry.path().string());
        }
    }
    
    return result;
}

} // namespace tvk
