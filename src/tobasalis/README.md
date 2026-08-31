
 # Tobasa LIS Library

C++ library for integrating laboratory instruments using industry-standard LIS protocols.

## Overview

Tobasa LIS provides production-grade communication with laboratory analyzers and medical devices. It implements LIS2-A2 (ASTM) and HL7 message protocols over serial and TCP transports. The library handles protocol parsing, message framing, and device connectivity so your application can focus on result processing and integration logic.

## Features

**Protocol Support:**
- LIS2-A2 (ASTM) message format for laboratory instruments
- HL7 message format for healthcare information exchange
- Automatic delimiter detection and configurable parsing
- Full message/record abstractions for structured data access

**Transport Connectivity:**
- TCP/IP client and server modes
- Serial port (RS-232) communication
- Connection pooling and session management
- Automatic reconnection and retry logic
- Timeout and error handling

**Integration Capabilities:**
- Database result upload
- File export and archiving
- Message validation and error detection
- Framing and message boundary detection
- Configurable message handlers

## Building

The library is built as part of the main build system:

```bash
cmake -B build
cmake --build build
```

Enable LIS support with CMake:
```bash
cmake -B build -DTOBASA_USE_LIS_ENGINE=ON
cmake --build build
```

## Configuration

Settings are JSON-based in `appsettings.json`. Configure:


## Supported Devices

The library has been validated with:
- Indiko Plus
- DxH 500 Hematology Analyzer
- GEM Premier 3500 Blood Gas Analyzer
- Selectra Pro Clinical Chemistry Analyzer
- DIRUI H-500 Urine Analyzer
- DIRUI BCC-3600 Hematology Analyzer
- VIDAS
- VITEK 2 COMPACT

## Architecture

The library focuses on protocol handling:
- **Parsers** – Message and record parsing
- **Transports** – Serial and TCP connectivity
- **Framing** – Message boundary detection
- **Session Management** – Connection lifecycle

Application-specific logic (field mapping, domain models, UI, business rules) should be implemented in your application layer.

## Reliability Features

- Automatic message acknowledgment
- Error detection and reporting
- Connection retry with exponential backoff
- Timeout handling
- Framing error recovery

## Use Cases

- Laboratory Information System (LIS) integration
- Medical device instrument interfaces
- Result aggregation systems
- Healthcare data exchange

## Notes

All product names and trademarks are the property of their respective owners. This project implements communication protocols based on publicly available specifications and observed device behavior.

## References

- CLSI LIS01-A2 & LIS2-A2 implementation
- HL7 Message format standards
- Device manufacturer communication protocols

## License

GNU LESSER GENERAL PUBLIC LICENSE
