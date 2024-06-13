---
marp: true
title: uallsyms
author: 'Junhyeon Song'
theme: gaia
_class: lead
paginate: true
backgroundColor: #fff
backgroundImage: url('https://marp.app/assets/hero-background.svg')
math: mathjax
---

<style>
@font-face {
    font-family: 'NanumSquareRound';
    src: url(https://hangeul.pstatic.net/hangeul_static/webfont/NanumSquareRound/NanumSquareRoundR.eot);
    src: url(https://hangeul.pstatic.net/hangeul_static/webfont/NanumSquareRound/NanumSquareRoundR.eot?#iefix) format("embedded-opentype"), 
         url(https://hangeul.pstatic.net/hangeul_static/webfont/NanumSquareRound/NanumSquareRoundR.woff2) format("woff2"),
         url(https://hangeul.pstatic.net/hangeul_static/webfont/NanumSquareRound/NanumSquareRoundR.woff) format("woff"), 
         url(https://hangeul.pstatic.net/hangeul_static/webfont/NanumSquareRound/NanumSquareRoundR.ttf) format("truetype");
    unicode-range: U+AC00-D7A3;
}

section {
    font-family: Lato, 'Avenir Next', Avenir, 'Trebuchet MS', 'Segoe UI', sans-serif, 'NanumSquareRound';
}

</style>

# uallsyms

Try using kallsyms in userland

---

# Agenda

- Background
- Motivation
- Introduction to kallsyms
- Details of uallsyms
- Demo

---

# Background: abstract

uallsyms library는 취약점을 이용해 kallsyms를 userland에서 parsing한 뒤, 여러 kernel 함수들의 주소를 제공한다. 

=> 이를 이용해 하나의 exploit을 이용해 다양한 Linux Kernel에서 LPE를 달성할 수 있다.

**Keywords**
universal-exploit, kallsyms, LPE

--- 

# Background: LPE in the Linux kernel

**LPE?**

Local Privilege Escalation의 약자

Kernel 상에 존재하는 취약점을 이용해 normal user => root 를 달성하는 것을 의미한다.

LPE를 통해 root 권한으로 상승함으로써, normal user는 불가능한 권한 높은 행위들을 할 수 있게 된다.

---

# Background: LPE in the Linux kernel

**Credential in Linux kernel**

Linux kernel에서 권한은 task($\approx$ thread) 단위로 부여된다.

task에 해당하는 구조체는 task_struct 라는 이름으로 정의되어 있다.


```c
struct task_struct {
    struct thread_info  thread_info;
    unsigned int        __state;
    ...
    struct cred         cred;   /* credential of this task */
    ...
}
```

---

# Background: LPE in the Linux kernel

**Credential in Linux kernel**

Linux kernel에서 권한은 cred 라는 구조체에 의해서 관리 된다.

```c
struct cred {
    atomic_long_t   usage;
    kuid_t          uid;        /* real UID of the task */
    kgid_t          gid;        /* real GID of the task */
    kuid_t          suid;       /* saved UID of the task */
    kgid_t          sgid;       /* saved GID of the task */
    kuid_t          euid;       /* effective UID of the task */
    kgid_t          egid;       /* effective GID of the task */
    ...
};
```

---

# Background: LPE in the Linux kernel

**Credential in Linux kernel**

![w:460 h:400](../.images/tasks-and-creds.png) 

---

# Background: LPE in the Linux kernel

**Credential in Linux kernel**

- int commit_cred(struct cred *new)
    현재 task의 cred를 new로 변경하는 함수.

- struct cred init_cred
    init process의 cred에 해당하는 전역 변수; root 권한에 해당한다.

---

<style scoped>
pre {
  font-size: 80%;
}
</style>

# <!-- fit --> Background: Mitigations VS Exploit Tech

**Exploit Tech: ret2usr**

취약점을 이용해 실행 흐름을 userland에 미리 준비한 함수로 옮기는 방법 

```c
void win(void)
{
    int (*commit_creds)(void *);
    void *init_cred;

    /* will be running in kernel context */
    commit_creds = 0xffffffff81001234;  /* address of commit_creds */
    init_cred = 0xffffffff82004320;     /* address of init_cred */

    commit_creds(init_creds);

    system("/bin/sh");                  /* ?! */
}
```

---

<style scoped>
pre {
  font-size: 80%;
}
</style>

# <!-- fit --> Background: Mitigations VS Exploit Tech

**Exploit Tech: ret2usr**

userland로 돌아간 뒤, shell 실행 ([더 자세한 설명](https://pawnyable.cafe/linux-kernel/LK01/stack_overflow.html#ret2user-ret2usr))


```c
void win(void)
{
    int (*commit_creds)(void *);
    void *init_cred;

    /* below codes will be running in kernel context */
    commit_creds = 0xffffffff81001234;  /* address of commit_creds */
    init_cred = 0xffffffff82004320;     /* address of init_cred */

    commit_creds(init_creds);

    return_to_userland_and_spawn_shell(); 
}
```

---

# <!-- fit --> Background: Mitigations VS Exploit Tech
**KASLR**

Kernel Address Space Layout Randomization의 약자.

- KASLR disabled
    kernel base address: 0xffffffff81000000
    commit_creds: 0xffffffff81000000 + 0x1234

- KASLR enabled
    kernel base address: 0xffffffff81000000 + $\alpha$ (random value)
    commit_creds: 0xffffffff81000000 + $\alpha$ + 0x1234
    e.g.) address of commit_creds == 0xffffffff91201234

---

# Motivation: 

**Exploit에 필요한 주소를 구하는 방법**

1. 취약점을 이용해 임의의 kernel 주소를 leak 한다.
2. leak한 kernel 주소에서 미리 구한 offset를 빼서 kernel base 주소를 구한다.
3. 



---

