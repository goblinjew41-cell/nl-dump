# sub_774AF — CPUID wrapper

**Адрес:** `0x774AF`
**Размер:** `0x19` (25 байт)
**Классификация:** leaf-функция, единственная инструкция CPUID в теле

## Декомпиляция

```c
__int64 __fastcall sub_774AF(unsigned int a1, _DWORD *a2)
{
  __int64 result; // rax

  _RAX = a1;
  __asm { cpuid }                  // CPUID with EAX = a1 (leaf)
  *a2     = result;                // a2[0] = EAX
  a2[1]   = _RBX;                  // a2[1] = EBX
  a2[2]   = _RCX;                  // a2[2] = ECX
  a2[3]   = _RDX;                  // a2[3] = EDX
  return result;
}
```

## Дизассемблер (ожидаемый вид)

```
mov     eax, ecx                ; EAX = leaf
cpuid                           ; -> EAX/EBX/ECX/EDX
mov     [rdx],   eax
mov     [rdx+4], ebx
mov     [rdx+8], ecx
mov     [rdx+C], edx
ret
```

## Назначение

Стандартная обёртка над `CPUID`. Принимает:
- `a1` — **leaf** (значение `EAX`, до 0xFFFFFFFF; для hypervisor — обычно 0x40000000)
- `a2` — **out-buffer** минимум 16 байт (4 × uint32): EAX/EBX/ECX/EDX

Используется для сбора:
- **leaf 0** — vendor string (12 байт в EBX/EDX/ECX)
- **leaf 1** — feature flags: SSE3, SSE4.1/4.2, AVX, AES-NI, RDRAND и т.п.
- **leaf 7, sub 0** — extended features: AVX2, AVX-512, SHA, BMI1/2 и т.п.
- **leaf 0x40000000** — hypervisor vendor (VMware/Hyper-V/VBox/KVM и т.п.)

## Кто вызывает

Только одна функция в IDB: **`sub_AD54A0`** (см. `02_…`). Это не Neverlose-HWID — внутри этой обёртки LuaJIT определяет CPU-фичи для JIT-компиляции.

В raw memory dump CS2 это единственный найденный CPUID wrapper. Если Neverlose-сторона собирала CPUID в другой функции (например, с inline `cpuid` вместо call в `sub_774AF`), она не видна через статический анализ — её можно искать только по runtime-трейсу через `dbg_*` инструменты MCP.

## Что лежит рядом (0x774xx…0x10cxxx)

IDA обнаружила несколько функций с inline-CPUID (0x78309, 0x79298, 0x7938B, 0x794A2, 0x795A9, 0x7988A, 0x79CC7, 0x79F58, 0x7A578, 0x7A676, 0x7A8A4, …) — это LuaJIT-генерированный код ASM-оптимизатора, который генерирует инструкции для каждого нативного CPUID-leaf.

## Файл IDA

IDB: `C:\Users\Zalman\Desktop\nl_cs2\NLCS2\NL\cs2_212C3300000.bin.i64`
Функция уже переименована в исходном сеансе, но рекомендую дать ей имя `cpuid_wrapper` или `host_cpuid`.