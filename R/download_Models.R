#' @export
mitie_download_models <- function(
    language = c("english", "spanish", "german"),
    destdir = file.path("~", "mitie-models"),
    overwrite = FALSE,
    mode = c("auto", "return_paths", "return_root")
) {
  language <- match.arg(language)
  mode <- match.arg(mode)

  destdir <- normalizePath(path.expand(destdir), winslash = "/", mustWork = FALSE)
  if (!dir.exists(destdir)) {
    dir.create(destdir, recursive = TRUE, showWarnings = FALSE)
  }

  releases <- list(
    english = list(
      url = "https://github.com/mit-nlp/MITIE/releases/download/v0.4/MITIE-models-v0.2.tar.bz2",
      archive = "MITIE-models-v0.2.tar.bz2",
      type = "tar.bz2",
      expected_dir = "MITIE-models/english"
    ),
    spanish = list(
      url = "https://github.com/mit-nlp/MITIE/releases/download/v0.4/MITIE-models-v0.2-Spanish.zip",
      archive = "MITIE-models-v0.2-Spanish.zip",
      type = "zip",
      expected_dir = "MITIE-models/spanish"
    ),
    german = list(
      url = "https://github.com/mit-nlp/MITIE/releases/download/v0.4/MITIE-models-v0.2-German.tar.bz2",
      archive = "MITIE-models-v0.2-German.tar.bz2",
      type = "tar.bz2",
      expected_dir = "MITIE-models/german"
    )
  )

  spec <- releases[[language]]
  archive_path <- file.path(destdir, spec$archive)
  root_dir <- file.path(destdir, "MITIE-models")
  lang_dir <- file.path(root_dir, language)

  need_download <- TRUE
  if (file.exists(archive_path) && !overwrite) {
    need_download <- FALSE
  }

  if (dir.exists(lang_dir) && !overwrite) {
    need_download <- FALSE
  }

  if (overwrite && dir.exists(lang_dir)) {
    unlink(lang_dir, recursive = TRUE, force = TRUE)
  }

  if (need_download || !dir.exists(lang_dir)) {
    message("Downloading MITIE model archive: ", spec$url)
    utils::download.file(
      url = spec$url,
      destfile = archive_path,
      mode = "wb",
      quiet = FALSE
    )

    message("Extracting archive: ", archive_path)

    if (identical(spec$type, "zip")) {
      utils::unzip(archive_path, exdir = destdir)
    } else if (identical(spec$type, "tar.bz2")) {
      utils::untar(archive_path, exdir = destdir)
    } else {
      stop("Unsupported archive type: ", spec$type, call. = FALSE)
    }
  }

  if (!dir.exists(lang_dir)) {
    stop(
      "Model extraction seems to have failed. Expected folder not found: ",
      lang_dir,
      call. = FALSE
    )
  }

  ner_model <- file.path(lang_dir, "ner_model.dat")
  feature_extractor <- file.path(lang_dir, "total_word_feature_extractor.dat")
  binary_relations_dir <- file.path(lang_dir, "binary_relations")

  out <- list(
    root = normalizePath(root_dir, winslash = "/", mustWork = FALSE),
    language_dir = normalizePath(lang_dir, winslash = "/", mustWork = FALSE),
    ner_model = normalizePath(ner_model, winslash = "/", mustWork = FALSE),
    total_word_feature_extractor = normalizePath(feature_extractor, winslash = "/", mustWork = FALSE),
    binary_relations_dir = normalizePath(binary_relations_dir, winslash = "/", mustWork = FALSE)
  )

  if (mode == "return_root") {
    return(out$root)
  }
  if (mode == "return_paths") {
    return(out)
  }

  out
}


#' @export
mitie_download_and_load_ner_model <- function(
    language = c("english", "spanish", "german"),
    destdir = file.path("~", "mitie-models"),
    overwrite = FALSE
) {
  language <- match.arg(language)

  paths <- mitie_download_models(
    language = language,
    destdir = destdir,
    overwrite = overwrite,
    mode = "return_paths"
  )

  if (!file.exists(paths$ner_model)) {
    stop("NER model file not found: ", paths$ner_model, call. = FALSE)
  }

  mitie_load_ner_model(paths$ner_model)
}
