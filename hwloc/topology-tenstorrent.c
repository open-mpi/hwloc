/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2026 Tenstorrent USA, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * See COPYING in top-level directory.
 */

#include "private/autogen/config.h"
#include "hwloc.h"
#include "hwloc/plugins.h"

#include "private/misc.h"
#include "private/debug.h"

/*
 * Chip metadata in tt-metal lives under (relative to the repo root, e.g. $TT_METAL_HOME):
 *   tt_metal/soc_descriptors/*.yaml
 *   tt_metal/core_descriptors/*.yaml
 * PCI device IDs match tt_metal/third_party/umd/device/api/umd/device/pcie/pci_ids.h
 */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef HWLOC_HAVE_TT_YAML
#include <yaml.h>
#endif

/* Ioctl ABI matches Tenstorrent KMD (see tt-metal UMD pci_device.cpp / ioctl.h). */
#define TENSTORRENT_IOCTL_MAGIC 0xFA
#define TENSTORRENT_IOCTL_GET_DEVICE_INFO _IO(TENSTORRENT_IOCTL_MAGIC, 0)

struct tenstorrent_get_device_info_in {
  uint32_t output_size_bytes;
};

struct tenstorrent_get_device_info_out {
  uint32_t output_size_bytes;
  uint16_t vendor_id;
  uint16_t device_id;
  uint16_t subsystem_vendor_id;
  uint16_t subsystem_id;
  uint16_t bus_dev_fn;
  uint16_t max_dma_buf_size_log2;
  uint16_t pci_domain;
};

struct tenstorrent_get_device_info {
  struct tenstorrent_get_device_info_in in;
  struct tenstorrent_get_device_info_out out;
};

struct hwloc__tt_dev {
  int kmd_id;
  unsigned domain;
  unsigned bus;
  unsigned dev;
  unsigned func;
  uint16_t vendor_id;
  uint16_t device_id;
  uint16_t subsystem_vendor_id;
  uint16_t subsystem_id;
};

struct hwloc__tt_desc_files {
  uint16_t device_id;
  const char *model;
  const char *soc_yaml;
  const char *core_yaml;
};

/* PCI device IDs: Tenstorrent UMD pci_ids.h (TT_WORMHOLE_PCI_DEVICE_ID, etc.) */
static const struct hwloc__tt_desc_files hwloc__tt_desc_files[] = {
  { 0x401e, "Tenstorrent Wormhole B0",
    "wormhole_b0_80_arch.yaml", "wormhole_b0_80_arch.yaml" },
  { 0xb140, "Tenstorrent Blackhole",
    "blackhole_140_arch.yaml", "blackhole_140_arch.yaml" },
  { 0xfeed, "Tenstorrent Grendel",
    "quasar_32_arch.yaml", "quasar_simulation_8x4_arch.yaml" },
};

static const struct hwloc__tt_desc_files *
hwloc__tt_lookup_desc_files(uint16_t device_id)
{
  unsigned u;
  for (u = 0; u < sizeof(hwloc__tt_desc_files) / sizeof(hwloc__tt_desc_files[0]); u++)
    if (hwloc__tt_desc_files[u].device_id == device_id)
      return &hwloc__tt_desc_files[u];
  return NULL;
}

#ifdef HWLOC_HAVE_TT_YAML
/*
 * Best-effort YAML descriptor parsing.  All errors (file open, parse, missing
 * keys, wrong node types) are swallowed silently — the path-only
 * TenstorrentSocDescriptor / TenstorrentCoreDescriptor infos that the caller
 * already emitted remain the only guaranteed output.  Extra info keys are
 * added only when the corresponding fields are present and well-formed.
 */

static const char *
hwloc__tt_yaml_scalar(yaml_node_t *node)
{
  if (!node || node->type != YAML_SCALAR_NODE)
    return NULL;
  return (const char *) node->data.scalar.value;
}

