# Minicoro

Repository: https://github.com/edubart/minicoro
Commit: `02dad0f8b7cbb12fe6e216ae7a76db15ca55cd7b`
License: MIT, retained in `LICENSE` and in the upstream header.

Unmodified `minicoro.h` SHA-256:
`c4205e8db0a95456dfde9f73f071609c6d2cad2ebfd1d74ed0a9254f121caa2f`

The native frame backend uses compiled stack-switch assembly. It requires no
JIT, executable-memory allocation or OS worker thread for the game loop.
Windows game builds retain their existing Windows fiber backend.
