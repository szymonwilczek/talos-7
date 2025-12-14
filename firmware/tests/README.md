# Talos 7 Firmware Unit Tests

Unit tests for Talos 7 firmware using the Unity Test Framework.


## Requirements 

- CMake 3.13+
- Kompilator C (gcc/clang)

## Running 

```bash
cd firmware/tests
mkdir build && cd build
cmake ..
make
./run_tests
```

## Structure 

```
tests/
├── unity/             # Unity Test Framework
├── mocks/             # Hardware mocks (pico, flash, tusb)
├── test_*.c           # Tests files
├── test_runner.c      # Main runner
└── CMakeLists.txt     # CMake config
```

## Tests 

| File | Testing | Tests |
|------|---------|-------|
| `test_hardware_interface.c` | Mapping HID keycodes, GPIO mock | 10 |
| `test_exec_midi.c` | MIDI clamping, velocity/channel fallbacks | 13 |
| `test_cdc_cmd_write.c` | SET_MACRO parsing, validation | 9 |

## Adding new tests 

1. Create new file `test_xxx.c`
2. Add `void run_xxx_tests(void)` 
3. Add to `test_runner.c`
4. Add to `CMakeLists.txt`
