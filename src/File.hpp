#pragma once

#include <stdint.h>
#include <windows.h>
#include <stdlib.h>
// #include "../Memory.h"
// #include "../shared.h"
#include "assert.h"

namespace File
{
    void * MemoryAlloc( size_t size )
    {
        return calloc( 1, size );
    }
    void MemoryFree( void * ptr )
    {
        free( ptr );
    }

    struct file_watch
    {
        FILETIME createTime;
        FILETIME writeTime;
        ULARGE_INTEGER already_handled_this_write = {};
        char* filePath = 0;
    };

    struct file_content
    {
        bool isOK = false;
        char* bytes = 0;
        int count = 0;
        int extraBytesAllocated = 0; // used for sentinels, ...
        char* errorMessage = NULL;
        char* filePath = nullptr; // for GPU to identify file/bitmap
    };

    void Write( char* filePath, char* bytes, int bytesCount )
    {
        HANDLE fileHandle = CreateFileA(
                                filePath,
                                GENERIC_WRITE,//DWORD                 dwDesiredAccess,
                                FILE_SHARE_WRITE,// do not lock the file ... DWORD                 dwShareMode,
                                0,
                                CREATE_NEW | OPEN_EXISTING, // if exists?
                                FILE_ATTRIBUTE_NORMAL, // FILE_ATTRIBUTE_NORMAL, //0,
                                0 // @fixme no async?
                            );
        if( fileHandle != INVALID_HANDLE_VALUE )
        {
            DWORD bytesWritten = 0;
            DWORD bytesToWrite = bytesCount;
            // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile
            WriteFile( fileHandle, bytes, bytesToWrite, &bytesWritten, 0 );
            if( bytesToWrite != bytesWritten )
            {
                __debugbreak();
            }
        }
        else
        {
            __debugbreak();
        }

        // @todo handle failure
    }

    bool Changed( file_watch* watch )
    {
        struct Defer
        {
            HANDLE fileHandle = NULL;
            void ClosingOfHandle( HANDLE _fileHandle )
            {
                fileHandle = _fileHandle;
            }
            ~Defer()
            {
                CloseHandle( fileHandle );
            }
        } defer;

        HANDLE fileHandle = CreateFileA(
                                watch->filePath,
                                GENERIC_READ,
                                FILE_SHARE_READ,// do not lock the file, @maybe FILE_SHARE_WRITE so I can write file while I'm in debugger to avoid msg box
                                0,
                                OPEN_EXISTING, // file must exist
                                FILE_ATTRIBUTE_READONLY,
                                0
                            );
        defer.ClosingOfHandle( fileHandle );

        FILETIME createTime = { 0 };
        FILETIME writeTime = { 0 };
        if( GetFileTime( fileHandle, &createTime, 0, &writeTime ) )
        {
            ULARGE_INTEGER current_write;
            //ULARGE_INTEGER previous_write;
            //previous_write.LowPart = watch->writeTime.dwLowDateTime;
            //previous_write.HighPart = watch->writeTime.dwHighDateTime;

            current_write.LowPart = writeTime.dwLowDateTime;
            current_write.HighPart = writeTime.dwHighDateTime;

            // We are supposed to convert o i64 with this weird way, also time has strange resolution [1]
            // https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
            // maybe it is just unneccessary???

            uint64_t SECONDS = 1000 * 1000 * 10; // in 100 nanoseconds [1]
            bool report_as_changed = false;

            if( !watch->already_handled_this_write.QuadPart )
            {
                report_as_changed = true;
            }
            else
            {
                // has few seconds passed (have some mercy on the harddrive!)
                if( ( watch->already_handled_this_write.QuadPart - current_write.QuadPart ) >= 2 * SECONDS )
                {
                    report_as_changed = true;
                }
            }

            if( report_as_changed )
            {
                watch->already_handled_this_write = current_write;
            }

            return report_as_changed;
        }
        return false;
    }

    //
    // - extra bytes allocated are because it is useful to create sentinels, to search file content for values
    // - watch is optional
    //
    file_content Read( char* filePath, file_watch* watch, int extraBytesAllocated )
    {
        file_content result = {0};

        result.filePath = filePath;

        // https://docs.microsoft.com/en-us/windows/win32/fileio/creating-and-opening-files
        HANDLE fileHandle = CreateFileA(
                                filePath,
                                GENERIC_READ,    // DWORD                 dwDesiredAccess,
                                FILE_SHARE_READ, // do not lock the file ... DWORD                 dwShareMode,
                                0,
                                OPEN_EXISTING,   // if exists?
                                FILE_ATTRIBUTE_READONLY, // FILE_ATTRIBUTE_NORMAL, //0,
                                0
                            );
        if( fileHandle == INVALID_HANDLE_VALUE )
        {
            result.errorMessage = "Invalid file handle";
        }
        else
        {
            DWORD fileSize = GetFileSize( fileHandle, 0 );
            if( fileSize == 0 )
            {
                result.errorMessage = "Empty file";
            }
            else
            {
                char* fileMemoryBuffer = ( char* ) MemoryAlloc( fileSize + extraBytesAllocated );
                if( fileMemoryBuffer == NULL )
                {
                    result.errorMessage = "Failed to allocate memory for file content";
                }
                else
                {
                    DWORD bytesRead = 0;

                    if( ReadFile( fileHandle, fileMemoryBuffer, fileSize, &bytesRead, 0 ) == 0 )
                    {
                        result.errorMessage = "Can't read file for some reason";
                    }
                    else
                    {
                        result.extraBytesAllocated = extraBytesAllocated;
                        result.count = fileSize;
                        result.bytes = fileMemoryBuffer;
                        result.isOK = true;

                        // Create watch? (optional)
                        if( watch )
                        {
                            FILETIME createTime = {0}, writeTime = {0};
                            if( GetFileTime( fileHandle, &createTime, 0, &writeTime ) )
                            {
                                watch->createTime = createTime;
                                watch->writeTime = writeTime;
                                watch->filePath = filePath;

                                // Consider loaded file as handled
                                ULARGE_INTEGER current_write;
                                current_write.HighPart = writeTime.dwHighDateTime;
                                current_write.LowPart = writeTime.dwLowDateTime;
                                watch->already_handled_this_write = current_write;
                            }
                            else
                            {
                                __debugbreak();
                            }
                        }
                    }
                }
            }

            CloseHandle( fileHandle ); // do not lock the file
        }

        return result;
    }

    file_content Read( char* filePath, file_watch* watch )
    {
        return Read( filePath, watch, 0 );
    }

    file_content Read( char* filePath )
    {
        return Read( filePath, NULL, 0 );
    }

    void Free( file_content* file )
    {
        if( file->isOK )
        {
            assert( file->bytes );

            MemoryFree( file->bytes );

            file->isOK = false; // ???
            file->errorMessage = "Memory deallocated. Not necessary error though.";
        }
    }
};