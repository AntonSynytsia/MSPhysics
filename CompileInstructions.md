# Compile Instructions

## Windows
Refer to the following instructions to compile <tt>msp_lib.so</tt> and <tt>newton.dll</tt> for Windows, on Visual Studio:

1. Set build configuration to <tt>Release (X.Y)</tt>.
2. Set build platform to <tt>x86</tt> or <tt>x64</tt>.
2. Build Solution.
3. Copy <tt>.../VS/[Win32/x64]/Release (X.Y)/msp_lib/msp_lib.so</tt>
   to the following binary folder:
   <tt>/RubyExtension/MSPhysics/libraries/stage/[win32/win64]/[X.Y]/</tt>
4. Copy <tt>.../VS/[Win32/x64]/Release (X.Y)/newton/newton.dll</tt>
   to the following binary folder:
   <tt>/RubyExtension/MSPhysics/libraries/stage/[win32/win64]/</tt>

## Mac OS X
Refer to the following instructions to compile <tt>msp_lib.bundle</tt> and <tt>newton.dylib</tt> for Mac OS X, on xCode:

1. Set active scheme to <tt>Ruby (X.Y) - [32/64]</tt>
2. Execute <tt>(Menu) Product > Archive</tt>
3. Export the built archive to your documents.
4. Locate <tt>msp_lib.bundle</tt> within the exported archive.
5. Copy <tt>msp_lib.bundle</tt> to the following binary folder:
   <tt>/RubyExtension/MSPhysics/libraries/stage/[osx32/osx64]/[X.Y]/</tt>
6. Locate newton.dylib</tt> within the exported archive.
7. Copy <tt>newton.dylib</tt> to the following binary folder:
   <tt>/RubyExtension/MSPhysics/libraries/stage/[osx32/osx64]/</tt>