static yaml_node_t *
hwloc__tt_yaml_map_get(yaml_document_t *doc, yaml_node_t *map, const char *key)
{
  yaml_node_pair_t *pair;
  if (!map || map->type != YAML_MAPPING_NODE)
    return NULL;
  for (pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++) {
    yaml_node_t *k = yaml_document_get_node(doc, pair->key);
    const char *ks = hwloc__tt_yaml_scalar(k);
    if (ks && !strcmp(ks, key))
      return yaml_document_get_node(doc, pair->value);
  }
  return NULL;
}

static void
hwloc__tt_yaml_add_scalar(hwloc_obj_t osdev, const char *info_key,
                          yaml_document_t *doc, yaml_node_t *map, const char *yaml_key)
{
  const char *s = hwloc__tt_yaml_scalar(hwloc__tt_yaml_map_get(doc, map, yaml_key));
  if (s && *s)
    hwloc_obj_add_info(osdev, info_key, s);
}

/* Extract first two scalar items of a sequence as ("a", "b").  Returns 1 on success. */
static int
hwloc__tt_yaml_seq_pair(yaml_document_t *doc, yaml_node_t *seq,
                        const char **a_out, const char **b_out)
{
  yaml_node_item_t *items;
  if (!seq || seq->type != YAML_SEQUENCE_NODE)
    return 0;
  if (seq->data.sequence.items.top - seq->data.sequence.items.start < 2)
    return 0;
  items = seq->data.sequence.items.start;
  *a_out = hwloc__tt_yaml_scalar(yaml_document_get_node(doc, items[0]));
  *b_out = hwloc__tt_yaml_scalar(yaml_document_get_node(doc, items[1]));
  return (*a_out && *b_out) ? 1 : 0;
}

static void
hwloc__tt_yaml_add_seq_count(hwloc_obj_t osdev, const char *info_key,
                             yaml_node_t *seq)
{
  if (seq && seq->type == YAML_SEQUENCE_NODE) {
    ptrdiff_t n = seq->data.sequence.items.top - seq->data.sequence.items.start;
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", (long) n);
    hwloc_obj_add_info(osdev, info_key, buf);
  }
}

static void
hwloc__tt_yaml_parse_soc(const char *path, hwloc_obj_t osdev)
{
  FILE *fh;
  yaml_parser_t parser;
  yaml_document_t doc;
  yaml_node_t *root, *grid;
  int parser_ok = 0, doc_ok = 0;

  fh = fopen(path, "r");
  if (!fh)
    return;
  if (!yaml_parser_initialize(&parser))
    goto out_file;
  parser_ok = 1;
  yaml_parser_set_input_file(&parser, fh);
  if (!yaml_parser_load(&parser, &doc))
    goto out_parser;
  doc_ok = 1;
  root = yaml_document_get_root_node(&doc);
  if (!root || root->type != YAML_MAPPING_NODE)
    goto out_doc;

  hwloc__tt_yaml_add_scalar(osdev, "TenstorrentArch",         &doc, root, "arch_name");
  hwloc__tt_yaml_add_scalar(osdev, "TenstorrentTensixL1Size", &doc, root, "worker_l1_size");
  hwloc__tt_yaml_add_scalar(osdev, "TenstorrentDRAMBankSize", &doc, root, "dram_bank_size");
  hwloc__tt_yaml_add_scalar(osdev, "TenstorrentEthL1Size",    &doc, root, "eth_l1_size");
  hwloc__tt_yaml_add_scalar(osdev, "TenstorrentDRAMViewSize", &doc, root, "dram_view_size");

  /* tt-metal SoC descriptors store the worker grid as a mapping
   * {x_size: <cols>, y_size: <rows>}.  x is the column axis, y the row axis. */
  grid = hwloc__tt_yaml_map_get(&doc, root, "grid");
  if (grid && grid->type == YAML_MAPPING_NODE) {
    hwloc__tt_yaml_add_scalar(osdev, "TenstorrentTensixGridCols", &doc, grid, "x_size");
    hwloc__tt_yaml_add_scalar(osdev, "TenstorrentTensixGridRows", &doc, grid, "y_size");
  }

  hwloc__tt_yaml_add_seq_count(osdev, "TenstorrentFunctionalWorkerCount",
                               hwloc__tt_yaml_map_get(&doc, root, "functional_workers"));
  hwloc__tt_yaml_add_seq_count(osdev, "TenstorrentHarvestedWorkerCount",
                               hwloc__tt_yaml_map_get(&doc, root, "harvested_workers"));

out_doc:
  if (doc_ok) yaml_document_delete(&doc);
out_parser:
  if (parser_ok) yaml_parser_delete(&parser);
out_file:
  fclose(fh);
}

