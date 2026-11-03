# Independent-review numerical evidence

Recorded November 3, 2026 (Asia/Taipei). This extends the host analysis after
review of report revision `1b034e8056f15bd1b2deb3f3d6a724c1da339cf0`.
The submitted C and RV32I implementations are unchanged from
`dfd1e8504a0e7f16d6fc15a02acdd7bd7ba09bae`.

Host: Windows with MSYS2 clang64 Clang 21.1.1. AddressSanitizer and
UndefinedBehaviorSanitizer were enabled, with recovery disabled. This is a
host numerical check, not a guest timing run or a macOS execution claim.

From the repository root in a configured Bash environment:

```sh
mkdir -p build/mytests
${HOST_CC:-cc} -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -iquote . \
    mytests/numeric_reference.c src/bf16/bf16.c src/fixed/fixed.c \
    -lm -o build/mytests/numeric_reference
build/mytests/numeric_reference
```

On the recorded Windows run, `HOST_CC=/clang64/bin/clang` and the MSYS2
`/usr/bin` and `/clang64/bin` directories were on PATH. The successful stdout
is retained in `numeric-reference.log`; the command returned zero without
sanitizer diagnostics. The BF16 measurements repeat the earlier analysis.

For fixed point, all 200,169 products are compared against the low 32 bits of
an independently computed signed 64-bit, rounded reference. The three groups
are the original 100,000 full-range xorshift32 pairs (seed `0x12345678`), the
cross product of 13 directed raw values, and 100,000 bounded pairs from seed
`0x5eedc0de`. Bounded raw operands are `(random & 0x3ffffff) - 33554432`,
representing values in `[-32,32)` and ensuring their products do not wrap.

Absolute-error analysis is applied only when the rounded 64-bit reference
fits int32_t. If raw inputs are A and B and the actual raw result is R, the
exact error is `(R * 2^20 - A * B) / 2^40`. The signed integer subtraction is
performed before conversion to double, avoiding floating-point cancellation.
It compares the output with the exact product of already represented inputs;
it does not include error from quantizing external real-valued inputs.

There are 100,565 nonwrapping cases and 99,604 wrapping cases. Directed and
bounded groups both attain absolute error `524288 / 2^40 = 2^-21`, exactly
half an output unit. The log records each group's witness. The explicit raw
ties `(3,524288)` and `(-3,524288)` produce 2 and -1, respectively: both have
positive signed error `2^-21`, as ties round toward positive infinity.

Wrapping cases still undergo the bit-exact comparison, but they do not enter
the small-error maximum. This separates arithmetic contract verification from
quantization accuracy rather than hiding large wraparound errors in a bound
that does not apply to them. These samples do not exhaust all operand pairs.
