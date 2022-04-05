# SymbolGenerator
A tool to automatically generate a module-exports definition file (`.def` file) 
from one or more compiled C++ translation units (`.obj` files) based on user provided filters.  
This removes the need to mark all exported symbols in a project with `__declspec(dllexport)`, 
while simultaneously avoiding issues that CMake's `WINDOWS_EXPORT_ALL_SYMBOLS` property causes,
like hitting the 64K symbol limit from unnecessarily exported symbols.

### Building
- This is a Windows-only library. Linux platforms do not have the same issues, since symbols are exported by default
and there is no 64K symbol limit. This library will not build on non-Windows platforms.
- [Conan](https://conan.io/) (and therefore [Python](https://www.python.org/downloads/)) is required to install the project's dependencies (`pip install conan`).
- [CMake](https://cmake.org/download/) is required, together with some generator to build the project with (e.g. [Ninja](https://ninja-build.org/)).

To build the project (with Ninja):
```shell
mkdir out
cd out
cmake -DCMAKE_BUILD_TYPE=[DEBUG|RELEASE] -G Ninja -DCMAKE_C_COMPILER=[MSVC|clang-cl] -DCMAKE_CXX_COMPILER=[MSVC|clang-cl] ../
cmake --build ./[debug|release] --target all
```

### Usage
The program takes as its input a set of `.obj` files and some regex filters, and produces a `.def` file containing all symbols that are matched by said filters.  
The command line arguments are as follows:
- `-lib`:     the name of the DLL that will be created using the generated `.def` file.
- `-i`:       the directory containing the `.obj` to process. The provided path is searched recursively.
- `-o`:       the path of the output `.def` file.
- `-y`:       a list of regexes for namespaces to include. Includes all symbols in the given namespace and all subnamespaces.
Namespace should be a top-level namespace, or its parent should already be included by another `-y` parameter.
- `-n`:       a list of regexes for namespaces to exclude. Excludes all symbols in the given namespace and all subnamespaces.
Namespace need not be a top-level namespace.
- `-yo`:      a list of regexes of force-included symbols. These symbols are always included, even if they are in an excluded namespace.
- `-no`:      a list of regexes of force-excluded symbols. These symbols are always excluded, even if they are in an included namespace.
- `-cache`:   if provided, the results of the program will be cached so that it can run faster the next time it is invoked with the same arguments.
Caching occurs on a per-obj-file basis.
- `-verbose`: if provided, logs additional information to stdout.
- `-j`:       if provided, the number of threads used to process objects. Defaults to number of threads of the current device.

The `-lib`, `-i` and `-o` parameters are required. All other parameters are optional (Although you should provide at least one to match anything).

Example:
```shell
SymbolGenerator.exe -cache -lib VoxelEngine -i ../out/debug/CMakeFiles/VoxelEngine.dir -o ./VoxelEngine.def -y ve -n .*detail.* .*impl.* meta -yo .*vertex_layout.*
```
This command creates `./VoxelEngine.def` which can be used to create `VoxelEngine.dll`, by taking symbols from all `.obj` files in the `VoxelEngine.dir` directory.
All symbols in the `ve` namespace and nested namespaces therein are included, except nested namespaces containing the text `detail` or `impl` or named `meta`.
Symbols containing the text `vertex_layout` are always included, even if they are in such an excluded namespace.