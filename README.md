# macs

[![codecov](https://codecov.io/gh/slanzi00/macs/graph/badge.svg)](https://codecov.io/gh/slanzi00/macs)

Command-line tool to compute **Maxwellian-Averaged Cross Sections (MACS)** for astrophysical nucleosynthesis studies. Cross-section data is fetched live from the [IAEA EXFOR](https://www-nds.iaea.org/exfor/) nuclear database.

## Physics background

The MACS at thermal energy $kT$ is defined as:

$$\langle\sigma v\rangle / v_T = \frac{2}{\sqrt{\pi}} \frac{\mu^2}{(kT)^2} \int_0^\infty \sigma(E)\, E\, e^{-\mu E / kT}\, dE$$

where $\mu = A/(1+A)$ is the reduced mass in atomic mass units and $E$ is the centre-of-mass energy. The integral is evaluated numerically with the trapezoidal rule over the energy grid returned by EXFOR.

## Requirements

- C++23 compiler (GCC 14+ or Clang 18+)
- CMake 3.20+
- [vcpkg](https://vcpkg.io) (dependencies are managed via `vcpkg.json`)

Dependencies installed automatically by vcpkg:

| Package | Purpose |
|---|---|
| `cpr` | HTTP client |
| `nlohmann-json` | JSON parsing |
| `spdlog` | Logging |
| `bfgroup-lyra` | CLI argument parsing |
| `doctest` | Unit testing |

## Build

```bash
export VCPKG_ROOT=/path/to/vcpkg

cmake -S . -B build -G "Ninja Multi-Config" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

cmake --build build --config Release
```

## Install

```bash
cmake --install build --config Release
# or to a custom prefix
cmake --install build --config Release --prefix ~/.local
```

The binary is installed as `macs-run`.

## Usage

```
macs-run [options]

  -e, --element <nuclide>      Nuclide to query, e.g. Er-166 (repeatable)
  -l, --library <lib>          Nuclear data library, e.g. ENDF/B-VIII.1 (repeatable)
  -t, --temperature <keV>      Temperature in keV for MACS computation (repeatable)
  -r, --reaction <reaction>    Reaction type [default: N,G]
      --xs                     Print cross-section table (energy vs sigma) instead of MACS
  -v, --verbose                Enable debug logging to stderr
  -h, --help                   Show this help
```

Nuclides must be in `Symbol-A` format (e.g. `Er-166`, `Fe-56`, `Au-197`).
Logs and errors go to **stderr**; data goes to **stdout** — safe to redirect or pipe.

### Examples

**MACS for Er-166 at stellar temperatures:**

```bash
macs-run -e Er-166 -l ENDF/B-VIII.1 -t 8 -t 25 -t 30 -t 90
```

```
# Er-166 ENDF/B-VIII.1 N,G
# temperature[keV]	MACS[mb]
8.0	1199.056606
25.0	665.818215
30.0	606.106442
90.0	324.175037
```

**Compare multiple libraries:**

```bash
macs-run -e Er-166 -l ENDF/B-VIII.1 -l JEFF-4.0 -l JENDL-5 -t 30
```

```
# Er-166 ENDF/B-VIII.1 N,G
# temperature[keV]	MACS[mb]
30.0	606.106442
# Er-166 JEFF-4.0 N,G
# temperature[keV]	MACS[mb]
30.0	659.520829
# Er-166 JENDL-5 N,G
# temperature[keV]	MACS[mb]
30.0	681.549681
```

**Compare multiple nuclides:**

```bash
macs-run -e Er-164 -e Er-166 -e Er-168 -l ENDF/B-VIII.1 -t 30
```

**Dump raw cross-section data (no temperature needed):**

```bash
macs-run -e Er-166 -l ENDF/B-VIII.1 --xs
```

```
# Er-166 ENDF/B-VIII.1 (N,G)
# energy[eV]	cross_section[barn]	d_cross_section[barn]
1.000000e-05	1.234567e+03	0.000000e+00
...
```

**Pipe to a file or plotting tool:**

```bash
macs-run -e Er-166 -l ENDF/B-VIII.1 --xs > er166_xs.tsv
macs-run -e Er-166 -l ENDF/B-VIII.1 -t 30 2>/dev/null | gnuplot ...
```

## Tests

```bash
cmake --build build --config Debug
ctest --test-dir build --build-config Debug --output-on-failure
```

Integration tests (require network access) are skipped by default:

```bash
./build/tests/Debug/macs-tests --test-suite="*integration*"
```

## CI

Automated builds and tests run on GitHub Actions on every push and pull request to `main`. See [`.github/workflows/ci.yml`](.github/workflows/ci.yml).
