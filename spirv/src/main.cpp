#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#define SPV_ENABLE_UTILITY_CODE
#include "spirv.h"
#include "File.hpp"

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

bool is_printable( char byte )
{
    if( byte >= 'a' && byte <= 'z' ) return true;
    if( byte >= 'A' && byte <= 'Z' ) return true;
    if( byte >= '0' && byte <= '9' ) return true;
    if( byte == '_' || byte == '-' || byte == '.' ) return true;
    return false;
}

void parse_header( Reader * r, int *maximalId )
{
    printf( "Parsing header\n");

    int32_t magic = reader_read32( r );
    assert( magic == 0x07230203 );

    int32_t version = reader_read32( r );

    int32_t generator = reader_read32( r );
    int32_t bound = reader_read32( r );
    int32_t schema = reader_read32( r ); 

    //SpvOpCapability()

    assert( r->isOK );

    int major = (version >> 16)&0xff;
    int minor = (version >> 8)&0xff;
    printf( "version = %d.%d 0x%x\n", major, minor, version );
    printf( "generator = 0x%x\n", generator );
    printf( "bound = %d (maximal id)\n", bound );
    printf( "schema = %d\n", schema );

    printf( "at 0x%x / %d\n", r->index, r->index );

    printf( "\n\n" );

    assert( r->isOK );

    assert( bound > 0 );
    *maximalId = bound;
}

char * reader_read_word_aligned_string( Reader *r )
{
    if( !r->isOK ) return NULL;

    char * str = r->bytes + r-> index;

    // skip string to nearest word-aligned
    while( true )
    {
        assert( r->index < r->count ); // invalid string
        if( r->bytes[ r->index ] == 0 )
        {
            r->index++; // skip end of string
            while( r->index % 4 != 0 )
            {
                r->index++;
            }

            goto done;
        }
        else
        {
            r->index++;
        }     
    }

done:

    assert( r->index % 4 == 0 );

    return str;
}

struct EntryPoint {
    SpvExecutionModel execMode;
    int32_t entryPointId;
    char * functionName;
    std::vector<int> ids;
};

struct Variable {
    int id;
    char * name;
    SpvOp type;
};

Variable * get_or_add_to_vector( std::vector<Variable> &vec, int id )
{
    for( auto v : vec )
    {
        if( v.id == id )
        {
            return &v;
        }
    }

    Variable empty = {};
    vec.push_back( empty );
    Variable * result = &( vec[ vec.size() - 1 ] );
    return result;
}

int main( int argc, char **argv )
{
    char* filePath = "frag.spv";
    File::file_content f = File::Read( filePath, NULL, 0 );
    assert(f.isOK);
    Reader r = reader_from_file( f );

    int maximalId = 0;
    parse_header( &r, &maximalId );



    std::vector<EntryPoint> entryPoints;
    std::vector<int> functions;
    std::vector<int> ids;
    std::vector<Variable> variables;

    while( true )
    {
        if(!r.isOK) break;

        assert( r.index % 4 == 0 );

        SpvOp op = (SpvOp) reader_read16( &r );
        int countWords = reader_read16( &r );
        bool hasResult = false;
        bool hasType = false;

        int nextIndex = r.index + ( countWords - 1 ) * 4; // words to skip
        SpvHasResultAndType(op,&hasResult,&hasType);

        //if(hasResult)
        printf("%s %d %s %s\n", SpvOpToString(op),countWords,hasResult? " result":"",hasType?" type":"");


        if( op == SpvOpLoad )
        {

        }
        else if( op == SpvOpStore )
        {

        }
        else if( op == SpvOpTypeFunction )
        {

        }
        else if( op == SpvOpName )
        {
            int id = reader_read32( &r );
            Variable * var = get_or_add_to_vector( variables, id );
            var->id = id;
            var->name = reader_read_word_aligned_string( &r );

            printf( " - %d -> %s \n", var->id, var->name );
        }
        else if( op == SpvOpTypePointer )
        {

        }
        else if( op == SpvOpVariable )
        {

            SpvOp resultType = (SpvOp) reader_read32( &r );
            int32_t id = reader_read32( &r );
            SpvStorageClass storageClass = (SpvStorageClass) reader_read32( &r );

            char * idName = "?";
            for( auto v : variables ) { if( v.id == id ){ idName = v.name; }}

            printf( " - id = %d (%s) \n", id, idName );
            printf( " - type = %s \n", SpvOpToString( resultType ));
            printf( " - storage = %s \n", SpvStorageClassToString( storageClass ));

            // assert( resultType == SpvOpTypePointer ); ???
            assert( storageClass != SpvStorageClassGeneric );
            

            //printf(" - entry")
            printf("[");
            for(int i = 0; i < countWords * 4; i++)
            {
                char byte = r.bytes[r.index+i];
                if(is_printable( byte ))
                {
                    printf("%c", byte);
                }
                else
                {
                    printf("[%d]", byte);
                }
            }
            printf("]\n");
        }
        else if( op == SpvOpEntryPoint )
        {
            EntryPoint entryPoint = {};
            entryPoint.execMode = (SpvExecutionModel) reader_read32( &r );
            entryPoint.entryPointId = reader_read32( &r );
            entryPoint.functionName = reader_read_word_aligned_string( &r );


            assert(entryPoint.execMode == SpvExecutionModelFragment); // @test

            while( r.index < nextIndex )
            {
                int32_t id = reader_read32( &r );
                assert( id > 0 );
                assert( id < maximalId );
                entryPoint.ids.push_back( id );
            }

            entryPoints.push_back(entryPoint);
        }

        r.index = nextIndex;
    }

    assert(entryPoints.size() == 1);

    printf( "ENTRY POINTS:\n" );
    for( auto entryPoint : entryPoints )
    {
        printf(" - exec mode= %s\n", SpvExecutionModelToString(entryPoint.execMode));
        printf(" - entry function id = %d\n", entryPoint.entryPointId );
        printf(" - function name = '%s'\n", entryPoint.functionName );
        printf( " - ids: ");
        for( auto id : entryPoint.ids )
        {
            char * idName = "?";
            for( auto v : variables ) { if( v.id == id ){ idName = v.name; }}

            printf( " %s (%d)", idName, id );
        }
        printf( "\n" );
    }

    // type, result, operand


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
