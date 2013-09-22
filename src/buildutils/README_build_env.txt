%% --------------------------------------------------------------  
%% (C)Copyright 2001,2007,                                         
%% International Business Machines Corporation,                    
%% Sony Computer Entertainment, Incorporated,                      
%% Toshiba Corporation,                                            
%%                                                                 
%% All Rights Reserved.                                            
%% --------------------------------------------------------------  
%% PROLOG END TAG zYx                                              

           SDK SAMPLES AND LIBRARY BUILD ENVIRONMENT OVERVIEW

The SDK Samples and Library package provides a build environment to assist
in the construction of fully functional makefiles. This document provides
an overview of the commonly used build environment features. For samples 
of using the build environment, see the various Makefiles found within the 
SDK. 


A. Build Environment Files

   The build environment consist of the following three (3) files. They
   are processed by the make utility in the order specified.

   make.header - Includes definitions required in advance to processing the
                 Makefile. This file includes make.env.

   make.env    - File used to specify the default preferred compilers and tools
                 to be used by make. These can be over-ridden by the definition
                 of a user environment variables or edits to this file.

                 HOST_COMPILER  - specifies the default compiler to be used 
                                  for code targeted to execute on the host 
                                  system.
                 PPU_COMPILER   - specifies the default compiler to be used
                                  for code targeted to execute on the PPE;
                                  either "gcc" or "xlc".
                 PPU32_COMPILER - specifies the default compiler to be used
                                  for 32-bit code targeted to execute on the
                                  PPE; either "gcc" or "xlc".
                 PPU64_COMPILER - specifies the default compiler to be used
                                  for 64-bit code targeted to execute on the
                                  PPE; either "gcc" or "xlc".
                 SPU_COMPILER   - specifies the default compiler to be used 
                                  for code targeted to run on a SPE, either 
                                  "gcc" or "xlc".
                 XLC_VERSION    - specifies the version of the default xlc 
                                  compiler.                             
                 SPU_TIMING     - specifies whether by default that a static 
                                  analysis timing file is created for all 
                                  generated assembly (.s) files.

   make.footer - File that specifies all the build rules to be applied. SDK 
                 users are not expected to modify this file, but can be used
                 to understand the default flags and build procedures.

B. Anatomy of a Typical Makefile

   All Makefiles have a common basic structure and may include the following
   entries in order:

   1. Specifies the subdirectories to visit before building the target
      objects in this directory. The subdirs are specified as:

        DIRS := <list of space separated subdirectories>

   2. Specifies the build targets and target processor. It should be noted
      that the build environment only accommodates one processor type for 
      each Makefile (directory). Building code for multiple processors or 
      requires a separate directory and Makefile.

      To specify the processor type for the target

        TARGET_PROCESSOR := "spu" | "ppu" | "ppu64" | "host"

      To specify a program or list of program targets to be built:

        PROGRAM  := <target program> 
        PROGRAMS := <list of space separated target programs>

      Alternatively, the program or programs and processor type for the target
      can be specified with a single definition.

        PROGRAM_<target> := <target program>
        PROGRAMS_<target>:= <list if space separated target programs>


      To specify the name of a library target (either shared or non-shared) 
      to be built, use one of the following:

        LIBRARY         := <library archive>
        SHARED_LIBRARY  := <shared library archive>
        
      To specify a SPU programs to be embeded into a PPU executable as a
      linked library, specify one of the following (depending upon whether
      the library is to be embedded in a 32-bit or 64-bit PPU executable,
      respectively):

        LIBRARY_embed   := <library archive>
        LIBRARY_embed64 := <library archive>


      To specify other build targets that are not programs or libraries,
      the following define can be included:

        OTHER_TARGETS := <other space separated targets>


   3. Specify an optional list of objects, compile and link options. 

      To specify additional source directories besides the current directory,
      a VPATH directive can be specified.

        VPATH := <list of colon separate paths>

      By default, if the objects are not specified, the target (library or 
      program) will include all objects corresponding to the source files
      in the source directories (as specified by VPATH). Alternately, the
      list of objects can be directly specified for the program.

        OBJS := <list of space separated objects>

      To specify the objects for each (or a specific) program:

        OBJS_<program> := <list of space separated objects>

      There are several defines for specifying compilation, assembler, and
      link flags. They include:


        CPPFLAGS          Specifies flags to be used by the C preprocessor.
        CPPFLAGS_gcc      Specifies flags to be used by the C preprocessor only
                          when compiling using gcc.
        CPPFLAGS_xlc      Specifies flags to be used by the C preprocessor only
                          when compiling using xlc.                     
        
        CC_OPT_LEVEL      Specifies the optimization level. Default is -O3. 
                          Setting to optimization level to "CC_OPT_LEVEL_DEBUG"
                          will build to code to debugging.
                
        CFLAGS            Specifies additional C compilation flags.
        CFLAGS_gcc        Specifies flags to be used for C compilation only
                          when compiling using gcc.
        CFLAGS_xlc        Specifies flags to be used for C preprocessor only
                          when compiling using xlc.                     
        CFLAGS_<program>  Specifies the specific C compilation flags to be 
                          applied to the specified program.


        CXXFLAGS          Specifies additional C++ compilation flags.
        CXXFLAGS_<program> Specifies the specific C++ compilation flags to be 
                          applied to the specified program.

        INCLUDE           Specifies the additional include directories. T

        ASFLAGS           Specifies the additional assembler flags.
        ASFLAGS_<program> Specifies the additional assembler flags to be 
                          applied to the specified program.

        LDFLAGS           Specifies the additional linker options.
        LDFLAGS_<program> Specifies the additional linker options to be applied
                          when linking the specified program.
        
        IMPORTS           Specifies the libraries to be linked into the 
                          targets.
        IMPORTS_<program> Specifies the libraries to be linked into the 
                          specified target.
        
        TARGET_INSTALL_DIR Specifies the directory in which all built targets
                          are installed.
        INSTALL_FILES     Specifies the list of file to be installed.
        INSTALL_DIR       Specifies the directory all INSTALL_FILES are shipped
                          to during installation.


   4. Include make.footer. This is simply done by the following Makefile lines,
      where the number of "../" depends upon the relative path depth within
      the SDK directory structure. 

        ifdef CELL_TOP
           include $(CELL_TOP)/buildutils/make.footer
        else
           include ../../buildutils/make.footer
        endif

   5. In rare occurrences, it may be required to include additional, post 
      make.footer build rules. If required, they will be placed here.

