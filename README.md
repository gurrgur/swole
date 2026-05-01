```bash
# test out libSwell.so drop-in
cmake -S . -B build -G Ninja
cmake --build build
cp ./build/libSwell.so testing/reaper_linux_x86_64/REAPER/libSwell.so
testing/reaper_linux_x86_64/REAPER/reaper > log.txt
```