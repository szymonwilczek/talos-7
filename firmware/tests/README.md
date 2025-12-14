# Talos 7 Firmware Unit Tests

Unit tests for Talos 7 firmware using the Unity Test Framework.


## Requirements 

- CMake 3.13+
- C compiler (gcc/clang)

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
├── unity/             # unity test framework
├── mocks/             # hardware mocks (pico, flash, tusb, gpio, watchdog)
├── test_*.c           # test files
├── test_runner.c      # main runner
└── CMakeLists.txt     # cmake config
```

## Tests 

| File | Testing | Tests |
|------|---------|-------|
| `test_macro_config.c` | CRC32, layer switching, factory defaults, structures | 17 |
| `test_hardware_interface.c` | HID keycodes mapping, GPIO mock | 10 |
| `test_exec_midi.c` | MIDI clamping, velocity/channel fallbacks | 13 |
| `test_cdc_cmd_write.c` | SET_MACRO parsing, validation | 9 |

**Total (currently): 49 tests**

## Adding new tests 

1. Create new file `test_xxx.c`
2. Add `void run_xxx_tests(void)` 
3. Add to `test_runner.c`
4. Add to `CMakeLists.txt`
