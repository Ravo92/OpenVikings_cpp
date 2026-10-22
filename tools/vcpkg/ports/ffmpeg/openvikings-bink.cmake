# OpenVikings uses FFmpeg only for native Windows Bink 1 playback.
# Apply this last, after the upstream port's feature/platform switches.
if(NOT VCPKG_TARGET_IS_WINDOWS OR VCPKG_TARGET_IS_UWP OR NOT VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    message(FATAL_ERROR "OpenVikings' FFmpeg overlay requires a static Windows desktop triplet.")
endif()

set(openvikings_ffmpeg_features core avcodec avformat swresample swscale)
foreach(feature IN LISTS FEATURES)
    if(NOT feature IN_LIST openvikings_ffmpeg_features)
        message(FATAL_ERROR "Unsupported feature '${feature}' in OpenVikings' Bink-only FFmpeg overlay.")
    endif()
endforeach()
foreach(feature avcodec avformat swresample swscale)
    if(NOT feature IN_LIST FEATURES)
        message(FATAL_ERROR "OpenVikings' Bink-only FFmpeg overlay requires '${feature}'.")
    endif()
endforeach()

string(APPEND OPTIONS " --disable-everything --disable-programs --disable-network --disable-avdevice --disable-avfilter --disable-mediafoundation --disable-d3d11va --disable-d3d12va --disable-dxva2 --disable-vulkan --enable-small --enable-decoder=bink,binkaudio_dct,binkaudio_rdft --enable-demuxer=bink --enable-protocol=file")
