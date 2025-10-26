@if not defined CL_IS_DEFINED (
    @rem https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-160
    @rem @call "c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    @rem @set CL_IS_DEFINED=1
    call "c:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    set CL_IS_DEFINED=1
)

@rem /DEBUG:fastlink is not supported by remedybg
@rem /Z7 full debuggin information
@rem /Zo debug info for optimized code
@rem /Zi debug, separate .pdb files (But why? remedybg? Because /ZI has edit and continue)
@rem /O2 optimize
@rem /d1reportTime ... https://aras-p.info/blog/2019/01/21/Another-cool-MSVC-flag-d1reportTime/
@rem /d2cgsummary ... https://aras-p.info/blog/2017/10/23/Best-unknown-MSVC-flag-d2cgsummary/
@rem useful build time link ... http://coding-scars.com/investigating-cpp-compile-times-1/
@rem /E ... expand preprocessor
@rem /Fo ... folder with .obj
@rem /Fd ... folder with vs104.pdb "Program Database"
set debug=/DEBUG:FULL /Z7 /Zo /Zi
set paths=/Fo.\exe\ /Fd.\exe\
cl src/main.cpp %debug% %paths% /link user32.lib opengl32.lib gdi32.lib Winmm.lib /out:exe\main.exe

@rem Might work too???
@rem
@rem pushd exe
@rem cl ../src/main.cpp %debug% /link user32.lib opengl32.lib gdi32.lib Winmm.lib
@rem popd