/*
 * Core descriptors are not flat: the top-level mapping is keyed by board
 * variant (blackhole: "unharvested"/"1xharvested"/...; wormhole: "galaxy"/
 * "nebula_x1"/...).  For variants whose value directly contains
 * compute_with_storage_grid_range / dispatch_cores (blackhole-style), emit
 * per-variant info keys suffixed with "_<variant>".  Deeper nesting
 * (wormhole's row/col sub-mappings) is intentionally not walked — the
 * variants are still listed so applications know what's available.
 */
static void
hwloc__tt_yaml_parse_core(const char *path, hwloc_obj_t osdev)
{
  FILE *fh;
  yaml_parser_t parser;
  yaml_document_t doc;
  yaml_node_t *root;
  yaml_node_pair_t *pair;
  int parser_ok = 0, doc_ok = 0;
  char variants_buf[256];
  size_t variants_len = 0;

  fh = fopen(path, "r");
  if (!fh)
    return;
  if (!yaml_parser_initialize(&parser))
    goto out_file;
  parser_ok = 1;
  yaml_parser_set_input_file(&parser, fh);
  if (!yaml_parser_load(&parser, &doc))
    goto out_parser;
  doc_ok = 1;
  root = yaml_document_get_root_node(&doc);
  if (!root || root->type != YAML_MAPPING_NODE)
    goto out_doc;

  variants_buf[0] = '\0';

  for (pair = root->data.mapping.pairs.start; pair < root->data.mapping.pairs.top; pair++) {
    yaml_node_t *k = yaml_document_get_node(&doc, pair->key);
    yaml_node_t *v = yaml_document_get_node(&doc, pair->value);
    const char *variant = hwloc__tt_yaml_scalar(k);
    yaml_node_t *cgr;
    char info_key[96];

    if (!variant || !v || v->type != YAML_MAPPING_NODE)
      continue;

    /* Accumulate "var1,var2,..." regardless of whether we can parse the inner shape. */
    {
      size_t need = strlen(variant) + (variants_len ? 1 : 0);
      if (variants_len + need + 1 < sizeof(variants_buf)) {
        if (variants_len)
          variants_buf[variants_len++] = ',';
        memcpy(variants_buf + variants_len, variant, strlen(variant));
        variants_len += strlen(variant);
        variants_buf[variants_len] = '\0';
      }
    }

    cgr = hwloc__tt_yaml_map_get(&doc, v, "compute_with_storage_grid_range");
    if (cgr && cgr->type == YAML_MAPPING_NODE) {
      static const struct { const char *yaml_key; const char *prefix; } rk[2] = {
        { "start", "TenstorrentComputeGridStart_" },
        { "end",   "TenstorrentComputeGridEnd_"   },
      };
      unsigned i;
      for (i = 0; i < 2; i++) {
        yaml_node_t *seq = hwloc__tt_yaml_map_get(&doc, cgr, rk[i].yaml_key);
        const char *x, *y;
        if (hwloc__tt_yaml_seq_pair(&doc, seq, &x, &y)) {
          char buf[64];
          snprintf(info_key, sizeof(info_key), "%s%s", rk[i].prefix, variant);
          snprintf(buf, sizeof(buf), "%s,%s", x, y);
          hwloc_obj_add_info(osdev, info_key, buf);
        }
      }
    }

    snprintf(info_key, sizeof(info_key), "TenstorrentDispatchCoreCount_%s", variant);
    hwloc__tt_yaml_add_seq_count(osdev, info_key,
                                 hwloc__tt_yaml_map_get(&doc, v, "dispatch_cores"));
  }

  if (variants_len)
    hwloc_obj_add_info(osdev, "TenstorrentCoreDescriptorVariants", variants_buf);

out_doc:
  if (doc_ok) yaml_document_delete(&doc);
out_parser:
  if (parser_ok) yaml_parser_delete(&parser);
out_file:
  fclose(fh);
}
#endif /* HWLOC_HAVE_TT_YAML */

