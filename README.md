libSprawl is a collection of libraries I've been working on in my spare time. Some of it is production ready. Some of it isn't. Some of the functionality implements things that exist in the c++ standard library in a cross-platform way with predictable performance across platforms. Some of the functionality is new. And some of it is expansions on standard library functionality.

Included in the library:

*libSprawl::collections:*
Implements various templated container types:
- _sprawl::collections::Array_ - A static array implementation similar to std::array
- _sprawl::collections::BinaryTree_ - Currently non-functional. Do not use. I need to fix it.
- _sprawl::collections::BitVector_ - A dynamically expanding vector of booleans stored as one bit each
- _sprawl::collections::BitSet_ - A statically sized set of booleans stored as one bit each
- _sprawl::collections::ConcurrentQueue_ - An array-based unbounded concurrent queue implementation that is, in the vast majority of circumstances, wait-free, with zero Compare And Swap operations and no set maximum capacity. Seems to work, but until I make a formal proof, use with caution.
- _sprawl::collections::Dequeue_ - A double-ended queue implementation implemented as a ring buffer rathher than the more common linked list of array blocks.
- _sprawl::collections::ForwardList_ - A simple singly-linked list
- _sprawl::collections::HashMap_ - A hash map implementation that allows data to be accessed by multiple keys, and allows the keys to be specified as being part of the data so it doesn't require redundant memory storage for keys it can look up on the data itself. Determination of where to look for keys and which key to use are handled at compile time; in the event the compiler can't distinguish which key to use by type alone, an integer template parameter can be passed to resolve the ambiguity. Also guarantees the property that, when iterating data, the data will be iterated in the same order it was inserted.
- _sprawl::collections::List_ - A simple doubly-linked list
- _sprawl::collections::Vector_ - A simple vector implementation

*libSprawl::filesystem*
Implements various path and filesystem operations in a cross-platform way. The interface is heavily inspired by the Python os and os.path libraries and contains a large portion of their functionality.

*libSprawl::hash*
Contains implementations of Murmur3 and CityHash algorithms. Stub headers for Murmur2 and FNV1a are not implemented yet.

*libSprawl::if*
Template metaprogramming helper classes that enable a proper type to be determined at compile time following the if/elseif/else/endif pattern. See UnitTests_If.cpp for examples.

*libSprawl::logging*
Logging system. Currently a bit of a work in progress; everything in it is functional but there are a number of performance enhancements that have been identified but not yet implemented. Also includes functionality to retrieve backtraces in a cross-platform way.

*libSprawl::memory*
Contains a lock-free thread-local pool allocator, a wrapper to enable it to be used with STL types, and an OpaqueType class that can create properly-aligned memory based on either a specified size or a list of types to support the pImpl idiom without requiring any dynamic memory allocation.

*libSprawl::network*
Contains two networking implementations:
- _sprawl::network_ - Deprecated. Do not use. Will be deleted eventually.
- _sprawl::async_network_ - Asynchronous TCP and UDP networking libraries. Functional, but have been earmarked for a number of performance and functionality improvements.

*libSprawl::serialization*
Set of serializers and deserializers connected by a unified interface:
- _sprawl::serialization::BinarySerializer_ - Serializes to and from a custom binary format
- _sprawl::serialization::JSONSerializer_ - Serializes to and from JSON
- _sprawl::serialization::MongoSerializer_ - Serializes to and from MongoDB BSON format
- _sprawl::serialization::ReplicableSerializer_ - Using one of the other serializers as a backend, calculates deltas of objects as they change, and serializes into a format that can be interpreted by a ReplicableDeserializer at the receiving end to apply the changes
- _sprawl::serialization::MongoReplicableSerializer_ - A ReplicableSerializer that can build its deltas in a format appropriate for applying an update to a MongoDB document

- _sprawl::serialization::JSONToken_ - A fast and fully-functional JSON builder and parser with a user-friendly interface. Relies on the user to manage the source string's lifetime when reading.

*libSprawl::string*
String classes:
- _sprawl::String_ - A reference-counted immutable string class with small value optimization. The reference counting is likely going to be removed in the future.
- _sprawl::StringBuilder_ - A class very similar to std::stringstream, optimized for creating strings and capable of converting from other types to strings.
- _sprawl::StringLiteral_ - A wrapper around a raw c string pointer and a size, will not make any copies or perform any dynamic memory allocation, but relies on the user to manage the source string's lifetime. A sprawl::String constructed from a StringLiteral will likewise continue to reference the original source string without doing any copies.

*libSprawl::tag*
A compile-time string library that treats individual strings as discrete types, where "Hello" and "World" are thus different actual types at compile time, and as such can be used as template parameters. Supports almost the entire python string library's functionality, all executed at compile time and transmuting tag types into new types representing the results of the operation. The resulting type can then have its contents and size accessed at runtime.

In addition to the functionality in the python library, also supports compile-time Murmur3 hash calculation of strings. It can also stringify types without any RTTI and can convert from strings into integers and booleans.

See UnitTests_Tag.cpp for examples.

*libSprawl::threading*
Various threading support libraries:
- _sprawl::threading::ConditionVariable_ - Basic condition variable
- _sprawl::threading::Coroutine_ - Cross-platform implementation of a continuation/fiber to support generators and cooperative multitasking models
- _sprwal::threading::Event_ - Similar to a condition variable, but uses kernel objects such as Event and eventfd to avoid the requirement to use an accompanying mutex and boolean. Unlike a condition variable, this will not return until it is set and it will catch sets that happen while it's not waiting. Also unlike a condition variable, multiple Event objects can be waited on at once; however, Event only supports one reader thread. With multiple reader threads, the number of threads that will see the event is undefined.
- _sprawl::threading::Mutex_ - Basic mutex implementation
- _sprawl::threading::Thread_ - Basic thread implementation. Contrary to std::thread, this thread does not start until you call Start(), and exceptions thrown in the thread can be caught and rethrown in Join()
- _sprawl::threading::ThreadLocal_ - Basic thread local variable. Obsoleted by C++11 thread_local.
- _sprawl::threading::ThreadManager_ - A complex thread pool implementation that allows each thread to be given a set of flags for what types of tasks it can execute. A task will only be executed by a thread that's marked as capable of executing it, but a thread can be given the ability to execute multiple types of tasks. Uses _sprawl::collections::ConcurrentQueue_ for quick wait-free enqueuing of tasks.

*libSprawl::time*
Simple nanosecond-resolution cross-platform steady and system clocks, and resolution conversion utilities.

*Misc*
Sprawl has a unique error-handling system when exceptions are disabled. Functions that can throw exceptions have a return type of ErrorState<T>. When exceptions are enabled, ErrorState<T> is an alias of T. When exceptions are not enabled, ErrorState<T> is an opaque type that may contain T or may contain an exception. In this case, it provides functionality for checking if there was an error and handling it before retrieving the underlying data; in debug builds, or with SPRAWL_ERRORSTATE_STRICT defined (and SPRAWL_ERRORSTATE_PERMISSIVE not defined), if you attempt to read T from it while it contains an error, it will print a message and abort; in release builds or with SPRAWL_ERRORSTATE_STRICT defined (and SPRAWL_ERRORSTATE_PERMISSIVE not defined), it will return garbage memory.