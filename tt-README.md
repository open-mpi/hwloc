# Tenstorrent support in hwloc

Documentation License:

Creative Commons CC-BY (https://creativecommons.org/licenses/by/4.0/)

Software License:

Apache 2.0 for Tenstorrent code. Reference tt-LICENSE_understanding.txt. If
tt-LICENSE-DOCS is present, also reference it here. For forks, document the
original license and Tenstorrent's Apache 2.0 additions

This branch of hwloc adds a Linux discovery component that exposes Tenstorrent
accelerators (kernel character devices under `/dev/tenstorrent/`) as hwloc OS
device objects, plus an optional public interoperability header surface for
applications that want to map between hwloc objects and tt-metal / tt-umd
identifiers.

Discovery does **not** link against tt-metal or tt-umd; it uses only KMD
ioctls on `/dev/tenstorrent/`. The optional IOP headers are header-only and
only require a tt-metal checkout (or its vendored UMD tree) to install the
metal-aware variant.

See the upstream `README` for general hwloc usage; this document only covers
the Tenstorrent-specific build options, tests, and command-line workflow.

Security Note:

review tt-SECURITY.md for details on security disclosure.

---

## 1. Build options

Generate the Autotools files once after a fresh checkout, then configure and
build:

```bash
./autogen.sh
./configure [options...]
make
make install   # optional
```

### 1.1 Discovery component (default on Linux)

On Linux, the Tenstorrent discovery component is built automatically as long
as I/O discovery is enabled. The relevant switches:

| Flag | Effect |
|---|---|
| *(none)* | Enabled by default on Linux. |
| `--disable-tenstorrent` | Build everything else but skip the Tenstorrent component. |
| `--enable-tenstorrent` | Redundant on Linux; fails configure on other targets. |
| `--disable-io` | Disables **all** I/O discovery (PCI, OSDev, CUDA, Tenstorrent, ...). |
| `--enable-plugins=tenstorrent` | Build the component as a runtime-loadable `hwloc_tenstorrent.so`. |

The configure run prints a `**** Tenstorrent configuration` block that
summarises whether the Linux target was accepted. `/dev/tenstorrent/` may be
absent on the build machine (e.g. CI / cross-build) — the component is still
compiled and will populate topology when the directory exists at runtime.

### 1.2 Interoperability headers (off by default)

Two independent switches control whether the public `hwloc/tenstorrent.h` and
`hwloc/tenstorrent_metal.h` headers get installed. They are header-only and
do **not** change discovery behaviour — they only add helpers that
applications can `#include`.

| Flag | Installs / enables | External dependency |
|---|---|---|
| `--enable-tt-metal-free-iop` | `hwloc/tenstorrent.h` | none |
| `--enable-tt-metal-iop` | `hwloc/tenstorrent.h` + `hwloc/tenstorrent_metal.h` | requires `--with-tt-metal-path` |
| `--with-tt-metal-path=DIR` | — | source-tree path; see below |
| `--enable-tt-yaml` | Parses Tenstorrent SoC / core descriptor YAML files at discovery time. | requires `libyaml` (yaml-0.1 pkg-config module) **and** the Tenstorrent discovery component |

`--enable-tt-metal-iop` implies `--enable-tt-metal-free-iop`.

`--enable-tt-yaml` is independent of the IOP header flags — it only affects
runtime discovery behaviour and does not change which headers get installed.
Install `libyaml-dev` / `libyaml-devel` before passing the flag, or set
`PKG_CONFIG_PATH` so `pkg-config --modversion yaml-0.1` succeeds. Configure
errors out cleanly if the dependency or the discovery component is missing.

**Runtime fallback contract.** When the YAML parser is compiled in,
discovery still works without any YAML files present. If
`TT_METAL_HOME` is unset, the matching SoC/core YAMLs do not exist, or a
file fails to parse, the component records the same path-only info keys it
would have produced without `--enable-tt-yaml`. The parsed-field info keys
are only added when parsing succeeds; the build never *requires* YAML files
at runtime.

`--with-tt-metal-path=DIR` is the **tt-metal source-tree root**, not an
install prefix. Configure looks for:

```
DIR/tt_metal/third_party/umd/device/api/umd/device/pcie/pci_ids.h
```

and adds `-I DIR/tt_metal/third_party/umd/device/api` to the compile flags
(also exposed via `pkg-config --cflags hwloc`). An installed tt-metal under
e.g. `/usr/local` will not have that nested layout and is not supported by
this switch.

### 1.3 Optional runtime environment

| Variable | Effect |
|---|---|
| `TT_METAL_HOME=/path/to/tt-metal` | If set and the matching YAMLs exist, hwloc records `TenstorrentSocDescriptor` / `TenstorrentCoreDescriptor` infos with absolute paths. |
| `HWLOC_COMPONENTS_VERBOSE=1` | Prints component registration / discovery order on stderr. |
| `HWLOC_SHOW_ERRORS=tenstorrent` | Surfaces stderr messages from the Tenstorrent component when it fails to open or ioctl a device. |
| `HWLOC_PLUGINS_PATH=/path` | When discovery is built as a plugin (`--enable-plugins`), use this to locate `hwloc_tenstorrent.so` at runtime. |

### 1.4 Configure summary lines to expect

After `./configure`, two lines tell you what was enabled:

```
Probe / display I/O devices: PCI(linux) LinuxIO Tenstorrent
Tenstorrent IOP headers:     yes (hwloc/tenstorrent.h + tenstorrent_metal.h)
```

- The `Probe / display I/O devices:` list contains `Tenstorrent` whenever
  the discovery component was built.
- The `Tenstorrent IOP headers:` line is `no`, `yes (hwloc/tenstorrent.h)`,
  or `yes (hwloc/tenstorrent.h + tenstorrent_metal.h)` depending on the IOP
  flags.

### 1.5 Common invocations

```bash
# Discovery only (default on Linux)
./configure
make

# Discovery + install hwloc/tenstorrent.h
./configure --enable-tt-metal-free-iop

# Discovery + install both IOP headers (needs tt-metal checkout)
./configure --enable-tt-metal-iop \
            --with-tt-metal-path=/path/to/tt-metal \
            --prefix=/path/to/install

# Discovery as a runtime-loadable plugin
./configure --enable-plugins=tenstorrent
```

---

## 2. Tests

Two test programs cover the Tenstorrent surface. Both live under
`tests/hwloc/` and are wired into the standard `make check` framework, so
the build only includes the IOP test when the corresponding flag is
configured.

| Test program | Source | Built when | Covers |
|---|---|---|---|
| `tenstorrent` | `tests/hwloc/tenstorrent.c` | always | Discovery component — finds OS device objects, validates infos. No-op pass if `/dev/tenstorrent/` is absent. |
| `tenstorrent-iop` | `tests/hwloc/tenstorrent-iop.c` | `--enable-tt-metal-free-iop` (or `--enable-tt-metal-iop`) | Compiles and exercises `hwloc/tenstorrent.h` helpers; when built with `--enable-tt-metal-iop`, additionally compiles in the `hwloc/tenstorrent_metal.h` path via `-DHWLOC_TT_METAL_IOP_TEST=1`. |

The IOP test's metal-header branch is enabled automatically by the
Makefile when `HWLOC_TT_METAL_IOP` is true (see
`tests/hwloc/Makefile.am:172-177`). You do not need to set
`-DHWLOC_TT_METAL_IOP_TEST` manually.

### 2.1 Running the tests

```bash
# Just the discovery test
make -C tests/hwloc check-tenstorrent

# Just the IOP test
make -C tests/hwloc check-TESTS TESTS=tenstorrent-iop

# Both
make -C tests/hwloc check-TESTS TESTS="tenstorrent tenstorrent-iop"

# Full hwloc test suite (slow)
make check
```

A successful run prints a single `PASS:` line per test and a summary block.

> **Do not** run `make check TESTS=tenstorrent` from the repository root.
> `TESTS` leaks into `MAKEFLAGS` and breaks unrelated subdirs (e.g. `xml/`).
> Use `check-tenstorrent` or the `make -C tests/hwloc ...` forms above.

### 2.2 What the tests actually check

`tenstorrent` (the discovery test):

- Initializes a topology with PCI/OSDev kept (`HWLOC_TYPE_FILTER_KEEP_IMPORTANT`).
- If at least one `/dev/tenstorrent/*` device exists, walks the resulting OS
  device objects and verifies subtype, naming, and the presence of expected
  info keys (`PCIBusID`, `GPUVendor=Tenstorrent`, `TenstorrentDeviceIndex`, ...).
- If no devices exist on the build host, it short-circuits to a pass — the
  important property is that the topology load succeeds and the component
  registered without error.

`tenstorrent-iop`:

- Loads a topology with PCI + OSDev kept.
- Calls `hwloc_tenstorrent_get_osdev_by_name(topology, "tenstorrent0")`.
- If that returns an object, also calls
  `hwloc_tenstorrent_obj_is_osdev()` and `hwloc_tenstorrent_get_osdev_cpuset()`.
- When `HWLOC_TT_METAL_IOP_TEST` is defined (i.e. `--enable-tt-metal-iop`),
  also compiles in a call to `hwloc_tenstorrent_get_osdev_by_pci_product()`
  exercising the metal-header path.
- Like the discovery test, it tolerates an absent device — the goal is to
  prove the public helpers link and execute, not to require physical hardware.

### 2.3 Verifying the IOP install

After `make install`:

```bash
ls $PREFIX/include/hwloc/tenstorrent*.h
# expect: tenstorrent.h  (and tenstorrent_metal.h if --enable-tt-metal-iop)

pkg-config --cflags hwloc
# with --enable-tt-metal-iop, the cflags include
#   -I<tt-metal>/tt_metal/third_party/umd/device/api
```

---

## 3. Command-line workflow

All hwloc command-line tools see Tenstorrent OS devices the same way they
see any other I/O object — by default I/O is filtered out, so use one of
the `--whole-io` / `--filter` switches to keep them visible.

OS devices are named `tenstorrent0`, `tenstorrent1`, ... in **PCI sort
order**, which may not match the raw KMD minor numbers.

### 3.1 `lstopo` — visual map

```bash
# ASCII boxes (best for terminal viewing); include I/O
lstopo --whole-io --of ascii

# Plain text tree (no boxes)
lstopo-no-graphics
lstopo --of console

# Verbose mode — prints each object's info keys
lstopo --whole-io -v --of console

# Save to a file
lstopo --whole-io --of ascii - > topo.txt
```

Expected layout for a multi-card box: each `CoProc(Tenstorrent)` leaf sits
under its PCI bridge and HostBridge, parented under whichever NUMA package
the PCIe root complex belongs to.

### 3.2 `hwloc-info` — query

```bash
# Topology summary (depths only — does not show device names)
hwloc-info

# All attributes for one device (verbose, with I/O kept)
hwloc-info -v --whole-io os=tenstorrent0

# Just the interesting info keys for one device
hwloc-info --whole-io os=tenstorrent0 | grep -E 'PCI|GPU|Tenstorrent'

# Loop over all four (adjust count to your machine)
for i in 0 1 2 3; do
  echo "=== tenstorrent$i ==="
  hwloc-info --whole-io os=tenstorrent$i | grep -E 'info |GPUModel|PCIBusID'
done

# All I/O objects, filtered to Tenstorrent
hwloc-info --whole-io --descendants io all | grep -i tenstorrent
```

Note: the `os=` prefix is required in location syntax; the `-v` / `--verbose`
flag must come **before** the location argument.

### 3.3 Expected info keys

After `hwloc_topology_load()` (or via `hwloc-info -v`), each Tenstorrent OS
device carries:

| Key | Meaning |
|---|---|
| `TenstorrentDeviceIndex` | hwloc-assigned index (matches OS-device name). |
| `PCIBusID` | Full PCI BDF, e.g. `0000:31:00.0`. |
| `PCIVendor` / `PCIDevice` | Hex IDs of the PCI function. |
| `PCISubsystemVendor` / `PCISubsystemDevice` | Subsystem IDs when reported. |
| `GPUVendor` | Always `Tenstorrent`. |
| `GPUModel` | Resolved model name for known PCI device IDs. |
| `TenstorrentSocDescriptor` | Absolute YAML path; only when `TT_METAL_HOME` points at a tt-metal tree containing the matching file. |
| `TenstorrentCoreDescriptor` | Likewise, from `tt_metal/core_descriptors/`. |

When the build also has `--enable-tt-yaml` and the YAML file at the path
above can be opened and parsed, the following best-effort keys are added.
Each key is omitted silently if the corresponding field is missing or
malformed — the path-only keys above remain the only guaranteed output.

From the **SoC descriptor** (verified against tt-metal `main` for
`wormhole_b0_80_arch.yaml` and `blackhole_140_arch.yaml`):

| Key | Source field |
|---|---|
| `TenstorrentArch` | `arch_name` scalar (e.g. `WORMHOLE_B0`, `BLACKHOLE`) |
| `TenstorrentTensixGridCols` | `grid.x_size` |
| `TenstorrentTensixGridRows` | `grid.y_size` |
| `TenstorrentTensixL1Size` | `worker_l1_size` (bytes per tensix L1) |
| `TenstorrentDRAMBankSize` | `dram_bank_size` (bytes per DRAM bank) |
| `TenstorrentEthL1Size` | `eth_l1_size` (bytes per ethernet-core L1) |
| `TenstorrentDRAMViewSize` | `dram_view_size` (bytes per DRAM view) |
| `TenstorrentFunctionalWorkerCount` | length of `functional_workers` sequence |
| `TenstorrentHarvestedWorkerCount` | length of `harvested_workers` sequence |

From the **core descriptor**: this file isn't flat — the top level is keyed
by board variant (e.g. `unharvested` / `1xharvested` / `2xharvested` on
blackhole, `galaxy` / `nebula_x1` / `nebula_x2` on wormhole). For each
variant where `compute_with_storage_grid_range` and/or `dispatch_cores` sit
*directly* under the variant (blackhole-style), the parser emits per-variant
info keys with the variant name as suffix:

| Key | Source field |
|---|---|
| `TenstorrentCoreDescriptorVariants` | comma-separated list of top-level variant names found |
| `TenstorrentComputeGridStart_<variant>` | `<variant>.compute_with_storage_grid_range.start`, formatted `"x,y"` |
| `TenstorrentComputeGridEnd_<variant>` | `<variant>.compute_with_storage_grid_range.end`, formatted `"x,y"` |
| `TenstorrentDispatchCoreCount_<variant>` | length of `<variant>.dispatch_cores` sequence |

Wormhole's core descriptors nest the same fields under additional
`row`/`col` sub-mappings; only the variant list is currently emitted in that
case. Applications that need the per-row/per-col detail should read the
YAML path themselves from `TenstorrentCoreDescriptor`.

A topology-level info `Backend=Tenstorrent` is also set whenever at least
one device was added during that load.

### 3.4 Optional YAML descriptor paths

The two `TenstorrentSocDescriptor` / `TenstorrentCoreDescriptor` infos are
populated only when:

1. `TT_METAL_HOME` is exported and points at a tt-metal repo.
2. The default YAMLs for the device's PCI ID exist under
   `$TT_METAL_HOME/tt_metal/soc_descriptors/<chip>.yaml` and
   `$TT_METAL_HOME/tt_metal/core_descriptors/<chip>.yaml`.

Example:

```bash
export TT_METAL_HOME=$HOME/tt-metal
hwloc-info --whole-io os=tenstorrent0 | grep -E 'TenstorrentSoc|TenstorrentCore'
```

### 3.5 Debugging discovery

```bash
# Show component registration / discovery order
HWLOC_COMPONENTS_VERBOSE=1 hwloc-info --whole-io os=tenstorrent0

# Surface stderr from the Tenstorrent component on ioctl/open failures
HWLOC_SHOW_ERRORS=tenstorrent lstopo --whole-io --of console

# Make sure you're running the build you just installed, not a system hwloc
which lstopo hwloc-info
./utils/hwloc/lstopo --whole-io --of console        # from the build tree
$PREFIX/bin/lstopo --whole-io --of console           # from the install tree
```

---

## 4. Scope and limitations

- **Linux only.** The component is not built on macOS, *BSD, Windows, etc.
- **Device-level only.** hwloc creates one OS device per Tenstorrent PCI
  function. It does **not** model individual Tensix cores or NOC hops —
  that detail remains the responsibility of tt-metal and the application.
- **No userspace library dependency at runtime.** Discovery uses KMD ioctls;
  tt-metal / tt-umd are not loaded.
- **The IOP headers are header-only.** They expose lookup helpers (`by_name`,
  `by_pci_product`, `is_osdev`, `get_osdev_cpuset`) for applications that
  already have an hwloc topology. They do not link or open devices.

---

ALL RIGHTS RESERVED

Copyright 2026 Tenstorrent USA, Inc.
All Rights Reserved.

This software and its source code are proprietary and confidential.
This project is NOT open source.

No part of this software may be used, copied, modified, merged,
distributed, sublicensed, or sold without the express written
permission of Tenstorrent USA, Inc.

Access to this repository does not constitute or imply a license
of any kind. No license, express or implied, is granted herein.

For licensing inquiries, contact: legal@tenstorrent.com
