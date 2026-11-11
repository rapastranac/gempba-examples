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

The workflow first looks for a **branch with the same name** in the gempba repo. If it doesn't exist and the run is for a pull request, it looks for the pull request's **base branch** there. If neither exists, CI falls back to `main`.

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
- *Pull request into a paired branch.* You open a PR from `21-add-tests` into `414-rename-foo-to-bar`, which exists in both repos. gempba has no `21-add-tests`, so CI builds against gempba's `414-rename-foo-to-bar`. Pull requests into `main` and into numbered branches (`[0-9]+-**`) run CI.

## Build locally

`CMakeLists.txt` resolves gempba in this order:

1. An installed gempba reachable via `find_package` (`CMAKE_PREFIX_PATH`).
2. A sibling source tree at `../gempba` (folded in via `add_subdirectory`).
3. Otherwise, configure fails with the canonical "gempba not found" error.

So if you have `gempba` cloned next to this repo, no install step is required — open it in CLion, or use one of the helper scripts:

```sh
# Linux
bash scripts/build_linux.sh            # Release, every example, hwloc ON
bash scripts/build_linux.sh Debug OFF  # Debug, multithreading examples only

# Windows / MSYS2
bash scripts/build_windows.sh         # Release, every example, hwloc ON
```

Or skip the helpers and use cmake directly:

```sh
cmake -S . -B .build
cmake --build .build --parallel
```

Binaries land in `bin/`: `multithreading_*` and `multiprocessing_*`. The multiprocessing examples need gempba's MPI component; pass `-DGEMPBA_EXAMPLES_MPI=OFF` to build only the multithreading ones against gempba's core.

## Run on a cluster

`cluster/` builds `multiprocessing_benchmark` against a Slurm cluster's own MPI and runs it across nodes. Check out gempba next to this repository, then on a login node:

```sh
bash cluster/build.sh               # fmt, spdlog and hwloc from gempba's build_deps.sh, then the benchmark
sbatch cluster/benchmark.sbatch     # after setting its --account
```

- `cluster/modules.sh` holds the modules both load, set for the Digital Research Alliance of Canada clusters; change it for another cluster.
- The job runs 6 nodes x 2 ranks x 2 threads at depth 26; each extra level of `-max_depth` doubles the work.
- Its output goes to `report/` and `errors/`. Each rank logs `mca_pml_ucx_init` when Open MPI runs over UCX.

## Stress tests

`tests/` holds two synthetic stress tests that need no input files. Each one walks a full binary tree and checks two answers that don't depend on how the work was split: the number of leaves (`2^depth`) and the best leaf (`2^depth - 1`). A run fails when either answer is wrong, or when it doesn't finish within 60 s.

- `multithreading_leaf_visitor`: one process on the quasi-horizontal load balancer, on 2, 8 and 16 threads.
- `multiprocessing_leaf_visitor`: MPI ranks with 2 threads each, on the semi-centralized scheduler and the quasi-horizontal load balancer, on 2, 4 and 8 ranks.

They oversubscribe small machines on purpose, so that hand-offs race. With Open MPI, `--oversubscribe` is added for you.

```sh
cmake -S . -B .build -DGEMPBA_EXAMPLES_STRESS_TESTS=ON
cmake --build .build --parallel
ctest --test-dir .build -L stress --output-on-failure
```

Add `--repeat until-fail:20` for a longer soak.

## License

MIT — see [LICENSE](LICENSE).
