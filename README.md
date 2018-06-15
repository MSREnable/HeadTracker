# HeadTracker

HeadTracker relies on the [dlib](dlib.net) library. Rather than compiling from [source](https://github.com/davisking/dlib), the best choice is to install using [vcpkg](https://github.com/Microsoft/vcpkg). 

To install vcpkg, in a Powershell Administrator prompt:
```
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg integrate powershell
```

Once you have vcpkg installed, you will then need to install dlib
```
.\vcpkg install dlib:x64-windows
```

Note: 
dlib requires openblas (among other dependencies). openblas is not supported on x86, so you MUST use the x64-windows option when installing dlib for it to work.

Note:
You will need to run the app in release mode. In debug mode, dlib performance is not usable.
