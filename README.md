# common-lib

Common utility library for personal project.

> **Note:** Windows support is currently unstable. Linux is the recommended platform.

## Requirements

- CMake 3.16 or later
- C++17-compatible compiler (GCC, Clang, MSVC)
- Linux (recommended) or Windows

## Features

- **Core** — Logging, error codes, exceptions, factory, observer, singleton patterns
- **Threading** — Thread wrapper, task executor, timer
- **Container** — Work queues including lock-free and size-bounded variants
- **Communication** — Event system, message passing, socket and UART serial communication
- **Async** — Asio-based asynchronous TCP/UDP, event broker and transport
- **Lifecycle** — Application and resource lifecycle management
- **Extensions** — CH341 USB-Serial bridge driver wrapper, math utilities (angles, matrices, filters)

## Third-party Libraries

- [Asio](https://think-async.com/Asio/) — Asynchronous I/O
- [GoogleTest](https://github.com/google/googletest) — Unit testing
- [FlatBuffers](https://google.github.io/flatbuffers/) — Serialization

## Build

```bash
mkdir build
cmake -B build
cmake --build build
```

### Build Options

| Option | Default | Description |
|---|---|---|
| `COMMON_LIB_BUILD_TESTING` | `ON` | Build unit tests |
| `COMMON_LIB_STRICT_MODE` | `ON` | Enable strict compiler warnings |
| `EVENT_THREADS` | `4` | Number of threads used by EventBus |
| `ENABLE_COVERAGE` | `OFF` | Enable code coverage reporting |

Example:
```bash
cmake -B build
cmake --build build
```

## Test

```bash
cmake --build build
cd build && ctest --output-on-failure
```

## Code Coverage

```bash
cmake -B build -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target coverage
```

Results are available at `build/coverage/html/index.html`.

## License

See [LICENSE](LICENSE) for details.
