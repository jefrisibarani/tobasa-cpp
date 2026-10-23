# App Resource Management

## Overview

`app::Resource` looks up resources compiled into the executable and returns a
non-owning byte span. It does not read files from disk. Callers such as the
app server's static-file handler implement their own disk fallback.

The public API is declared by [`app::Resource`](../src/app_resource.h#L10).
The main lookup code is in [`Resource::get`](../src/app_resource.cpp#L73).

### Key Features

- **Embedded lookup**: read the resource arrays generated during the build.
- **Direct memory access**: data is returned as
  `nonstd::span<const unsigned char>`.
- **Separate contexts**: templates, web files, configuration, and other files
  use different context names.
- **Caller-selected disk access**: filesystem reads are performed by the
  caller, not by `Resource::get()`.
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
| **Timezone Data** | `res/tzdata/` | Not an `app::Resource` context | Data for the bundled date/tz implementation | Conditional** |

**Legend:**
- `*` Needs `TOBASA_BUILD_IN_MEMORY_RESOURCES=ON`.
- `**` Timezone assets are generated when `TOBASA_BUILD_IN_MEMORY_TZDB=ON`
  and the selected standard library does not provide the required timezone
  support.
- Configuration and TLS resource arrays are generated in every app-server
  build. Templates and web-root arrays are conditional.

### Resource Context Mapping

Use these context names when calling the resource API:

```
appview          -> views/                   (HTML templates)
appview_lis      -> views_lis/               (LIS templates)
wwwroot          -> wwwroot/                 (static web files)
config           -> configuration_embed/     (default configuration)
tls_asset        -> tls_asset/               (TLS certificates and keys)
```

`tzdata` is consumed by the date/time implementation, not by
`app::Resource::get()`.

---

## Loading Strategy

### Resource Resolution Flow

The lookup normalizes path separators and searches the generated array for the
requested context. It returns an empty span for a missing resource, an
unsupported context, or a context that was not compiled into this build. It
does not verify a context against a separate registry or open a filesystem
path.

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
                |  (using `/` as the separator)
                |
                +- Embedded lookup
                |  `- Search the compiled array for the requested context
                |
                `- Return a span, or an empty span when not found
                   (no disk read is performed)
```

### Resource Lookup Flow for Templates

For a template, include the path and the `appview` context:

```cpp
// views/index.html with {% include 'partial.html' %}

Resource::getString("views/index.html", "appview")
  -> Search the compiled template array when embedding is enabled
  -> Return an empty string when the resource is not embedded
  
// For an included partial:
// extends/includes use the appview context automatically.
Resource::getString("partial.html", "appview")  // NOT "views/partial.html"
  -> Search the compiled array for "partial.html"
```

Template rendering selects embedded or filesystem access in its caller. The
app server's `/` handler also checks the document root when an embedded static
file is missing. Those caller-specific fallbacks are not performed by
`Resource::get()`.

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

The top-level project declares both options with an `ON` default. The app
server directory also declares them with an `OFF` default, but CMake does not
replace an option already set in the top-level cache. In a normal root build,
the top-level values therefore control generation. The resource option
controls templates and `wwwroot`; configuration and TLS arrays are always
generated. The runtime `useInMemoryResources` setting can choose between
embedded and disk resources only when the binary was built with the embedded
template/web-root arrays.

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
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_assets.cpp
    COMMAND generate_tzdata_assets_file
        ${PROJECT_SOURCE_DIR}/res/tzdata
        tzdata
        ${CMAKE_CURRENT_BINARY_DIR}/resources/tzdata_assets.cpp
        getTzdataAsset
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
| Timezone Data | `res/tzdata/` | `` | `getTzdataAsset` | Date/tz asset data; not an `app::Resource` context |

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

The resource path normalizer uses `/` as its separator; it does not convert
backslashes. A backslash is treated as part of a path component, so use
forward slashes consistently:

```cpp
app::Resource::get("views/dashboard/panel.html", "appview");
```