static void
hwloc__tt_add_metal_descriptor_infos(hwloc_obj_t osdev, uint16_t device_id)
{
  const char *root = getenv("TT_METAL_HOME");
  const struct hwloc__tt_desc_files *desc;
  struct stat st;
  char path[512];
  int n;

  if (!root || !*root) {
    static int warned_missing_tt_metal_home;
    if (!warned_missing_tt_metal_home) {
      warned_missing_tt_metal_home = 1;
      fprintf(stderr,
              "hwloc/tenstorrent: TT_METAL_HOME is not set; TenstorrentSocDescriptor and "
              "TenstorrentCoreDescriptor infos will not be added "
              "(set TT_METAL_HOME to the root of a tt-metal checkout).\n");
    }
    return;
  }

  desc = hwloc__tt_lookup_desc_files(device_id);
  if (!desc)
    return;

  n = snprintf(path, sizeof(path), "%s/tt_metal/soc_descriptors/%s", root, desc->soc_yaml);
  if (n <= 0 || (size_t) n >= sizeof(path)) {
    fprintf(stderr,
            "hwloc/tenstorrent: TT_METAL_HOME=%s: soc descriptor path exceeds buffer "
            "(tt_metal/soc_descriptors/%s)\n",
            root, desc->soc_yaml);
  } else if (stat(path, &st) < 0) {
    fprintf(stderr,
            "hwloc/tenstorrent: TT_METAL_HOME=%s: cannot access Tenstorrent soc descriptor %s: %s\n",
            root, path, strerror(errno));
  } else if (!S_ISREG(st.st_mode)) {
    fprintf(stderr,
            "hwloc/tenstorrent: TT_METAL_HOME=%s: Tenstorrent soc descriptor is not a regular file: %s\n",
            root, path);
  } else {
    hwloc_obj_add_info(osdev, "TenstorrentSocDescriptor", path);
#ifdef HWLOC_HAVE_TT_YAML
    hwloc__tt_yaml_parse_soc(path, osdev);
#endif
  }

  n = snprintf(path, sizeof(path), "%s/tt_metal/core_descriptors/%s", root, desc->core_yaml);
  if (n <= 0 || (size_t) n >= sizeof(path)) {
    fprintf(stderr,
            "hwloc/tenstorrent: TT_METAL_HOME=%s: core descriptor path exceeds buffer "
            "(tt_metal/core_descriptors/%s)\n",
            root, desc->core_yaml);
  } else if (stat(path, &st) < 0) {
    fprintf(stderr,
            "hwloc/tenstorrent: TT_METAL_HOME=%s: cannot access Tenstorrent core descriptor %s: %s\n",
            root, path, strerror(errno));
  } else if (!S_ISREG(st.st_mode)) {
    fprintf(stderr,
            "hwloc/tenstorrent: TT_METAL_HOME=%s: Tenstorrent core descriptor is not a regular file: %s\n",
            root, path);
  } else {
    hwloc_obj_add_info(osdev, "TenstorrentCoreDescriptor", path);
#ifdef HWLOC_HAVE_TT_YAML
    hwloc__tt_yaml_parse_core(path, osdev);
#endif
  }
}

static int hwloc__tt_dev_cmp(const void *a, const void *b)
{
  const struct hwloc__tt_dev *da = (const struct hwloc__tt_dev *) a;
  const struct hwloc__tt_dev *db = (const struct hwloc__tt_dev *) b;
  if (da->domain != db->domain)
    return (int) da->domain - (int) db->domain;
  if (da->bus != db->bus)
    return (int) da->bus - (int) db->bus;
  if (da->dev != db->dev)
    return (int) da->dev - (int) db->dev;
  return (int) da->func - (int) db->func;
}

