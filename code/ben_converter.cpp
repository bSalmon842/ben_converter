/*
Project: Viewport Shader Code Converter
File: ben_converter.cpp
Author: Brock Salmon
Notice: (C) Copyright 2021 by Brock Salmon. All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>

#define function static
#define local_persist static
#define global static

typedef unsigned __int8 u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

typedef __int8 s8;
typedef __int16 s16;
typedef __int32 s32;
typedef __int64 s64;

typedef size_t mem_index;

typedef __int32 b32;
typedef bool b8;

typedef float f32;
typedef double f64;

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

 inline void CopyChars(char *dest, char *src, s32 size)
{
    while (size--)
    {
        *dest++ = *src++;
    }
}

 inline void SetChars(char *dest, char value, s32 size)
{
    while (size--)
    {
        *dest++ = value;
    }
}

inline s32 StringLength(char *str)
{
    s32 result = 0;
    while (*str++)
	{
		++result;
	}
    return result;
}

inline s32 GetFileSize(FILE *file)
{
    fseek(file, 0, SEEK_END);
    s32 result = ftell(file);
    fseek(file, 0, SEEK_SET);
    return result;
}

inline void CopyCharsAndAdvancePtr(char *destBuffer, char *srcBuffer, s32 srcSize, s32 *bufAdv)
{
    CopyChars(destBuffer + *bufAdv, srcBuffer, srcSize);
     *bufAdv += srcSize;
}

inline void ReadFileAndCopyContents(FILE *file, s32 fileSize, char *buffer, s32 *bufAdv)
{
    char *fileContents = (char *)malloc(fileSize);
    SetChars(fileContents, ' ', fileSize);
    fread(fileContents, 1, fileSize, file);
    CopyCharsAndAdvancePtr(buffer, fileContents, fileSize, bufAdv);
        free(fileContents);
}

inline void ConvertNotation(char *outBuffer, s32 charIndex, char elementNumber, s32 outBufferSize, s32 *conversionCount)
{
    outBuffer[charIndex] = '[';
    outBuffer[charIndex + 1] = elementNumber;
    s32 swapIndex = outBufferSize - 1;
    while (true)
    {
        char oldValue = outBuffer[swapIndex - 1];
        outBuffer[swapIndex - 1] = outBuffer[swapIndex];
        outBuffer[swapIndex] = oldValue;
        
        --swapIndex;
        
        if (swapIndex == (charIndex + 2))
        {
            outBuffer[swapIndex] = ']';
            break;
        }
    }
    --(*conversionCount);
}

s32 main(s32 argc, char **argv)
{
    if (argc != 3)
    {
        printf("Incorrect Number of Arguments, format is: [program executable] [in-file] [out-file]\neg: program.exe a.txt b.txt");
    }
    else
    {
        // TODO(bSalmon): Get files, merge them into one buffer
        FILE *structLibFile = fopen("StructLib.osl", "r");
        FILE *ue4StdFile = fopen("UE4Std.osl", "r");
        FILE *mfLibFile = fopen("MFLib.osl", "r");
        if (!(mfLibFile && ue4StdFile && structLibFile))
        {
            if (!mfLibFile)
            {
                printf("Could not find MFLib.osl\n");
            }
            if (!ue4StdFile)
            {
                printf("Could not find UE4Std.osl\n");
            }
            if (!structLibFile)
            {
                printf("Could not find StructLib.osl\n");
            }
        }
        else
        {
            s32 structLibSize = GetFileSize(structLibFile);
            s32 ue4StdSize = GetFileSize(ue4StdFile);
            s32 mfLibSize = GetFileSize(mfLibFile);
            
            char *importHeaderComment = "\n// IMPORTED FUNCTIONS START //";
            char *importFooterComment = "\n// IMPORTED FUNCTIONS END //\n\n";
            char *importStructLibComment = "\n\n// StructLib.osl //\n\n";
            char *importUE4StdComment = "\n\n// UE4Std.osl //\n\n";
            char *importMFLibComment = "\n\n// MFLib.osl //\n\n";
            s32 importCommentsSize = (StringLength(importHeaderComment) + StringLength(importFooterComment) +
                                      StringLength(importStructLibComment) + StringLength(importUE4StdComment) + StringLength(importMFLibComment));
            
            s32 maxImportSize = importCommentsSize + mfLibSize + ue4StdSize + structLibSize;
            char *maxImportBuffer = (char *)malloc(maxImportSize);
            SetChars(maxImportBuffer, ' ', maxImportSize);
            
            s32 bufAdv = 0;
              CopyCharsAndAdvancePtr(maxImportBuffer, importHeaderComment, StringLength(importHeaderComment), &bufAdv);
            CopyCharsAndAdvancePtr(maxImportBuffer, importStructLibComment, StringLength(importStructLibComment), &bufAdv);
            ReadFileAndCopyContents(structLibFile, structLibSize, maxImportBuffer, &bufAdv);
            
            CopyCharsAndAdvancePtr(maxImportBuffer, importUE4StdComment, StringLength(importUE4StdComment), &bufAdv);
            ReadFileAndCopyContents(ue4StdFile, ue4StdSize, maxImportBuffer, &bufAdv);
            
            CopyCharsAndAdvancePtr(maxImportBuffer, importMFLibComment, StringLength(importMFLibComment), &bufAdv);
            ReadFileAndCopyContents(mfLibFile, mfLibSize, maxImportBuffer, &bufAdv);
            CopyChars(maxImportBuffer + bufAdv, importFooterComment, StringLength(importFooterComment));
            
            fclose(mfLibFile);
            fclose(ue4StdFile);
            fclose(structLibFile);
            
        char *inFilepath = argv[1];
        char *outFilepath = argv[2];
        
        FILE *inFile = fopen(inFilepath, "r");
        FILE *outFile = fopen(outFilepath, "w");
        if (inFile && outFile)
        {
                s32 inFileSize = GetFileSize(inFile);
            char *inBuffer = (char *)malloc(inFileSize);
            fread(inBuffer, 1, inFileSize, inFile);
        fclose(inFile);
            
            s32 conversionCount = 0;
            for (s32 charIndex = 0; charIndex < inFileSize; ++charIndex)
            {
                if (inBuffer[charIndex] == '.' && ((charIndex + 1) != inFileSize))
                {
                    char advCheck = inBuffer[charIndex + 1];
                    if (advCheck == 'r' || advCheck == 'g' || advCheck == 'b' || 
                        advCheck == 'x' || advCheck == 'y' || advCheck == 'z')
                    {
                        ++conversionCount;
                    }
                }
            }
            
            s32 outBufferSize = maxImportSize + inFileSize + conversionCount;
            
            char *outBuffer = (char *)malloc(outBufferSize);
            SetChars(outBuffer, ' ', outBufferSize);
            CopyChars(outBuffer, maxImportBuffer, maxImportSize);
            CopyChars(outBuffer + maxImportSize, inBuffer, inFileSize);
            free(inBuffer);
            
            for (s32 charIndex = 0; charIndex < outBufferSize; ++charIndex)
            {
                if (outBuffer[charIndex] == '.' && !((outBuffer[charIndex + 2] >= 'A' && outBuffer[charIndex + 2] <= 'Z') || (outBuffer[charIndex + 2] >= 'a' && outBuffer[charIndex + 2] <= 'z')))
                {
                    char advCheck = outBuffer[charIndex + 1];
                    if (advCheck == 'r' || advCheck == 'x')
                    {
                        ConvertNotation(outBuffer, charIndex, '0', outBufferSize, &conversionCount);
                    }
                    else if (advCheck == 'g' || advCheck == 'y')
                    {
                        ConvertNotation(outBuffer, charIndex, '1', outBufferSize, &conversionCount);
                    }
                    else if (advCheck == 'b' || advCheck == 'z')
                    {
                        ConvertNotation(outBuffer, charIndex, '2', outBufferSize, &conversionCount);
                    }
                }
                
                if (conversionCount <= 0)
                {
                    break;
                }
            }
            
            fwrite(outBuffer, outBufferSize, 1, outFile);
            fclose(outFile);
            free(outBuffer);
            }
        }
    }
    
    return 0;
}