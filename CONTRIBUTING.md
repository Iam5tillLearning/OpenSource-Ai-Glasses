# 贡献指南

[English](#contributing-guide-english) | [中文](#贡献指南-中文)

---

## 贡献指南 (中文)

感谢您对AI智能眼镜开源项目的关注！我们欢迎各种形式的贡献。

### 🤝 如何贡献

#### 1. 报告问题
- 在提交issue前，请先搜索现有issue避免重复
- 使用清晰的标题描述问题
- 提供详细的复现步骤
- 附上相关的日志、截图或视频

#### 2. 提交代码
1. Fork本仓库到您的账号下
2. 克隆您fork的仓库到本地
3. 创建新的特性分支：`git checkout -b feature/your-feature-name`
4. 进行您的修改
5. 提交更改：`git commit -m "描述您的更改"`
6. 推送到您的fork：`git push origin feature/your-feature-name`
7. 创建Pull Request到主仓库

#### 3. 改进文档
文档改进同样重要！您可以：
- 修正文档中的错误
- 添加使用示例
- 完善API说明
- 翻译文档

### 🛠️ 开发环境

开发环境唯一获取方式：
- https://github.com/makevary/AIGLASS_DEV_ENV

#### 固件开发
需要瑞芯微RV1106B SDK环境，详见：
- [开发环境搭建指南](docs/ENV_SETUP.md)
- [固件烧录指南](docs/FIRMWARE_FLASHING.md)

#### 应用开发
使用交叉编译工具链，详见：
- [应用开发指南](docs/APPLICATION_DEVELOPMENT.md)

### 📝 代码规范

#### C/C++代码
- 遵循K&R代码风格
- 使用有意义的变量名和函数名
- 添加必要的注释说明复杂逻辑
- 避免使用魔数，使用宏定义常量

#### 提交信息规范
使用[Conventional Commits](https://www.conventionalcommits.org/)格式：

```
<类型>: <简短描述>

[可选的详细描述]

[可选的脚注]
```

类型包括：
- `feat`: 新功能
- `fix`: 修复bug
- `docs`: 文档更新
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建/工具链相关

示例：
```
feat: 添加WiFi自动连接功能

- 实现WiFi配置保存
- 添加开机自动连接
- 支持多个WiFi热点

Closes #123
```

### 🔍 Pull Request流程

1. 确保您的代码遵循项目的代码规范
2. 如果添加新功能，请更新相关文档
3. 确保不会破坏现有功能
4. PR标题清晰描述改动内容
5. 在PR描述中说明：
   - 改动的目的
   - 解决的问题
   - 测试方法

### 📚 开发领域

欢迎在以下领域贡献：

#### 🐛 Bug修复
- 修复已知问题
- 提高系统稳定性

#### ✨ 新功能
- 硬件驱动优化
- 新的应用场景实现
- 性能优化

#### 📖 文档
- 完善API文档
- 添加教程和示例
- 改进说明文档

#### 🌐 国际化
- 翻译文档为其他语言
- 添加多语言支持

### ⚠️ 注意事项

1. **不要提交**：
   - 编译产物（.o, .so, .img等）
   - IDE配置文件（除非是项目共用的）
   - 个人配置信息
   - 大型二进制文件

2. **安全性**：
   - 不要在代码中硬编码密码、密钥
   - 注意SQL注入、XSS等安全问题
   - 谨慎处理用户输入

3. **许可证**：
   - 确保您的贡献与项目许可证（Apache 2.0）兼容
   - 不要复制有版权限制的代码

### 💬 讨论与交流

- **GitHub Issues**: 报告bug、提出功能建议
- **GitHub Discussions**: 技术讨论、使用交流
- **Email**: iam5stilllearning@foxmail.com

### 🙏 致谢

感谢所有为本项目做出贡献的开发者！

---

## Contributing Guide (English)

Thank you for your interest in contributing to the AI Smart Glasses open-source project!

### 🤝 How to Contribute

#### 1. Reporting Issues
- Search existing issues before creating a new one
- Use a clear and descriptive title
- Provide detailed reproduction steps
- Include relevant logs, screenshots, or videos

#### 2. Contributing Code
1. Fork the repository
2. Clone your fork locally
3. Create a feature branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Commit: `git commit -m "Describe your changes"`
6. Push: `git push origin feature/your-feature-name`
7. Create a Pull Request

#### 3. Improving Documentation
Documentation improvements are valuable! You can:
- Fix documentation errors
- Add usage examples
- Improve API documentation
- Translate documentation

### 🛠️ Development Environment

Single source for development environment:
- https://github.com/makevary/AIGLASS_DEV_ENV

#### Firmware Development
Requires Rockchip RV1106B SDK environment. See:
- [Development Environment Setup Guide](docs/ENV_SETUP.en.md)
- [Firmware Flashing Guide](docs/FIRMWARE_FLASHING.en.md)

#### Application Development
Uses cross-compilation toolchain. See:
- [Application Development Guide](docs/APPLICATION_DEVELOPMENT.en.md)

### 📝 Code Standards

#### C/C++ Code
- Follow K&R code style
- Use meaningful variable and function names
- Add necessary comments for complex logic
- Use macro definitions instead of magic numbers

#### Commit Message Format
Follow [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>: <short description>

[optional detailed description]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation update
- `refactor`: Code refactoring
- `test`: Test-related
- `chore`: Build/toolchain related

Example:
```
feat: add WiFi auto-connect feature

- Implement WiFi configuration save
- Add auto-connect on boot
- Support multiple WiFi hotspots

Closes #123
```

### 🔍 Pull Request Process

1. Ensure your code follows project standards
2. Update relevant documentation for new features
3. Ensure existing functionality is not broken
4. Use a clear PR title
5. In PR description, explain:
   - Purpose of changes
   - Problem solved
   - Testing methodology

### 📚 Contribution Areas

We welcome contributions in:

#### 🐛 Bug Fixes
- Fix known issues
- Improve system stability

#### ✨ New Features
- Hardware driver optimization
- New use case implementations
- Performance improvements

#### 📖 Documentation
- Complete API documentation
- Add tutorials and examples
- Improve guides

#### 🌐 Internationalization
- Translate documentation
- Add multi-language support

### ⚠️ Important Notes

1. **Do not commit**:
   - Build artifacts (.o, .so, .img, etc.)
   - IDE configuration files (unless shared)
   - Personal configuration
   - Large binary files

2. **Security**:
   - No hardcoded passwords or keys
   - Watch for SQL injection, XSS, etc.
   - Handle user input carefully

3. **License**:
   - Ensure compatibility with Apache 2.0 license
   - Do not copy copyrighted code

### 💬 Communication

- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: Technical discussions
- **Email**: iam5stilllearning@foxmail.com

### 🙏 Acknowledgments

Thanks to all contributors to this project!
