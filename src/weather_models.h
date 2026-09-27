#pragma once
#include <string.h>

// Open-Meteo "models" values offered in the web UI (data/app.js keeps the same
// list with labels). An empty id keeps Open-Meteo's automatic best_match. All
// listed models were checked to return temperature, weather code and daily
// min/max; BOM ACCESS-G is left out because it returned no data.
constexpr const char *OPEN_METEO_MODELS[] = {
  "icon_seamless",             // DWD ICON (Germany)
  "ecmwf_ifs025",              // ECMWF IFS
  "meteoswiss_icon_seamless",  // MeteoSwiss
  "geosphere_seamless",        // GeoSphere Austria
  "meteofrance_seamless",      // Meteo-France
  "knmi_seamless",             // KNMI (Netherlands)
  "dmi_seamless",              // DMI (Denmark)
  "ukmo_seamless",             // UK Met Office
  "metno_seamless",            // MET Nordic
  "italia_meteo_arpae_icon_2i",// ItaliaMeteo ARPAE
  "gfs_seamless",              // NOAA GFS
  "gem_seamless",              // Environment Canada GEM
  "jma_seamless",              // JMA (Japan)
};

inline bool isOpenMeteoModel(const char *id) {
  if (!id || !*id) return true;
  for (const char *model : OPEN_METEO_MODELS) {
    if (strcmp(model, id) == 0) return true;
  }
  return false;
}
