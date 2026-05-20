#pragma once

// jtUI event 数据已下沉到 hui::object 层（消除 Widget 反向依赖 hui::event 造成的循环）。
// 本头保留为转发头，旧 include 路径 `hui/event/events.hpp` 继续可用。
// 维护：jtai 团队（曾能混 <jwhna1@gmail.com>）

#include "hui/object/event.hpp"
