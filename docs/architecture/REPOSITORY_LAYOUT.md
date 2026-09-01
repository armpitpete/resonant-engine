# M0.18 — Repository Structure

Status: **APPROVED**

```text
core/
  include/resonant/   public portable headers
  src/                private portable implementation
hosts/
  browser/            browser/WASM wrapper work
  vst3/               plugin wrapper work
  embedded/           device wrapper work
tests/
  unit/
  property/
  regression/
  fixtures/
tools/
  render/             offline host
docs/
  architecture/       normative contracts
  research/           observations/references, non-normative
  decisions/          ADR register/template
cmake/                 build checks/helpers
.github/workflows/     CI
```

No speculative `oscillators/`, `waveguides/`, `pipes/`, `strings/` or graph hierarchy is created before a model needs it. That avoids prematurely encoding one synthesis theory into the filesystem.

Generated builds stay outside the source tree and are ignored. Public includes are `core/include/resonant`; private implementation is `core/src`; fixtures/research/ADRs have dedicated locations. Naming uses lower-case paths and PascalCase public C++ headers/types.

The legacy top-level `include/resonant` probe location from the first foundation PR is removed by the M0 completion change so there is one canonical public-header location.

**M0.18 Repository Structure: APPROVED.**
