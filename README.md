# gempba-examples

Consumer-side examples for [gempba](https://github.com/rapastranac/gempba). This repo is built against an **installed** gempba (`find_package(gempba)`) so every change exercises the public API and the install/`gempbaConfig.cmake` chain on every PR.

## Status

Bootstrap. The migration of the example tree from `gempba/examples/` is tracked in [gempba#86](https://github.com/rapastranac/gempba/issues/86) and depends on [gempba#85](https://github.com/rapastranac/gempba/pull/85). Until that lands, this repo carries only a smoke test that proves the cross-repo CI plumbing works.

## How CI finds gempba

The workflow defaults to fetching gempba from the **same GitHub owner** that hosts this repo: `${{ github.repository_owner }}/gempba`. So the upstream fork at `rapastranac/gempba-examples` pulls upstream `rapastranac/gempba`, and a fork at `apdistributedsystems/gempba-examples` pulls `apdistributedsystems/gempba`. Override with the repository variable `GEMPBA_OWNER` if you need to mix orgs.

Branch selection: the workflow looks for a **same-named branch** in the gempba repo first and falls back to `main` if it doesn't exist. So for non-breaking gempba PRs no examples-side branch is needed; for API-breaking PRs you push paired branches with the same name in both repos before opening the gempba PR.

## Build locally

```sh
# 1. install gempba somewhere
cmake -S /path/to/gempba -B /path/to/gempba/build -DCMAKE_INSTALL_PREFIX=/path/to/install
cmake --build /path/to/gempba/build
cmake --install /path/to/gempba/build

# 2. build the examples against that install
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/install
cmake --build build
```

Pass `-DGEMPBA_MULTIPROCESSING=ON` (or `OFF`) at gempba's configure step; the examples consume whichever mode you installed.

## License

MIT — see [LICENSE](LICENSE).
