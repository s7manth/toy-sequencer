find_package(Protobuf QUIET REQUIRED)

if (Protobuf_FOUND)
  message(STATUS "Protobuf found: ${Protobuf_VERSION}")
  set(PROTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/messages.proto)
  set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/src/generated)

  file(MAKE_DIRECTORY ${GENERATED_DIR})

  set(GENERATED_CC ${GENERATED_DIR}/messages.pb.cc)
  set(GENERATED_H ${GENERATED_DIR}/messages.pb.h)

  add_custom_command(
    OUTPUT ${GENERATED_CC} ${GENERATED_H}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            --cpp_out=${GENERATED_DIR}
            ${PROTO_SRC}
    DEPENDS ${PROTO_SRC}
    COMMENT "Generating protobuf C++ sources in ${GENERATED_DIR}"
    VERBATIM)

  # Create a custom target for generation, then depend on it from the library
  add_custom_target(generate_msg_protos DEPENDS ${GENERATED_CC} ${GENERATED_H})

  add_library(msg_protos STATIC ${GENERATED_CC})
  add_dependencies(msg_protos generate_msg_protos)
  target_include_directories(msg_protos PUBLIC ${Protobuf_INCLUDE_DIRS} ${GENERATED_DIR})
  # Link against imported protobuf target to pull Abseil deps transitively
  if(TARGET protobuf::libprotobuf)
    target_link_libraries(msg_protos PUBLIC protobuf::libprotobuf)
  else()
    # Fallback for older CMake/protobuf packages
    target_link_libraries(msg_protos PUBLIC ${Protobuf_LIBRARIES})
  endif()

  # On some systems, protobuf does not propagate Abseil dependencies.
  # Try to link Abseil log internals explicitly if available to satisfy
  # symbols like absl::log_internal::LogMessage* and MakeCheckOpString.
  find_package(absl CONFIG QUIET)
  if(absl_FOUND)
    foreach(absl_lib IN ITEMS log_internal log_internal_check_op raw_logging_internal strings base)
      if(TARGET absl::${absl_lib})
        target_link_libraries(msg_protos PUBLIC absl::${absl_lib})
      endif()
    endforeach()
  endif()
  target_compile_definitions(msg_protos PUBLIC HAS_PROTOBUF=1)
  target_compile_features(msg_protos PUBLIC cxx_std_20)

  # Provide an interface target if consumers only need includes
  add_library(msg_protos_includes INTERFACE)
  target_include_directories(msg_protos_includes INTERFACE ${GENERATED_DIR})
else()
  message(WARNING "Protobuf not found. Generated message types will be unavailable.")
endif()