static int hwloc__tt_read_info(int fd, struct hwloc__tt_dev *dev)
{
  struct tenstorrent_get_device_info info;

  memset(&info, 0, sizeof(info));
  info.in.output_size_bytes = sizeof(info.out);
  if (ioctl(fd, TENSTORRENT_IOCTL_GET_DEVICE_INFO, &info) < 0)
    return -1;

  dev->domain = info.out.pci_domain;
  dev->bus = (unsigned) (info.out.bus_dev_fn >> 8);
  dev->dev = (unsigned) ((info.out.bus_dev_fn >> 3) & 0x1fu);
  dev->func = (unsigned) (info.out.bus_dev_fn & 7u);
  dev->vendor_id = info.out.vendor_id;
  dev->device_id = info.out.device_id;
  dev->subsystem_vendor_id = info.out.subsystem_vendor_id;
  dev->subsystem_id = info.out.subsystem_id;
  return 0;
}

static int hwloc__tt_collect_sorted(struct hwloc__tt_dev **devs_out, unsigned *nr_out)
{
  DIR *dir;
  struct dirent *ent;
  struct hwloc__tt_dev *list = NULL;
  unsigned nr = 0, alloc = 0;

  *devs_out = NULL;
  *nr_out = 0;

  dir = opendir("/dev/tenstorrent");
  if (!dir) {
    if (errno != ENOENT && HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_TENSTORRENT))
      fprintf(stderr, "hwloc/tenstorrent: could not open /dev/tenstorrent: %s\n", strerror(errno));
    return 0;
  }

  while ((ent = readdir(dir)) != NULL) {
    char *end = NULL;
    long idl;
    int fd;
    struct hwloc__tt_dev dev;
    char path[96];

    if (ent->d_name[0] == '.')
      continue;
    idl = strtol(ent->d_name, &end, 10);
    if (!end || *end || idl < 0 || idl > 65535)
      continue;

    snprintf(path, sizeof(path), "/dev/tenstorrent/%ld", idl);
    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
      if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_TENSTORRENT))
        fprintf(stderr, "hwloc/tenstorrent: could not open %s: %s\n", path, strerror(errno));
      continue;
    }

    memset(&dev, 0, sizeof(dev));
    dev.kmd_id = (int) idl;
    if (hwloc__tt_read_info(fd, &dev) < 0) {
      if (HWLOC_SHOW_ERRORS(HWLOC_SHOWMSG_TENSTORRENT))
        fprintf(stderr, "hwloc/tenstorrent: TENSTORRENT_IOCTL_GET_DEVICE_INFO failed on %s: %s\n",
                path, strerror(errno));
      close(fd);
      continue;
    }
    close(fd);

    if (nr == alloc) {
      unsigned nalloc = alloc ? alloc * 2 : 8;
      struct hwloc__tt_dev *nlist = realloc(list, nalloc * sizeof(*list));
      if (!nlist) {
        free(list);
        closedir(dir);
        return -1;
      }
      list = nlist;
      alloc = nalloc;
    }
    list[nr++] = dev;
  }
  closedir(dir);

  if (nr)
    qsort(list, nr, sizeof(*list), hwloc__tt_dev_cmp);

  *devs_out = list;
  *nr_out = nr;
  return 0;
}

