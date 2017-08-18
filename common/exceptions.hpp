/*
 * Exceptions defined using this template in errors.hpp
 *
 * This template defines the data in the sprawl::ExceptionId enum, the associated string output for enum value, and an exception class that derives from
 * the class generated for another enum value. It also provides a typedef for each value to make it easier for users to handle the exceptions.
 */
//                Base Class           Specific Error                Error String                                           Typedef                   //
SPRAWL_EXCEPTION( GENERAL_EXCEPTION,   UNIMPLEMENTED_VIRTUAL_METHOD, "The method called is not implemented on this object", UnimplementedVirtualMethod )

//                Base Class           Specific Error                Error String                                           Typedef                   //
SPRAWL_EXCEPTION( GENERAL_EXCEPTION,   SERIALIZATION_ERROR,          "Serialization Error",                                 SerializationError         )
SPRAWL_EXCEPTION( SERIALIZATION_ERROR, INVALID_DATA,                 "Invalid input data",                                  InvalidData                )
SPRAWL_EXCEPTION( SERIALIZATION_ERROR, SERIALIZATION_OVERFLOW,       "Serialization overflow",                              SerializationOverflow      )

//                Base Class           Specific Error                Error String                                           Typedef                   //
SPRAWL_EXCEPTION( SERIALIZATION_ERROR, JSON_ERROR,                   "JSON Error",                                          JsonError                  )
SPRAWL_EXCEPTION( JSON_ERROR,          JSON_TYPE_MISMATCH,           "Requested field does not match requested type",       JsonTypeMismatch           )
SPRAWL_EXCEPTION( JSON_ERROR,          INVALID_JSON_DATA,            "Input data is not valid JSON",                        InvalidJsonData            )

//                Base Class           Specific Error                Error String                                           Typedef                   //
SPRAWL_EXCEPTION( GENERAL_EXCEPTION,   COROUTINE_EXCEPTION,          "Coroutine Error",                                     CoroutineError             )
SPRAWL_EXCEPTION( COROUTINE_EXCEPTION, INVALID_COROUTINE_TYPE,       "Yield/Receive called on coroutine whose type did"      
                                                                             " not match the expected type.",               InvalidCoroutineType       )
SPRAWL_EXCEPTION( COROUTINE_EXCEPTION, INVALID_SEND_TYPE,            "Yield/Receive called on coroutine whose SendType"  
                                                                             " did not match the passed type in size.",     InvalidSendType            )
SPRAWL_EXCEPTION( COROUTINE_EXCEPTION, INVALID_YIELD_TYPE,           "Yield/Receive called on coroutine whose YieldType" 
                                                                             " did not match the passed type in size.",     InvalidYieldType           )

//                Base Class           Specific Error                Error String                                           Typedef                   //
SPRAWL_EXCEPTION( GENERAL_EXCEPTION,   CONTAINER_EXCEPTION,          "Container Error",                                     ContainerError             )
SPRAWL_EXCEPTION( CONTAINER_EXCEPTION, OUT_OF_RANGE,                 "Attempted to retrieve a value outside the allocated"
																			 " range of the container.",                    OutOfRangeError            )

