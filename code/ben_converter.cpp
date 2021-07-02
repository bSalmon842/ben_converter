/*
Project: Viewport Shader Code Converter
File: ben_converter.cpp
Author: Brock Salmon
Notice: (C) Copyright 2021 by Brock Salmon. All Rights Reserved.
*/

#include <stdio.h>
#include <stdlib.h>

#define function static

typedef __int32 s32;
typedef __int32 b32;
typedef float f32;

#if CONVERTER_SLOW
#define ASSERT(check) if(!(check)) {*(s32 *)0 = 0;}
#define INVALID_CODE_PATH ASSERT(false)
#else
#define ASSERT(check)
#define INVALID_CODE_PATH
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define MAX(a, b) ((a > b) ? a : b)

struct FunctionInfo
{
    char *typeString;
    s32 typeSize;
    
    char *nameString;
    s32 nameSize;
    
    char *paramsString;
    s32 paramsSize;
    
    char *bodyString;
    s32 bodySize;
    
    s32 size;
    b32 foundInFile;
};

struct StructInfo
{
    char *structString;
    s32 size;
    
    char *nameString;
    s32 nameSize;
    
    char *contentsString;
    s32 contentsSize;
    b32 foundInFile;
};

struct DefineInfo
{
    char *defineString;
    s32 size;
    
    char *nameString;
    s32 nameSize;
    
    char *bodyString;
    s32 bodySize;
    
    b32 foundInFile;
};

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

