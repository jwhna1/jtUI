# Contributing

Welcome to collaborate on this repository as a long-evolving runtime project.

The collaboration principles at this stage are simple:

1. Stabilize module boundaries first, then deepen functionality.
2. When adding public APIs, prioritize the long-term stability of `C++20` and the future `C ABI`.
3. The platform layer, rendering layer, and widget layer must not leak into each other.
4. Update the corresponding documentation **before** making large structural changes.

Before submitting a change, please at least run:

- `cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++`
- `cmake --build build`

If your change introduces a new system library or third-party dependency, please also update `README.md` and the relevant files under `docs/`.

---

# 贡献指南

欢迎把这个仓库当成一个长期演进的 runtime 项目来协作。

当前阶段的协作原则很简单：

1. 先稳住模块边界，再补功能深度。
2. 新增公共 API 时，优先考虑 `C++20` 和未来 `C ABI` 的长期稳定性。
3. 平台层、渲染层和 widget 层不要相互穿透。
4. 大改结构前，先更新对应文档。

提交前建议至少完成：

- `cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++`
- `cmake --build build`

如果你的改动依赖新的系统库或第三方依赖，请同时更新 `README.md` 和相关 `docs/` 说明。
