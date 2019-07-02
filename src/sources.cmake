set (LIBNDSL_SRC_PREFIX ${libndsl_SOURCE_DIR}/src/ndsl)

# coroutine
set(COROUTINE_SRC ${LIBNDSL_SRC_PREFIX}/coroutine/platform/coroutine_gnu_x86_64.s
                  ${LIBNDSL_SRC_PREFIX}/coroutine/Coroutine.cc)

# framework
set(FRAMEWORK_SRC ${LIBNDSL_SRC_PREFIX}/framework/EventLoop.cc
                  ${LIBNDSL_SRC_PREFIX}/framework/Event.cc
                  ${LIBNDSL_SRC_PREFIX}/framework/Service.cc)

# net
set(NET_SRC ${LIBNDSL_SRC_PREFIX}/net/ipv4/tcp/tcp_hook.cc
            ${LIBNDSL_SRC_PREFIX}/net/ipv4/tcp/TCPConnection.cc
            ${LIBNDSL_SRC_PREFIX}/net/ipv4/tcp/TCPListener.cc
            ${LIBNDSL_SRC_PREFIX}/net/ipv4/EndPoint.cc
            ${LIBNDSL_SRC_PREFIX}/net/helper.cc)

set(LIBNDSL_SRC ${COROUTINE_SRC} ${FRAMEWORK_SRC} ${NET_SRC})