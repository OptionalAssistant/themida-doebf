# 🛠️ Deobfuscation Tool

A tool for emulating and analyzing obfuscated functions inside a binary file. It maps the binary into memory, sets up a virtual CPU environment, and executes the obfuscated code starting from a specified RVA.

## 🚀 Usage

```bash
program -b <binary_path> -o <log_path> -rva <rva> -sectionStart <start_addr> -sectionSize <size>
✅ Required arguments:
Argument	Description
-b <path>	Path to the binary file (e.g., .exe, .dll)
-o <path>	Path to the output log file (e.g., D:\log.txt)
-rva <rva>	RVA of the first instruction of the obfuscated function. Supports decimal or hexadecimal formats
-sectionStart <addr>	RVA of the beginning of the obfuscated section. Supports decimal or hexadecimal formats
-sectionSize <size>	Size of the obfuscated section in bytes. Supports decimal or hexadecimal formats

🆘 Optional:
Argument	Description
-help	Show help message
```

⚠️ Important
The -rva argument must point to the first instruction of the obfuscated function inside the obfuscated section.

All numeric arguments (-rva, -sectionStart, -sectionSize) can be specified as decimal (e.g., 1234) or hexadecimal (e.g., 0x4D2).

📄 Output
The emulator will execute the obfuscated function starting from the specified RVA and log execution details to the file specified with -o.

## 🚀 Building
Supported IDE: MSVS2019 and up
The tool uses [zasm framework](https://github.com/zyantific/zasm) (x86-64 Assembler based on Zydis), so please setup env var named `ZASM` that points to your local copy of the repo
