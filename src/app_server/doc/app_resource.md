# App Resource Management

## Overview

Tobasa uses `app::Resource` to load files either from the executable or from
the filesystem. This lets you ship one executable when needed, while still
letting you work with normal files during development.

The public API is declared by [`app::Resource`](../src/app_resource.h#L10).
The main lookup code is in [`Resource::get`](../src/app_resource.cpp#L73).

### Key Features

- **Two resource sources**: use embedded data or files on disk.
- **Direct memory access**: data is returned as
  `nonstd::span<const unsigned char>`.
- **Separate contexts**: templates, web files, configuration, and other files
  use different context names.
- **Filesystem fallback**: a missing embedded file can be read from disk.
- **Single-file deployment**: include selected resources in the executable.
- **Smaller builds**: embed only the resource groups you need.
- **Runtime selection**: use `web::conf::Webapp::useInMemoryResources` to
  choose embedded or disk resources when the build supports both.

---

## Resource Types

Resources are grouped by type. When you request one, pass the matching context
name as the second argument:

| Type | Location | Context | Purpose | Always Embedded |
|------|----------|---------|---------|-----------------|
| **HTML Templates** | `views/` | `appview` | Inja2 templates, layouts, and partials for the web UI | No* |
| **LIS Templates** | `views_lis/` | `appview_lis` | Templates used by the LIS module | No* |
| **Static Assets** | `wwwroot/` | `wwwroot` | CSS, JavaScript, images, fonts, and other web files | No* |
| **Configuration** | `configuration_embed/` | `config` | Default `appsettings.json` and related configuration files | **YES** |
| **TLS Certificates** | `tls_asset/` | `tls_asset` | Server certificates, private keys, and certificate chains | **YES** |
| **Timezone Data** | `res/tzdata/` | `tzdata` | IANA timezone data with comments removed | Conditional** |

**Legend:**
- `*` Needs `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON`.
- `**` Needs `TOBASA_BUILD_IN_MEMORY_TZDB=ON`.
- Embedded resources are placed in the executable during the build.

### Resource Context Mapping

Use these context names when calling the resource API:

```
appview          -> views/                   (HTML templates)
appview_lis      -> views_lis/               (LIS templates)
wwwroot          -> wwwroot/                 (static web files)
config           -> configuration_embed/     (default configuration)
tls_asset        -> tls_asset/               (TLS certificates and keys)
```

---

## Loading Strategy

### Resource Resolution Flow

The resource lookup code normalizes the path before looking for the file. It
also checks that the context is valid.

**Directory Structure:**
```
app_server/
|- webservice.exe
|- views/
|- views_lis/
|- wwwroot/
|  |- assets/
|     |- images/
|     |- css/
|     `- js/
|- configuration/
|  `- appsettings.json
`- tls_asset/
   |- server.crt
   `- server.key
```

```
+---------------------------------+
|  Application requests a file   |
|  Resource::get(path, context)  |
+---------------+----------------+
                |
                +- Path normalization
                |  (backslash -> forward slash)
                |
                +- Context validation
                |  (appview, wwwroot, config, and so on)
                |
                +- Embedded lookup
                |  when the resource build option is enabled
                |  `- Search the compiled resource array
                |
                +- Filesystem fallback
                |  Read base_path/context/path from disk
                |
                `- Return the result
                   nonstd::span<const unsigned char>
```

### Resource Lookup Flow for Templates

For a template, include the path and the `appview` context:

```cpp
// views/index.html with {% include 'partial.html' %}

Resource::getString("views/index.html", "appview")
  -> [Embedded]  Search the compiled template array
  -> [Fallback]  Read from disk: <base>/views/index.html
  
// For an included partial:
// extends/includes use the appview context automatically.
Resource::getString("partial.html", "appview")  // NOT "views/partial.html"
  -> [Embedded]  Search for "partial.html"
  -> [Fallback]  Read from disk: <base>/views/partial.html
```

---

## Build Configuration

### CMake Build Flags

#### Top-Level Configuration (`CMakeLists.txt`)

```cmake
# Embed application resources in the executable
option(TOBASA_BUILD_IN_MEMORY_RESOURCES "Whether to use compiled resources"     ON )

# Embed timezone data in the executable
option(TOBASA_BUILD_IN_MEMORY_TZDB      "Whether to use compiled timezone data" ON )
```

#### Module-Level Override (`src/app_server/CMakeLists.txt`)

The app server can override the global defaults:

```cmake
# App server defaults
option(TOBASA_BUILD_IN_MEMORY_RESOURCES "Whether to use compiled resources"     OFF)
option(TOBASA_BUILD_IN_MEMORY_TZDB      "Whether to use compiled timezone data" OFF)

# Add generated resource files only when embedding is enabled
if(TOBASA_BUILD_IN_MEMORY_RESOURCES)
    target_compile_definitions(app_server PRIVATE TOBASA_BUILD_IN_MEMORY_RESOURCES)
    target_sources(app_server PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/resources/template_resources.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/resources/wwwroot_resources.cpp
        # ... etc
    )
endif()

# Build LIS templates only when both options are enabled
if(TOBASA_BUILD_LIS_ENGINE AND TOBASA_BUILD_IN_MEMORY_RESOURCES)
    target_sources(app_server PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/resources/template_lis_resources.cpp
    )
endif()
```

---

## Resource Generation

### Generation Tools

The build uses CMake tools to turn files into C++ static arrays:

#### 1. `generate_resources_file` - Binary File Conversion

**Purpose:** Turn a directory of files into C++ resource arrays.

**Location:** `cmake/generate_resources.cpp`

**Command:**
```
generate_resources_file <source_dir> <base_folder> <output_file> <function_name> <namespace>
```

**Parameters:**
- `source_dir`: Directory containing the input files, such as `views/`.
- `base_folder`: Virtual path prefix used by the resource context.
- `output_file`: C++ file to generate.
- `function_name`: Lookup function to generate.
- `namespace`: C++ namespace for the generated code.

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
   // One entry is generated for each file, for example views/index.html.
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

**Purpose:** Turn IANA timezone files into C++ arrays and remove comments.

**Location:** `cmake/generate_tzdata_assets.cpp`

**Features:**
- Removes lines that start with `#`.
- Removes XML-style comments such as `<!-- ... -->`.
- Uses text mode so Windows line endings are kept correctly.
- Writes filtered files to `tzdata_filtered/` during the build.
- Reduces the generated data size by about 30-40% by removing comments.

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
- Configuration and TLS resources are always generated.
- Templates and `wwwroot` are generated only when
  `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON`.
- LIS templates need both `TOBASA_BUILD_LIS_ENGINE=ON` and
  `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON`.
- Timezone data is generated only when `TOBASA_BUILD_IN_MEMORY_TZDB=ON`.

---

## Best Practices

### 1. Resource Organization

Keep each kind of file in its matching context:

**DO:**
```
views/                    # appview context
  |- index.html
  |- dashboard/
  |  `- dashboard.html
  `- common/
     |- header.html
     `- footer.html

wwwroot/                  # wwwroot context
  |- css/
  |  `- main.css
  |- js/
  |  `- app.js
  `- assets/
     `- images/
```

**DON'T:** Put resources from different contexts in the same folder.

### 2. Path Consistency

Use forward slashes in resource paths:

```cpp
app::Resource::get("views/dashboard/panel.html", "appview");
```

Backslashes are normalized, but using them makes paths harder to read and can
cause inconsistent paths between platforms:

```cpp
app::Resource::get("views\\dashboard\\panel.html", "appview");  // Works but avoid it
```
