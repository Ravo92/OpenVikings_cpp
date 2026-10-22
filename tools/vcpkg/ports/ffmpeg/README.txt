OpenVikings Bink-only FFmpeg overlay
===================================

This directory snapshots the FFmpeg port from microsoft/vcpkg commit
1460b31b08c42cc2e9ac2c79f45ec8707e2675e2 (FFmpeg 8.1.2).
The imported port files retain their upstream names and MIT license
(LICENSE.vcpkg). The FFmpeg source is fetched and hash-checked by that port
and retains its own LGPL license.

Local changes to the upstream port:
- portfile.cmake includes openvikings-bink.cmake immediately before its
  final Building Options message, after all platform/feature options.
- openvikings-bink.cmake restricts the package to the libraries and codecs
  used by OpenVikings. No upstream patches or source hashes were changed.

Enabled media components:
- Decoder: bink, binkaudio_dct, binkaudio_rdft
- Demuxer: bink
- Protocol: file
- Libraries: avcodec, avformat, avutil, swresample, swscale

All media components start disabled; only the above are re-enabled.
Encoders, muxers, unrelated decoders/demuxers/parsers/bitstream filters,
network protocols, devices, filters, command-line programs and hardware
video backends are excluded. FFmpeg selects the required DSP/transform
helpers itself. Size optimization is enabled; CPU optimizations remain
available. The game's existing D3D11 renderer and Windows PCM output
remain independent of FFmpeg hardware backends.

The root vcpkg-configuration.json selects this overlay. Win32 and x64 use
their existing static-library/dynamic-CRT triplets. The overlay changes
the package ABI, so vcpkg replaces the former broad package on the next
build/restore. MSBuild also tracks these files as dependency-restore
inputs; changing the profile must not reuse a stale restore stamp.

Do not edit files beneath vcpkg_installed: those are generated caches.
When updating the upstream baseline, refresh the entire port snapshot,
retain the profile include, and review the configure options/dependencies.
