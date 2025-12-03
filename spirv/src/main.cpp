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
    for( int i = startIndex; i < r.count; i++ )
    {
        r.isOK = true;
        r.index = i;
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


void parse_header( Reader * r )
{
    printf( "Parsing header\n");

    int32_t magic = reader_read32( r );
    assert( magic == 0x07230203 );

    int32_t version = reader_read32( r );

    int32_t generator = reader_read32( r );
    int32_t bound = reader_read8( r );
    int32_t schema = reader_read8( r );

    //SpvOpCapability()

    assert( r->isOK );

    printf( "version = 0x%x\n", version );
    printf( "generator = 0x%x\n", generator );
    printf( "bound = %d\n", bound );
    printf( "schema = %d\n", schema );

    printf( "at 0x%x / %d\n", r->index, r->index );

    printf( "\n\n" );
}

void find_all_places_where_instructions_are( Reader r )
{
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
}

struct SpirvToken {
    SpvOp op;
    int bits;
    int index;
    char *name;
};


void print_spirv_token_vector( std::vector<SpirvToken> * v )
{

}

bool find_code_sequence( Reader r, std::vector<SpirvToken> * codeToFind, std::vector<SpirvToken> * foundCode /* modified */ )
{
    printf("Can I find the code?\n");
    {
        int startIndex = r.index;

        printf("starting at index %d\n", startIndex);
        printf("--------------------\n");

        std::vector<int32_t> foundOffsets;


        for(int i = 0; i < codeToFind->size(); i++ )
        {
            int32_t op = (*codeToFind)[ i ].op;

            // op is 16 bit
            assert( op < 65000 ); 

            Reader rr = r;
            rr.isOK = true;

            // try other alignment if first one doesn't match

            int findIndex = 0;
            bool foundSomething = false;

            for(int offset = 0; offset < sizeof(int16_t); offset++)
            {
                if( find_16( rr, startIndex + offset, op, &findIndex ))
                {
                    foundOffsets.push_back(findIndex);
                    startIndex = findIndex + sizeof( uint16_t );

                    SpirvToken found = {};
                    found.op = (SpvOp) op;
                    found.bits = 16;
                    found.name = (char*) SpvOpToString( (SpvOp) op );
                    found.index = findIndex;
                    foundCode->push_back( found );

                    foundSomething = true;

                    if( foundSomething ) // find just first one
                    {
                        break;
                    }
                }
            }

            if( foundSomething ) // find just first one
            {
                continue;
            }

            /*
            for(int offset = 0; offset < sizeof(int32_t); offset++)
            {
                if( find_32( rr, startIndex + offset, op, &findIndex ))
                {
                    foundOffsets.push_back(findIndex);
                    startIndex = findIndex + sizeof( uint32_t );

                    SpirvToken found = {};
                    found.op = (SpvOp) op;
                    found.bits = 32;
                    found.name = (char*) SpvOpToString( (SpvOp) op );
                    found.index = findIndex;
                    foundCode.push_back( found );

                    foundSomething = true;

                    if( foundSomething ) // find just first one
                    {
                        break;
                    }
                }
            }
            */

            // find whole code or do not continue
            if( !foundSomething )
            {
                break;
            }
        }

        //if(codeToFind.size() == foundOffsets.size())
        {
            //printf("FOUND THE CODE!!!\n");
            for(int i = 0; i < foundCode->size(); i++ )
            {
                SpirvToken found = (*foundCode)[ i ];
                printf("%s // line %d\n", found.name, found.index + 1);
            }
        }
    }

    return codeToFind->size() == foundCode->size();
}

void print_instructions_from_different_offsets( Reader r )
{
    printf( "\n\n");
    printf( "Printing instructions at different offsets\n");
    printf( "------------------------------------------\n");
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
}

void find_valid_instructions( Reader r, std::vector<SpirvToken> * validInstructions )
{
    printf("Search for negative space\n");
    printf("-----------------\n");
    printf("[ ] code isn't there 16 or 32bit; [+] valid instruction ");
    {
        int startIndex = r.index;


        printf("Visually:\n");
        for( int i = startIndex; i < r.count; i++ )
        {
            Reader rr = r;
            rr.isOK = true;
            rr.index = i;
            SpvOp op8 = (SpvOp) reader_read8( &rr );
            SpvOp op16 = (SpvOp) reader_read16( &rr );
            SpvOp op32 = (SpvOp) reader_read16( &rr );
            char * str8 = (char*) SpvOpToString( op8 );
            char * str16 = (char*) SpvOpToString( op16 );
            char * str32 = (char*) SpvOpToString( op32 );
            bool isMiss8 = string_equals( str8, "Unknown" ) || string_equals( str8, "OpNop" );
            bool isMiss16 = string_equals( str16, "Unknown" ) || string_equals( str16, "OpNop" );
            bool isMiss32 = string_equals( str32, "Unknown" ) || string_equals( str32, "OpNop" );

            // No need to add same instruction 2 or 3 times

            // this might be too big instruction size??? THere will be zeroes anyway so it is ok
            if(!isMiss32)
            {
                SpirvToken info = {};
                info.op = op32;
                info.index = i;
                info.bits = 32;
                info.name = (char*) SpvOpToString(op32);
                validInstructions->push_back(info);
            }
            else if(!isMiss16)
            {
                SpirvToken info = {};
                info.op = op16;
                info.index = i;
                info.bits = 16;
                info.name = (char*) SpvOpToString(op16);
                validInstructions->push_back(info);
            }
            // this probably isn't the size we want, add it just in case
            else if(!isMiss8)
            {
                SpirvToken info = {};
                info.op = op8;
                info.index = i;
                info.bits = 8;
                info.name = (char*) SpvOpToString(op8);
                validInstructions->push_back(info);
            }

            printf("%c", ( isMiss8 && isMiss16 && isMiss32 )? ' ': '+');
        }

        printf("List of valid instructions (too bad we don't have :hover in text mode)\n");
        printf("Lines go from 1\n");
        int previouslyPrintedIndex = -1;
        char * INDENT = "   ";
        for(int i = 0; i < validInstructions->size(); i++ )
        {
            SpirvToken info = (*validInstructions)[ i ];
            if(info.index == previouslyPrintedIndex) {
                printf(", ");
            } else if( i > 0 ) {
                printf("\n");
                printf( INDENT );
                printf( "%d: ", info.index + 1); // lines go from 1 as in text editor
            } else if( i == 0 ) {
                printf( INDENT );
                printf( "%d: ", info.index + 1); // lines go from 1 as in text editor
            }
            printf( "%s (%d)", info.name, info.bits);
            previouslyPrintedIndex = info.index;
        }
        printf("\n");
    }
    printf("\n");    
}

void add_spirv_token( std::vector<SpirvToken> * v, SpvOp op )
{
    SpirvToken t = {};
    t.op = op;
    t.name = (char*) SpvOpToString( op );
    t.index = v->size();
    v->push_back(t);
}

void search_for_code_sequence_inside_another( std::vector<SpirvToken> & codeToFind, std::vector<SpirvToken> & validInstructions )
{
    printf( "\n\n" );
    printf( "Searching code inside of valid instructions\n" );
    printf( "-------------------------------------------\n" );
    int searchIndex = 0;

    std::string dottedOutput;

    for( int codeIndex = 0; codeIndex < codeToFind.size(); codeIndex++ )
    {
        SpvOp op = codeToFind[ codeIndex ].op;

        printf( "%s ... ", codeToFind[ codeIndex ].name );
        int originalSearchIndex = searchIndex;
        std::string out2;
        while( true )
        {
            if( searchIndex >= validInstructions.size())
            {

                dottedOutput += " ?";

                searchIndex = originalSearchIndex + 1;
                printf( "NOT FOUND -> skipping to %d\n", searchIndex );
                break;
            }

            if( op == validInstructions[ searchIndex ].op)
            {
                /*
                SpirvToken f = {};
                f.op = op;
                f.index = searchIndex;
                f.name = codeToFind[ codeIndex ].name;
                found.push_back( f );
                */

                out2 += " ";
                out2 += codeToFind[codeIndex].name;
                dottedOutput += out2;

                printf( "FOUND at %d\n", searchIndex );
                searchIndex++;
                break;
            }
            else
            {
                out2 += " -";
                searchIndex++;
            }
        }
    }

    printf( "\n" );
    printf( "visually: [-] is skipped [?] is for one not found\n");
    printf( "%s\n", dottedOutput.c_str());
}

int main( int argc, char **argv )
{
    char* filePath = "frag.spv";
    File::file_content f = File::Read( filePath, NULL, 0 );
    assert(f.isOK);
    Reader r = reader_from_file( f );

    parse_header( &r );

    find_all_places_where_instructions_are( r );

    std::vector<SpirvToken> codeToFind;
    add_spirv_token(&codeToFind, SpvOpCapability); //  "OpCapability" );
    add_spirv_token(&codeToFind, SpvOpExtInstImport); //  "OpExtInstImport" );
    add_spirv_token(&codeToFind, SpvOpCapability); //  "OpCapability");// Shader
    add_spirv_token(&codeToFind, SpvOpExtInstImport); //  "OpExtInstImport");// "GLSL.std.450"
    add_spirv_token(&codeToFind, SpvOpMemoryModel); //  "OpMemoryModel");// Logical GLSL450
    add_spirv_token(&codeToFind, SpvOpEntryPoint); //  "OpEntryPoint");// Fragment %main "main" %outColor %fragColor
    add_spirv_token(&codeToFind, SpvOpExecutionMode); //  "OpExecutionMode");// %main OriginUpperLeft
    add_spirv_token(&codeToFind, SpvOpSource); //  "OpSource");// GLSL 450
    add_spirv_token(&codeToFind, SpvOpSourceExtension); //  "OpSourceExtension");// "GL_GOOGLE_cpp_style_line_directive"
    add_spirv_token(&codeToFind, SpvOpSourceExtension); //  "OpSourceExtension");// "GL_GOOGLE_include_directive"
    add_spirv_token(&codeToFind, SpvOpName); //  "OpName");// %main "main"
    add_spirv_token(&codeToFind, SpvOpName); //  "OpName");// %outColor "outColor"
    add_spirv_token(&codeToFind, SpvOpName); //  "OpName");// %fragColor "fragColor"
    add_spirv_token(&codeToFind, SpvOpDecorate); //  "OpDecorate");// %outColor Location 0
    add_spirv_token(&codeToFind, SpvOpDecorate); //  "OpDecorate");// %fragColor Location 1



    std::vector<SpirvToken> foundCode;

    find_code_sequence( r, &codeToFind, &foundCode );


    std::vector<SpirvToken> validInstructions;

    find_valid_instructions( r, &validInstructions );

    search_for_code_sequence_inside_another( codeToFind, validInstructions );

    {
        std::vector<SpirvToken> v;
        add_spirv_token(&v, SpvOpTypeVoid); //        %void = OpTypeVoid
        add_spirv_token(&v, SpvOpTypeFunction); //           %3 = OpTypeFunction %void
        add_spirv_token(&v, SpvOpTypeFloat); //       %float = OpTypeFloat 32
        add_spirv_token(&v, SpvOpTypeVector); //     %v4float = OpTypeVector %float 4
        add_spirv_token(&v, SpvOpTypePointer); // %_ptr_Output_v4float = OpTypePointer Output %v4float
        add_spirv_token(&v, SpvOpVariable); //    %outColor = OpVariable %_ptr_Output_v4float Output
        add_spirv_token(&v, SpvOpTypePointer); // %_ptr_Input_v4float = OpTypePointer Input %v4float
        add_spirv_token(&v, SpvOpVariable); //   %fragColor = OpVariable %_ptr_Input_v4float Input
        add_spirv_token(&v, SpvOpTypeInt); //        %uint = OpTypeInt 32 0
        add_spirv_token(&v, SpvOpConstant); //      %uint_0 = OpConstant %uint 0
        add_spirv_token(&v, SpvOpTypePointer); // %_ptr_Input_float = OpTypePointer Input %float
        add_spirv_token(&v, SpvOpConstant); //      %uint_1 = OpConstant %uint 1
        add_spirv_token(&v, SpvOpConstant); //      %uint_2 = OpConstant %uint 2
        add_spirv_token(&v, SpvOpConstant); //      %uint_3 = OpConstant %uint 3
        add_spirv_token(&v, SpvOpFunction); //        %main = OpFunction %void None %3
        add_spirv_token(&v, SpvOpLabel); //           %5 = OpLabel
        add_spirv_token(&v, SpvOpAccessChain); //          %15 = OpAccessChain %_ptr_Input_float %fragColor %uint_0
        add_spirv_token(&v, SpvOpLoad); //          %16 = OpLoad %float %15
        add_spirv_token(&v, SpvOpAccessChain); //          %18 = OpAccessChain %_ptr_Input_float %fragColor %uint_1
        add_spirv_token(&v, SpvOpLoad); //          %19 = OpLoad %float %18
        add_spirv_token(&v, SpvOpAccessChain); //          %21 = OpAccessChain %_ptr_Input_float %fragColor %uint_2
        add_spirv_token(&v, SpvOpLoad); //          %22 = OpLoad %float %21
        add_spirv_token(&v, SpvOpAccessChain); //          %24 = OpAccessChain %_ptr_Input_float %fragColor %uint_3
        add_spirv_token(&v, SpvOpLoad); //          %25 = OpLoad %float %24
        add_spirv_token(&v, SpvOpCompositeConstruct); //          %26 = OpCompositeConstruct %v4float %16 %19 %22 %25
        add_spirv_token(&v, SpvOpStore);// %outColor %26
        add_spirv_token(&v, SpvOpReturn);//
        add_spirv_token(&v, SpvOpFunctionEnd);//


        search_for_code_sequence_inside_another( v, validInstructions );
    }

    printf( "\n\n" );
    printf( "these weren't found in sequence ...\n" );
    {
        std::vector<SpirvToken> v;
        add_spirv_token(&v, SpvOpLabel); //           %5 = OpLabel
        add_spirv_token(&v, SpvOpAccessChain); //          %15 = OpAccessChain %_ptr_Input_float %fragColor %uint_0
        add_spirv_token(&v, SpvOpLoad); //          %16 = OpLoad %float %15
        add_spirv_token(&v, SpvOpAccessChain); //          %18 = OpAccessChain %_ptr_Input_float %fragColor %uint_1
        add_spirv_token(&v, SpvOpLoad); //          %19 = OpLoad %float %18
        add_spirv_token(&v, SpvOpAccessChain); //          %21 = OpAccessChain %_ptr_Input_float %fragColor %uint_2
        add_spirv_token(&v, SpvOpLoad); //          %22 = OpLoad %float %21
        add_spirv_token(&v, SpvOpAccessChain); //          %24 = OpAccessChain %_ptr_Input_float %fragColor %uint_3
        add_spirv_token(&v, SpvOpLoad); //          %25 = OpLoad %float %24
        add_spirv_token(&v, SpvOpCompositeConstruct); //          %26 = OpCompositeConstruct %v4float %16 %19 %22 %25
        add_spirv_token(&v, SpvOpStore);// %outColor %26
        add_spirv_token(&v, SpvOpReturn);//
        add_spirv_token(&v, SpvOpFunctionEnd);//

        search_for_code_sequence_inside_another( v, validInstructions );
    }

    print_instructions_from_different_offsets( r );

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
