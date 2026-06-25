#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void extract_features(const float* ax, const float* ay, const float* az, int length, float* out_features);

#ifdef __cplusplus
}
#endif
