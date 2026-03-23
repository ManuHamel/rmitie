// [[Rcpp::plugins(cpp11)]]
#include <Rcpp.h>

extern "C" {
#include <mitie.h>
}

using namespace Rcpp;

// -----------------------------------------------------------------------------
// Holder
// -----------------------------------------------------------------------------

struct MitieNerModel {
  mitie_named_entity_extractor* ner;

  MitieNerModel() : ner(nullptr) {}
  ~MitieNerModel() {
    if (ner != nullptr) {
      mitie_free(ner);
      ner = nullptr;
    }
  }
};

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static void stop_if_null(void* ptr, const std::string& msg) {
  if (ptr == nullptr) {
    stop(msg);
  }
}

static std::vector<std::string> charpp_to_vector(char** tokens) {
  std::vector<std::string> out;
  if (tokens == nullptr) {
    return out;
  }

  for (size_t i = 0; tokens[i] != nullptr; ++i) {
    out.push_back(tokens[i]);
  }
  return out;
}

static std::vector<std::string> as_std_vector(CharacterVector x) {
  std::vector<std::string> out(x.size());
  for (R_xlen_t i = 0; i < x.size(); ++i) {
    out[i] = Rcpp::as<std::string>(x[i]);
  }
  return out;
}

static std::vector<char*> make_c_token_array(
    const std::vector<std::string>& tokens,
    std::vector< std::vector<char> >& backing
) {
  backing.clear();
  backing.reserve(tokens.size());

  std::vector<char*> ptrs;
  ptrs.reserve(tokens.size() + 1);

  for (size_t i = 0; i < tokens.size(); ++i) {
    backing.emplace_back(tokens[i].begin(), tokens[i].end());
    backing.back().push_back('\0');
    ptrs.push_back(backing.back().data());
  }

  ptrs.push_back(nullptr);
  return ptrs;
}

static std::string collapse_tokens(
    const std::vector<std::string>& tokens,
    size_t start,
    size_t len
) {
  std::string out;
  for (size_t i = start; i < start + len; ++i) {
    if (i > start) out += " ";
    out += tokens[i];
  }
  return out;
}

// -----------------------------------------------------------------------------
// External pointer finalizer
// -----------------------------------------------------------------------------

static SEXP ner_model_wrap(MitieNerModel* model) {
  XPtr<MitieNerModel> ptr(model, true);
  ptr.attr("class") = CharacterVector::create("mitie_ner_model");
  return ptr;
}

static XPtr<MitieNerModel> get_ner_model(SEXP xp) {
  XPtr<MitieNerModel> ptr(xp);
  if (!ptr || ptr->ner == nullptr) {
    stop("Invalid or unloaded MITIE NER model.");
  }
  return ptr;
}

// -----------------------------------------------------------------------------
// Exported functions
// -----------------------------------------------------------------------------

// [[Rcpp::export]]
SEXP mitie_load_ner_model(std::string model_path) {
  MitieNerModel* model = new MitieNerModel();
  model->ner = mitie_load_named_entity_extractor(model_path.c_str());

  if (model->ner == nullptr) {
    delete model;
    stop("Unable to load MITIE NER model: " + model_path);
  }

  return ner_model_wrap(model);
}

// [[Rcpp::export]]
List mitie_tokenize(std::string text) {
  unsigned long* offsets = nullptr;
  char** tokens = mitie_tokenize_with_offsets(text.c_str(), &offsets);

  if (tokens == nullptr) {
    stop("MITIE tokenization failed.");
  }

  std::vector<std::string> toks;
  std::vector<unsigned long> offs;

  for (size_t i = 0; tokens[i] != nullptr; ++i) {
    toks.push_back(tokens[i]);
    offs.push_back(offsets[i]);
  }

  mitie_free(tokens);
  mitie_free(offsets);

  return List::create(
    _["tokens"] = wrap(toks),
    _["offsets"] = wrap(offs)
  );
}

// [[Rcpp::export]]
DataFrame mitie_extract_entities(SEXP model_xptr, CharacterVector tokens_r) {
  XPtr<MitieNerModel> model = get_ner_model(model_xptr);

  std::vector<std::string> tokens = as_std_vector(tokens_r);
  std::vector< std::vector<char> > backing;
  std::vector<char*> token_ptrs = make_c_token_array(tokens, backing);

  mitie_named_entity_detections* dets =
    mitie_extract_entities(model->ner, token_ptrs.data());

  if (dets == nullptr) {
    stop("MITIE entity extraction failed.");
  }

  const unsigned long n = mitie_ner_get_num_detections(dets);

  IntegerVector start(n);
  IntegerVector length(n);
  CharacterVector tag(n);
  NumericVector score(n);
  CharacterVector text(n);

  for (unsigned long i = 0; i < n; ++i) {
    unsigned long pos = mitie_ner_get_detection_position(dets, i);
    unsigned long len = mitie_ner_get_detection_length(dets, i);
    const char* tagstr = mitie_ner_get_detection_tagstr(dets, i);
    double s = mitie_ner_get_detection_score(dets, i);

    start[i] = static_cast<int>(pos + 1); // R is 1-based
    length[i] = static_cast<int>(len);

    if (tagstr == nullptr) {
      tag[i] = NA_STRING;
    } else {
      tag[i] = tagstr;
    }

    score[i] = s;
    text[i] = collapse_tokens(tokens, pos, len);
  }

  mitie_free(dets);

  return DataFrame::create(
    _["start"] = start,
    _["length"] = length,
    _["tag"] = tag,
    _["score"] = score,
    _["text"] = text,
    _["stringsAsFactors"] = false
  );
}

// [[Rcpp::export]]
CharacterVector mitie_ner_tags(SEXP model_xptr) {
  XPtr<MitieNerModel> model = get_ner_model(model_xptr);

  unsigned long n = mitie_get_num_possible_ner_tags(model->ner);
  CharacterVector out(n);

  for (unsigned long i = 0; i < n; ++i) {
    const char* tag = mitie_get_named_entity_tagstr(model->ner, i);

    if (tag == nullptr) {
      out[i] = NA_STRING;
    } else {
      out[i] = tag;
    }
  }

  return out;
}
