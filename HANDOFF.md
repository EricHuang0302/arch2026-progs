# Moving and reproducing the assignment project

Repository: https://github.com/EricHuang0302/arch2026-progs

Clone the full repository on the other computer. Do not use a shallow clone,
a linked worktree, GitHub's web editor, or the ZIP download for development:
the assignment's history checks need the complete Git history.

```sh
git clone https://github.com/EricHuang0302/arch2026-progs.git
cd arch2026-progs
```

The `main` branch contains the submission and HW2_REPORT.md, the English
report ready to paste into HackMD. CodeTrial is managed separately by the
student and is outside this project handoff.

## Destination computer: macOS

The destination is macOS. The commands below follow the official
[Homebrew make](https://formulae.brew.sh/formula/make),
[Homebrew Bash](https://formulae.brew.sh/formula/bash), and
[xPack installation](https://xpack-dev-tools.github.io/riscv-none-elf-gcc-xpack/docs/install/)
instructions. They have not been executed on the destination Mac. The measured
results come from Windows and the independent Linux CI; rerun the checks on
the Mac after installation. The guest remains RV32I on either host architecture.

Install Apple's Command Line Tools first. Check with `xcode-select -p`; if
they are absent, run `xcode-select --install` and finish its installer before
continuing. Install Homebrew from https://brew.sh if it is not already present.
On Apple Silicon, use a native arm64 terminal for the native toolchain archive.

```sh
brew install bash make coreutils gnu-sed grep dtc bc cppcheck aspell gh python
export PATH="$(brew --prefix bash)/bin:$(brew --prefix make)/libexec/gnubin:$PATH"
export PATH="$(brew --prefix coreutils)/libexec/gnubin:$(brew --prefix gnu-sed)/libexec/gnubin:$(brew --prefix grep)/libexec/gnubin:$PATH"
bash --version
make --version
aspell dump dicts
```

Confirm Bash is at least 4 and make is GNU make. Homebrew installs GNU make as
`gmake`; its `gnubin` PATH entry above also exposes it as `make`. Using an
interactive zsh is fine: scripts with an env-based Bash shebang find the new
Bash through PATH. Keep this PATH in each terminal used for Git hooks and builds.
The English dictionary must appear in `aspell dump dicts`.

Use a virtual environment outside the checkout for the pinned formatter and
plotting library. This avoids modifying Homebrew's managed Python installation
and avoids accidentally committing environment files.

```sh
python3 -m venv "$HOME/.local/share/arch2026/venv"
. "$HOME/.local/share/arch2026/venv/bin/activate"
python -m pip install clang-format==20.1.8 matplotlib
export CLANG_FORMAT="$(command -v clang-format)"
export HOST_CC=clang
clang-format --version
clang --version
```

The [15.2.0-1 release](https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/tag/v15.2.0-1)
publishes both `darwin-arm64` and `darwin-x64` archives and their `.sha` files.
Select the running host architecture and verify the archive before extraction:

```sh
toolchain_root="$HOME/.local/opt"
mkdir -p "$toolchain_root"
(
    set -e
    cd "$toolchain_root"
    case "$(uname -m)" in
        arm64|aarch64) host_archive=darwin-arm64 ;;
        x86_64) host_archive=darwin-x64 ;;
        *) echo "Unsupported host architecture" >&2; exit 1 ;;
    esac
    version=15.2.0-1
    archive="xpack-riscv-none-elf-gcc-$version-$host_archive.tar.gz"
    base="https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v$version"
    curl --fail --location --remote-name "$base/$archive"
    curl --fail --location --remote-name "$base/$archive.sha"
    shasum -a 256 -c "$archive.sha"
    tar -xzf "$archive"
)
export PATH="$toolchain_root/xpack-riscv-none-elf-gcc-15.2.0-1/bin:$PATH"
export CROSS_COMPILE=riscv-none-elf-
riscv-none-elf-gcc --version
gh auth login
```

From the full project clone, build the native emulator and the guest programs:

```sh
make toolchain
make
make test
make sanitize
make unittest
make indent-check
make cycles
make dump
make size
```

Do not apply the Windows emulator patch or export the MSYS2 MAKEFLAGS on macOS.
The normal instructor build chooses its Darwin host paths. If a different host
compiler version changes C sanitizer diagnostics, keep the diagnostics and
resolve the cause; do not disable sanitizers or change the guest architecture.

To check with a clean current instructor harness, run:

```sh
git clone https://github.com/sysprog21/arch2026-progs.git .trusted
.trusted/scripts/grade.sh --submission . --chain --out results
```

Run `make update` and `make grade` before final submission as required by the
assignment. For exact measurement reproduction instead, pin
`RV32EMU_REV=158b57e9eab99d59e01141cd0e477b06b44b8fe3` before its first build.
The report distinguishes the recorded revision from a future updated run.

## Recommended environment: Ubuntu or Ubuntu in WSL 2

These instructions use x86-64 Linux. WSL 2 needs Windows virtualization
support enabled and a working Ubuntu distribution. If WSL is unavailable,
use native Linux or the MSYS2 notes below. Do not change the target ISA to
make a local build work.

```sh
sudo apt-get update
sudo apt-get install build-essential git curl gh python3 python3-venv \
    python3-matplotlib pipx device-tree-compiler bc cppcheck aspell aspell-en
pipx install clang-format==20.1.8
export PATH="$HOME/.local/bin:$PATH"
export CLANG_FORMAT="$(command -v clang-format)"
clang-format --version
gh auth login
```

Use the same cross compiler as the measurements to reproduce their counts.
The archive and checksum are the same published release used by the
instructor's CI. Extracting the archive does not install into system folders.

```sh
toolchain_root="$HOME/.local/opt"
mkdir -p "$toolchain_root"
(
    cd "$toolchain_root"
    version=15.2.0-1
    archive="xpack-riscv-none-elf-gcc-$version-linux-x64.tar.gz"
    base="https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v$version"
    curl --fail --location --remote-name "$base/$archive"
    curl --fail --location --remote-name "$base/$archive.sha"
    sha256sum -c "$archive.sha"
    tar -xzf "$archive"
)
export PATH="$toolchain_root/xpack-riscv-none-elf-gcc-15.2.0-1/bin:$PATH"
riscv-none-elf-gcc --version
```

Run these from the project root:

```sh
make toolchain
make
make test
make sanitize
make indent-check
make cycles
make dump
make size
```

`make` installs the Git hooks and enables the fork's Actions. Keep the
formatting and cross-compiler paths in the environment when committing.
The four C optimization levels must all pass, not just the default -O2.

For the final instructor-harness check, use an additional complete clone:

```sh
git clone https://github.com/sysprog21/arch2026-progs.git .trusted
.trusted/scripts/grade.sh --submission . --chain --out results
```

Before final submission, update the emulator and rerun the grade:

```sh
make update
make grade
```

Do not skip a hook or edit the harness to change a verdict. Add the instructor
remote once after cloning, then rebase and rechain when incorporating changes:

```sh
git remote add upstream https://github.com/sysprog21/arch2026-progs.git
git fetch upstream
git rebase upstream/main
scripts/chain.sh rechain upstream/main
```

## Native Windows with MSYS2

This project was also tested on Windows using MSYS2 Bash 5.2, MSYS2 make
4.4.1, xPack GCC 15.2.0-1 for Windows, and clang64 Clang 21.1.1 for host
ASan/UBSan. The formatter must still be version 20.1.8.

In an MSYS2 terminal, install these packages:

```sh
pacman -S --needed make gcc git python dtc bc diffutils aspell aspell-en \
    mingw-w64-clang-x86_64-clang \
    mingw-w64-clang-x86_64-compiler-rt \
    mingw-w64-clang-x86_64-cppcheck
```

Install the Windows x86-64 archive of xPack GCC 15.2.0-1 from the same release
as above and verify its `.sha` file. Put its `bin` directory on PATH. Install
GitHub CLI and log in; make sure `gh` is on the MSYS2 PATH. Install the pinned
formatter, for example `python -m pip install clang-format==20.1.8`, and set
`CLANG_FORMAT` to that executable's MSYS2 path. If more than one formatter
is installed, verify the selected executable prints 20.1.8.

Keep LF line endings in this checkout:

```sh
git config core.autocrlf false
git config core.eol lf
export PATH="/usr/bin:/clang64/bin:$PATH"
export HOST_CC=/clang64/bin/clang
export CROSS_COMPILE=riscv-none-elf-
export MAKEFLAGS="EMU=third_party/rv32emu/build/rv32emu.exe DEV_OBJS= USER_MEM_SIZE=128 HOST_CC=/clang64/bin/clang"
```

The native emulator needs a host-only compatibility adjustment: MSYS2's
signal-driven mmap paging does not work with this revision. The patch in
`dev/windows-rv32emu.patch` selects the existing malloc fallback. The empty
`DEV_OBJS` excludes unused kernel device objects that require Linux headers;
ELF-loader system emulation does not use them. Guest instructions, counters,
test code, and implementation sources are unchanged. The official Linux CI
uses the instructor's unmodified emulator and independently checks the score.

```sh
make third_party/rv32emu/.git/source-ready
git -C third_party/rv32emu apply ../../dev/windows-rv32emu.patch
make
make test
make sanitize
```

After `make update`, reapply the host patch and rebuild before running tests:

```sh
git -C third_party/rv32emu apply ../../dev/windows-rv32emu.patch
make -C third_party/rv32emu CROSS_COMPILE= DEV_OBJS= USER_MEM_SIZE=128 \
    ENABLE_ELF_LOADER=1 ENABLE_SDL=0 ENABLE_SDL_MIXER=0 \
    ENABLE_EXT_M=0 ENABLE_EXT_A=0 ENABLE_EXT_F=0 ENABLE_EXT_C=0
make grade
```

Use MSYS2 Python for the shell-based build scripts. For Windows Python to run
`scripts/cycles.py`, keep `CROSS_COMPILE` set: otherwise the argument parser
tries to execute a shell script as a Windows program before parsing options.
Windows Python with matplotlib can produce charts from the same ELF runs:
Set `WINDOWS_PYTHON` to the full MSYS2-style path of that Windows Python,
since the first `python.exe` on PATH might be MSYS2's separate installation.

```sh
"$WINDOWS_PYTHON" scripts/cycles.py --emu third_party/rv32emu/build/rv32emu.exe \
    --prefix "$CROSS_COMPILE" --out results
```

## Extra checks and table regeneration

From a built checkout:

```sh
bash mytests/run_diameter.sh measurements/report-final
bash mytests/run_target_stress.sh
python3 mytests/optimization_experiments.py
mkdir -p build/mytests
${HOST_CC:-cc} -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all -iquote . \
    mytests/numeric_reference.c src/bf16/bf16.c src/fixed/fixed.c \
    -lm -o build/mytests/numeric_reference
build/mytests/numeric_reference
bash dev/export_disassembly.sh
python3 dev/plot_evidence.py --directory measurements/report-final
python3 dev/plot_bf16_error.py
```

The diameter sweep checks all 2,644 hardest cubes with each C optimization
level, validates both returned move sequences, and records maximum calls.
Its measured counts include counter-read overhead, making them conservative
for the instruction budget. The host numerical check measures BF16 error
and compares 100,000 signed products with native 64-bit arithmetic.

The required tables are committed; generation is not required to build:

```sh
python3 mytests/generate_bf16_table.py
${HOST_CC:-cc} -O2 mytests/generate_rubik_tables.c \
    -o build/mytests/generate_rubik_tables
build/mytests/generate_rubik_tables
git diff -- src/bf16/sqrt-table.inc src/rubik/tables.inc src/rubik/tables-asm.inc
```

The Rubik generator temporarily explores all states on the host, then retains
only 12,224 near states, 2,644 diameter keys, and orientation transitions.
The submitted solver searches on target and has no complete-state table.

## Transfer checklist

- Complete Git history and the main branch, including all source commits.
- `src/` including the three generated `.inc` files; do not copy only `.c`
  and `.S` files.
- Unmodified instructor framework, contract headers, and tracked workflow.
- `mytests/`, `dev/`, this document, and `PROJECT_GOAL.md`.
- `HW2_REPORT.md`, including its pinned image and source links.
- Tracked `measurements/` evidence and its compiler/emulator metadata.
- Install tools and rebuild `build/` and `third_party/` on the destination;
  those are caches, not portable deliverables.
- Verify `make test`, `make sanitize`, and the final main CI run before use.
- Keep account keys and other secrets outside the repository.

Online HackMD publication, CodeTrial setup, interview, and recording are the
student's separate responsibilities.
