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

#define INCLUDES_CHAR_COUNT 69
#define INCLUDES_TEXT "#include \"StructLib.osl\"\n#include \"UE4std.osl\"\n#include \"MFLib.osl\"\n\n"

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
        char *inFilepath = argv[1];
        char *outFilepath = argv[2];
        
        FILE *inFile = fopen(inFilepath, "r");
        FILE *outFile = fopen(outFilepath, "w");
        if (inFile && outFile)
        {
            fseek(inFile, 0, SEEK_END);
            s32 inFileSize = ftell(inFile);
            fseek(inFile, 0, SEEK_SET);
            
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
            
            s32 outBufferSize = INCLUDES_CHAR_COUNT + inFileSize + conversionCount;
            
            char *outBuffer = (char *)malloc(outBufferSize);
            SetChars(outBuffer, ' ', outBufferSize);
            CopyChars(outBuffer + INCLUDES_CHAR_COUNT, inBuffer, inFileSize);
            
            free(inBuffer);
            
            CopyChars(outBuffer, INCLUDES_TEXT, INCLUDES_CHAR_COUNT);
            
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
    
    return 0;
}