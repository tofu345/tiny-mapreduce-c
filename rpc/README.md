# Remote Procedure Call Server

**TODO:
https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly**

non-goals:

- dynamic binding (finding the server on the remote machine at runtime)
- authentication

goals:

- a very basic packing/serialization/marshalling toolbox

## Serialization

For my use-case, I need to be able to pack and unpack:

- [x] uint32_t
- [x] uint64_t
- [x] enums (may be 8 bits or more)
- [x] strings (with 16 bit length prepended)
- [x] time (assumed to be uint64_t, _fight me_)
- [x] arrays of structs of the the following types (with 32 bit array length
      prepended). the server and client will have to agree, on what number to
      treat as the length.

## Protocol

The caller sends:

```
RPC[procnum][arguments size][arguments]
```

where: procnum and num_arguments are unsigned 32 bit numbers.

And waits for a response with an unsigned 32 bit length prepended.
