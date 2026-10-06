#pragma once
#include <ctype.h>
#include <stddef.h>
#include <string.h>

// A location entered as postal code, e.g. "01067", "01067 Dresden",
// "Dresden, 01067", "D-01067", "DE 01067" or "AT-1010". Leading zeros stay
// intact because the code is kept as text. The optional country prefix uses
// ISO letters or the classic single-letter forms (D, A, B, F, I, L).
struct PostalQuery {
  char code[6];
  char country[3]; // lower case ISO code, empty when not given
};

inline void postalCountryFromPrefix(const char *prefix, size_t length, char *country) {
  country[0] = '\0';
  if (length == 2) {
    country[0] = static_cast<char>(tolower(static_cast<unsigned char>(prefix[0])));
    country[1] = static_cast<char>(tolower(static_cast<unsigned char>(prefix[1])));
    country[2] = '\0';
    return;
  }
  if (length != 1) return;
  static const char *const singles[][2] = {{"D", "de"}, {"A", "at"}, {"B", "be"}, {"F", "fr"}, {"I", "it"}, {"L", "lu"}};
  const char letter = static_cast<char>(toupper(static_cast<unsigned char>(prefix[0])));
  for (const auto &entry : singles) {
    if (entry[0][0] == letter) {
      strcpy(country, entry[1]);
      return;
    }
  }
}

// Checks one token: optional 1-2 letters, optional '-', then 4-5 digits.
inline bool parsePostalToken(const char *token, size_t length, PostalQuery &out) {
  size_t letters = 0;
  while (letters < length && isalpha(static_cast<unsigned char>(token[letters]))) ++letters;
  if (letters > 2) return false;
  size_t start = letters;
  if (letters && start < length && token[start] == '-') ++start;
  const size_t digits = length - start;
  if (digits < 4 || digits > 5) return false;
  for (size_t i = start; i < length; ++i) {
    if (!isdigit(static_cast<unsigned char>(token[i]))) return false;
  }
  memcpy(out.code, token + start, digits);
  out.code[digits] = '\0';
  postalCountryFromPrefix(token, letters, out.country);
  return true;
}

// An upper-case two-letter prefix may also stand as its own word ("DE 01067").
inline bool parsePostalQuery(const char *text, PostalQuery &out) {
  out.code[0] = '\0';
  out.country[0] = '\0';
  const char *previous = nullptr;
  size_t previousLength = 0;
  const char *p = text;
  while (*p) {
    while (*p == ' ' || *p == ',' || *p == '\t') ++p;
    const char *token = p;
    while (*p && *p != ' ' && *p != ',' && *p != '\t') ++p;
    const size_t length = static_cast<size_t>(p - token);
    if (!length) continue;
    if (parsePostalToken(token, length, out)) {
      if (!out.country[0] && previous && previousLength == 2 &&
          isupper(static_cast<unsigned char>(previous[0])) && isupper(static_cast<unsigned char>(previous[1]))) {
        postalCountryFromPrefix(previous, previousLength, out.country);
      }
      return true;
    }
    previous = token;
    previousLength = length;
  }
  return false;
}
