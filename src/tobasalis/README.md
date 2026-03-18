
 # Tobasa LIS Library
C++ library for talking to laboratory instruments using LIS2-A2
(ASTM) and HL7 protocols.

## Overview
Tobasa LIS reads and writes lab messages to instruments. It supports common
lab transports (serial port and TCP) and parses message formats (LIS2-A2 /
ASTM and HL7). The library handles framing, delimiters, and parsing so your
application can focus on business logic.

## Features
- Supports LIS2-A2 (ASTM) and HL7 message formats
- Transport: TCP/IP (client/server) and serial (RS-232)
- Automatic delimiter detection and configurable delimiters
- Parsers and message/record abstractions to process lab data
- Session and connection management for reliable transfers
- Helpers to save or forward results (DB upload, file export)

## Core Components
- `engine` — main runtime and instrument manager
- `parser` / `message` / `record` — parsing and message model
- `connection_tcp`, `connection_rs232` — transport implementations
- `netsession` / `netsession_manager` — session handling for TCP/serial
- `settings` — JSON-configurable options (see `conf::Engine` in settings.h)

## Configuration
Settings are JSON-based (see `conf::Engine` in `settings.h`). You can
configure:

- TCP client/server addresses and ports
- Serial port parameters (COM name, baud, parity, data/stop bits)
- Timeouts, retry counts, and framing options
- Delimiters and auto-detection for message fields
- DB connection and result saving options


## Notes
- The library focuses on protocol handling (parsing, framing, transports).
  Keep application-specific logic (mapping fields to domain objects, UI,
  business rules) outside the parser.
- Check `include/tobasalis/lis` for concrete classes and configuration fields.

- All product names and trademarks are the property of their respective owners.
  This project implements communication protocols based on publicly available information and observed device behavior.


## Thank you
- OO implementation CLSI LIS01-A2 & LIS2-A2: 
    https://www.nuget.org/packages/Essy.LIS.LIS01-A2
    https://www.nuget.org/packages/Essy.LIS.LIS02A2

- HL7 Message parser and composer: 
    HL7-dotnetcore https://github.com/Efferent-Health/HL7-dotnetcore

## License
GNU LESSER GENERAL PUBLIC LICENSE
