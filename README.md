# libpcsc-cpp

![European Regional Development Fund](https://github.com/open-eid/DigiDoc4-Client/blob/master/client/images/EL_Regionaalarengu_Fond.png)

C++ library for accessing smart cards using the PC/SC API.

## Usage

Example how to list available readers, connect to the smart card in first
reader and transmit an APDU:

    auto readers = listReaders();
    auto card = readers[0].connectToCard();
    auto command = CommandApdu::fromBytes({0x2, 0x1, 0x3, 0x4});

    auto transactionGuard = card->beginTransaction();
    auto response = card->transmit(command);

See more examples in [tests](tests).

## Building

In Ubuntu:

    apt install build-essential pkg-config cmake libgtest-dev valgrind libpcsclite-dev
    sudo bash -c 'cd /usr/src/googletest && cmake . && cmake --build . --target install'

    cd build
    cmake .. # optionally with -DCMAKE_BUILD_TYPE=Debug
    cmake --build . # optionally with VERBOSE=1

## Testing

Build as described above, then, inside the `build` directory, run:

    ctest # or 'valgrind --leak-check=full ctest'

`ctest` runs tests that use the _libscard-mock_ library to mock PC/SC API calls.

There are also integration tests that use the real operating system PC/SC
service, run them inside `build` directory with:

    ./libpcsc-cpp-test-integration

## Development guidelines

- Format code with `scripts/clang-format.sh` before committing
- See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)
