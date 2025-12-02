#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#define SPV_ENABLE_UTILITY_CODE
#include "spirv.h"
#include "File.hpp"

bool match32( char * bytes, int index, int32_t value )
{
    bool result = true;
    for( int i = 0; i < 4; i++ )
    {
        char byte = (value >> (i*8)) & 0xff;
        if( bytes[ index + i ] != byte )
        {
            result = false;
        }
    }

    return result;
}

struct Reader {
    char * bytes;
    int index;
    int count;
    bool isOK;
};

Reader reader_from_file( File::file_content f )
{
    Reader r = {};
    r.bytes = f.bytes;
    r.index = 0;
    r.count = f.count;
    r.isOK = true;
    return r;
}

int8_t reader_read8( Reader *r )
{
    if( !r->isOK ) return 0;
    if( r->index + 1 >= r->count )
    {
        r->isOK = false;
        return 0;
    }

    char value = r->bytes[ r->index ];
    r->index += 1;
    return value;
}

int32_t reader_read16( Reader * r )
{
    uint8_t a = reader_read8( r );
    uint8_t b = reader_read8( r );
    uint32_t value = a | (b << 8);
    return value;
}

int32_t reader_read32( Reader * r )
{
    uint8_t a = reader_read8( r );
    uint8_t b = reader_read8( r );
    uint8_t c = reader_read8( r );
    uint8_t d = reader_read8( r );
    uint32_t value = a | (b << 8) | (c<<16) | (d<<24);
    return value;
}

void find_integer( Reader r, int startIndex, int32_t wanted, char * wanted_as_symbol )
{
    // we can emulate peeking because we passed struct by copy

    printf( "searching for '%d' (%s):\n", wanted, wanted_as_symbol );
    for(int i = startIndex; i < r.count; i++ )
    {
        r.index = i;
        r.isOK = true;
        if( reader_read8( &r ) == wanted )
        {
            printf( "at %d 0x%x [8]\n", i, i );
        }
        if( reader_read16( &r ) == wanted )
        {
            printf( "at %d 0x%x [16]\n", i, i );
        }
        if( reader_read32( &r ) == wanted )
        {
            printf( "at %d 0x%x [32]\n", i, i );
        }
    }
}

bool string_equals( char * a, char * b )
{
    int i = 0;
    while( true )
    {
        if( !a )
        {
            if( !b )
            {
                return true;
            }
            return false;
        }

        if( !a[ i ] )
        {
            if( !b[ i ] )
            {
                return true;
            }
            return false;
        }

        i++;
    }
}

bool find_16( Reader r, int startIndex, int16_t wanted, int * findIndex )
{
    r.isOK = true; // just in case 

    for( int i = startIndex; i < r.count; i++ )
    {
        r.index = startIndex;
        int16_t value = reader_read16( &r );

        if( !r.isOK ) // invalid read at the end of content
        {
            return false;
        }

        if( value == wanted )
        {
            *findIndex = i;
            return true;
        }
    }

    return false;
}

