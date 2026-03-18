
# Tobasa Common Library

`libtobasa` is the root of the Tobasa Framework.  It provides a
collection of small, self‑contained utilities that are used by almost
all of the higher‑level libraries in the repository. 
 

## Dependencies
* [nlohmann/json](https://github.com/nlohmann/json) – lightweight JSON
  parser/serializer used throughout the framework.
* [utf8cpp](https://github.com/nemtrif/utf8cpp) – UTF‑8 string helpers.
* [`date`/`tz`](https://github.com/HowardHinnant/date) – calendaring and
  timezone support (bundled or std‑library when available).
* [fmt](https://fmt.dev/) – formatting library (or `std::format` on
  recent compilers).

Optional dependencies are added by the libraries that build on top of
`tobasa` (e.g. OpenSSL for `tobasahttp`), but they are not required for
the common utility code.

## Key features
The root library exposes a number of useful components; a short
summary follows.

* **Configuration** – `tbs::Config` is a thread‑safe singleton that
  loads JSON configuration files and provides safe access with generic
  `getOption<>()`/`getNestedOption<>()` helpers.
* **Logging** – `tbs::Logger` wraps a pluggable `LogSink` and provides
  formatted log calls (`logD()`, `logW()`, …) that are used by all
  other modules.
* **String/UTF8 utilities** – functions for splitting, trimming,
  random string/number generation, and UTF‑8 validation.
* **Cryptography** – SHA1/256/512 helpers and random salt generation.
* **File/data readers** – abstract `DataReader` with concrete
  implementations (`FileReader`, `BytesReader`, …) to decouple I/O from
  business logic.


## License
GNU LESSER GENERAL PUBLIC LICENSE

(see also the root README for an overview of the entire Tobasa
framework)
