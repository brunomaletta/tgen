# Contributing to tgen

Thank you for helping improve tgen. This guide covers local setup, the pre-commit gate, documentation, and releases.

## Local setup

Requirements:

- C++17 compiler (GCC or Clang)
- GNU Make
- Google Test (`libgtest-dev` on Debian/Ubuntu)
- Python 3 (doc scripts)
- Doxygen 1.14+ (optional, for docs)
- `clang-format` (formatting)

```bash
git clone https://github.com/brunomaletta/tgen.git
cd tgen
make test          # run the test suite
make check         # lint + Werror tests + local benchmark regression
```

Useful targets:

| Target | Purpose |
|--------|---------|
| `make test_clang` | Tests with Clang |
| `make test_asan` | AddressSanitizer + UBSan |
| `make stresstest` | Infinite randomized seed loop |
| `make lint` | Auto-format C++ sources |
| `make doc` | Build Doxygen site + LLM docs |
| `make benchmark` | Regenerate benchmark JSON |

## Code style

- Match existing naming and patterns in `single_include/tgen.h`.
- Run `make lint` before committing (or `make lint-check` in CI).
- Keep changes focused; the library is a single header — avoid unrelated refactors.
- Document new public APIs in the matching `docs/*.dox` file.
- Add tests in `tests/` for new behavior.

## Pre-commit gate

`make check` runs:

1. `make lint-check` — clang-format
2. `make test WERROR=1` — GCC tests with warnings as errors
3. `make benchmark-check-local` — performance regression vs local baseline

If benchmark regression is intentional, refresh the baseline:

```bash
make benchmark-baseline-local
```

CI (`/.github/workflows/ci.yml`) also runs Clang/ASan tests, examples, benchmark smoke/regression, and Python doc script tests.

## Documentation

- Module docs live in `docs/*.dox` (one file per `@defgroup`).
- Main page: `docs/README.md`
- LLM-friendly markdown: `make llms` → `docs/build/llms.txt`

After API changes:

```bash
make doc
make test   # still required
cd docs && python3 -m unittest test_llms_gen test_benchmark_render test_benchmark_check -v
```

## Adding a hack

Use the GitHub issue template **Hack request** (`.github/ISSUE_TEMPLATE/hack_request.yml`) to propose new adversarial generators. Implementations go in `tgen::hack` with:

- Clear target algorithm / bug class
- Complexity documented in the `.dox` entry
- Tests in `tests/hack.cpp` validating structure and invariants

## Pull requests

1. Fork and branch from `main`.
2. Implement + test + document.
3. Run `make check`.
4. Open a PR with a short summary and test plan.

## Release checklist

Use this when cutting a versioned release (e.g. v1.4.0):

- [ ] All changes merged to `main`; CI green on `main`
- [ ] Run `make check` locally
- [ ] Update version strings if needed:
  - README install URL (`wget …/vX.Y.Z/…`)
  - `docs/Doxyfile` → `PROJECT_NUMBER`
  - Header comment in `single_include/tgen.h` (if present)
- [ ] Regenerate docs: `make doc`
- [ ] If benchmarks changed intentionally: `make benchmark-baseline-ci` and commit `benchmarks/ci_baseline.json`
- [ ] Verify GitHub Pages deploy (`docs.yml`) updated the live site

## Questions

Open a [GitHub issue](https://github.com/brunomaletta/tgen/issues) for bugs, feature requests, or hack proposals.
