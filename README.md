# DLP
## 环境配置
1.Capstone -next branch
2.Toolchain -rv32i（如果有现成.riscv文件可以不需要）

## 运行设置

### 代码修改：
    需要修改.riscv文件地址（目前写死在src/Getbinarycode.cpp testcode函数中，后续修改为接收参数）

### 编译：
    $ g++ *.cpp -lcapstone
