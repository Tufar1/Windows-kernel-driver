cmake_minimum_required(VERSION 4.0)
set(CMAKE_C_COMPILER "C:\\Program Files\\LLVM\\bin\\clang-cl.exe")
set(CMAKE_CXX_COMPILER "C:\\Program Files\\LLVM\\bin\\clang-cl.exe")
project(Wyvern)

set(WDK_PATH "C:\\Program Files (x86)\\Windows Kits\\10")
set(WDK_INCLUDES "${WDK_PATH}\\Include\\10.0.26100.0\\km")
set(WDK_LIBS "${WDK_PATH}\\lib\\10.0.26100.0\\km\\x64")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_LINKER "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/link.exe")
set(CMAKE_CXX_STANDARD_LIBRARIES "")

add_executable(${PROJECT_NAME}
	main.cpp
	helpers.cpp
	hook.cpp
	interrupts.cpp
	memory.cpp
	scanner.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${WDK_INCLUDES} ${LIBCXX_INCLUDE_DIR})
target_link_directories(${PROJECT_NAME} PRIVATE ${WDK_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE ntoskrnl ${LIBCXX_LIBRARY_PATH})

target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN64 _AMD64_ AMD64 _WIN32_WINNT=0x0A00  WINVER=0x0A00  WINNT=1  NTDDI_VERSION=0xA000010)
target_link_options(${PROJECT_NAME} PRIVATE
    -nodefaultlib
    LINKER:/MANIFEST:NO
    LINKER:/SUBSYSTEM:NATIVE
    LINKER:/NOLOGO
    LINKER:/NODEFAULTLIB
    LINKER:/MACHINE:X64
    LINKER:/ENTRY:DriverEntry
    LINKER:/DRIVER
    LINKER:/DEBUG:FULL
    LINKER:/PDB:${PROJECT_NAMEl}.pdb
    LINKER:/DYNAMICBASE
    LINKER:/FIXED:NO
)

target_compile_options(${PROJECT_NAME} PRIVATE /GS- /GR- /EHs- /EHc- /w)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".sys"
    OUTPUT_NAME "${PROJECT_NAME}"
)
