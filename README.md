# gempba-examples

Consumer-side examples for [gempba](https://github.com/rapastranac/gempba). This repo is built against an **installed** gempba (`find_package(gempba)`) so every change exercises the public API and the install/`gempbaConfig.cmake` chain on every PR.

## How CI finds gempba

Every CI run needs to clone gempba from somewhere. The workflow figures out **which owner** and **which branch** automatically, so most users never configure anything.

### Which owner

The workflow clones gempba from the **same GitHub owner that hosts your fork of this repo**, using `${{ github.repository_owner }}`.

Worked examples:

- *Personal fork.* You're `alice` and you forked both repos. CI on `alice/gempba-examples` clones from `alice/gempba`. No configuration needed.
- *Organization mirror.* `acme-corp` mirrors both repos. CI on `acme-corp/gempba-examples` clones from `acme-corp/gempba`. Same — no configuration.
- *Mixed owners.* You forked **only** `gempba-examples` and want to test against the canonical upstream gempba. Override the default by setting a repository variable: **Settings → Secrets and variables → Actions → Variables → New repository variable**, name `GEMPBA_OWNER`, value `<upstream-owner>`. CI will then clone from `<upstream-owner>/gempba` regardless of who owns this repo.

### Which branch

The workflow first looks for a **branch with the same name** in the gempba repo. If it doesn't exist, CI falls back to `main`.

Worked examples:

- *Examples-only change.* You fixed a typo in `multithreading/foo.cpp` and push branch `fix-foo-typo`. That branch doesn't exist in the gempba repo, so CI builds against `gempba@main`. Nothing to coordinate.
- *gempba-only change that doesn't break the API.* You optimized an internal gempba function. Open the PR on `gempba` only — the gempba PR's CI looks for the same-named branch in `gempba-examples`, doesn't find it, and falls back to `gempba-examples@main` to validate the install surface.
- *Breaking API change* (e.g., you renamed a public function). Both repos have to move together. Push the **same branch name** to both repos *before* opening the PR:

  ```sh
  # in both repos, on the same starting commit:
  git checkout -b rename-foo-to-bar
  # ... make the paired changes ...
  git push -u origin rename-foo-to-bar
  ```

  CI on either PR will now find its sibling branch and build them together. After both PRs merge to `main`, delete the paired branches.

## Build locally

`CMakeLists.txt` resolves gempba in this order:

1. An installed gempba reachable via `find_package` (`CMAKE_PREFIX_PATH`).
2. A sibling source tree at `../gempba` (folded in via `add_subdirectory`).
3. Otherwise, configure fails with the canonical "gempba not found" error.

So if you have `gempba` cloned next to this repo, no install step is required — open it in CLion, or use one of the helper scripts:

```sh
# Linux
bash scripts/build_linux.sh           # Release, MP=OFF, hwloc ON
bash scripts/build_linux.sh Debug ON  # Debug, MP=ON

# Windows / MSYS2
bash scripts/build_windows.sh         # Release, MP=OFF, hwloc ON
```

Or skip the helpers and use cmake directly:

```sh
cmake -S . -B .build -DGEMPBA_MULTIPROCESSING=ON
cmake --build .build --parallel
```

Binaries land in `bin/` — `multithreading_*` for `MP=OFF`, `multiprocessing_*` for `MP=ON`. The two are mutually exclusive in a single build because gempba's consumer-facing namespace is mode-selected; flip `GEMPBA_MULTIPROCESSING` and reconfigure to switch modes.

## License

MIT — see [LICENSE](LICENSE).
