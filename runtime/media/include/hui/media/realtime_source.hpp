#pragma once

// RealtimeSource<T> / RealtimeRingBuffer<T> / LatestValue<T> 已下沉到 hui::property
// （高频数据通道是通用基础设施，不只媒体用）。本头保留为转发头，旧 include 路径
// `hui/media/realtime_source.hpp` 继续可用。
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

#include "hui/property/realtime_source.hpp"
