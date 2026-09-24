# sub_AD54A0 — инициализация CPU feature detection (LuaJIT)

**Адрес:** `0xAD54A0`
**Размер:** `0x1E2` (482 байт)
**Классификация:** complex
**Контекст:** встроенный LuaJIT 2.1.1774946682, инициализирует runtime через CPUID

## Декомпиляция

```c
__int64 __fastcall sub_AD54A0(_QWORD *a1)
{
  __int64 v2;
  unsigned int v3, v4;
  int v6;
  unsigned int v7;
  unsigned int v8[4];
  _DWORD v9[4];

  v2 = a1[2];                                             // state ptr
  v3 = 0;
  if ( sub_774AF(0, v8) && sub_774AF(1u, &v6) )           // CPUID leaf 0, then 1
  {
    v4 = (16 * (v7 & 1)) | (32 * ((v7 >> 19) & 1));        // ECX bit 0  = SSE3
                                                         // bit 19       = SSE4.1
    if ( v8[0] < 7 )                                       // max leaf < 7 → старый CPU
    {
      v3 = (16 * (v7 & 1)) | (32 * ((v7 >> 19) & 1));
    }
    else
    {
      sub_774AF(7u, v9);                                   // CPUID leaf 7
      v3 = v4 | (((v9[1] >> 8) & 1) << 6);                // EBX bit 8 = AVX2
    }
  }
  *(_DWORD *)(v2 + 976) = 0x3FF0001 | v3;                 // flag = arch bits
  *(_OWORD *)(v2 + 2444) = xmmword_DF2FE0;
  *(_OWORD *)(v2 + 2460) = xmmword_DF2FF0;
  *(_OWORD *)(v2 + 2476) = xmmword_DF3000;
  *(_OWORD *)(v2 + 2488) = *(__int128 *)((char *)&xmmword_DF3000 + 12);
  *(_QWORD *)(a1[2] + 440i64) = 0x504D4D500000i64;          // "PMMP\0\0\0" — magic sig LuaJIT
  sub_AC51C0(a1[2]);                                       // init JIT engine
  if ( *(_DWORD *)(a1[2] + 442i64) != 1347243344 )         // "PMMP" sanity
    a1[11] = 0i64;                                         // fail path
  sub_AC7C60(a1, "Windows", 7);
  sub_AC7C60(a1, &unk_F59A64, 3);
  sub_AC7C30(a1, 20199);
  sub_AC7C60(a1, "LuaJIT 2.1.1774946682", 21);
  sub_B22560(a1, "jit",     &unk_DF2E60, &unk_DF2E30);
  sub_B22AA0(a1, "jit.util", sub_AD5750, a1[9]);
  sub_B22560(a1, "jit.opt",  &unk_DF2FD0, &unk_DF2EF8);
  a1[11] -= 16i64;
  return 1i64;
}
```

## Назначение

Это **LuaJIT 2.1.1774946682** bootstrap-функция — она регистрирует встроенные LuaJIT-модули (`jit`, `jit.util`, `jit.opt`) в Lua-стейте, идентифицирует платформу ("Windows") и записывает информацию о CPU-фичах (SSE3 / SSE4.1 / AVX2) в state по смещению `+0x3D0`.

CPUID-leaf'ы, которые она зовёт:
- `0` — vendor string, max basic leaf
- `1` — feature flags (ECX: bit0=SSE3, bit19=SSE4.1)
- `7` — extended features (EBX: bit8=AVX2), если поддерживается

## Почему не Neverlose-HWID

Несмотря на то, что функция собирает **CPU-info через CPUID**, это не чит-HWID:
1. Версия строки `"LuaJIT 2.1.1774946682"` — точная версия liblua, идущая в комплекте с Source 2 engine.
2. Регистрируются модули `jit.util`, `jit.opt` — стандартный LuaJIT API.
3. Magic-сигнатура `"PMMP"` (`0x504D4D50`) по смещению `+440` — это известный LuaJIT-байткод-формат.

Никаких сетевых вызовов, никаких обращений к реестру / файлам / WMI. **Source 2 engine использует LuaJIT для скриптинга, и эта функция его инициализирует.**

## Кто её вызывает

```
sub_AC51C0 (JIT engine init) → sub_AD54A0
sub_B22560 (luaL_register)
sub_B22AA0 (lua_register)
sub_AD7C60 (lua_pushlstring)
sub_AD7C30 (lua_pushinteger)
```

Это полностью LuaJIT-stack, не Neverlose.

## IDB

`C:\Users\Zalman\Desktop\nl_cs2\NLCS2\NL\cs2_212C3300000.bin.i64`

Рекомендуемое имя: `lua_jit_init` или `lj_lib_register_jit`.