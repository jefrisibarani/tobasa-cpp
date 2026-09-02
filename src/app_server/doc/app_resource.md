# App Resource Management

## Overview

Tobasa implements a **flexible asset loading system** through `app::Resource`, supporting both embedded binary resources and filesystem fallback. \
This enables containerized deployments without external file dependencies while maintaining development flexibility with disk-based resource access.

The public resource API is declared by [`app::Resource`](../src/app_resource.h#L10),
and the main lookup behavior is implemented by [`Resource::get`](../src/app_resource.cpp#L73).

### Key Features

- **Dual-mode operation**: Embedded binary resources or filesystem fallback
- **Zero-copy access**: Uses `nonstd::span<const unsigned char>` for direct memory access
- **Context-aware routing**: Resources organized by type and context
- **Automatic fallback**: Gracefully degrades from in-memory to disk access
- **Single-file deployment**: Optional all-in-one executable with embedded assets
- **Optimized build**: Comment stripping and selective resource embedding
- **Runtime configuration**: Toggle resource source at runtime via `web::conf::Webapp::useInMemoryResources`

---

## Resource Types

Resources are organized by type and accessed via context identifiers. Each type has a specific purpose and loading strategy:

| Type | Location | Context | Purpose | Always Embedded |
|------|----------|---------|---------|-----------------|
| **HTML Templates** | `views/` | `appview` | Inja2 templates for UI rendering, page layouts, partials | No* |
| **LIS Templates** | `views_lis/` | `appview_lis` | Healthcare Laboratory Information System (LIS) module views | No* |
| **Static Assets** | `wwwroot/` | `wwwroot` | CSS stylesheets, JavaScript files, images, web fonts, media | No* |
| **Configuration** | `configuration_embed/` | `config` | Default `appsettings.json` and runtime configuration files | **YES** |
| **TLS Certificates** | `tls_asset/` | `tls_asset` | X.509 server certificates, private keys, certificate chains | **YES** |
| **Timezone Data** | `res/tzdata/` | `tzdata` | IANA timezone database with comment filtering | Conditional** |

**Legend:**
- \* Requires `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON` flag
- \*\* Requires `TOBASA_BUILD_IN_MEMORY_TZDB=ON` flag
- Embedded resources compiled into binary at build time

### Resource Context Mapping

```
appview          → views/                   (HTML templates)
appview_lis      → views_lis/               (LIS-specific templates)
wwwroot          → wwwroot/                 (Static web assets)
config           → configuration_embed/     (Default configuration)
tls_asset        → tls_asset/               (TLS certificates/keys)
```

---

## Loading Strategy

### Resource Resolution Flow

Path normalization is performed as part of the resource lookup process.


**Directory Structure:**
```
app_server/
├── webservice.exe
├── views/
├── views_lis/
├── wwwroot/
│   ├── assets/
│   │   ├── images/
│   │   ├── css/
│   │   └── js/
├── configuration/
│   └── appsettings.json
└── tls_asset/
    ├── server.crt
    └── server.key
```


```
┌─────────────────────────────────┐
│  Application Requests Resource  │
│  Resource::get(path, context)   │
└────────────┬────────────────────┘
             │
             ├─ Path Normalization
             │  (backslash → forward slash)
             │
             ├─ Context Validation
             │  (appview, wwwroot, config, etc.)
             │
             ├─ In-Memory Lookup
             │  if (TOBASA_BUILD_IN_MEMORY_RESOURCES)
             │  └─ Compiled resource array lookup
             │
             ├─ Fallback to Filesystem
             │  Read from disk: base_path/context/path
             │
             └─ Return Result
                nonstd::span<const unsigned char>
```

### Resource Lookup Flow for Templates

```cpp
// views/index.html with {% include 'partial.html' %}

Resource::getString("views/index.html", "appview")
  → [In-Memory]  Lookup compiled template array
  → [Fallback]   Read from disk: <base>/views/index.html
  
// For included partial:
// Note: extends/includes resolve in appview context automatically
Resource::getString("partial.html", "appview")  // NOT "views/partial.html"
  → [In-Memory]  Lookup compiled "partial.html"
  → [Fallback]   Read from disk: <base>/views/partial.html
```

---

## Build Configuration

### CMake Build Flags

#### Top-Level Configuration (`CMakeLists.txt`)

```cmake
# Enable in-memory resource embedding
option(TOBASA_BUILD_IN_MEMORY_RESOURCES "Whether to use compiled resources"     ON )

# Enable in-memory timezone database
option(TOBASA_BUILD_IN_MEMORY_TZDB      "Whether to use compiled timezone data" ON )
```

#### Module-Level Override (`src/app_server/CMakeLists.txt`)

```cmake
# App server can override global defaults
option(TOBASA_BUILD_IN_MEMORY_RESOURCES "Whether to use compiled resources"     OFF)
option(TOBASA_BUILD_IN_MEMORY_TZDB      "Whether to use compiled timezone data" OFF)

# Conditional source file inclusion
if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
    target_compile_definitions(app_server PRIVATE TOBASA_BUILD_IN_MEMORY_RESOURCES)
    target_sources(app_server PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/resources/template_resources.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/resources/wwwroot_resources.cpp
        # ... etc
    )
endif()

# LIS templates only if both conditions met
if(TOBASA_BUILD_LIS_ENGINE AND TOBASA_BUILD_IN_MEMORY_RESOURCES)
    target_sources(app_server PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/resources/template_lis_resources.cpp
    )
endif()
```
---

## Resource Generation

### Generation Tools

Two CMake-based code generators convert binary files to C++ static arrays:

#### 1. `generate_resources_file` - Binary File Conversion

**Purpose:** Convert directory of binary files to C++ static arrays

**Location:** `cmake/generate_resources.cpp`

**Command:**
```
generate_resources_file <source_dir> <base_folder> <output_file> <function_name> <namespace>
```

**Parameters:**
- `source_dir`: Input directory (e.g., `views/`)
- `base_folder`: Virtual path prefix in resource context
- `output_file`: Generated C++ source file
- `function_name`: Generated lookup function name
- `namespace`: Output namespace

**Example CMake Usage:**
```cmake
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources/template_resources.cpp
    COMMAND generate_resources_file 
        ${PROJECT_SOURCE_DIR}/views
        views
        ${CMAKE_CURRENT_BINARY_DIR}/resources/template_resources.cpp
        getTemplateResources
        appview
    DEPENDS generate_resources_file ${PROJECT_SOURCE_DIR}/views
    COMMENT "Generating HTML template resources..."
)
```

**Output C++ Structure:**
```cpp
namespace tbs::res::appview {
   // For each file: views/index.html
   const size_t resources_0_size = 1234;
   static const unsigned char resources_0[] = {
      0x3c, 0x21, 0x44, 0x4f, 0x43, 0x54, 0x59, 0x50,  // <!DOCTYP
      0x45, 0x20, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x0a,  // E html>.
      // ... 16 bytes per line
   };
   static constexpr nonstd::span<const unsigned char> resources_0_span(resources_0, resources_0_size);
   
   // Lookup function
   nonstd::span<const unsigned char> getTemplateResources(const std::string& path) 
   {
      if (path == "views/index.html")
         return resources_0_span;
      return nonstd::span<const unsigned char>();
   }
}
```

#### 2. `generate_tzdata_assets` - Timezone Data Conversion

**Purpose:** Convert IANA timezone data to C++ arrays with comment filtering

**Location:** `cmake/generate_tzdata_assets.cpp`

**Features:**
- Filters out comment lines (starting with `#`)
- Removes XML-style comments (`<!-- ... -->`)
- Converts in text mode (preserves line endings for Windows)
- Outputs to `tzdata_filtered/` during build
- Reduces file size by 30-40% through comment removal

**Command:**
```
generate_tzdata_assets_file <source_dir> <base_folder> <output_file> <function_name> <namespace>
```

**Example:**
```cmake
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_resources.cpp
    COMMAND generate_tzdata_assets_file
        ${PROJECT_SOURCE_DIR}/res/tzdata
        tzdata
        ${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_resources.cpp
        getTzdataResources
        tzdata
)
```

### Generated Resource Commands

| Resource | Source | Output | Function | Context |
|----------|--------|--------|----------|---------|
| HTML Templates | `views/` | `template_resources.cpp` | `getTemplateResources` | `appview` |
| LIS Templates | `views_lis/` | `template_lis_resources.cpp` | `getTemplateLisResources` | `appview_lis` |
| Static Assets | `wwwroot/` | `wwwroot_resources.cpp` | `getWwwrootResources` | `wwwroot` |
| Configuration | `configuration_embed/` | `config_resources.cpp` | `getConfigResources` | `config` |
| TLS Assets | `tls_asset/` | `tls_asset_resources.cpp` | `getTlsAssetResources` | `tls_asset` |
| Timezone Data | `res/tzdata/` | `tzdata_resources.cpp` | `getTzdataResources` | `tzdata` |

**Conditional Generation:**
- Config & TLS: Always generated
- Templates & wwwroot: Only if `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON`
- LIS templates: Only if `TOBASA_BUILD_LIS_ENGINE=ON` AND `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON`
- Timezone data: Only if `TOBASA_BUILD_IN_MEMORY_TZDB=ON`

---

## Best Practices

### 1. Resource Organization

**DO:** Keep resources organized by type and context
```
views/                    # appview context
  ├── index.html
  ├── dashboard/
  │   └── dashboard.html
  └── common/
      ├── header.html
      └── footer.html

wwwroot/                  # wwwroot context
  ├── css/
  │   └── main.css
  ├── js/
  │   └── app.js
  └── assets/
      └── images/
```

**DON'T:** Mix resources from different contexts in one folder

### 2. Path Consistency

**DO:** Use forward slashes in resource paths
```cpp
app::Resource::get("views/dashboard/panel.html", "appview");
```

**DON'T:** Use backslashes (they'll be normalized, but inconsistent)
```cpp
app::Resource::get("views\\dashboard\\panel.html", "appview");  // Works but ugly
```
