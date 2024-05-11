# kallsyms

## Introduction

Linux Kernel에서는 문제 발생 지점에 대한 정보를 제공하기 위해, oops 또는 panic이 발생했을 때 각 레지스터에 들어있는 값과 call trace 등 다양한 정보를 제공합니다.
예를 들어 아래와 같이 stack이 구성되어 있고, B 실행 중에 panic이 발생했다고 가정해봅시다.

```
+-------------------------------+
|   B에서 사용하는 지역 변수들    |
|                               |
+-------------------------------+
|            B의 SFP            |
+-------------------------------+
|            B의 RET            |
+-------------------------------+
|   A에서 사용하는 지역 변수들    |
|                               |
+-------------------------------+
|            A의 SFP            |
+-------------------------------+
|            A의 RET            |
+-------------------------------+
|  main에서 사용하는 지역 변수들  |
|                               |
+-------------------------------+
|          main의 SFP           |
+-------------------------------+
|          main의 RET           |
+-------------------------------+
```

oops 혹은 panic이 발생하면, 여러가지 fault 처리 함수를 거쳐 최종적으로 register 상태와 call trace를 출력하는 `show_trace_log_lvl()` 함수가 호출됩니다. 그리고 `show_trace_log_lvl()` 함수는 call trace를 복원하기 위해 stack frame에 맞추어 파싱합니다. 

`show_trace_log_lvl()`에서 call trace를 구하는 방법을 더 자세히 알아봅시다. x86-64 architecture에서 stack frame은 아래와 같이 saved rbp(a.k.a. stack frame pointer)가 linked list처럼 연결되어 있고, 함수가 종료되고 돌아갈 return address는 saved rbp 바로 아래에 위치합니다.

![alt text](../.images/stack-frame.png)

[1] 

따라서 oops 혹은 panic이 발생한 함수의 rbp 부터 차례대로 stack을 순회하면 각 함수의 return address를 알 수 있게 되고, 따라서 call trace도 복원할 수 있게 됩니다. 그런데 stack에는 return address만 존재하지 그 주소가 어떤 함수의 어떤 부분인지에 대한 정보는 전혀 나와있지 않습니다. 하지만 linux kernel에서는 oops 혹은 panic이 발생할 때 call trace가 아래와 같이 주소만 출력하는 것이 아닌 주소에 해당하는 함수까지 출력됩니다. 어떻게 이 주소에 해당하는 함수를 알아낸 것일까요?
```
Call Trace:
 [<ffffffff8100dead>] panic+0xdead
 [<ffffffff8100bbbb>] B+0xbb
 [<ffffffff8100aaaa>] A+0xaa
 [<ffffffff81001234>] main+0x11
```

정답은 바로 kallsyms를 이용하는 것입니다. kallsyms는 커널의 symbol 정보를 압축해서 저장하며, 함수 뿐만 아니라 전역변수(`CONFIG_KALLSYMS_ALL`이 활성화된 경우)애 대한 정보 또한 가지고 있습니다. kallsyms가 정확히 어떻게 동작하는지는 아래에서 더 자세히 알아보겠습니다.

## script/kallsyms.c

해당 파일 맨 위에 있는 주석에 대강의 설명이 나와 있습니다.

이 파일은 다른 kernel source들과 관계없이 stand-alone하게 컴파일할 수 있으며, symbol 정보를 담고있는 assembly 코드를 작성하는 역할을 합니다.

symbol 정보를 저장하는 과정에서 table compression을 사용하고, table compression은 symbol에서 가장 많이 사용되는 substring(이를 token이라고 함)을 symbol에서 사용하지 않는 char code에 매핑하는 방식으로 동작합니다.

예를 들어 "write_" 와 같이 자주 사용하는 token을 사용하지 않는 char code 0xf7에 매핑하는 것이죠. 

이를 통해 약 50% 가량의 압축률을 달성할 수 있었다고 합니다.

