MP1
===

# Usage

1. `make`
2. Start the servers
   - On each machine: `./grep -s`
3. Search for `foo`
   - From any machine: `./grep -c foo`

## Running Tests

1. `make`
2. `./grep -s tests/logs/searchme.txt &`
3. `cd tests/`
4. `python test-server.py`

Or:

1. `cd tests`
2. `python make-random-logs.py`
3. `cd ../`
4. `make && ./grep -s tests/logs/searchme-copy.txt &`
5. `cd tests`
6. `python test-server.py`

Or:

1. Start all servers with their default log files
2. `cd tests/`
3. `./full-scale-test.sh`

# Network Protocol

## Requests Contain
- UUID
  - Same UUID goes out to each server from the client
- Query string

## Response Format
(Network data is always big-endian.)
First 4 bytes are the status. If status == 0, there follows an output stream.

## Server tracks
- Request list
Each request has
- UUID
- Query string
- Socket
