R bindings for the MITIE C++ library (Named Entity Recognition, NLP utilities).

# 📦 Features

- Tokenization
- Named Entity Recognition (NER)
- Direct interface to MITIE C++ library via Rcpp
- Lightweight and fast (no Python dependency)

---

# Installation

## Mitie installation

```r
run_cmd <- function(cmd, args = character(), error_msg = NULL, echo = TRUE) {
  if (echo) {
    shown <- paste(c(shQuote(cmd), args), collapse = " ")
    message(">> ", shown)
  }

  status <- system2(command = cmd, args = args)
  if (!identical(status, 0L)) {
    stop(if (is.null(error_msg)) paste("Command failed:", cmd) else error_msg, call. = FALSE)
  }
  invisible(TRUE)
}

check_cmd <- function(cmd) {
  nzchar(Sys.which(cmd))
}

write_mitie_env <- function(include_dir,
                            lib_dir,
                            bin_dir = NULL,
                            scope = c("session", "renviron")) {
  scope <- match.arg(scope)

  Sys.setenv(
    MITIE_INCLUDE = include_dir,
    MITIE_LIB = lib_dir
  )

  if (!is.null(bin_dir)) {
    Sys.setenv(MITIE_BIN = bin_dir)
  }

  if (scope == "renviron") {
    renv <- path.expand("~/.Renviron")
    old <- if (file.exists(renv)) readLines(renv, warn = FALSE) else character()

    old <- old[!grepl("^MITIE_INCLUDE=", old)]
    old <- old[!grepl("^MITIE_LIB=", old)]
    old <- old[!grepl("^MITIE_BIN=", old)]

    new_lines <- c(
      old,
      sprintf('MITIE_INCLUDE="%s"', normalizePath(include_dir, winslash = "/", mustWork = FALSE)),
      sprintf('MITIE_LIB="%s"', normalizePath(lib_dir, winslash = "/", mustWork = FALSE))
    )

    if (!is.null(bin_dir)) {
      new_lines <- c(
        new_lines,
        sprintf('MITIE_BIN="%s"', normalizePath(bin_dir, winslash = "/", mustWork = FALSE))
      )
    }

    writeLines(new_lines, renv)
    message("Variables added to ~/.Renviron")
  }

  message("MITIE_INCLUDE = ", Sys.getenv("MITIE_INCLUDE"))
  message("MITIE_LIB     = ", Sys.getenv("MITIE_LIB"))
  message("MITIE_BIN     = ", Sys.getenv("MITIE_BIN"))

  invisible(TRUE)
}

install_mitie_linux <- function(use_sudo = TRUE,
                                persist = TRUE,
                                prefix = "/usr/local",
                                clean = TRUE) {
  if (!check_cmd("apt-get")) {
    stop("This Linux function is intended for Debian/Ubuntu (apt-get).", call. = FALSE)
  }

  apt_pkgs <- c("build-essential", "cmake", "git")

  if (use_sudo) {
    run_cmd("sudo", c("apt-get", "update"), "apt-get update failed.")
    run_cmd("sudo", c("apt-get", "install", "-y", apt_pkgs),
            "APT installation of MITIE build dependencies failed.")
  } else {
    run_cmd("apt-get", c("update"), "apt-get update failed.")
    run_cmd("apt-get", c("install", "-y", apt_pkgs),
            "APT installation of MITIE build dependencies failed.")
  }

  src_dir <- file.path(tempdir(), "MITIE-src")
  if (dir.exists(src_dir) && clean) {
    unlink(src_dir, recursive = TRUE, force = TRUE)
  }

  run_cmd("git", c("clone", "--depth", "1", "https://github.com/mit-nlp/MITIE.git", src_dir),
          "Failed to clone MITIE repository.")

  build_dir <- file.path(src_dir, "mitielib", "build")
  dir.create(build_dir, recursive = TRUE, showWarnings = FALSE)

  run_cmd("cmake", c(
    "-S", file.path(src_dir, "mitielib"),
    "-B", build_dir,
    "-DCMAKE_BUILD_TYPE=Release",
    paste0("-DCMAKE_INSTALL_PREFIX=", prefix)
  ), "CMake configuration for MITIE failed.")

  run_cmd("cmake", c("--build", build_dir, "--config", "Release", "--parallel"),
          "MITIE build failed.")

  if (use_sudo && normalizePath(prefix, winslash = "/", mustWork = FALSE) == "/usr/local") {
    run_cmd("sudo", c("cmake", "--install", build_dir),
            "MITIE installation failed.")
  } else {
    run_cmd("cmake", c("--install", build_dir),
            "MITIE installation failed.")
  }

  include_candidates <- c(
    file.path(prefix, "include"),
    "/usr/local/include",
    "/usr/include"
  )

  lib_candidates <- c(
    file.path(prefix, "lib"),
    file.path(prefix, "lib64"),
    "/usr/local/lib",
    "/usr/lib/x86_64-linux-gnu",
    "/usr/lib"
  )

  bin_candidates <- c(
    file.path(prefix, "bin"),
    "/usr/local/bin",
    "/usr/bin"
  )

  include_dir <- include_candidates[
    file.exists(file.path(include_candidates, "mitie.h")) |
      file.exists(file.path(include_candidates, "mitie", "mitie.h"))
  ][1]

  lib_dir <- lib_candidates[
    file.exists(file.path(lib_candidates, "libmitie.so")) |
      file.exists(file.path(lib_candidates, "libmitie.a")) |
      file.exists(file.path(lib_candidates, "libmitie.dll.a"))
  ][1]

  bin_dir <- bin_candidates[
    file.exists(file.path(bin_candidates, "mitie")) |
      file.exists(file.path(bin_candidates, "ner_stream")) |
      file.exists(file.path(bin_candidates, "train_ner"))
  ][1]

  if (is.na(include_dir)) {
    include_dir <- file.path(prefix, "include")
    warning("MITIE header not detected automatically.")
  }

  if (is.na(lib_dir)) {
    lib_dir <- file.path(prefix, "lib")
    warning("MITIE library not detected automatically.")
  }

  if (is.na(bin_dir)) {
    bin_dir <- file.path(prefix, "bin")
  }

  write_mitie_env(
    include_dir = include_dir,
    lib_dir = lib_dir,
    bin_dir = bin_dir,
    scope = if (persist) "renviron" else "session"
  )

  invisible(list(
    prefix = prefix,
    include = include_dir,
    lib = lib_dir,
    bin = bin_dir,
    source = src_dir
  ))
}

install_mitie_macos <- function(persist = TRUE,
                                prefix = "/usr/local",
                                clean = TRUE) {
  if (!check_cmd("brew")) {
    stop("Homebrew is not available.", call. = FALSE)
  }

  run_cmd("brew", c("install", "cmake", "git"),
          "Homebrew installation of MITIE build dependencies failed.")

  src_dir <- file.path(tempdir(), "MITIE-src")
  if (dir.exists(src_dir) && clean) {
    unlink(src_dir, recursive = TRUE, force = TRUE)
  }

  run_cmd("git", c("clone", "--depth", "1", "https://github.com/mit-nlp/MITIE.git", src_dir),
          "Failed to clone MITIE repository.")

  build_dir <- file.path(src_dir, "mitielib", "build")
  dir.create(build_dir, recursive = TRUE, showWarnings = FALSE)

  run_cmd("cmake", c(
    "-S", file.path(src_dir, "mitielib"),
    "-B", build_dir,
    "-DCMAKE_BUILD_TYPE=Release",
    paste0("-DCMAKE_INSTALL_PREFIX=", prefix)
  ), "CMake configuration for MITIE failed.")

  run_cmd("cmake", c("--build", build_dir, "--config", "Release", "--parallel"),
          "MITIE build failed.")

  run_cmd("cmake", c("--install", build_dir),
          "MITIE installation failed.")

  brew_prefix <- trimws(system2("brew", "--prefix", stdout = TRUE))

  include_candidates <- c(
    file.path(prefix, "include"),
    file.path(brew_prefix, "include")
  )

  lib_candidates <- c(
    file.path(prefix, "lib"),
    file.path(brew_prefix, "lib")
  )

  bin_candidates <- c(
    file.path(prefix, "bin"),
    file.path(brew_prefix, "bin")
  )

  include_dir <- include_candidates[
    file.exists(file.path(include_candidates, "mitie.h")) |
      file.exists(file.path(include_candidates, "mitie", "mitie.h"))
  ][1]

  lib_dir <- lib_candidates[
    file.exists(file.path(lib_candidates, "libmitie.dylib")) |
      file.exists(file.path(lib_candidates, "libmitie.a"))
  ][1]

  bin_dir <- bin_candidates[
    file.exists(file.path(bin_candidates, "mitie")) |
      file.exists(file.path(bin_candidates, "ner_stream")) |
      file.exists(file.path(bin_candidates, "train_ner"))
  ][1]

  if (is.na(include_dir)) {
    include_dir <- file.path(prefix, "include")
    warning("MITIE header not detected automatically.")
  }

  if (is.na(lib_dir)) {
    lib_dir <- file.path(prefix, "lib")
    warning("MITIE library not detected automatically.")
  }

  if (is.na(bin_dir)) {
    bin_dir <- file.path(prefix, "bin")
  }

  write_mitie_env(
    include_dir = include_dir,
    lib_dir = lib_dir,
    bin_dir = bin_dir,
    scope = if (persist) "renviron" else "session"
  )

  invisible(list(
    prefix = prefix,
    include = include_dir,
    lib = lib_dir,
    bin = bin_dir,
    source = src_dir
  ))
}


install_mitie_windows_msys2 <- function(rtools_root = "C:/rtools45",
                                        repo = "ucrt64",
                                        persist = TRUE,
                                        prefix = "C:/rtools45/ucrt64",
                                        clean = TRUE) {
  run_cmd <- function(cmd, args = character(), error_msg = NULL, echo = TRUE) {
    if (echo) {
      message(">> ", paste(c(shQuote(cmd), args), collapse = " "))
    }
    status <- system2(command = cmd, args = args)
    if (!identical(status, 0L)) {
      stop(if (is.null(error_msg)) paste("Command failed:", cmd) else error_msg, call. = FALSE)
    }
    invisible(TRUE)
  }

  write_mitie_env <- function(include_dir,
                              lib_dir,
                              bin_dir = NULL,
                              scope = c("session", "renviron")) {
    scope <- match.arg(scope)

    Sys.setenv(
      MITIE_INCLUDE = include_dir,
      MITIE_LIB = lib_dir
    )

    if (!is.null(bin_dir)) {
      Sys.setenv(MITIE_BIN = bin_dir)
    }

    if (scope == "renviron") {
      renv <- path.expand("~/.Renviron")
      old <- if (file.exists(renv)) readLines(renv, warn = FALSE) else character()

      old <- old[!grepl("^MITIE_INCLUDE=", old)]
      old <- old[!grepl("^MITIE_LIB=", old)]
      old <- old[!grepl("^MITIE_BIN=", old)]

      new_lines <- c(
        old,
        sprintf('MITIE_INCLUDE="%s"', normalizePath(include_dir, winslash = "/", mustWork = FALSE)),
        sprintf('MITIE_LIB="%s"', normalizePath(lib_dir, winslash = "/", mustWork = FALSE))
      )

      if (!is.null(bin_dir)) {
        new_lines <- c(
          new_lines,
          sprintf('MITIE_BIN="%s"', normalizePath(bin_dir, winslash = "/", mustWork = FALSE))
        )
      }

      writeLines(new_lines, renv)
      message("Variables added to ~/.Renviron")
    }

    message("MITIE_INCLUDE = ", Sys.getenv("MITIE_INCLUDE"))
    message("MITIE_LIB     = ", Sys.getenv("MITIE_LIB"))
    message("MITIE_BIN     = ", Sys.getenv("MITIE_BIN"))

    invisible(TRUE)
  }

  patch_mitie_cmakelists <- function(cmake_file) {
    if (!file.exists(cmake_file)) {
      stop("CMakeLists.txt not found: ", cmake_file, call. = FALSE)
    }

    txt <- readLines(cmake_file, warn = FALSE)

    txt <- gsub(
      "cmake_minimum_required\\s*\\(\\s*VERSION\\s*2\\.[0-9.]+\\s*\\)",
      "cmake_minimum_required(VERSION 3.5)",
      txt,
      perl = TRUE
    )

    has_project <- any(grepl("^\\s*project\\s*\\(", txt, ignore.case = TRUE))

    if (!has_project) {
      idx <- grep("^\\s*cmake_minimum_required\\s*\\(", txt, ignore.case = TRUE)
      if (length(idx) >= 1) {
        txt <- append(txt, "project(MITIE)", after = idx[1])
      } else {
        txt <- c("cmake_minimum_required(VERSION 3.5)", "project(MITIE)", txt)
      }
    }

    writeLines(txt, cmake_file, useBytes = TRUE)
    invisible(TRUE)
  }

  first_existing <- function(paths) {
    hit <- paths[file.exists(paths)]
    if (length(hit)) hit[1] else NA_character_
  }

  bash <- file.path(rtools_root, "usr", "bin", "bash.exe")
  git_exe <- file.path(rtools_root, "usr", "bin", "git.exe")

  if (!file.exists(bash)) {
    stop("bash.exe not found in Rtools/MSYS2: ", bash, call. = FALSE)
  }

  prefix_root <- switch(
    repo,
    ucrt64  = file.path(rtools_root, "ucrt64"),
    clang64 = file.path(rtools_root, "clang64"),
    mingw64 = file.path(rtools_root, "mingw64"),
    stop("repo must be 'ucrt64', 'clang64' or 'mingw64'", call. = FALSE)
  )

  pkg_prefix <- switch(
    repo,
    ucrt64  = "mingw-w64-ucrt-x86_64",
    clang64 = "mingw-w64-clang-x86_64",
    mingw64 = "mingw-w64-x86_64"
  )

  pkgs <- c(
    "make",
    "git",
    "pkgconf",
    paste0(pkg_prefix, "-gcc"),
    paste0(pkg_prefix, "-cmake"),
    paste0(pkg_prefix, "-ninja")
  )

  pacman_cmd <- sprintf(
    "pacman -S --needed --noconfirm %s",
    paste(pkgs, collapse = " ")
  )

  run_cmd(
    bash,
    c("-lc", shQuote(pacman_cmd)),
    "MSYS2 installation of MITIE build dependencies failed."
  )

  src_dir <- file.path(tempdir(), "MITIE-src")
  if (dir.exists(src_dir) && clean) {
    unlink(src_dir, recursive = TRUE, force = TRUE)
  }

  git_bin <- if (file.exists(git_exe)) git_exe else Sys.which("git")
  if (!nzchar(git_bin)) {
    stop("git not found in Rtools/MSYS2 or PATH.", call. = FALSE)
  }

  run_cmd(
    git_bin,
    c("clone", "--depth", "1", "https://github.com/mit-nlp/MITIE.git", src_dir),
    "Failed to clone MITIE repository."
  )

  src_mitielib <- file.path(src_dir, "mitielib")
  build_dir <- file.path(src_mitielib, "build")
  dir.create(build_dir, recursive = TRUE, showWarnings = FALSE)

  patch_mitie_cmakelists(file.path(src_mitielib, "CMakeLists.txt"))

  cmake_exe <- file.path(prefix_root, "bin", "cmake.exe")
  gcc_exe   <- file.path(prefix_root, "bin", "gcc.exe")
  gxx_exe   <- file.path(prefix_root, "bin", "g++.exe")
  ninja_exe <- file.path(prefix_root, "bin", "ninja.exe")

  if (!file.exists(cmake_exe)) stop("cmake.exe not found: ", cmake_exe, call. = FALSE)
  if (!file.exists(gcc_exe))   stop("gcc.exe not found: ", gcc_exe, call. = FALSE)
  if (!file.exists(gxx_exe))   stop("g++.exe not found: ", gxx_exe, call. = FALSE)
  if (!file.exists(ninja_exe)) stop("ninja.exe not found: ", ninja_exe, call. = FALSE)

  cmake_args <- c(
    "-S", normalizePath(src_mitielib, winslash = "/", mustWork = FALSE),
    "-B", normalizePath(build_dir, winslash = "/", mustWork = FALSE),
    "-G", "Ninja",
    paste0("-DCMAKE_MAKE_PROGRAM=", normalizePath(ninja_exe, winslash = "/", mustWork = FALSE)),
    paste0("-DCMAKE_C_COMPILER=", normalizePath(gcc_exe, winslash = "/", mustWork = FALSE)),
    paste0("-DCMAKE_CXX_COMPILER=", normalizePath(gxx_exe, winslash = "/", mustWork = FALSE)),
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
    "-Wno-dev",
    paste0("-DCMAKE_INSTALL_PREFIX=", normalizePath(prefix, winslash = "/", mustWork = FALSE))
  )

  run_cmd(
    cmake_exe,
    cmake_args,
    "CMake configuration for MITIE failed."
  )

  run_cmd(
    cmake_exe,
    c("--build", normalizePath(build_dir, winslash = "/", mustWork = FALSE), "--parallel"),
    "MITIE build failed."
  )

  # Dossiers cibles permanents
  include_dst <- file.path(prefix, "include")
  lib_dst <- file.path(prefix, "lib")
  bin_dst <- file.path(prefix, "bin")

  dir.create(include_dst, recursive = TRUE, showWarnings = FALSE)
  dir.create(lib_dst, recursive = TRUE, showWarnings = FALSE)
  dir.create(bin_dst, recursive = TRUE, showWarnings = FALSE)

  # Recherche des fichiers MITIE réellement produits
  header_src <- first_existing(c(
    file.path(src_mitielib, "include", "mitie.h"),
    file.path(src_mitielib, "mitie.h"),
    file.path(build_dir, "include", "mitie.h")
  ))

  dll_src <- first_existing(c(
    file.path(src_mitielib, "libmitie.dll"),
    file.path(build_dir, "libmitie.dll"),
    file.path(build_dir, "Release", "libmitie.dll")
  ))

  implib_src <- first_existing(c(
    file.path(src_mitielib, "libmitie.dll.a"),
    file.path(build_dir, "libmitie.dll.a"),
    file.path(build_dir, "Release", "libmitie.dll.a"),
    file.path(src_mitielib, "mitie.lib"),
    file.path(build_dir, "mitie.lib"),
    file.path(build_dir, "Release", "mitie.lib")
  ))

  staticlib_src <- first_existing(c(
    file.path(src_mitielib, "libmitie.a"),
    file.path(build_dir, "libmitie.a"),
    file.path(build_dir, "Release", "libmitie.a")
  ))

  exe_sources <- c(
    mitie      = first_existing(c(file.path(build_dir, "mitie.exe"), file.path(src_mitielib, "mitie.exe"))),
    ner_stream = first_existing(c(file.path(build_dir, "ner_stream.exe"), file.path(src_mitielib, "ner_stream.exe"))),
    train_ner  = first_existing(c(file.path(build_dir, "train_ner.exe"), file.path(src_mitielib, "train_ner.exe")))
  )

  if (is.na(header_src)) {
    warning("MITIE header not found; MITIE_INCLUDE may not be usable until mitie.h is copied manually.")
  } else {
    ok <- file.copy(header_src, file.path(include_dst, "mitie.h"), overwrite = TRUE)
    if (!ok) stop("Failed to copy mitie.h to ", include_dst, call. = FALSE)
  }

  if (is.na(dll_src)) {
    warning("libmitie.dll not found after build.")
  } else {
    ok <- file.copy(dll_src, file.path(bin_dst, "libmitie.dll"), overwrite = TRUE)
    if (!ok) stop("Failed to copy libmitie.dll to ", bin_dst, call. = FALSE)
  }

  if (!is.na(implib_src)) {
    dst_name <- basename(implib_src)
    ok <- file.copy(implib_src, file.path(lib_dst, dst_name), overwrite = TRUE)
    if (!ok) stop("Failed to copy import library to ", lib_dst, call. = FALSE)
  } else if (!is.na(staticlib_src)) {
    ok <- file.copy(staticlib_src, file.path(lib_dst, "libmitie.a"), overwrite = TRUE)
    if (!ok) stop("Failed to copy static library to ", lib_dst, call. = FALSE)
  } else {
    warning("MITIE import/static library not found after build.")
  }

  for (nm in names(exe_sources)) {
    src <- exe_sources[[nm]]
    if (!is.na(src)) {
      file.copy(src, file.path(bin_dst, basename(src)), overwrite = TRUE)
    }
  }

  write_mitie_env(
    include_dir = include_dst,
    lib_dir = lib_dst,
    bin_dir = bin_dst,
    scope = if (persist) "renviron" else "session"
  )

  found_files <- c(
    list.files(include_dst, full.names = TRUE, recursive = FALSE),
    list.files(lib_dst, full.names = TRUE, recursive = FALSE),
    list.files(bin_dst, full.names = TRUE, recursive = FALSE)
  )

  found_files <- found_files[
    grepl("mitie(\\.h|\\.dll|\\.dll\\.a|\\.a|\\.lib|\\.exe)?$", basename(found_files), ignore.case = TRUE) |
      grepl("^ner_stream\\.exe$", basename(found_files), ignore.case = TRUE) |
      grepl("^train_ner\\.exe$", basename(found_files), ignore.case = TRUE)
  ]

  if (length(found_files)) {
    message("Files installed:")
    cat(paste(found_files, collapse = "\n"), "\n")
  }

  invisible(list(
    prefix = prefix,
    include = include_dst,
    lib = lib_dst,
    bin = bin_dst,
    packages = pkgs,
    source = src_dir,
    build = build_dir
  ))
}

install_mitie <- function(...) {
  sys <- Sys.info()[["sysname"]]

  if (identical(sys, "Linux")) {
    return(install_mitie_linux(...))
  }
  if (identical(sys, "Darwin")) {
    return(install_mitie_macos(...))
  }
  if (identical(sys, "Windows")) {
    return(install_mitie_windows_msys2(...))
  }

  stop("OS not supported automatically: ", sys, call. = FALSE)
}

# Example Windows / Rtools45 / UCRT64
install_mitie_windows_msys2(
  rtools_root = "C:/rtools45",
  repo = "ucrt64",
  persist = TRUE,
  prefix = "C:/rtools45/ucrt64",
  clean = TRUE
)
```

## Package installation 

```r
install.packages("remotes")
remotes::install_github("ManuHamel/rmitie")
```

# Example 

```r
library(rmitie)

> paths <- mitie_download_models("english")
> model <- mitie_load_ner_model(paths$ner_model)
> tok <- mitie_tokenize("Barack Obama was born in Hawaii and studied at Harvard University.")
> mitie_extract_entities(model, tok$tokens)
  start length          tag    score               text
1     1      2       PERSON 1.114526       Barack Obama
2     6      1     LOCATION 1.623440             Hawaii
3    10      2 ORGANIZATION 1.244351 Harvard University
```
