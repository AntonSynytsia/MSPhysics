# Windows
Follow instructions below to compile msp_lib.so and newton.dll for Windows, on Visual Studio:

1. Set build configuration to Release (X.Y).
2. Set build platform to x86 or x64.
2. Build Solution.
3. Copy .../VS/[Win32/x64]/Release (X.Y)/msp_lib/msp_lib.so
   to the following binary folder:
   /RubyExtension/MSPhysics/libraries/stage/[win32/win64]/[X.Y]/
4. Copy .../VS/[Win32/x64]/Release (X.Y)/newton/newton.dll
   to the following binary folder:
   /RubyExtension/MSPhysics/libraries/stage/[win32/win64]/

# Max OS X
Follow instructions below to compile msp_lib.bundle and newton.dylib for Mac OS X, on xCode:

1. Set active scheme to "Ruby (X.Y) - [32/64]"
2. Execute (Menu) Product > Archive
3. Export the built archive to your documents.
4. Locate msp_lib.bundle within the exported archive.
5. Copy msp_lib.bundle to the following binary folder:
   /RubyExtension/MSPhysics/libraries/stage/[osx32/osx64]/[X.Y]/
6. Locate newton.dylib within the exported archive.
7. Copy newton.dylib to the following binary folder:
   /RubyExtension/MSPhysics/libraries/stage/[osx32/osx64]/
