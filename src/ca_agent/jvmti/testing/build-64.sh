g++ -D_INLINEINFO_ -c -I/root/jdk/jdk1.7.0/include -I/root/jdk/jdk1.7.0/include/linux -I/root/ca_agent_inlining/64bit/ca_agent_inlining/jvmti /root/ca_agent_inlining/64bit/ca_agent_inlining/jvmti/io.cpp jncread.cpp
g++ -o jncread /usr/lib64/libbfd-2.17.50.0.6-9.el5.so io.o jncread.o

