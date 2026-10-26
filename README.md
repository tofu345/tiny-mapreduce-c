An implementation of [Dean & Ghemawat, "MapReduce: Simplified Data Processing
on Large Clusters"](https://dl.acm.org/doi/10.1145/1327452.1327492) following
this video [building a distributed mapreduce from
scratch](https://www.youtube.com/watch?v=svtj0nNxKGE&t=3731s)
[https://github.com/tajpouria/tiny-mapreduce].

Designed to be run only on a single machine (for now) due to communication over
UNIX domain sockets, with a very tiny RPC protocol over TCP with serialization
modelled after [Beej's Guide to Network
Programming](https://beej.us/guide/bgnet/html/split/slightly-advanced-techniques.html#serialization)
