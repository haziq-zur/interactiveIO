# InteractiveIO Test Suite

This directory contains automated tests for the InteractiveIO project.

## Test Types

### 1. SCPI Core Tests (`scpi_tests.exe`)
Tests for the core instrument communication functionality:
- TCP/IP connection utilities
- SCPI command parsing and formatting
- VISA connection handling (requires NI-VISA installed)

**Run:**
```bash
.\build\tests\scpi_tests.exe
```

### 2. GUI Tests (`gui_tests.exe`)
Qt Test-based tests for the graphical user interface:
- UI element existence and state management
- Connection state transitions
- Dark mode styling
- Command history navigation
- Output formatting and clearing
- User interaction tooltips

**Run:**
```bash
.\build\tests\gui_tests.exe
```

**Verbose output:**
```bash
.\build\tests\gui_tests.exe -v2
```

**Run specific test:**
```bash
.\build\tests\gui_tests.exe testProtocolComboBox
```

## Test Coverage

### GUI Tests Include:
- ✅ Initial state verification
- ✅ UI elements existence
- ✅ Disconnected state validation
- ✅ Protocol combo box configuration
- ✅ Command history navigation
- ✅ Output append and clear functionality
- ✅ Dark mode stylesheet application
- ✅ Tooltip verification
- ✅ Connection state transitions

### Core Tests Include:
- TCP/IP socket connection validation
- SCPI command utilities
- VISA resource string parsing
- Connection error handling

## Building Tests

Tests are automatically built when you build the project:

```bash
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\6.10.2\mingw_64" -DBUILD_GUI=ON
cmake --build build --config Release
```

## Requirements

- **For Core Tests**: Google Test (automatically fetched by CMake)
- **For GUI Tests**: Qt6 Core, Widgets, and Test modules

## CI/CD Integration

Tests can be run in CI/CD pipelines using CTest:

```bash
cd build
ctest --output-on-failure
```

Or run specific test suite:
```bash
ctest -R GUITests --output-on-failure
```

## Adding New Tests

### For GUI Tests
1. Add test functions to `test_gui_mainwindow.cpp` as private slots
2. Follow the naming convention: `testFeatureName()`
3. Use Qt Test macros: `QVERIFY()`, `QCOMPARE()`, etc.
4. Rebuild to include new tests

### For Core Tests
1. Add test files to the tests directory
2. Use Google Test framework: `TEST()`, `EXPECT_EQ()`, etc.
3. Update `tests/CMakeLists.txt` to include new test files
4. Rebuild to include new tests

## Test Results

All GUI tests currently pass:
```
Totals: 13 passed, 0 failed, 0 skipped
```

## Known Limitations

- GUI tests run without the application stylesheet applied (applied in main())
- Connection tests require actual instrument connections or mocking
- Some VISA tests may fail without NI-VISA runtime installed
