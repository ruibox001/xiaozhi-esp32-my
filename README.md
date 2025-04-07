
xz

webrtc报错配置
1、libpeer编译报错
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-error=incompatible-pointer-types")

2、menuconfig
搜索all versions -> mbedTLS v3.x related  ->  选中：Support DTLS protocol (all versions) -> DTLS-based configurations

3、类型强制转换
g_ps.transport.recv = (TransportRecv_t)ssl_transport_recv;
g_ps.transport.send = (TransportSend_t)ssl_transport_send;

4、编译报错-format
在main的CMakeLists.txt中加入
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")