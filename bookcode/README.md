# Learn Makefile and Commands

## .lst 文件
- 命令执行流程，对应会执行 `make pmemkv` 和 `make listing`
  - `make pmemkv` 对应编译源代码，并依赖库
  - `make listing` 对应执行 make pmemkv.lst
    - 对应执行 `expand -t 4 < $^ | cat -n > $@`
      - 其中 `$^` 代表的是所有依赖文件列表，使用空格分割；`$@` 代表的是目标文件。`expand -t 4 < pmemkv.cpp | cat -n > pmemkv.lst`
      - `expand -t 4` 指定制表符所代表的空白字符个数为 4
      - 其实就是将代码转换了一下，生成了 .lst 文件。
```makefile
all: pmemkv listings

listings: pmemkv.lst

%.lst: %.cpp
	expand -t 4 < $^ | cat -n > $@

pmemkv: pmemkv.cpp
	$(CXX) -o pmemkv pmemkv.cpp -lpmemkv

clean:
	$(RM) *.o core a.out

clobber: clean
	$(RM) pmemkv *.lst

.PHONY: all clean clobber listings
```