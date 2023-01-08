# test1

the host opens a tcp port for listening
the host starts the service process via std::system
the service get the port via arg and connects back
the host starts polling via rpc (zpp_bits serialization and function dispatcher)
the service serves rpc

