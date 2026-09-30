# 项目长期约定

- **内核模块开发（kernel-module/）默认只做内核态路径**：
  - 不写用户态兼容分支（如 pu_port.h 的 `#else /* 用户态 */` 部分）
  - 不做用户态 gcc 编译/链接/运行验证
  - Makefile 以 kbuild（`make -C <ksrc> M=$(pwd) modules`）为唯一主路径
  - 仅当用户明确要求时才添加用户态支持或用户态编译验证
- 只允许修改 kernel-module/ 下的文件；workflow/scripts 等其他配置只读不改（除非用户明确要求）
- 结构性调整前先确认意图，不要把"优化"变成重组
- 验证方式：改动后直接让用户推 CI 跑内核构建（CONFIG_WERROR=y，需保证零告警：空参数原型写 `(void)`、unused 用 PU_UNUSED、查表失败必须处理未初始化变量）
