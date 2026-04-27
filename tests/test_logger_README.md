# NAS Ingestion Daemon - Logger

## Build
mkdir build && cd build
cmake ..
make

## Run
./test_logger

## View logs
journalctl -t test-logger

## Notes
- Uses syslog
- Supports log levels