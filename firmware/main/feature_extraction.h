#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Calculates the 7 features required by the decision tree
// 0: mean_x
// 1: mean_y
// 2: mean_z
// 3: signal_variance
// 4: max_abs_jerk
// 5: peak_to_peak
// 6: kurtosis
// Output features array must be at least size 7.
void extract_features(const float* ax, const float* ay, const float* az, int length, float* out_features);

#ifdef __cplusplus
}
#endif
