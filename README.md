## Building 

```shell 
cmake --build build
```

## Testing 

```shell 
# Run all tests
ctest --test-dir build

# Run tests with verbose output
ctest --test-dir build --verbose

# Run tests and show output on failure
ctest --test-dir build --output-on-failure
```