# Project Conventions

## Build
```bash
./build.sh
```

## Test
```
./run_tests.sh
```

## Formatting
**Only format `.cpp` and `.hpp` files. NEVER run clang-format on CMakeLists.txt or any `.txt` file — it will corrupt them.**
```bash
clang-format -i src/**/*.cpp src/**/*.hpp
```
