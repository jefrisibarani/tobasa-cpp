
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


## Configuration
Settings are JSON-based (see `conf::Engine` in `settings.h`). You can
configure:

- TCP client/server addresses and ports
- Serial port parameters (COM name, baud, parity, data/stop bits)
- Timeouts, retry counts, and framing options
- Delimiters and auto-detection for message fields
- DB connection and result saving options


## Supported Devices
- Indiko Plus
- DxH 500 Hematology Analyzer
- GEM Premier 3500 Blood Gas Analyzer
- Selectra Pro Clinical Chemistry Analyzer
- DIRUI H-500 Urine Analyzer
- DIRUI BCC-3600 Hematology Analyzer
- VIDAS
- VITEK 2 COMPACT


## Notes
- The library focuses on protocol handling (parsing, framing, transports).
  Keep application-specific logic (mapping fields to domain objects, UI,
  business rules) outside the parser.
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