```c
/* Generate assembler source containing symbol information
 *
 * Copyright 2002       by Kai Germaschewski
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Usage: kallsyms [--all-symbols] [--absolute-percpu]
 *                         [--base-relative] [--lto-clang] in.map > out.S
 *
 *      Table compression uses all the unused char codes on the symbols and
 *  maps these to the most used substrings (tokens). For instance, it might
 *  map char code 0xF7 to represent "write_" and then in every symbol where
 *  "write_" appears it can be replaced by 0xF7, saving 5 bytes.
 *      The used codes themselves are also placed in the table so that the
 *  decompresion can work without "special cases".
 *      Applied to kernel symbols, this usually produces a compression ratio
 *  of about 50%.
 *
 */
```
[2]

그럼 이제 `main()` 함수부터 차근차근 더 자세히 분석해봅시다.

command line option들에 대한 파싱을 하고 여러 함수를 호출하는 것을 확인할 수 있습니다. 여러가지 함수가 호출되지만 중요한 함수 몇 가지만 자세히 알아보겠습니다.

```c
static void usage(void)
{
	fprintf(stderr, "Usage: kallsyms [--all-symbols] [--absolute-percpu] "
			"[--base-relative] [--lto-clang] in.map > out.S\n");
	exit(1);
}

int main(int argc, char **argv)
{
	while (1) {
		static const struct option long_options[] = {
			{"all-symbols",     no_argument, &all_symbols,     1},
			{"absolute-percpu", no_argument, &absolute_percpu, 1},
			{"base-relative",   no_argument, &base_relative,   1},
			{"lto-clang",       no_argument, &lto_clang,       1},
			{},
		};

		int c = getopt_long(argc, argv, "", long_options, NULL);

		if (c == -1)
			break;
		if (c != 0)
			usage();
	}

	if (optind >= argc)
		usage();

	read_map(argv[optind]);
	shrink_table();
	if (absolute_percpu)
		make_percpus_absolute();
	sort_symbols();
	if (base_relative)
		record_relative_base();
	optimize_token_table();
	write_src();

	return 0;
}
```

먼저 `read_map()`을 살펴봅시다.

`in` 에 해당하는 .map 파일을 열어서 `read_symbol()`을 이용해 `struct sym_entry` 형식으로 symbol을 읽어서 `table`에 순서대로 저장하고 있습니다.

```c
struct sym_entry {
	unsigned long long addr;
	unsigned int len;
	unsigned int seq;
	unsigned int start_pos;
	unsigned int percpu_absolute;
	unsigned char sym[];
};

static struct sym_entry **table;

static void read_map(const char *in)
{
	FILE *fp;
	struct sym_entry *sym;
	char *buf = NULL;
	size_t buflen = 0;

	fp = fopen(in, "r");
	if (!fp) {
		perror(in);
		exit(1);
	}

	while (!feof(fp)) {
		sym = read_symbol(fp, &buf, &buflen);
		if (!sym)
			continue;

		sym->start_pos = table_cnt;

		if (table_cnt >= table_size) {
			table_size += 10000;
			table = realloc(table, sizeof(*table) * table_size);
			if (!table) {
				fprintf(stderr, "out of memory\n");
				fclose(fp);
				exit (1);
			}
		}

		table[table_cnt++] = sym;
	}

	free(buf);
	fclose(fp);
}
```

