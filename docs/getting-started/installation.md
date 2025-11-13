# Installation

This guide will help you build and install Hemlock on your system.

## Prerequisites

### Required Dependencies

Hemlock requires the following dependencies to build:

- **C Compiler**: GCC or Clang
- **Make**: GNU Make
- **libffi-dev**: Foreign Function Interface library (for FFI support)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential libffi-dev
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc make libffi-devel
```

**Arch Linux:**
```bash
sudo pacman -S base-devel libffi
```

**macOS:**
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install libffi via Homebrew
brew install libffi
```

## Building from Source

### 1. Clone the Repository

```bash
git clone https://github.com/nbeerbower/hemlock.git
cd hemlock
```

### 2. Build Hemlock

```bash
make
```

This will compile the Hemlock interpreter and place the executable in the current directory.

### 3. Verify Installation

```bash
./hemlock --version
```

You should see the Hemlock version information.

### 4. Test the Build

Run the test suite to ensure everything works correctly:

```bash
make test
```

All tests should pass. If you see any failures, please report them as an issue.

## Installing System-Wide (Optional)

To install Hemlock system-wide (e.g., to `/usr/local/bin`):

```bash
sudo make install
```

This allows you to run `hemlock` from anywhere without specifying the full path.

## Running Hemlock

### Interactive REPL

Start the Read-Eval-Print Loop:

```bash
./hemlock
```

You'll see a prompt where you can type Hemlock code:

```
Hemlock REPL
> print("Hello, World!");
Hello, World!
> let x = 42;
> print(x * 2);
84
>
```

Exit the REPL with `Ctrl+D` or `Ctrl+C`.

### Running Programs

Execute a Hemlock script:

```bash
./hemlock program.hml
```

With command-line arguments:

```bash
./hemlock program.hml arg1 arg2 "argument with spaces"
```

## Directory Structure

After building, your Hemlock directory will look like this:

```
hemlock/
├── hemlock           # Compiled interpreter executable
├── src/              # Source code
├── include/          # Header files
├── tests/            # Test suite
├── examples/         # Example programs
├── docs/             # Documentation
├── stdlib/           # Standard library
├── Makefile          # Build configuration
└── README.md         # Project README
```

## Build Options

### Debug Build

Build with debug symbols and no optimization:

```bash
make debug
```

### Clean Build

Remove all compiled files:

```bash
make clean
```

Rebuild from scratch:

```bash
make clean && make
```

## Troubleshooting

### libffi Not Found

If you get errors about missing `ffi.h` or `-lffi`:

1. Ensure `libffi-dev` is installed (see dependencies above)
2. Check if `pkg-config` can find it:
   ```bash
   pkg-config --cflags --libs libffi
   ```
3. If not found, you may need to set `PKG_CONFIG_PATH`:
   ```bash
   export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
   ```

### Compilation Errors

If you encounter compilation errors:

1. Ensure you have a C99-compatible compiler
2. Try using GCC instead of Clang (or vice versa):
   ```bash
   make CC=gcc
   ```
3. Check that all dependencies are installed

### Test Failures

If tests fail:

1. Check that you have the latest version of the code
2. Try rebuilding from scratch:
   ```bash
   make clean && make test
   ```
3. Report the issue on GitHub with the test output

## Next Steps

- [Quick Start Guide](quick-start.md) - Write your first Hemlock program
- [Tutorial](tutorial.md) - Learn Hemlock step-by-step
- [Language Guide](../language-guide/syntax.md) - Explore Hemlock features
