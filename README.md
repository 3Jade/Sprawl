Sprawl
======

Sprawl is a collection of various C++ libraries. Currently, the library consists of:

*sprawl::collections* - A library of replacement collections that don't go through the STL, and offer both performance and versatility improvements, as well as more predictable implementation, compared to the STL counterparts. Hashing is done with Murmur3 for hash containers, and memory is allocated by default from the sprawl::DynamicPoolAllocator.

*sprawl::hash* - A collection of hashing algorithms. Currently, only Murmur3 has been implemented, but there are plans to implement CityHash, CRC32, FNV-1, FNV-1a, and other popular hashing algorithms. (This is not, however, a collection of cryptographically secure hashes.)

*sprawl::memory* - Intended to contain various memory allocation algorithms. Currently only contains a statically-sized pool allocator, which is optimized for allocating many items of a known size, as opposed to the more versatile algorithms used by malloc that can allocate items of variable sizes. Additional allocators will be added over time.

*sprawl::network* - Contains an asynchronous, callback-based networking library, supporting both the TCP and UDP protocols. UDP is a work-in-progress, but currently supports reliable data transmission, and will soon support ordered transmission. Packets sent through UDP must currently be smaller than the path MTU; in the future, the library will split large packets up instead.

*sprawl::string* - Contains an efficient statically-sized, ref-counted, immutable string class, reducing the overhead of classes like std::string by avoiding copying when possible. Additionally, strings under 128 bytes (configurable) can be allocated much more efficiently thanks to the sprawl::DynamicPoolAllocator and the use of a statically-sized buffer for strings that fit into it. sprawl::String also supports the python String.format() syntax very closely. The string library also contains sprawl::StringBuilder, which can be used as an alternative to String.format() to efficiently build string data and then convert it to the immutable sprawl::String format once construction of the data has completed.

*sprawl::serialization* - A data serialization library to send data over the network in binary, JSON, or MongoDB (BSON) formats. Also includes a "replicable" serialization method that detects changes to data and sends only the data that has changed, minimizing both bandwidth usage and edit conflicts if data is sent from multiple sources. A special replicable serializer is available for MongoDB that generates delta-based queries to be sent to the database to update objects, as opposed to the more compact format typically used by the Replicable serializers to cut down on network bandwidth.

This collection is in its infancy, so any user should be aware that there are missing features and likely bugs with the library, and that the library is in a state of metamorphosis and the API may change dramatically in the future.
