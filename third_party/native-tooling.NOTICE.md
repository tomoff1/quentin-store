# Native PS5 application tooling

Source: https://github.com/blackbearreloaded/ps5-native-app-boilerplate
Commit: b8dc4a3fe2e05e826b037bd68824751c7468a6d7
Copyright (C) 2026 BlackBearReloaded
License: GPL-3.0-or-later

The complete corresponding source, including application CRT, C++ allocation
bridge, diagnostic renderer, LLVM-to-SCE converter, FSELF writer and runtime
generator, is included as:

`sources/ps5-native-app-boilerplate-b8dc4a3fe2e05e826b037bd68824751c7468a6d7.zip`

Archive SHA-256:
`35c10f586bc3a2e80526920f059a851007a8e4c709915a68ee4c1d0b28c47ca8`

The source-authored `sce_module/libc.prx` is generated from the tracked
semantic manifests, reproduced twice and checked against the upstream
recorded digest. It does not contain Sony's implementation code.
Signed runtime SHA-256:
`e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036`

The host FSELF inspector uses zlib, whose license is separately provided in
`zlib.LICENSE.txt`. The native launcher uses the public PS5 Payload SDK headers
and import stubs; these are supplied and documented by the main project.
