Sprawl
======

Sprawl is a collection of C++ libraries, many of which are implemented as header-only libraries (with attempts being made to change those that are not header-only to be so). Currently, the library consists of:

*sprawl::multiaccess* - An unordered map library with multiple accessors. Presently, this library only accepts two accessors, and one must be a string while the other is an integer. Additionally, the library currently uses the inferior djb2 hash algorithm. Future plans for this library are to include an unlimited number of accessors of any hashable type, and a switch to the murmur3 hash algorithm.

*sprawl::multitype* - A library to enable hosting of multiple heterogeneous types in a single collection - i.e., a vector that contains a string, an integer, a custom type, etc. Currently, the library contains wrappers for std::vector and std::unordered_map, with overall limited functionality. The hope in the future is to remove these wrappers and enable the user to create heterogeneous collections of any type they desire.

*sprawl::format* - A library that enables string formatting in the same manner as python, ActionScript, C#, and other languages, using "{0}{1}" syntax. Currently only the basic functionality has been implemented; future plans include full support for the complete Python syntax, but presently it only supports numbered arguments with no modifiers.

*sprawl::network* - A connection-based network library that supports both TCP connections and UDP pseudo-connections. UDP currently supports sending both reliably and unreliably on a per-packet basis, but support for maintaining order is currently unimplemented. Future plans include support for ordered UDP, as well as adding connectionless UDP support.

*sprawl::serialization* - A data serialization library to send data over the network in binary, JSON, YAML, or MongoDB (BSON) formats. Also includes a "replicable" serialization method that detects changes to data and sends only the data that has changed, minimizing both bandwidth usage and edit conflicts if data is sent from multiple sources.

This collection is in its infancy, so any user should be aware that there are missing features and likely bugs with the library, and that the library is in a state of metamorphosis and the API may change dramatically in the future.
