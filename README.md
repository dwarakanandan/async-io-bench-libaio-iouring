# dmlab - Benchmarking IO Operations

A benchmarking tool for comparing sync-io, libaio and io_uring performance
<br />
<br />

##### Prerequisites

- Linux Kernel version > 5.1
- libaio
- liburing
<br />
<br />

##### Installing libaio
`libaio` is a wrapper around the Linux `aio` library. Clone libaio from source and build it.

```shell
git clone https://pagure.io/libaio.git
cd libaio
make
```
<br />
##### Installing liburing
`liburing` is a wrapper around the Linux `io_uring` library. Clone liburing from source and build it.

```shell
git clone https://github.com/axboe/liburing.git
cd liburing
make
```
<br />
#### Build
Clone the repository, and make sure the prerequisites are organized as follows. Run `make` to build the application into `build/`.
```shell
(parent directory)
|
---dmlab/
|
---libaio/src
|
---liburing/src
```
<br />
##### Build tested with:

- g++ 9.3.0
- Linux kernel 5.4.0

