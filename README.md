# UNIX Web Proxy Server

## Build the server

```shell
make clean
make 
```

## Run the server

```shell
./server [port]
```

`port`: port number to bind the server at. If it is not provided, it will be `3918` by default.

## Features

- multi-threading
- HTTP forwarding support
- HTTPS forwarding support