int main( int argc, char **argv )
{
    printf( "Hello" );
    
    char* filePath = "frag.spv";
    File::file_content f = File::Read( filePath, NULL, 0 );
    assert(f.isOK);

    Reader r = reader_from_file( f );

    int32_t magic = reader_read32( &r );
    assert( magic == 0x07230203 );

    int32_t version = reader_read32( &r );

    int32_t generator = reader_read32( &r );
    int32_t bound = reader_read8( &r );
    int32_t schema = reader_read8( &r );

    //SpvOpCapability()

    assert( r.isOK );

    printf( "version = 0x%x\n", version );
    printf( "generator = 0x%x\n", generator );
    printf( "bound = %d\n", bound );
    printf( "schema = %d\n", schema );

    printf( "at 0x%x / %d\n", r.index, r.index );
    find_integer( r, r.index, SpvOpCapability, "OpCapability" );
    find_integer( r, r.index, SpvOpExtInstImport, "OpExtInstImport" );
    find_integer( r, r.index, SpvOpCapability, "OpCapability");// Shader
    find_integer( r, r.index, SpvOpExtInstImport, "OpExtInstImport");// "GLSL.std.450"
    find_integer( r, r.index, SpvOpMemoryModel, "OpMemoryModel");// Logical GLSL450
    find_integer( r, r.index, SpvOpEntryPoint, "OpEntryPoint");// Fragment %main "main" %outColor %fragColor
    find_integer( r, r.index, SpvOpExecutionMode, "OpExecutionMode");// %main OriginUpperLeft
    find_integer( r, r.index, SpvOpSource, "OpSource");// GLSL 450
    find_integer( r, r.index, SpvOpSourceExtension, "OpSourceExtension");// "GL_GOOGLE_cpp_style_line_directive"
    find_integer( r, r.index, SpvOpSourceExtension, "OpSourceExtension");// "GL_GOOGLE_include_directive"
    find_integer( r, r.index, SpvOpName, "OpName");// %main "main"
    find_integer( r, r.index, SpvOpName, "OpName");// %outColor "outColor"
    find_integer( r, r.index, SpvOpName, "OpName");// %fragColor "fragColor"
    find_integer( r, r.index, SpvOpDecorate, "OpDecorate");// %outColor Location 0
    find_integer( r, r.index, SpvOpDecorate, "OpDecorate");// %fragColor Location 1

    std::vector<int32_t> codeToFind;
    codeToFind.push_back((int32_t) SpvOpCapability); //  "OpCapability" );
    codeToFind.push_back((int32_t) SpvOpExtInstImport); //  "OpExtInstImport" );
    codeToFind.push_back((int32_t) SpvOpCapability); //  "OpCapability");// Shader
    codeToFind.push_back((int32_t) SpvOpExtInstImport); //  "OpExtInstImport");// "GLSL.std.450"
    codeToFind.push_back((int32_t) SpvOpMemoryModel); //  "OpMemoryModel");// Logical GLSL450
    codeToFind.push_back((int32_t) SpvOpEntryPoint); //  "OpEntryPoint");// Fragment %main "main" %outColor %fragColor
    codeToFind.push_back((int32_t) SpvOpExecutionMode); //  "OpExecutionMode");// %main OriginUpperLeft
    codeToFind.push_back((int32_t) SpvOpSource); //  "OpSource");// GLSL 450
    codeToFind.push_back((int32_t) SpvOpSourceExtension); //  "OpSourceExtension");// "GL_GOOGLE_cpp_style_line_directive"
    codeToFind.push_back((int32_t) SpvOpSourceExtension); //  "OpSourceExtension");// "GL_GOOGLE_include_directive"
    codeToFind.push_back((int32_t) SpvOpName); //  "OpName");// %main "main"
    codeToFind.push_back((int32_t) SpvOpName); //  "OpName");// %outColor "outColor"
    codeToFind.push_back((int32_t) SpvOpName); //  "OpName");// %fragColor "fragColor"
    codeToFind.push_back((int32_t) SpvOpDecorate); //  "OpDecorate");// %outColor Location 0
    codeToFind.push_back((int32_t) SpvOpDecorate); //  "OpDecorate");// %fragColor Location 1

    printf("Can I find the code?\n");
    printf("--------------------\n");
    {
        std::vector<int32_t> foundOffsets;

        int startIndex = r.index;

        for(int i = 0; i < codeToFind.size(); i++ )
        {
            int32_t op = codeToFind[ i ];

            Reader rr = r;
            rr.isOK = true;

            // try other alignment if first one doesn't match

            int findIndex = 0;
            if( find_16( rr, startIndex, op, &findIndex ))
            {
                foundOffsets.push_back(findIndex);
                startIndex = findIndex + sizeof( uint16_t );
            }
            else if( find_16( rr, startIndex + 1, op, &findIndex ))
            {
                foundOffsets.push_back(findIndex);
                startIndex = findIndex + sizeof( uint16_t );
            }
            else
            {
                break;
            }
        }

        if(codeToFind.size() == foundOffsets.size())
        {
            printf("FOUND THE CODE!!!\n");
            for(auto i: foundOffsets)
            {
                printf("%d",i);
            }

        }
    }

    printf("Search for negative space\n");
    printf("-----------------\n");
    printf("[ ] code isn't there 16 or 32bit; [+] valid instruction ");
    {
        int startIndex = r.index;

        struct ValidInfo {
            SpvOp op;
            int bits;
            int index;
            char *name;
        };
        std::vector<ValidInfo> validInstructions;

        printf("Visually:\n");
        for( int i = startIndex; i < r.count; i++ )
        {
            Reader rr = r;
            rr.isOK = true;
            rr.index = i;
            SpvOp op16 = (SpvOp) reader_read16( &rr );
            SpvOp op32 = (SpvOp) reader_read16( &rr );
            char * str16 = (char*) SpvOpToString( op16 );
            char * str32 = (char*) SpvOpToString( op32 );
            bool isMiss16 = string_equals( str16, "Unknown" ) || string_equals( str16, "OpNop" );
            bool isMiss32 = string_equals( str32, "Unknown" ) || string_equals( str32, "OpNop" );
            if(!isMiss16)
            {
                ValidInfo info = {};
                info.op = op16;
                info.index = i;
                info.bits = 16;
                info.name = (char*) SpvOpToString(op16);
                validInstructions.push_back(info);
            }
            if(!isMiss32)
            {
                ValidInfo info = {};
                info.op = op32;
                info.index = i;
                info.bits = 32;
                info.name = (char*) SpvOpToString(op32);
                validInstructions.push_back(info);
            }
            printf("%c", ( isMiss16 && isMiss32 )? ' ': '+');
        }
        printf("List of valid instructions (too bad we don't have :hover in text mode)\n");
        printf("Lines go from 1\n");
        int previouslyPrintedIndex = -1;
        for(int i = 0; i < validInstructions.size(); i++ )
        {
            ValidInfo info = validInstructions[ i ];
            if(info.index == previouslyPrintedIndex) {
                printf(", ");
            } else if( i > 0 ) {
                printf("\n");
                printf( "%d: ", info.index + 1); // lines go from 1 as in text editor
            } else if( i == 0 ) {
                printf( "%d: ", info.index + 1); // lines go from 1 as in text editor
            }
            printf( "%s (%d)", info.name, info.bits);
        }
        printf("\n");
    }
    printf("\n");

    // 32 bit, offset 16 (29%)
    // 16 bit, offset 14 (31%) 16 (31%)
    //
    int currentIndex = r.index;

    for(int offset = 0; offset < 4; offset++ )
    {
        int startIndex = currentIndex + offset;
        printf( "\nprinting 16bit from %d 0x%x\n", startIndex, startIndex );
        printf(    "--------------------\n");

        int misses = 0;
        // int count = (r.count - startIndex) / 2;

        r.isOK = true;
        r.index = startIndex;
        int count = 0;
        {
            while ( r.isOK )
            {
                SpvOp op = (SpvOp) reader_read16( &r );
                char * str = (char*) SpvOpToString( op );
                bool isMiss = string_equals( str, "Unknown" ) || string_equals( str, "OpNop" );
                if( isMiss )
                {
                    misses++;
                }
                count++;
                printf( " %s", isMiss? "-" : str );
            }
        }
        printf( "\n===== %f %% of correct instructions \n", (float) (count-misses) / count * 100.0f );
    }

    for(int offset = 0; offset < 4; offset++ )
    {
        int startIndex = currentIndex + offset;
        printf( "\nprinting 32bit from %d 0x%x\n", startIndex, startIndex );
        printf(    "--------------------\n");

        int misses = 0;
        // int count = (r.count - startIndex) / 4;

        r.isOK = true;
        r.index = startIndex;
        int count = 0;
        {
            while ( r.isOK )
            {
                SpvOp op = (SpvOp) reader_read32( &r );
                char * str = (char*) SpvOpToString( op );
                bool isMiss = string_equals( str, "Unknown" ) || string_equals( str, "OpNop" );
                if( isMiss )
                {
                    misses++;
                }
                count++;
                printf( " %s", isMiss? "-" : str );
            }
        }
        printf( "===== %f %% of correct instructions \n", (float) (count-misses) / count * 100.0f );
    }

    // SpvOp op = (SpvOp) 69;
    // printf("%s",SpvOpToString(op));
    return 0;
}

