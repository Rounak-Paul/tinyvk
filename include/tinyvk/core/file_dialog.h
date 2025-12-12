/**
 * @file file_dialog.h
 * @brief Cross-platform file dialog utilities for TinyVK
 */

#pragma once

#include "../core/types.h"
#include <string>
#include <vector>
#include <optional>

namespace tvk {

/**
 * @brief File filter for dialogs
 */
struct FileFilter {
    std::string name;       // e.g., "Image Files"
    std::string pattern;    // e.g., "*.png;*.jpg;*.jpeg;*.bmp"
    
    FileFilter(const std::string& n, const std::string& p) : name(n), pattern(p) {}
};

/**
 * @brief Common file filters
 */
namespace Filters {
    inline FileFilter Images() { return {"Image Files", "*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.gif"}; }
    inline FileFilter AllFiles() { return {"All Files", "*.*"}; }
    inline FileFilter Text() { return {"Text Files", "*.txt;*.md;*.json;*.xml;*.yaml;*.yml"}; }
    inline FileFilter Models() { return {"3D Models", "*.obj;*.fbx;*.gltf;*.glb"}; }
    inline FileFilter Audio() { return {"Audio Files", "*.wav;*.mp3;*.ogg;*.flac"}; }
}

/**
 * @brief File dialog utilities
 */
class FileDialog {
public:
    /**
     * @brief Open a file dialog to select a single file
     * @param filters File type filters
     * @param defaultPath Starting directory path
     * @return Selected file path, or empty if cancelled
     */
    static std::optional<std::string> OpenFile(
        const std::vector<FileFilter>& filters = {Filters::AllFiles()},
        const std::string& defaultPath = "");

    /**
     * @brief Open a file dialog to select multiple files
     * @param filters File type filters
     * @param defaultPath Starting directory path
     * @return Vector of selected file paths, empty if cancelled
     */
    static std::vector<std::string> OpenFiles(
        const std::vector<FileFilter>& filters = {Filters::AllFiles()},
        const std::string& defaultPath = "");

    /**
     * @brief Open a save file dialog
     * @param filters File type filters
     * @param defaultPath Starting directory/filename
     * @return Selected save path, or empty if cancelled
     */
    static std::optional<std::string> SaveFile(
        const std::vector<FileFilter>& filters = {Filters::AllFiles()},
        const std::string& defaultPath = "");

    /**
     * @brief Open a folder selection dialog
     * @param defaultPath Starting directory path
     * @return Selected folder path, or empty if cancelled
     */
    static std::optional<std::string> SelectFolder(const std::string& defaultPath = "");
};

/**
 * @brief File system utilities
 */
class FileSystem {
public:
    /**
     * @brief Check if a file exists
     */
    static bool Exists(const std::string& path);

    /**
     * @brief Get file extension (with dot)
     */
    static std::string GetExtension(const std::string& path);

    /**
     * @brief Get filename from path
     */
    static std::string GetFilename(const std::string& path);

    /**
     * @brief Get filename without extension
     */
    static std::string GetStem(const std::string& path);

    /**
     * @brief Get parent directory
     */
    static std::string GetDirectory(const std::string& path);

    /**
     * @brief Read entire file as string
     */
    static std::optional<std::string> ReadText(const std::string& path);

    /**
     * @brief Read entire file as binary data
     */
    static std::optional<std::vector<u8>> ReadBinary(const std::string& path);

    /**
     * @brief Write string to file
     */
    static bool WriteText(const std::string& path, const std::string& content);

    /**
     * @brief Write binary data to file
     */
    static bool WriteBinary(const std::string& path, const std::vector<u8>& data);

    /**
     * @brief Get current working directory
     */
    static std::string GetWorkingDirectory();

    /**
     * @brief List files in directory
     */
    static std::vector<std::string> ListDirectory(const std::string& path, bool recursive = false);
};

} // namespace tvk
