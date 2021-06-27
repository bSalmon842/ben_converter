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
    s32 counter = 0;
    while (counter < size)
    {
        *dest++ = *src++;
        counter++;
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

inline b32 IsStringAEqualAtB(char *a, char *b, s32 aSize)
{
    b32 result = true;
    
    for (s32 i = 0; i < aSize; ++i)
    {
        if (a[i] != b[i])
        {
            return false;
        }
    }
    
    return result;
}

inline void ConcatenateStrings(char *dest, char *a, s32 aSize, char *b, s32 bSize)
{
    for (s32 i = 0; i < aSize; ++i)
	{
		*dest++ = *a++;
	}
    
	for (s32 i = 0; i < bSize; ++i)
	{
		*dest++ = *b++;
	}
}

inline void InsertStringAIntoStringB(char *a, char *b, char *at)
{
    if (at == b)
    {
        char *temp = (char *)malloc(StringLength(a) + StringLength(b));
        ConcatenateStrings(temp, a, StringLength(a), b, StringLength(b));
        CopyChars(b, temp, StringLength(b));
        free(temp);
    }
    else
    {
        s32 diff = (s32)(at - b);
        char *temp1 = (char *)malloc(diff + 2);
        char *temp2 = (char *)malloc(StringLength(b) + StringLength(a));
        ConcatenateStrings(temp1, b, diff, a, StringLength(a));
        ConcatenateStrings(temp2, temp1, StringLength(a) + diff, at, StringLength(b) - diff);
        CopyChars(b, temp2, StringLength(b));
        free(temp2);
        free(temp1);
    }
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
    InsertStringAIntoStringB("]", outBuffer, &outBuffer[charIndex + 2]);
}

s32 main(s32 argc, char **argv)
{
    if (argc != 3)
    {
        printf("Incorrect Number of Arguments, format is: [program executable] [in-file] [out-file]\neg: program.exe a.txt b.txt");
    }
    else
    {
        FILE *structLibFile = fopen("StructLib.osl", "rb");
        FILE *ue4StdFile = fopen("UE4Std.osl", "rb");
        FILE *mfLibFile = fopen("MFLib.osl", "rb");
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
            
            // TODO(bSalmon): Find functions
            
        char *inFilepath = argv[1];
        char *outFilepath = argv[2];
        
        FILE *inFile = fopen(inFilepath, "rb");
        FILE *outFile = fopen(outFilepath, "wb");
        if (inFile && outFile)
        {
                s32 inFileSize = GetFileSize(inFile);
            char *inBuffer = (char *)malloc(inFileSize);
            fread(inBuffer, 1, inFileSize, inFile);
        fclose(inFile);
                
                s32 includesSize = 0;
                b32 foundInclude = false;
                for (s32 charIndex = 0; charIndex < inFileSize; ++charIndex)
                {
                    if (IsStringAEqualAtB("#include", &inBuffer[charIndex], 8))
                    {
                        foundInclude = true;
                    }
                    
                        if (foundInclude && inBuffer[charIndex] == '\n')
                        {
                        includesSize = charIndex + 1;
                        foundInclude = false;
                        }
                }
                
            s32 outBufferSize = maxImportSize + inFileSize;
            
            char *outBuffer = (char *)malloc(outBufferSize);
                SetChars(outBuffer, ' ', outBufferSize);
                
                CopyChars(outBuffer, inBuffer, includesSize);
                CopyChars(outBuffer + includesSize, maxImportBuffer, maxImportSize);
                CopyChars(outBuffer + includesSize + maxImportSize, inBuffer + includesSize, inFileSize - includesSize);
            free(inBuffer);
                
                s32 conversionCount = 0;
                for (s32 charIndex = 0; charIndex < outBufferSize; ++charIndex)
                {
                    if (outBuffer[charIndex] == '.' && ((charIndex + 1) != outBufferSize))
                    {
                        char advCheck = outBuffer[charIndex + 1];
                        if (advCheck == 'r' || advCheck == 'g' || advCheck == 'b' || 
                            advCheck == 'x' || advCheck == 'y' || advCheck == 'z')
                        {
                            ++conversionCount;
                        }
                    }
                }
                
                s32 includeCount = 0;
                for (s32 charIndex = 0; charIndex < outBufferSize; ++charIndex)
                {
                    if (outBuffer[charIndex] == '#' && ((charIndex + 1) != outBufferSize))
                    {
                        char advCheck = outBuffer[charIndex + 1];
                        if (advCheck == 'i')
                        {
                            ++includeCount;
                        }
                    }
                }
                
                outBufferSize += (conversionCount + (includeCount * 2));
                outBuffer = (char *)realloc(outBuffer, outBufferSize);
                
                // TODO(bSalmon): Insert comments
                for (s32 charIndex = 0; charIndex < outBufferSize; ++charIndex)
                {
                    if (IsStringAEqualAtB("#include", &outBuffer[charIndex], 8))
                    {
                        InsertStringAIntoStringB("//", outBuffer, &outBuffer[charIndex]);
                        charIndex += 10;
                    }
                }
                
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
            }
            
            fwrite(outBuffer, outBufferSize, 1, outFile);
            fclose(outFile);
            //free(outBuffer);
            }
        }
    }
    
    return 0;
}