/*
; SPIR-V
; Version: 1.0
; Generator: Google Shaderc over Glslang; 11
; Bound: 27
; Schema: 0
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %main "main" %outColor %fragColor
               OpExecutionMode %main OriginUpperLeft
               OpSource GLSL 450
               OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
               OpSourceExtension "GL_GOOGLE_include_directive"
               OpName %main "main"
               OpName %outColor "outColor"
               OpName %fragColor "fragColor"
               OpDecorate %outColor Location 0
               OpDecorate %fragColor Location 1
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
      %float = OpTypeFloat 32
    %v4float = OpTypeVector %float 4
%_ptr_Output_v4float = OpTypePointer Output %v4float
   %outColor = OpVariable %_ptr_Output_v4float Output
%_ptr_Input_v4float = OpTypePointer Input %v4float
  %fragColor = OpVariable %_ptr_Input_v4float Input
       %uint = OpTypeInt 32 0
     %uint_0 = OpConstant %uint 0
%_ptr_Input_float = OpTypePointer Input %float
     %uint_1 = OpConstant %uint 1
     %uint_2 = OpConstant %uint 2
     %uint_3 = OpConstant %uint 3
       %main = OpFunction %void None %3
          %5 = OpLabel
         %15 = OpAccessChain %_ptr_Input_float %fragColor %uint_0
         %16 = OpLoad %float %15
         %18 = OpAccessChain %_ptr_Input_float %fragColor %uint_1
         %19 = OpLoad %float %18
         %21 = OpAccessChain %_ptr_Input_float %fragColor %uint_2
         %22 = OpLoad %float %21
         %24 = OpAccessChain %_ptr_Input_float %fragColor %uint_3
         %25 = OpLoad %float %24
         %26 = OpCompositeConstruct %v4float %16 %19 %22 %25
               OpStore %outColor %26
               OpReturn
               OpFunctionEnd
*/