C. Make Targets

   The build environment provides several basic build targets. These include:
   
     all       - Default build target when no target is specified. Includes 
                 the building of the "dirs", "libraries", "programs", "misc_"
                 and "install" targets.
   
     dirs      - Traverse the make directory tree building the specified 
                 targets. This target is typically used in combination with
                 the "libraries", "programs", "misc_", or "install" targets.

     libraries - Build only the library (shared and non-shared) targets.

     programs  - Build only the program targets.

     misc_     - Builds miscellaneous targets including OTHER_TARGETS. See 
                 make.footer for a list of miscellaneous targets.

     install   - Install the built targets. See TARGET_INSTALL_DIR, 
                 INSTALL_FILES, and INSTALL_DIR defines.

     clean     - Remove all built and installed targets, including objects,
                 libraries, and programs.

     listenv   - Display the compilation environment to be applied to the 
                 current directory. Debug target useful for determining the 
                 targets, tools and flags to be used by make.


D. Other Useful Variables

   The following additional variables are defined by the build environment
   and can be used within Makefiles. The advantage of using these variables is
   that make.footer automatically handles ppu and spu differences (ie,
   /usr/include versus /usr/spu/include), 32bit and 64bit differences (ie,
   /usr/lib versus /usr/lib64), and blade or cross environment differences
   (ie, /usr/include versus /opt/cell/sysroot/usr/include).

     Variables indicating where files are 'read' from (-L and -I directories):
     $(SDKBIN) - bin directory path for official GA code
     $(SDKLIB) - library directory path for official GA code
     $(SDKINC) - include directory path for official GA code
     $(SDKEXBIN) - bin directory path for SDK example code
     $(SDKEXLIB) - library directory path for SDK example code
     $(SDKEXINC) - include directory path for SDK example code
     $(SDKPRBIN) - bin directory path for SDK prototype code
     $(SDKPRLIB) - library directory path for SDK prototype code
     $(SDKPRINC) - include directory path for SDK prototype code

     Variables indicating where files are installed (exported):
     $(EXP_SDKBIN) - bin directory path
     $(EXP_SDKLIB) - library directory path
     $(EXP_SDKINC) - include directory path

     Variables indicating where libraries will be at runtime, for use with the
     linker -R flag:
     $(SDKRPATH) - installed library directory path for official GA code
     $(SDKEXRPATH) - installed library directory path for SDK example code
     $(SDKPRRPATH) - installed library directory path for SDK prototype code

     Variables relating to the simulator:
     $(SYSTEMSIM_INCLUDE) - directory with callthru and related include files
                             (ie, profile.h)