static int
hwloc_tenstorrent_discover(struct hwloc_backend *backend, struct hwloc_disc_status *dstatus)
{
  struct hwloc_topology *topology = backend->topology;
  enum hwloc_type_filter_e filter;
  struct hwloc__tt_dev *devs = NULL;
  unsigned nr, i, added = 0;

  assert(dstatus->phase == HWLOC_DISC_PHASE_IO);

  hwloc_topology_get_type_filter(topology, HWLOC_OBJ_OS_DEVICE, &filter);
  if (filter == HWLOC_TYPE_FILTER_KEEP_NONE)
    return 0;

  if (hwloc__tt_collect_sorted(&devs, &nr) < 0)
    return -1;

  for (i = 0; i < nr; i++) {
    hwloc_obj_t osdev, parent;
    char buffer[128];

    osdev = hwloc_alloc_setup_object(topology, HWLOC_OBJ_OS_DEVICE, HWLOC_UNKNOWN_INDEX);
    snprintf(buffer, sizeof(buffer), "tenstorrent%u", i);
    osdev->name = strdup(buffer);
    osdev->subtype = strdup("Tenstorrent");
    osdev->depth = HWLOC_TYPE_DEPTH_UNKNOWN;
    osdev->attr->osdev.types = HWLOC_OBJ_OSDEV_COPROC;

    snprintf(buffer, sizeof(buffer), "%d", devs[i].kmd_id);
    hwloc_obj_add_info(osdev, "TenstorrentDeviceIndex", buffer);

    snprintf(buffer, sizeof(buffer), "%04x:%02x:%02x.%u",
             devs[i].domain, devs[i].bus, devs[i].dev, devs[i].func);
    hwloc_obj_add_info(osdev, "PCIBusID", buffer);

    snprintf(buffer, sizeof(buffer), "0x%04x", (unsigned) devs[i].vendor_id);
    hwloc_obj_add_info(osdev, "PCIVendor", buffer);
    snprintf(buffer, sizeof(buffer), "0x%04x", (unsigned) devs[i].device_id);
    hwloc_obj_add_info(osdev, "PCIDevice", buffer);
    snprintf(buffer, sizeof(buffer), "0x%04x", (unsigned) devs[i].subsystem_vendor_id);
    hwloc_obj_add_info(osdev, "PCISubsystemVendor", buffer);
    snprintf(buffer, sizeof(buffer), "0x%04x", (unsigned) devs[i].subsystem_id);
    hwloc_obj_add_info(osdev, "PCISubsystemDevice", buffer);

    hwloc_obj_add_info(osdev, "GPUVendor", "Tenstorrent");
    {
      const struct hwloc__tt_desc_files *desc = hwloc__tt_lookup_desc_files(devs[i].device_id);
      if (desc)
        hwloc_obj_add_info(osdev, "GPUModel", desc->model);
    }
    hwloc__tt_add_metal_descriptor_infos(osdev, devs[i].device_id);

    parent = hwloc_pci_get_parent_by_busid(topology, devs[i].domain, devs[i].bus, devs[i].dev, devs[i].func);
    if (!parent)
      parent = hwloc_get_root_obj(topology);

    hwloc_insert_object_by_parent(topology, parent, osdev);
    added++;
  }

  free(devs);

  if (added)
    hwloc_modify_infos(hwloc_topology_get_infos(topology), HWLOC_MODIFY_INFOS_OP_ADD, "Backend", "Tenstorrent");
  return 0;
}

static struct hwloc_backend *
hwloc_tenstorrent_component_instantiate(struct hwloc_topology *topology,
                                        struct hwloc_disc_component *component,
                                        unsigned excluded_phases __hwloc_attribute_unused,
                                        const void *_data1 __hwloc_attribute_unused,
                                        const void *_data2 __hwloc_attribute_unused,
                                        const void *_data3 __hwloc_attribute_unused)
{
  struct hwloc_backend *backend;

  backend = hwloc_backend_alloc(topology, component, 0);
  if (!backend)
    return NULL;
  backend->discover = hwloc_tenstorrent_discover;
  return backend;
}

static struct hwloc_disc_component hwloc_tenstorrent_disc_component = {
  "tenstorrent",
  HWLOC_DISC_PHASE_IO,
  HWLOC_DISC_PHASE_GLOBAL,
  hwloc_tenstorrent_component_instantiate,
  10,
  1,
  NULL
};

static int
hwloc_tenstorrent_component_init(unsigned long flags)
{
  if (flags)
    return -1;
  if (hwloc_plugin_check_namespace("tenstorrent", "hwloc_backend_alloc") < 0)
    return -1;
  return 0;
}

#ifdef HWLOC_INSIDE_PLUGIN
HWLOC_DECLSPEC extern const struct hwloc_component hwloc_tenstorrent_component;
#endif

const struct hwloc_component hwloc_tenstorrent_component = {
  HWLOC_COMPONENT_ABI,
  hwloc_tenstorrent_component_init, NULL,
  HWLOC_COMPONENT_TYPE_DISC,
  0,
  &hwloc_tenstorrent_disc_component
};