inline b32 IsAlphaNumeric(char c)
{
    b32 result = false;
    
    if (((c >= '0') && (c <= '9')) || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')))
    {
        result = true;
    }
    
    return result;
}

inline b32 IsTypeNameAcceptableCharacter(char c)
{
    b32 result = false;
    
    if (IsAlphaNumeric(c) || c == '_')
    {
        result = true;
    }
    
    return result;
}

inline b32 IsParamAcceptableCharacter(char c)
{
    b32 result = false;
    
    if (IsTypeNameAcceptableCharacter(c) || c == '(' || c == ')' || c == ' ' || c == ',')
    {
        result = true;
    }
    
    return result;
}

enum FunctionSegment
{
    FuncSeg_ReturnType,
    FuncSeg_FuncName,
    FuncSeg_Params,
    FuncSeg_Body,
};

enum StructSegment
{
    StructSeg_Name,
    StructSeg_Contents,
};

enum DefineSegment
{
    DefSeg_Name,
    DefSeg_Params,
    DefSeg_Body,
};

 function b32 IsStringDefine(DefineInfo *defInfo, char *str, s32 strSize)
{
    b32 result = false;
    
    if (IsStringAEqualAtB("#define ", str, 8))
    {
        DefineSegment currSegment = DefSeg_Name;
        DefineSegment lastSegment = DefSeg_Name;
        b32 isFirstCharInSegment = true;
        b32 ignoreNextNewLine = false;
        b32 inQuotes = false;
        b32 defCheck = true;
        
        defInfo->defineString = str;
        defInfo->size = 8;
        
        defInfo->nameString = str + 8;
        defInfo->nameSize = 0;
        for (s32 charIndex = 8; charIndex < strSize && defCheck; ++charIndex)
        {
            if (lastSegment != currSegment)
            {
                 isFirstCharInSegment = true;
                lastSegment = currSegment;
            }
            
            switch (currSegment)
            {
                case DefSeg_Name:
                {
                    if (str[charIndex] != ' ' && str[charIndex] != '(')
                    {
                        defInfo->nameSize++;
                    }
                    else if (str[charIndex] == '(')
                    {
                        currSegment = DefSeg_Params;
                    }
                    else
                    {
                        currSegment = DefSeg_Body;
                    }
                }break;
                
                case DefSeg_Params:
                {
                    if (str[charIndex] == ' ')
                    {
                        currSegment = DefSeg_Body;
                    }
                }break;
                
                case DefSeg_Body:
                {
                    if (isFirstCharInSegment)
                    {
                        defInfo->bodyString = &str[charIndex];
                        defInfo->bodySize = 1;
                        isFirstCharInSegment = false;
                    }
                    else
                    {
                        if (str[charIndex] == '"')
                        {
                            inQuotes = !inQuotes;
                        }
                        
                        if (str[charIndex] == '\\' && !inQuotes)
                        {
                            ignoreNextNewLine = true;
                        }
                        
                        if (str[charIndex] == '\n' && !ignoreNextNewLine)
                        {
                            defCheck = false;
                            defInfo->bodySize++;
                            result = true;
                            defInfo->size = charIndex + 2;
                        }
                        else if (str[charIndex] == '\n' && ignoreNextNewLine)
                        {
                            ignoreNextNewLine = false;
                        }
                        else
                        {
                            defInfo->bodySize++;
                        }
                    }
                }break;
                
                default:
                {
                    INVALID_CODE_PATH;
                }break;
            }
        }
    }
    
    return result;
}

function b32 IsStringStruct(StructInfo *structInfo, char *str, s32 strSize)
{
    b32 result = false;
    
    if (IsStringAEqualAtB("\nstruct", str, 7))
    {
        StructSegment currSegment = StructSeg_Name;
        StructSegment lastSegment = StructSeg_Name;
        b32 isFirstCharInSegment = true;
        b32 structCheck = true;
        s32 braceCount = 0;
        
        structInfo->structString = str + 1;
        structInfo->size = 7;
        
        structInfo->nameString = str + 8;
        structInfo->nameSize = 0;
        for (s32 charIndex = 8; charIndex < strSize && structCheck; ++charIndex)
        {
            if (lastSegment != currSegment)
            {
                isFirstCharInSegment = true;
                lastSegment = currSegment;
            }
            
            switch (currSegment)
            {
                case StructSeg_Name:
                {
                    if (IsTypeNameAcceptableCharacter(str[charIndex]))
                    {
                        structInfo->nameSize++;
                    }
                    else if (str[charIndex] != '{') {}
                    else
                    {
                        currSegment = StructSeg_Contents;
                    }
                }break;
                
                case StructSeg_Contents:
                {
                    if (isFirstCharInSegment)
                    {
                        structInfo->contentsString = &str[charIndex];
                        structInfo->contentsSize = 1;
                        isFirstCharInSegment = false;
                        braceCount = 1;
                    }
                    else
                    {
                        if (str[charIndex] == '{')
                        {
                            braceCount++;
                        }
                        else if (str[charIndex] == '}')
                        {
                            braceCount--;
                        }
                        
                        if (str[charIndex] == ';' && braceCount == 0)
                        {
                            structCheck = false;
                            structInfo->contentsSize++;
                            result = true;
                            structInfo->size = charIndex + 2;
                        }
                        else
                        {
                            structInfo->contentsSize++;
                        }
                    }
                }break;
                
                default:
                {
                    INVALID_CODE_PATH;
                }break;
            }
        }
    }
    
    return result;
}

 function b32 IsStringFunction(FunctionInfo *funcInfo, char *str, s32 strSize, b32 isStartOfSrcBuffer)
{
    b32 result = false;
    
    FunctionSegment currSegment = FuncSeg_ReturnType;
    FunctionSegment lastSegment = FuncSeg_ReturnType;
    b32 funcCheck = true;
    b32 isFirstCharInSegment = true;
    s32 braceCount = 0;
        if ((isStartOfSrcBuffer || str[-1] == '\n') && IsTypeNameAcceptableCharacter(*str))
        {
            for (s32 charIndex = 0; charIndex < strSize && funcCheck; ++charIndex)
        {
            if (lastSegment != currSegment)
            {
                isFirstCharInSegment = true;
                lastSegment = currSegment;
            }
            
                switch (currSegment)
                {
                    case FuncSeg_ReturnType:
                    {
                    if (IsTypeNameAcceptableCharacter(str[charIndex]))
                        {
                        if (isFirstCharInSegment)
                        {
                            funcInfo->typeString = &str[charIndex];
                            funcInfo->typeSize = 1;
                            isFirstCharInSegment = false;
                        }
                        else
                        {
                            funcInfo->typeSize++;
                        }
                    }
                    else if (str[charIndex] == ' ' && !isFirstCharInSegment)
                    {
                        currSegment = FuncSeg_FuncName;
                    }
                        else
                    {
                        funcCheck = false;
                        }
                    } break;
                    
                    case FuncSeg_FuncName:
                    {
                    if (IsTypeNameAcceptableCharacter(str[charIndex]) && str[charIndex + 1] != '(')
                    {
                        if (isFirstCharInSegment)
                        {
                            funcInfo->nameString = &str[charIndex];
                            funcInfo->nameSize = 1;
                            isFirstCharInSegment = false;
                        }
                        else
                        {
                            funcInfo->nameSize++;
                        }
                    }
                    else if (str[charIndex + 1] == '(')
                    {
                        currSegment = FuncSeg_Params;
                        funcInfo->nameSize++;
                    }
                    else
                    {
                        funcCheck = false;
                    }
                    } break;
                    
                    case FuncSeg_Params:
                {
                    if (IsParamAcceptableCharacter(str[charIndex]) && str[charIndex] != ')')
                    {
                        if (isFirstCharInSegment)
                        {
                            funcInfo->paramsString = &str[charIndex];
                            funcInfo->paramsSize = 1;
                            isFirstCharInSegment = false;
                        }
                        else
                        {
                            funcInfo->paramsSize++;
                        }
                    }
                    else if (str[charIndex] == ')' && !isFirstCharInSegment)
                    {
                        currSegment = FuncSeg_Body;
                        funcInfo->paramsSize++;
                    }
                    else
                    {
                        funcCheck = false;
                    }
                    } break;
                    
                    case FuncSeg_Body:
                {
                    if (str[charIndex] == '{')
                    {
                        braceCount++;
                    }
                    else if (str[charIndex] == '}')
                    {
                        braceCount--;
                    }
                    
                    if (str[charIndex] == '{' && isFirstCharInSegment)
                        {
                            funcInfo->bodyString = &str[charIndex];
                        funcInfo->bodySize = 1;
                            isFirstCharInSegment = false;
                    }
                    else if (str[charIndex] == '}' && braceCount == 0 && !isFirstCharInSegment)
                    {
                        funcCheck = false;
                        funcInfo->bodySize++;
                        result = true;
                        funcInfo->size = charIndex + 2;
                    }
                    else if (!isFirstCharInSegment)
                        {
                            funcInfo->bodySize++;
                        }
                    } break;
                    
                    default:
                    {
                        INVALID_CODE_PATH;
                    } break;
                }
            }
        }
    
    return result;
}

function b32 FilterImports(char *str, s32 strSize, DefineInfo **defList, s32 nextAvailableDefSlot, StructInfo **structList, s32 nextAvailableStructSlot, FunctionInfo **funcList, s32 nextAvailableFuncSlot)
{
    b32 result = false;
    
    s32 maxSlot = MAX(MAX(nextAvailableDefSlot, nextAvailableStructSlot), nextAvailableFuncSlot);
    for (s32 charIndex = 0; charIndex < strSize; ++charIndex)
    {
        for (s32 i = 0; i < maxSlot; ++i)
        {
            if ((*defList)[i].defineString)
            {
                if (IsStringAEqualAtB((*defList)[i].nameString, &str[charIndex], (*defList)[i].nameSize))
                {
                    if(!(*defList)[i].foundInFile)
                    {
                        (*defList)[i].foundInFile = true;
                        result = true;
                    }
                }
            }
            
            if ((*structList)[i].structString)
            {
                if (IsStringAEqualAtB((*structList)[i].nameString, &str[charIndex], (*structList)[i].nameSize))
                {
                    if (!(*structList)[i].foundInFile)
                    {
                        (*structList)[i].foundInFile = true;
                        result = true;
                    }
                }
            }
            
            if ((*funcList)[i].nameString)
            {
                if (IsStringAEqualAtB((*funcList)[i].nameString, &str[charIndex], (*funcList)[i].nameSize))
                {
                    if (!(*funcList)[i].foundInFile)
                    {
                        (*funcList)[i].foundInFile = true;
                        result = true;
                    }
                }
            }
        }
    }
    
    return result;
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
            
            char *importHeaderComment = "\n// IMPORTED CONTENTS START //\n";
            char *importFooterComment = "// IMPORTED CONTENTS END //\n\n";
            s32 importCommentsSize = (StringLength(importHeaderComment) + StringLength(importFooterComment));
            
            s32 maxImportSize = importCommentsSize + mfLibSize + ue4StdSize + structLibSize;
            char *maxImportBuffer = (char *)malloc(maxImportSize);
            SetChars(maxImportBuffer, ' ', maxImportSize);
            
            s32 bufAdv = 0;
              CopyCharsAndAdvancePtr(maxImportBuffer, importHeaderComment, StringLength(importHeaderComment), &bufAdv);
            ReadFileAndCopyContents(structLibFile, structLibSize, maxImportBuffer, &bufAdv);
            ReadFileAndCopyContents(ue4StdFile, ue4StdSize, maxImportBuffer, &bufAdv);
            ReadFileAndCopyContents(mfLibFile, mfLibSize, maxImportBuffer, &bufAdv);
            CopyChars(maxImportBuffer + bufAdv, importFooterComment, StringLength(importFooterComment));
            
            fclose(mfLibFile);
            fclose(ue4StdFile);
            fclose(structLibFile);
            
            DefineInfo *defList = (DefineInfo *)calloc(256, sizeof(DefineInfo));
            StructInfo *structList = (StructInfo *)calloc(256, sizeof(StructInfo));
            FunctionInfo *funcList = (FunctionInfo *)calloc(256, sizeof(FunctionInfo));
            
            s32 nextAvailableDefSlot = 0;
            s32 nextAvailableStructSlot = 0;
            s32 nextAvailableFuncSlot = 0;
            
            for (s32 charIndex = 0; charIndex < maxImportSize; ++charIndex)
            {
                DefineInfo def = {};
                StructInfo st = {};
                FunctionInfo func = {};
                
                if (IsStringDefine(&def, &maxImportBuffer[charIndex], maxImportSize - charIndex))
                {
                    defList[nextAvailableDefSlot++] = def;
                    charIndex += def.size - 1;
                }
                
                if (IsStringStruct(&st, &maxImportBuffer[charIndex], maxImportSize - charIndex))
                {
                    structList[nextAvailableStructSlot++] = st;
                    charIndex += st.size - 1;
                }
                
                if (IsStringFunction(&func, &maxImportBuffer[charIndex], maxImportSize - charIndex, (charIndex == 0)))
                {
                    funcList[nextAvailableFuncSlot++] = func;
                    charIndex += func.size - 1;
                }
            }
            
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
                
                FilterImports(inBuffer, inFileSize, &defList, nextAvailableDefSlot, &structList, nextAvailableStructSlot, &funcList, nextAvailableFuncSlot);
                
                b32 processing = true;
                b32 importFound = false;
                while (processing)
                {
                    for (s32 i = 0; i < nextAvailableDefSlot; ++i)
                    {
                        if (defList[i].foundInFile)
                        {
                            importFound = FilterImports(defList[i].bodyString, defList[i].bodySize, &defList, nextAvailableDefSlot, &structList, nextAvailableStructSlot, &funcList, nextAvailableFuncSlot);
                        }
                    }
                    
                    for (s32 i = 0; i < nextAvailableStructSlot; ++i)
                    {
                        if (structList[i].foundInFile)
                        {
                            importFound = FilterImports(structList[i].contentsString, structList[i].contentsSize, &defList, nextAvailableDefSlot, &structList, nextAvailableStructSlot, &funcList, nextAvailableFuncSlot);
                        }
                    }
                    
                    for (s32 i = 0; i < nextAvailableFuncSlot; ++i)
                    {
                        if (funcList[i].foundInFile)
                        {
                            importFound = FilterImports(funcList[i].paramsString, funcList[i].paramsSize + funcList[i].bodySize, &defList, nextAvailableDefSlot, &structList, nextAvailableStructSlot, &funcList, nextAvailableFuncSlot);
                        }
                    }
                    
                    if (!importFound)
                    {
                        processing = false;
                    }
                }
                CopyChars(outBuffer, inBuffer, includesSize);
                
                s32 runningSize = 0;
                CopyChars(outBuffer + includesSize, importHeaderComment, StringLength(importHeaderComment));
                runningSize += StringLength(importHeaderComment);
                CopyChars(outBuffer + includesSize + runningSize, "\n", 1);
                runningSize++;
                
                for (s32 i = 0; i < nextAvailableDefSlot; ++i)
                {
                    if (defList[i].foundInFile)
                    {
                        CopyChars(outBuffer + includesSize + runningSize, defList[i].defineString, defList[i].size);
                        runningSize += defList[i].size - 1;
                        CopyChars(outBuffer + includesSize + runningSize, "\n", 1);
                        runningSize++;
                    }
                }
                
                for (s32 i = 0; i < nextAvailableStructSlot; ++i)
                {
                    if (structList[i].foundInFile)
                    {
                        CopyChars(outBuffer + includesSize + runningSize, structList[i].structString, structList[i].size);
                        runningSize += structList[i].size - 1;
                        CopyChars(outBuffer + includesSize + runningSize, "\n\n", 2);
                        runningSize += 2;
                    }
                }
                
                for (s32 i = 0; i < nextAvailableFuncSlot; ++i)
                {
                    if (funcList[i].foundInFile)
                    {
                        CopyChars(outBuffer + includesSize + runningSize, funcList[i].typeString, funcList[i].size);
                        runningSize += funcList[i].size - 1;
                        CopyChars(outBuffer + includesSize + runningSize, "\n\n", 2);
                        runningSize += 2;
                    }
                }
                
                CopyChars(outBuffer + includesSize + runningSize, importFooterComment, StringLength(importFooterComment));
                runningSize += StringLength(importFooterComment);
                
                CopyChars(outBuffer + includesSize + runningSize, inBuffer + includesSize, inFileSize - includesSize);
                free(inBuffer);
                free(funcList);
                free(structList);
                free(defList);
                
                outBufferSize = runningSize + inFileSize;
                 outBuffer = (char *)realloc(outBuffer, outBufferSize);
                
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