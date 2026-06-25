#include "feature_extraction.h"
#include <math.h>

void extract_features(const float* ax, const float* ay, const float* az, int length, float* out_features) {
    if (length <= 0) return;

    float sum_x = 0, sum_y = 0, sum_z = 0;

    for (int i = 0; i < length; i++) {
        sum_x += ax[i];
        sum_y += ay[i];
        sum_z += az[i];
    }
    float mean_x = sum_x / length;
    float mean_y = sum_y / length;
    float mean_z = sum_z / length;

    float magnitudes[128];
    float sum_mag = 0;
    float max_mag = -10000.0f;
    float min_mag = 10000.0f;

    for (int i = 0; i < length && i < 128; i++) {
        magnitudes[i] = sqrtf(ax[i]*ax[i] + ay[i]*ay[i] + az[i]*az[i]);
        sum_mag += magnitudes[i];
        if (magnitudes[i] > max_mag) max_mag = magnitudes[i];
        if (magnitudes[i] < min_mag) min_mag = magnitudes[i];
    }
    float mean_mag = sum_mag / length;

    float peak_to_peak = max_mag - min_mag;

    float sum_var = 0;
    for (int i = 0; i < length && i < 128; i++) {
        float diff = magnitudes[i] - mean_mag;
        sum_var += (diff * diff);
    }
    float signal_variance = sum_var / length;

    float max_abs_jerk = 0;
    float dt = 1.0f / 50.0f;
    for (int i = 1; i < length && i < 128; i++) {
        float jerk = (magnitudes[i] - magnitudes[i-1]) / dt;
        float abs_jerk = fabsf(jerk);
        if (abs_jerk > max_abs_jerk) {
            max_abs_jerk = abs_jerk;
        }
    }

    float sum_m4 = 0;
    for (int i = 0; i < length && i < 128; i++) {
        float diff = magnitudes[i] - mean_mag;
        float diff2 = diff * diff;
        sum_m4 += (diff2 * diff2);
    }
    float m4 = sum_m4 / length;
    float m2 = signal_variance;
    float kurtosis = 0;
    if (m2 > 1e-6f) {
        kurtosis = (m4 / (m2 * m2)) - 3.0f;
    }

    out_features[0] = mean_x;
    out_features[1] = mean_y;
    out_features[2] = mean_z;
    out_features[3] = signal_variance;
    out_features[4] = max_abs_jerk;
    out_features[5] = peak_to_peak;
    out_features[6] = kurtosis;
}