`read_map()`의 입력으로 들어오는 .map 파일은 /boot/System.map-*을 찾아보면 쉽게 어떤 형식으로 이루어져 있는지 확인할 수 있습니다.
간단하게 .map 파일의 일부만 살펴보면, 아래와 같이 `<address> <type> <symbol name>`의 형식으로 이루어져 있는 것을 확인할 수 있습니다.
```
ffffffff81000000 T _stext
ffffffff81000000 T _text
ffffffff81000000 t pvh_start_xen
ffffffff81000080 T startup_64
ffffffff810000e0 T secondary_startup_64
ffffffff810000e5 T secondary_startup_64_no_verify
ffffffff81000270 t __pfx_verify_cpu
ffffffff81000280 t verify_cpu
ffffffff81000380 T __pfx_sev_verify_cbit
ffffffff81000390 T sev_verify_cbit
ffffffff810003f0 T soft_restart_cpu
ffffffff81000405 T vc_boot_ghcb
ffffffff81000470 T __pfx___startup_64
ffffffff81000480 T __startup_64
ffffffff81000ac0 T __pfx_startup_64_setup_env
ffffffff81000ad0 T startup_64_setup_env
ffffffff81000b50 t __pfx_set_mems_allowed
ffffffff81000b60 t set_mems_allowed
```

.map 형식에서 `struct sym_entry` 형식으로 변환해주는 `read_symbol()`을 더 자세히 살펴븝시다.

```c
struct sym_entry {
	unsigned long long addr;
	unsigned int len;
	unsigned int seq;
	unsigned int start_pos;
	unsigned int percpu_absolute;
	unsigned char sym[];
};

static struct sym_entry *read_symbol(FILE *in, char **buf, size_t *buf_len)
{
	char *name, type, *p;
	unsigned long long addr;
	size_t len;
	ssize_t readlen;
	struct sym_entry *sym;

	errno = 0;
	readlen = getline(buf, buf_len, in); 										[1]
	if (readlen < 0) {
		if (errno) {
			perror("read_symbol");
			exit(EXIT_FAILURE);
		}
		return NULL;
	}

	if ((*buf)[readlen - 1] == '\n')
		(*buf)[readlen - 1] = 0;

	addr = strtoull(*buf, &p, 16);												[2]

	if (*buf == p || *p++ != ' ' || !isascii((type = *p++)) || *p++ != ' ') { 	[3]
		fprintf(stderr, "line format error\n");
		exit(EXIT_FAILURE);
	}

	name = p;																	[4]
	len = strlen(name);

	if (len >= KSYM_NAME_LEN) {
		fprintf(stderr, "Symbol %s too long for kallsyms (%zu >= %d).\n"
				"Please increase KSYM_NAME_LEN both in kernel and kallsyms.c\n",
			name, len, KSYM_NAME_LEN);
		return NULL;
	}

	if (strcmp(name, "_text") == 0)
		_text = addr;

	/* Ignore most absolute/undefined (?) symbols. */
	if (is_ignored_symbol(name, type))
		return NULL;

	check_symbol_range(name, addr, text_ranges, ARRAY_SIZE(text_ranges));
	check_symbol_range(name, addr, &percpu_range, 1);

	/* include the type field in the symbol name, so that it gets
	 * compressed together */
	len++;

	sym = malloc(sizeof(*sym) + len + 1);
	if (!sym) {
		fprintf(stderr, "kallsyms failure: "
			"unable to allocate required amount of memory\n");
		exit(EXIT_FAILURE);
	}
	sym->addr = addr;
	sym->len = len;
	sym->sym[0] = type;
	strcpy(sym_name(sym), name);
	sym->percpu_absolute = 0;

	return sym;
}
```

[1]에서 `<addr> <type> <name>` 형식으로 되어 있는 .maps 파일의 1줄을 읽어 옵니다. [2]에서 16진수로 되어 있는 `<addr>`을 u64 형으로 변환합니다. [3]에서 읽어온 내용이 올바른 포맷으로 되어 있는지 확인하고, 올바르다면 `<type>`을 읽어옵니다. [4]에서는 `<name>`이 시작하는 pointer를 읽어오고 바로 그 길이까지 구해줍니다.


## /proc/kallsyms

`/proc/kallsyms` 


## 출처 및 참고 문헌
[1] https://course.ccs.neu.edu/cs4410sp20/lec_tail-calls_x64_notes.html 

[2] https://elixir.bootlin.com/linux/v6.8.7/source/scripts/kallsyms.c