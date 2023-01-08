# test1

1. the host opens a tcp port for listening
2. the host starts the service process via std::system
3. the service get the port via arg and connects back
4. the host starts polling via rpc (zpp_bits serialization and function dispatcher)
5. the service serves rpc

### example log output:
```
$ bin/test1_host
H: initialized zed_net
H: listening on 1337
H: started service
S: initialized zed_net
H: got connection from 127.0.0.1:56253
S: got 4 bytes
S: called poll
H: got 15 bytes
H: has_value
A 'haha'
S: got 4 bytes
S: called poll
H: got 15 bytes
H: has_value
A 'haha'
S: got 4 bytes
S: called poll
H: got 15 bytes
H: has_value
A 'haha'
```

