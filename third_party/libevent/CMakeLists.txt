# Copyright 2024 The Chromium Authors
# CMake build for libevent.

if(WIN32)
  # libevent is only used by base/message_loop/message_pump_libevent.cc,
  # which is excluded from the minimal CMake build. On Windows, the
  # libevent sources require a Windows-specific config.h and win32.c
  # (from WIN32-Code/) which are not present in this stripped-down
  # third_party snapshot. Provide an empty stub library so the link
  # dependency resolves cleanly without pulling in platform-specific
  # code that is never called.
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libevent_stub.c
    "/* libevent stub — not used in minimal base build (message_loop excluded). */\n"
    "/* intentionally empty */\n")
  add_library(libevent STATIC ${CMAKE_CURRENT_BINARY_DIR}/libevent_stub.c)
  target_include_directories(libevent PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/..
  )
  set_target_properties(libevent PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 99
  )
else()
  # Linux/epoll configuration.
  # Put the per-platform config.h into the build dir so the source tree stays
  # pristine. libevent sources include "config.h" with no path prefix, so the
  # build dir must appear BEFORE the source dir on the include path.
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/linux/config.h
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/linux/event-config.h
       DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

  add_library(libevent STATIC
    buffer.c
    evbuffer.c
    evdns.c
    event.c
    event_tagging.c
    evrpc.c
    evutil.c
    http.c
    log.c
    poll.c
    select.c
    signal.c
    strlcpy.c
    epoll.c
  )

  target_include_directories(libevent PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/..
  )

  set_target_properties(libevent PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 99
  )

  target_compile_options(libevent PRIVATE -w)
endif()
