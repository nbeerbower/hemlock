# Hemlock Standard Library

The Hemlock standard library provides essential modules and data structures for Hemlock programs.

## Available Modules

### Collections (`@stdlib/collections`)

Provides common data structures:
- **HashMap** - Hash table with O(1) average-case operations
- **Queue** - FIFO data structure
- **Stack** - LIFO data structure
- **Set** - Collection of unique values with set operations
- **LinkedList** - Doubly-linked list

See [docs/collections.md](docs/collections.md) for detailed documentation.

## Usage

Import modules using the `@stdlib/` prefix:

```hemlock
import { HashMap, Queue, Stack } from "@stdlib/collections";

let map = HashMap();
map.set("key", "value");

let q = Queue();
q.enqueue("item");
```

## Directory Structure

```
stdlib/
├── README.md           # This file
├── collections.hml     # Collections module implementation
└── docs/
    └── collections.md  # Collections module documentation
```

## Future Modules

Planned additions to the standard library:
- **io** - File I/O utilities and helpers
- **strings** - Advanced string manipulation
- **math** - Mathematical functions and constants
- **json** - JSON parsing and serialization
- **http** - HTTP client/server utilities
- **os** - Operating system interaction

## Contributing

When adding new stdlib modules:
1. Create the `.hml` file in `stdlib/`
2. Add comprehensive tests in `tests/stdlib_<module>/`
3. Document the module in `stdlib/docs/<module>.md`
4. Update this README with the new module

## Testing

All stdlib modules have comprehensive test coverage:

```bash
# Run all tests
make test

# Run specific module tests
make test | grep stdlib_collections
```

## License

Part of the Hemlock programming language.
