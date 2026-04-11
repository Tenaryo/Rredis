# Redis_Cpp

Redis server implementation in C++23, featuring an epoll-based event loop, RESP protocol support, master-replica replication, Pub/Sub, Streams, and more. Built from scratch with zero external dependencies.

## Features

### Data Types

| Type | Description |
|------|-------------|
| `string` | Basic key-value with TTL support |
| `list` | Doubly-linked list (deque-based) |
| `stream` | Append-only log with time-series IDs |

### Supported Commands

#### General

| Command | Description |
|---------|-------------|
| `PING` | Test connectivity |
| `ECHO` | Echo the given string |
| `INFO` | Server and replication info |
| `CONFIG GET` | Get configuration parameters (`dir`, `dbfilename`) |
| `KEYS` | List all stored keys |
| `TYPE` | Get the type of a key |
| `WAIT` | Wait for replica acknowledgments |

#### String Operations

| Command | Description |
|---------|-------------|
| `SET key value [EX seconds] [PX milliseconds]` | Set a key-value pair with optional TTL |
| `GET key` | Get the value of a key |
| `INCR key` | Atomically increment an integer value |

#### List Operations

| Command | Description |
|---------|-------------|
| `LPUSH key value [value ...]` | Push one or more values to the head |
| `RPUSH key value [value ...]` | Push one or more values to the tail |
| `LPOP key [count]` | Pop one or more values from the head |
| `LRANGE key start stop` | Get a range of elements |
| `LLEN key` | Get the length of a list |
| `BLPOP key timeout` | Blocking pop from the head |

#### Stream Operations

| Command | Description |
|---------|-------------|
| `XADD key ID field value [field value ...]` | Add an entry to a stream |
| `XRANGE key start end` | Query a range of entries |
| `XREAD [BLOCK ms] STREAMS key ID` | Read entries after a given ID |
| `XREAD [BLOCK ms] STREAMS key $` | Block until new entries arrive |

The `XADD` command supports automatic ID generation (`*`), auto-incrementing sequences (`<timestamp>-*`), and explicit IDs.

#### Pub/Sub

| Command | Description |
|---------|-------------|
| `SUBSCRIBE channel` | Subscribe to a channel |
| `UNSUBSCRIBE channel` | Unsubscribe from a channel |
| `PUBLISH channel message` | Publish a message to a channel |

Subscribed clients can only execute `SUBSCRIBE`, `UNSUBSCRIBE`, `PING`, `QUIT`, and `RESET`.

#### Transactions

| Command | Description |
|---------|-------------|
| `MULTI` | Start a transaction |
| `EXEC` | Execute all queued commands |
| `DISCARD` | Discard the transaction |

#### Replication

| Command | Description |
|---------|-------------|
| `REPLCONF` | Replica configuration handshake |
| `PSYNC` | Partial/full synchronization with master |

## Architecture

```
src/
  cli/              Command-line argument parser
  connection/       TCP connection abstraction (read/write buffers)
  event_loop/       epoll-based I/O event loop
  handler/          Command routing and execution
  protocol/         RESP protocol parser and encoder
  rdb/              RDB file parser (loads snapshots on startup)
  replica/          Master-replica handshake and command propagation
  server/           TCP server socket, server config, application runner
  store/            In-memory data store (String, List, Stream)
  block_manager/    Blocking operation manager (BLPOP, XREAD BLOCK)
  pubsub/           Pub/Sub channel manager
  util/             Integer parsing, string utilities
```

### Key Design Decisions

- **epoll event loop** -- All I/O is handled through a single-threaded epoll loop with configurable timeouts for blocking operations and WAIT.
- **Lazy expiration** -- Keys with TTL are checked for expiration on access (`find_valid_entry`) and periodically cleaned during `KEYS` calls.
- **RESP v2** -- Full parser for the Redis Serialization Protocol, supporting inline parsing of pipelined commands via `parse_one`.
- **Streaming replication** -- Write commands are automatically propagated to connected replicas. The `WAIT` command tracks replica acknowledgment offsets.
- **Zero dependencies** -- Uses only the C++23 standard library and POSIX APIs (epoll, sockets). No third-party libraries required.

## Usage

### Build

```bash
./build.sh          # Debug build (default)
./build.sh Release  # Release build
```

Requires:
- C++23 compiler (GCC 13+ recommended)
- CMake 3.21+

### Run

```bash
# Start a server on default port 6379
./build/redis

# Custom port
./build/redis --port 6380

# Start as replica of a master
./build/redis --port 6380 --replicaof "localhost 6379"

# Load an RDB file on startup
./build/redis --dir /data --dbfilename dump.rdb
```

### CLI Options

| Option | Description | Default |
|--------|-------------|---------|
| `--port <port>` | Server listening port | `6379` |
| `--replicaof "<host> <port>"` | Run as replica of the given master | (master mode) |
| `--dir <path>` | Directory containing the RDB file | (none) |
| `--dbfilename <name>` | RDB file name to load | (none) |

### Connect with redis-cli

```bash
redis-cli -p 6379
> SET mykey hello
OK
> GET mykey
"hello"
> SET tempkey value EX 10
OK
> INCR counter
(integer) 1
> LPUSH mylist a b c
(integer) 3
> LRANGE mylist 0 -1
1) "c"
2) "b"
3) "a"
> XADD mystream * name alice age 30
"1745000000000-0"
> XRANGE mystream - +
1) 1) "1745000000000-0"
   2) 1) "name"
      2) "alice"
      3) "age"
      4) "30"
```

## Testing

```bash
./run_tests.sh
```

Tests are built alongside the main binary. Each test file in `tests/` is compiled as a standalone executable and exercises specific subsystems:

- Command handling (SET, GET, INCR, KEYS, CONFIG GET, INFO)
- List operations (LPUSH, RPUSH, LPOP, LRANGE)
- Stream operations (XADD auto-ID, XRANGE, XREAD, XREAD BLOCK with `$`)
- Transactions (MULTI, EXEC, DISCARD)
- Pub/Sub (SUBSCRIBE, PUBLISH)
- Replication (handshake, command propagation, WAIT)
- RDB parsing
- CLI argument parsing

## Replication

Redis_Cpp supports master-replica replication:

1. **Master** starts normally, accepting connections.
2. **Replica** starts with `--replicaof "<master-host> <master-port>"` and performs a full handshake (PING -> REPLCONF -> PSYNC).
3. Write commands executed on the master are automatically propagated to all connected replicas.
4. Use `WAIT numreplicas timeout` to block until the specified number of replicas have acknowledged the writes.
