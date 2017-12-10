/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "StringUtils.h"
#include "../Include/SandboxAPI.h"

class QWidget;

#ifdef CreateDirectory
#undef CreateDirectory
#endif

struct IFileUtil
{
    //! File types used for File Open dialogs
    enum ECustomFileType
    {
        EFILE_TYPE_ANY,
        EFILE_TYPE_GEOMETRY,
        EFILE_TYPE_TEXTURE,
        EFILE_TYPE_SOUND,
        EFILE_TYPE_GEOMCACHE,
        EFILE_TYPE_LAST,
    };

    struct FileDesc
    {
        QString filename;
        unsigned int attrib;
        time_t  time_create;    //! -1 for FAT file systems
        time_t  time_access;    //! -1 for FAT file systems
        time_t  time_write;
        int64 size;
    };

    enum ETextFileType
    {
        FILE_TYPE_SCRIPT,
        FILE_TYPE_SHADER,
        FILE_TYPE_BSPACE,   // added back in 3.8 integration, may not end up needing this.
    };

    enum ECopyTreeResult
    {
        ETREECOPYOK,
        ETREECOPYFAIL,
        ETREECOPYUSERCANCELED,
        ETREECOPYUSERDIDNTCOPYSOMEITEMS,
    };

    struct ExtraMenuItems
    {
        QStringList names;
        int selectedIndexIfAny;

        ExtraMenuItems()
            : selectedIndexIfAny(-1) {}

        int AddItem(const QString& name)
        {
            names.push_back(name);
            return names.size() - 1;
        }
    };

    typedef DynArray<FileDesc> FileArray;

    typedef bool (* ScanDirectoryUpdateCallBack)(const QString& msg);

    virtual ~IFileUtil() = default;

    virtual bool ScanDirectory(const QString& path, const QString& fileSpec, FileArray& files, bool recursive = true, bool addDirAlso = false, ScanDirectoryUpdateCallBack updateCB = nullptr, bool bSkipPaks = false) = 0;

    virtual void ShowInExplorer(const QString& path) = 0;

    virtual bool CompileLuaFile(const char* luaFilename) = 0;
    virtual bool ExtractFile(QString& file, bool bMsgBoxAskForExtraction = true, const char* pDestinationFilename = nullptr) = 0;
    virtual void EditTextFile(const char* txtFile, int line = 0, ETextFileType fileType = FILE_TYPE_SCRIPT) = 0;
    virtual void EditTextureFile(const char* txtureFile, bool bUseGameFolder) = 0;

    //! dcc filename calculation and extraction sub-routines
    virtual bool CalculateDccFilename(const QString& assetFilename, QString& dccFilename) = 0;

    //! Reformat filter string for (MFC) CFileDialog style file filtering
    virtual void FormatFilterString(QString& filter) = 0;

    virtual bool SelectSaveFile(const QString& fileFilter, const QString& defaulExtension, const QString& startFolder, QString& fileName) = 0;

    //! Attempt to make a file writable
    virtual bool OverwriteFile(const QString& filename) = 0;

    //! Checks out the file from source control API.  Blocks until completed
    virtual bool CheckoutFile(const char* filename, QWidget* parentWindow = nullptr) = 0;

    //! Discard changes to a file from source control API.  Blocks until completed
    virtual bool RevertFile(const char* filename, QWidget* parentWindow = nullptr) = 0;

    //! Deletes a file using source control API.  Blocks until completed.
    virtual bool DeleteFromSourceControl(const char* filename, QWidget* parentWindow = nullptr) = 0;

    //! Creates this directory.
    virtual void CreateDirectory(const char* dir) = 0;

    //! Makes a backup file.
    virtual void BackupFile(const char* filename) = 0;

    //! Makes a backup file, marked with a datestamp, e.g. myfile.20071014.093320.xml
    //! If bUseBackupSubDirectory is true, moves backup file into a relative subdirectory "backups"
    virtual void BackupFileDated(const char* filename, bool bUseBackupSubDirectory = false) = 0;

    // ! Added deltree as a copy from the function found in Crypak.
    virtual bool Deltree(const char* szFolder, bool bRecurse) = 0;

    // Checks if a file or directory exist.
    // We are using 3 functions here in order to make the names more instructive for the programmers.
    // Those functions only work for OS files and directories.
    virtual bool Exists(const QString& strPath, bool boDirectory, FileDesc* pDesc = nullptr) = 0;
    virtual bool FileExists(const QString& strFilePath, FileDesc* pDesc = nullptr) = 0;
    virtual bool PathExists(const QString& strPath) = 0;
    virtual bool GetDiskFileSize(const char* pFilePath, uint64& rOutSize) = 0;

    // This function should be used only with physical files.
    virtual bool IsFileExclusivelyAccessable(const QString& strFilePath) = 0;

    // Creates the entire path, if needed.
    virtual bool CreatePath(const QString& strPath) = 0;

    // Attempts to delete a file (if read only it will set its attributes to normal first).
    virtual bool DeleteFile(const QString& strPath) = 0;

    // Attempts to remove a directory (if read only it will set its attributes to normal first).
    virtual bool RemoveDirectory(const QString& strPath) = 0;

    // Copies all the elements from the source directory to the target directory.
    // It doesn't copy the source folder to the target folder, only it's contents.
    // THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
    virtual ECopyTreeResult CopyTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse = true, bool boConfirmOverwrite = false) = 0;

    //////////////////////////////////////////////////////////////////////////
    // @param LPPROGRESS_ROUTINE pfnProgress - called by the system to notify of file copy progress
    // @param LPBOOL pbCancel - when the contents of this BOOL are set to TRUE, the system cancels the copy operation
    virtual ECopyTreeResult CopyFile(const QString& strSourceFile, const QString& strTargetFile, bool boConfirmOverwrite = false, void* pfnProgress = nullptr, bool* pbCancel = nullptr) = 0;

    // As we don't have a FileUtil interface here, we have to duplicate some code :-( in order to keep
    // function calls clean.
    // Moves all the elements from the source directory to the target directory.
    // It doesn't move the source folder to the target folder, only it's contents.
    // THIS FUNCTION IS NOT DESIGNED FOR MULTI-THREADED USAGE
    virtual ECopyTreeResult MoveTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse = true, bool boConfirmOverwrite = false) = 0;

    //
    virtual ECopyTreeResult MoveFile(const QString& strSourceFile, const QString& strTargetFile, bool boConfirmOverwrite = false) = 0;

    virtual void GatherAssetFilenamesFromLevel(std::set<QString>& rOutFilenames, bool bMakeLowerCase = false, bool bMakeUnixPath = false) = 0;

    // Get file attributes include source control attributes if available
    virtual uint32 GetAttributes(const char* filename, bool bUseSourceControl = true) = 0;

    // Returns true if the files have the same content, false otherwise
    virtual bool CompareFiles(const QString& strFilePath1, const QString& strFilePath2) = 0;

    virtual QString GetPath(const QString& path) = 0;
};
